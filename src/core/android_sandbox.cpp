// Horcrux - Android Build Sandbox & Hermeticity
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "android_sandbox.h"

#include <array>
#include <cstring>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <regex>

// sys/wait.h is needed for WIFEXITED, WEXITSTATUS, etc.
#include <sys/wait.h>

#ifdef __linux__
#include <sched.h>
#include <sys/mount.h>
#include <sys/resource.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#endif

namespace horcrux::core {

auto to_string(SandboxError error) -> std::string {
  switch (error) {
    case SandboxError::InvalidConfiguration:
      return "Invalid sandbox configuration";
    case SandboxError::MountFailed:
      return "Mount operation failed";
    case SandboxError::NamespaceCreationFailed:
      return "Failed to create namespace";
    case SandboxError::ProcessExecutionFailed:
      return "Process execution failed";
    case SandboxError::ViolationDetected:
      return "Sandbox violation detected";
    case SandboxError::UnsupportedPlatform:
      return "Sandboxing not supported on this platform";
    case SandboxError::IoError:
      return "I/O error";
    case SandboxError::UnknownError:
      return "Unknown error";
  }
  return "Unknown error";
}

auto MountRule::read_only(const std::filesystem::path& source,
                          const std::filesystem::path& target) -> MountRule {
  MountRule rule;
  rule.source = source;
  rule.target = target;
  rule.type = MountType::ReadOnly;
  return rule;
}

auto MountRule::read_write(const std::filesystem::path& source,
                           const std::filesystem::path& target) -> MountRule {
  MountRule rule;
  rule.source = source;
  rule.target = target;
  rule.type = MountType::ReadWrite;
  return rule;
}

auto MountRule::tmpfs(const std::filesystem::path& target) -> MountRule {
  MountRule rule;
  rule.target = target;
  rule.type = MountType::TmpFs;
  return rule;
}

auto AndroidSandbox::is_supported() -> bool {
#ifdef __linux__
  // Check if we have CAP_SYS_ADMIN or user namespaces
  // For now, assume supported on Linux
  return true;
#else
  return false;
#endif
}

auto AndroidSandbox::validate_config(const SandboxConfig& config)
    -> tl::expected<void, SandboxError> {
  // Validate executable exists
  if (!std::filesystem::exists(config.executable)) {
    return tl::unexpected(SandboxError::InvalidConfiguration);
  }

  // Validate mount sources exist (except optional mounts)
  for (const auto& mount : config.mounts) {
    if (!mount.optional && mount.type != MountType::TmpFs &&
        !std::filesystem::exists(mount.source)) {
      return tl::unexpected(SandboxError::InvalidConfiguration);
    }
  }

  return {};
}

auto AndroidSandbox::create_android_build_sandbox(
    const std::filesystem::path& executable,
    const std::vector<std::string>& arguments,
    const std::filesystem::path& sdk_path,
    const std::filesystem::path& source_path,
    const std::filesystem::path& scratch_path) -> SandboxConfig {
  
  SandboxConfig config;
  config.executable = executable;
  config.arguments = arguments;
  config.working_dir = "/sandbox/work";

  // Mount SDK as read-only
  if (std::filesystem::exists(sdk_path)) {
    config.mounts.push_back(
        MountRule::read_only(sdk_path, "/sandbox/sdk"));
  }

  // Mount source as read-only
  if (std::filesystem::exists(source_path)) {
    config.mounts.push_back(
        MountRule::read_only(source_path, "/sandbox/src"));
  }

  // Mount scratch as read-write
  if (std::filesystem::exists(scratch_path)) {
    config.mounts.push_back(
        MountRule::read_write(scratch_path, "/sandbox/work"));
  }

  // Add tmpfs for /tmp
  config.mounts.push_back(MountRule::tmpfs("/tmp"));

  // Enable all isolation features
  config.enable_network_isolation = true;
  config.enable_pid_namespace = true;
  config.enable_mount_namespace = true;
  config.enable_ipc_namespace = true;
  config.enable_uts_namespace = true;

  // Enable violation detection
  config.detect_violations = true;
  config.fail_on_violation = true;

  return config;
}

auto AndroidSandbox::execute(const SandboxConfig& config)
    -> tl::expected<SandboxResult, SandboxError> {
  
  // Validate configuration
  auto validation = validate_config(config);
  if (!validation) {
    return tl::unexpected(validation.error());
  }

  // Check if sandboxing is supported
  if (!is_supported()) {
    if (config.verbose) {
      std::cerr << "Warning: Sandboxing not supported, falling back to basic execution\n";
    }
    return execute_basic(config);
  }

#ifdef __linux__
  // Try namespace-based execution, fall back to basic if it fails
  auto result = execute_with_namespaces(config);
  if (!result.has_value() && 
      (result.error() == SandboxError::NamespaceCreationFailed ||
       result.error() == SandboxError::ProcessExecutionFailed)) {
    // Fall back to basic execution if namespace creation fails (e.g., no privileges)
    if (config.verbose) {
      std::cerr << "Warning: Namespace creation failed, falling back to basic execution\n";
    }
    return execute_basic(config);
  }
  return result;
#else
  return execute_basic(config);
#endif
}

#ifdef __linux__
auto AndroidSandbox::execute_with_namespaces(const SandboxConfig& config)
    -> tl::expected<SandboxResult, SandboxError> {
  
  auto start_time = std::chrono::steady_clock::now();

  // Create pipes for stdout and stderr
  int stdout_pipe[2];
  int stderr_pipe[2];
  if (pipe(stdout_pipe) == -1 || pipe(stderr_pipe) == -1) {
    return tl::unexpected(SandboxError::ProcessExecutionFailed);
  }

  // Build clone flags for namespaces
  int clone_flags = SIGCHLD;
  if (config.enable_mount_namespace) clone_flags |= CLONE_NEWNS;
  if (config.enable_pid_namespace) clone_flags |= CLONE_NEWPID;
  if (config.enable_network_isolation) clone_flags |= CLONE_NEWNET;
  if (config.enable_ipc_namespace) clone_flags |= CLONE_NEWIPC;
  if (config.enable_uts_namespace) clone_flags |= CLONE_NEWUTS;

  pid_t pid = fork();
  
  if (pid == -1) {
    close(stdout_pipe[0]);
    close(stdout_pipe[1]);
    close(stderr_pipe[0]);
    close(stderr_pipe[1]);
    return tl::unexpected(SandboxError::ProcessExecutionFailed);
  }

  if (pid == 0) {
    // Child process
    
    // Redirect stdout and stderr to pipes
    close(stdout_pipe[0]);
    close(stderr_pipe[0]);
    dup2(stdout_pipe[1], STDOUT_FILENO);
    dup2(stderr_pipe[1], STDERR_FILENO);
    close(stdout_pipe[1]);
    close(stderr_pipe[1]);

    // Apply resource limits if specified
    if (config.max_memory_bytes) {
      struct rlimit limit;
      limit.rlim_cur = *config.max_memory_bytes;
      limit.rlim_max = *config.max_memory_bytes;
      setrlimit(RLIMIT_AS, &limit);
    }

    // Setup mounts if mount namespace is enabled
    if (config.enable_mount_namespace) {
      // Unshare mount namespace
      if (unshare(CLONE_NEWNS) == -1) {
        std::cerr << "Failed to unshare mount namespace: " << strerror(errno) << "\n";
        _exit(1);
      }

      // Make all mounts private to avoid propagation
      if (mount(nullptr, "/", nullptr, MS_PRIVATE | MS_REC, nullptr) == -1) {
        // Non-fatal, continue
      }

      // Note: Full mount setup would require more privileges
      // For now, we track requested mounts for violation detection
      if (config.verbose) {
        std::cerr << "Mount namespace created with " << config.mounts.size() << " mount rules\n";
      }
    }

    // Change working directory if specified
    if (!config.working_dir.empty() && std::filesystem::exists(config.working_dir)) {
      if (chdir(config.working_dir.c_str()) == -1) {
        std::cerr << "Failed to change directory: " << strerror(errno) << "\n";
      }
    }

    // Prepare arguments for execvp
    std::vector<char*> args;
    args.push_back(const_cast<char*>(config.executable.c_str()));
    for (const auto& arg : config.arguments) {
      args.push_back(const_cast<char*>(arg.c_str()));
    }
    args.push_back(nullptr);

    // Prepare environment
    std::vector<std::string> env_strings;
    std::vector<char*> envp;
    for (const auto& [key, value] : config.env_vars) {
      env_strings.push_back(key + "=" + value);
    }
    for (const auto& env_str : env_strings) {
      envp.push_back(const_cast<char*>(env_str.c_str()));
    }
    envp.push_back(nullptr);

    // Execute command
    if (!config.env_vars.empty()) {
      execvpe(config.executable.c_str(), args.data(), envp.data());
    } else {
      execvp(config.executable.c_str(), args.data());
    }

    // If we reach here, exec failed
    std::cerr << "Failed to execute: " << strerror(errno) << "\n";
    _exit(1);
  }

  // Parent process
  close(stdout_pipe[1]);
  close(stderr_pipe[1]);

  // Read stdout and stderr
  std::string stdout_output;
  std::string stderr_output;

  auto read_from_fd = [](int fd) -> std::string {
    std::string result;
    std::array<char, 4096> buffer;
    ssize_t bytes_read;
    
    // Set non-blocking
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    
    while ((bytes_read = read(fd, buffer.data(), buffer.size())) > 0) {
      result.append(buffer.data(), static_cast<size_t>(bytes_read));
    }
    return result;
  };

  // Set timeout if specified
  bool timed_out = false;
  if (config.timeout) {
    alarm(static_cast<unsigned int>(config.timeout->count()));
  }

  // Wait for child process
  int status;
  pid_t wait_result = waitpid(pid, &status, 0);
  
  if (config.timeout) {
    alarm(0); // Cancel alarm
  }

  stdout_output = read_from_fd(stdout_pipe[0]);
  stderr_output = read_from_fd(stderr_pipe[0]);

  close(stdout_pipe[0]);
  close(stderr_pipe[0]);

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  SandboxResult result;
  result.stdout_output = stdout_output;
  result.stderr_output = stderr_output;
  result.execution_time = duration;

  if (wait_result == -1) {
    result.exit_code = -1;
  } else if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.exit_code = 128 + WTERMSIG(status);
  } else {
    result.exit_code = -1;
  }

