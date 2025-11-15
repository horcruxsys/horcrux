# Pull Request

## Description

<!-- Provide a clear and concise description of your changes -->

**What does this PR do?**


**Why is this change needed?**


**Related Issues:**
<!-- Link related issues: Closes #123, Fixes #456, Refs #789 -->


## Type of Change

<!-- Check all that apply -->

- [ ] 🐛 Bug fix (non-breaking change that fixes an issue)
- [ ] ✨ New feature (non-breaking change that adds functionality)
- [ ] 💥 Breaking change (fix or feature that would cause existing functionality to not work as expected)
- [ ] 📝 Documentation update
- [ ] ♻️ Code refactoring (no functional changes)
- [ ] ⚡ Performance improvement
- [ ] 🧪 Test updates
- [ ] 🔧 Build/tooling changes

## Testing

<!-- Describe the tests you ran and how to reproduce them -->

- [ ] All existing tests pass (`ctest --output-on-failure`)
- [ ] Added new tests for new functionality
- [ ] Added regression tests for bug fixes
- [ ] Tests are deterministic (no flaky tests)
- [ ] Tested on multiple platforms (if applicable):
  - [ ] Linux
  - [ ] macOS
  - [ ] Windows

**Test Commands:**
```bash
# Add specific test commands you used
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Debug
cmake --build . -- -j$(nproc)
ctest --output-on-failure
```

**Manual Testing:**
<!-- Describe any manual testing performed -->


## Documentation

- [ ] Updated relevant documentation in `docs/`
- [ ] Added/updated code comments for public APIs
- [ ] Updated README.md (if applicable)
- [ ] Added usage examples for new features

## Performance & Benchmarks

<!-- Required for performance-critical changes -->

- [ ] No performance impact (or performance improvement)
- [ ] Ran benchmarks before and after changes
- [ ] Benchmark results included below (if applicable)

**Benchmark Results:**
<!-- Include benchmark data for performance-related changes -->
```
| Operation | Before | After | Improvement |
|-----------|--------|-------|-------------|
| Example   | N/A    | N/A   | N/A         |
```

## Code Quality

- [ ] Code follows [coding standards](../docs/coding-standards.md)
- [ ] No compiler warnings
- [ ] Ran with sanitizers (AddressSanitizer, UndefinedBehaviorSanitizer)
- [ ] Code is thread-safe (if applicable)
- [ ] No undefined behavior
- [ ] Proper error handling with `std::expected`
- [ ] RAII principles applied
- [ ] Memory safety verified

**Sanitizer Commands:**
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_FLAGS="-fsanitize=address,undefined"
cmake --build . -- -j$(nproc)
ctest --output-on-failure
```

## Commit Guidelines

- [ ] Commits follow [conventional commit format](../CONTRIBUTING.md#commit-message-guidelines)
- [ ] Each commit is atomic and builds successfully
- [ ] Commit messages explain what and why (not how)

## Checklist

- [ ] I have read and followed the [Contributing Guidelines](../CONTRIBUTING.md)
- [ ] I have read and agree to the [Code of Conduct](../CODE_OF_CONDUCT.md)
- [ ] I have checked for similar PRs and issues
- [ ] My code follows the project's coding standards
- [ ] I have performed a self-review of my code
- [ ] I have commented complex or non-obvious code
- [ ] My changes generate no new warnings
- [ ] I have added tests that prove my fix/feature works
- [ ] New and existing tests pass locally
- [ ] Any dependent changes have been merged

## For AI-Assisted Development

<!-- If you used GitHub Copilot or similar AI tools -->

- [ ] Followed [GitHub Copilot Instructions](copilot-instructions.md)
- [ ] Verified AI suggestions align with project standards
- [ ] Manually reviewed and tested all AI-generated code

## Additional Context

<!-- Add any other context, screenshots, or information about the PR here -->


## Review Focus Areas

<!-- Help reviewers by highlighting specific areas that need attention -->

- 
- 

---

**Thank you for contributing to Horcrux! 🚀**

<!-- 
Before submitting:
1. Ensure all checkboxes are addressed
2. Fill in all required sections
3. Remove any sections that don't apply
4. Be responsive to review feedback
-->
