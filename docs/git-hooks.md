# Git Hooks Setup for Horcrux

This repository uses [Lefthook](https://github.com/evilmartians/lefthook) to manage git hooks for automated code quality checks.

## What Do The Hooks Do?

### Pre-Commit Hook

Runs automatically before each commit to ensure code quality:

1. **Format C++ Code** - Runs `clang-format` on all staged C++ files
2. **Validate YAML** - Checks YAML syntax with `yamllint`
3. **Validate GitHub Actions** - Verifies workflow files with `actionlint`
4. **Check Merge Conflicts** - Ensures no conflict markers remain
5. **Check TODOs** - Warns about TODOs without context

### Pre-Push Hook

Runs automatically before pushing to ensure the code works:

1. **Build Project** - Compiles the entire project with CMake
2. **Run Tests** - Executes all test suites

## Installation

### Install Lefthook

**On Linux/macOS:**

```bash
# Download and install
curl -fsSL https://raw.githubusercontent.com/evilmartians/lefthook/master/install.sh | bash

# Or download binary from GitHub releases
wget https://github.com/evilmartians/lefthook/releases/latest/download/lefthook_<version>_<platform>
chmod +x lefthook_*
sudo mv lefthook_* /usr/local/bin/lefthook
```

**On Windows:**

```powershell
# Using Scoop
scoop install lefthook

# Or download from GitHub releases
```

### Install Dependencies

The hooks require these tools:

```bash
# Ubuntu/Debian
sudo apt-get install -y clang-format yamllint

# macOS
brew install clang-format yamllint

# actionlint (all platforms)
bash <(curl -s https://raw.githubusercontent.com/rhysd/actionlint/main/scripts/download-actionlint.bash)
sudo mv ./actionlint /usr/local/bin/
```

### Activate Hooks

Once lefthook is installed, activate the hooks in your repository:

```bash
cd /path/to/horcrux
lefthook install
```

You should see:
```
sync hooks: ✔️ (pre-push, pre-commit)
```

## Usage

### Normal Workflow

Hooks run automatically:

```bash
# Format and validate automatically on commit
git add .
git commit -m "feat: add new feature"

# Build and test automatically on push
git push
```

### Testing Hooks Manually

Run hooks without committing:

```bash
# Test pre-commit checks
lefthook run pre-commit

# Test pre-push checks
lefthook run pre-push
```

### Skipping Hooks

**⚠️ Only skip hooks in emergencies!**

```bash
# Skip all hooks for one commit
LEFTHOOK=0 git commit -m "emergency: critical hotfix"

# Skip specific command
LEFTHOOK_EXCLUDE=build-project git push

# Skip pre-push entirely
git push --no-verify
```

## Troubleshooting

### Hook Failed: Format Check

If clang-format fails:

```bash
# Auto-fix formatting
clang-format -i src/**/*.cpp src/**/*.hpp

# Or let the hook fix it
git add .
git commit  # Hook will auto-format and re-stage files
```

### Hook Failed: YAML Validation

If yamllint fails:

```bash
# Check errors
yamllint .github/workflows/build.yml

# Fix issues (indentation, spacing, line length)
# Then commit again
```

### Hook Failed: GitHub Actions Validation

If actionlint fails:

```bash
# Check errors
actionlint .github/workflows/*.yml

# Fix issues (syntax errors, undefined variables)
# Then commit again
```

### Hook Failed: Build

If the build fails:

```bash
# Clean and rebuild manually
rm -rf build
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug -G Ninja
cmake --build .

# Fix build errors, then push again
```

### Hooks Not Running

If hooks don't run at all:

```bash
# Verify lefthook is installed
lefthook version

# Reinstall hooks
lefthook install

# Check hook files exist
ls -la .git/hooks/

# Verify lefthook.yml exists
cat lefthook.yml
```

## Configuration

The hook configuration is in `lefthook.yml` at the repository root. Modify this file to:

- Add new checks
- Change hook behavior
- Adjust parallel execution
- Add custom scripts

Example customization:

```yaml
pre-commit:
  commands:
    my-custom-check:
      glob: "*.cpp"
      run: ./scripts/my-check.sh {staged_files}
```

## CI Integration

The same checks run in CI (GitHub Actions) to catch any issues that bypass hooks. See `.github/workflows/build.yml` for CI configuration.

## Getting Help

If you encounter issues with git hooks:

1. Check this README
2. Review hook output for error messages
3. Test hooks manually: `lefthook run pre-commit`
4. Open an issue with:
   - Hook output
   - Lefthook version: `lefthook version`
   - OS and environment details

## Further Reading

- [Lefthook Documentation](https://github.com/evilmartians/lefthook/blob/master/docs/usage.md)
- [GitHub Actions Validation with actionlint](https://github.com/rhysd/actionlint)
- [YAML Linting with yamllint](https://yamllint.readthedocs.io/)
- [Contributing Guide](../CONTRIBUTING.md)