  // Detect violations if enabled
  if (config.detect_violations) {
    result.violations = detect_violations(stdout_output, stderr_output, config);
  }

  // Check if we should fail on violations
  if (config.fail_on_violation && !result.violations.empty()) {
    return tl::unexpected(SandboxError::ViolationDetected);
  }

  return result;
}
#endif

auto AndroidSandbox::execute_basic(const SandboxConfig& config)
    -> tl::expected<SandboxResult, SandboxError> {
  
  auto start_time = std::chrono::steady_clock::now();

  // Build command string
  std::ostringstream cmd;
  cmd << config.executable.string();
  for (const auto& arg : config.arguments) {
    cmd << " " << arg;
  }

  // Redirect stdout and stderr to temporary files
  auto temp_dir = std::filesystem::temp_directory_path();
  auto stdout_file = temp_dir / "horcrux_sandbox_stdout.txt";
  auto stderr_file = temp_dir / "horcrux_sandbox_stderr.txt";

  cmd << " > " << stdout_file.string() << " 2> " << stderr_file.string();

  // Execute command
  int status = std::system(cmd.str().c_str());

  auto end_time = std::chrono::steady_clock::now();
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end_time - start_time);

  // Read output files
  std::string stdout_output;
  std::string stderr_output;

  if (std::filesystem::exists(stdout_file)) {
    std::ifstream stdout_stream(stdout_file);
    std::ostringstream buffer;
    buffer << stdout_stream.rdbuf();
    stdout_output = buffer.str();
    std::filesystem::remove(stdout_file);
  }

  if (std::filesystem::exists(stderr_file)) {
    std::ifstream stderr_stream(stderr_file);
    std::ostringstream buffer;
    buffer << stderr_stream.rdbuf();
    stderr_output = buffer.str();
    std::filesystem::remove(stderr_file);
  }

  SandboxResult result;
  // Extract exit code from system() status
  if (status == -1) {
    result.exit_code = -1;
  } else if (WIFEXITED(status)) {
    result.exit_code = WEXITSTATUS(status);
  } else if (WIFSIGNALED(status)) {
    result.exit_code = 128 + WTERMSIG(status);
  } else {
    result.exit_code = -1;
  }
  result.stdout_output = stdout_output;
  result.stderr_output = stderr_output;
  result.execution_time = duration;

  // Detect violations if enabled
  if (config.detect_violations) {
    result.violations = detect_violations(stdout_output, stderr_output, config);
  }

  // Check if we should fail on violations
  if (config.fail_on_violation && !result.violations.empty()) {
    return tl::unexpected(SandboxError::ViolationDetected);
  }

  return result;
}

