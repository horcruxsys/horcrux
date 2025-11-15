#!/bin/bash
# Verification script for Horcrux git hooks setup
# This script checks that all required tools are installed and hooks are working

set -e

echo "🔍 Verifying Horcrux Git Hooks Setup"
echo "======================================"
echo

# Check for lefthook
echo "✓ Checking lefthook..."
if command -v lefthook &> /dev/null; then
    echo "  Found: $(lefthook version)"
else
    echo "  ❌ lefthook not found. Install from: https://github.com/evilmartians/lefthook"
    exit 1
fi

# Check for clang-format
echo "✓ Checking clang-format..."
if command -v clang-format &> /dev/null; then
    echo "  Found: $(clang-format --version | head -1)"
else
    echo "  ⚠️  clang-format not found. C++ formatting will not work."
    echo "     Install: sudo apt-get install clang-format (Ubuntu/Debian)"
    echo "              brew install clang-format (macOS)"
fi

# Check for yamllint
echo "✓ Checking yamllint..."
if command -v yamllint &> /dev/null; then
    echo "  Found: $(yamllint --version)"
else
    echo "  ⚠️  yamllint not found. YAML validation will not work."
    echo "     Install: pip install yamllint"
fi

# Check for actionlint
echo "✓ Checking actionlint (optional, used in CI)..."
if command -v actionlint &> /dev/null; then
    echo "  Found: $(actionlint --version | head -1)"
else
    echo "  ⚠️  actionlint not found (optional, mainly for CI)."
    echo "     Install: bash <(curl -s https://raw.githubusercontent.com/rhysd/actionlint/main/scripts/download-actionlint.bash)"
fi

# Check for CMake
echo "✓ Checking CMake..."
if command -v cmake &> /dev/null; then
    echo "  Found: $(cmake --version | head -1)"
else
    echo "  ❌ CMake not found. Build hooks will not work."
    echo "     Install: sudo apt-get install cmake (Ubuntu/Debian)"
    echo "              brew install cmake (macOS)"
    exit 1
fi

# Check for Ninja
echo "✓ Checking Ninja..."
if command -v ninja &> /dev/null; then
    echo "  Found: $(ninja --version)"
else
    echo "  ⚠️  Ninja not found. Using Make as fallback."
fi

# Check if hooks are installed
echo
echo "✓ Checking if hooks are installed..."
if [ -f .git/hooks/pre-commit ] && grep -q "lefthook" .git/hooks/pre-commit; then
    echo "  ✅ pre-commit hook is installed"
else
    echo "  ❌ pre-commit hook not installed. Run: lefthook install"
    exit 1
fi

if [ -f .git/hooks/pre-push ] && grep -q "lefthook" .git/hooks/pre-push; then
    echo "  ✅ pre-push hook is installed"
else
    echo "  ❌ pre-push hook not installed. Run: lefthook install"
    exit 1
fi

# Check lefthook.yml exists
echo
echo "✓ Checking lefthook configuration..."
if [ -f lefthook.yml ]; then
    echo "  ✅ lefthook.yml found"
else
    echo "  ❌ lefthook.yml not found"
    exit 1
fi

# Check .yamllint exists
if [ -f .yamllint ]; then
    echo "  ✅ .yamllint found"
else
    echo "  ⚠️  .yamllint not found"
fi

# Test pre-commit hooks (dry run)
echo
echo "✓ Testing pre-commit hooks (dry run)..."
if lefthook run pre-commit 2>&1 | grep -q "error"; then
    echo "  ⚠️  Pre-commit hooks reported errors (check above)"
else
    echo "  ✅ Pre-commit hooks OK"
fi

echo
echo "======================================"
echo "✅ Verification Complete!"
echo
echo "Your git hooks are set up and ready to use."
echo "Try making a commit to see the hooks in action."
echo
echo "For more information, see: docs/git-hooks.md"
