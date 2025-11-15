// Horcrux - Target Parser Implementation
// Copyright (C) 2025 Horcrux Project Contributors
// Licensed under the MIT License

#include "target_parser.h"

namespace horcrux::cli {

auto to_string(TargetParseError error) -> std::string {
  switch (error) {
  case TargetParseError::InvalidFormat:
    return "Invalid target format";
  case TargetParseError::EmptyTarget:
    return "Empty target specification";
  case TargetParseError::MissingPackage:
    return "Missing package in target specification";
  case TargetParseError::MissingTargetName:
    return "Missing target name in specification";
  }
  return "Unknown error";
}

auto parse_target(std::string_view target_spec) -> tl::expected<Target, TargetParseError> {
  if (target_spec.empty()) {
    return tl::unexpected(TargetParseError::EmptyTarget);
  }

  // Target must start with "//"
  if (!target_spec.starts_with("//")) {
    return tl::unexpected(TargetParseError::InvalidFormat);
  }

  // Remove "//" prefix
  target_spec.remove_prefix(2);

  // Find the colon separator
  auto colon_pos = target_spec.find(':');

  std::string package;
  std::string target_name;

  if (colon_pos == std::string_view::npos) {
    // No colon - use package name as target name
    package = std::string(target_spec);

    // Extract last component as target name
    auto last_slash = package.find_last_of('/');
    if (last_slash != std::string::npos) {
      target_name = package.substr(last_slash + 1);
    } else {
      target_name = package;
    }
  } else {
    // Split at colon
    package = std::string(target_spec.substr(0, colon_pos));
    target_name = std::string(target_spec.substr(colon_pos + 1));
  }

  if (package.empty()) {
    return tl::unexpected(TargetParseError::MissingPackage);
  }

  if (target_name.empty()) {
    return tl::unexpected(TargetParseError::MissingTargetName);
  }

  return Target{std::move(package), std::move(target_name)};
}

} // namespace horcrux::cli