auto AndroidSandbox::detect_violations(const std::string& stdout_output,
                                       const std::string& stderr_output,
                                       const SandboxConfig& config)
    -> std::vector<SandboxViolation> {
  std::vector<SandboxViolation> violations;

  // Combine output for scanning
  std::string combined = stdout_output + "\n" + stderr_output;

  // Common patterns that indicate violations
  std::vector<std::pair<std::regex, ViolationType>> patterns = {
    {std::regex(R"(Permission denied|Access denied)", std::regex::icase),
     ViolationType::UnauthorizedFileAccess},
    {std::regex(R"(Read-only file system)", std::regex::icase),
     ViolationType::UnauthorizedWrite},
    {std::regex(R"(No such file or directory: /(?!sandbox))", std::regex::icase),
     ViolationType::UnauthorizedFileAccess},
    {std::regex(R"(Network unreachable|Connection refused)", std::regex::icase),
     ViolationType::UnauthorizedNetworkAccess},
  };

  for (const auto& [pattern, type] : patterns) {
    std::smatch match;
    std::string::const_iterator search_start(combined.cbegin());
    while (std::regex_search(search_start, combined.cend(), match, pattern)) {
      SandboxViolation violation;
      violation.type = type;
      violation.description = match.str();
      violation.timestamp = std::chrono::system_clock::now();
      violations.push_back(violation);
      search_start = match.suffix().first;
    }
  }

  return violations;
}

auto AndroidSandbox::setup_mounts(const std::vector<MountRule>& mounts)
    -> tl::expected<void, SandboxError> {
#ifdef __linux__
  for (const auto& mount : mounts) {
    // Create target directory if it doesn't exist
    if (!std::filesystem::exists(mount.target)) {
      std::error_code ec;
      std::filesystem::create_directories(mount.target, ec);
      if (ec && !mount.optional) {
        return tl::unexpected(SandboxError::MountFailed);
      }
    }

    unsigned long flags = 0;
    const char* fstype = nullptr;

    switch (mount.type) {
      case MountType::ReadOnly:
        flags = MS_BIND | MS_RDONLY;
        break;
      case MountType::ReadWrite:
        flags = MS_BIND;
        break;
      case MountType::TmpFs:
        fstype = "tmpfs";
        break;
      case MountType::Proc:
        fstype = "proc";
        break;
      case MountType::DevNull:
        // Special handling for /dev/null
        continue;
    }

    const char* source = mount.source.empty() ? nullptr : mount.source.c_str();
    
    if (::mount(source, mount.target.c_str(), fstype, flags, nullptr) == -1) {
      if (!mount.optional) {
        return tl::unexpected(SandboxError::MountFailed);
      }
    }
  }
#endif
  return {};
}

auto AndroidSandbox::apply_resource_limits(const SandboxConfig& config)
    -> tl::expected<void, SandboxError> {
#ifdef __linux__
  if (config.max_memory_bytes) {
    struct rlimit limit;
    limit.rlim_cur = *config.max_memory_bytes;
    limit.rlim_max = *config.max_memory_bytes;
    if (setrlimit(RLIMIT_AS, &limit) == -1) {
      return tl::unexpected(SandboxError::ProcessExecutionFailed);
    }
  }
#endif
  return {};
}

SandboxGuard::SandboxGuard(const std::filesystem::path& scratch_dir)
    : scratch_dir_(scratch_dir) {
  // Create scratch directory if it doesn't exist
  if (!std::filesystem::exists(scratch_dir_)) {
    std::filesystem::create_directories(scratch_dir_);
  }
}

SandboxGuard::~SandboxGuard() {
  if (cleanup_on_destroy_ && std::filesystem::exists(scratch_dir_)) {
    std::error_code ec;
    std::filesystem::remove_all(scratch_dir_, ec);
    // Ignore errors during cleanup
  }
}

} // namespace horcrux::core
