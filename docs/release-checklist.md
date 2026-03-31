# Release Checklist — v2026.0401.0

This checklist must be fully signed off before the release tag is pushed.
Each item must be checked by a named reviewer.

## 1. Code Freeze Verification

- [ ] Release branch `release/2026.0401` is cut from `main`.
- [ ] No feature commits merged after code freeze.
- [ ] All open P0 / P1 issues are resolved or explicitly deferred with documented rationale.
- [ ] All outstanding merge requests targeting the release branch have been reviewed.

## 2. CI Quality Gates

- [ ] Full CI matrix passes on the release branch (GCC 14, Clang 18, Ubuntu 24.04).
- [ ] AddressSanitizer (ASan) build passes with zero new errors.
- [ ] UndefinedBehaviorSanitizer (UBSan) build passes with zero new errors.
- [ ] ThreadSanitizer (TSan) build passes with zero new errors.
- [ ] `clang-tidy` static analysis reports no new warnings in release-critical paths.
- [ ] All regression suites pass:
  - [ ] `graph_regression_test`
  - [ ] `cache_regression_test`
  - [ ] `sandbox_regression_test`
  - [ ] `adapter_regression_test`
  - [ ] `plugin_lifecycle_regression_test`

## 3. Platform Smoke Tests

- [ ] Linux x86-64 (Ubuntu 24.04 LTS): install, `horcrux doctor`, and `horcrux build`.
- [ ] macOS arm64 (macOS 14 Sonoma): install, `horcrux doctor`, and `horcrux build`.
- [ ] Windows x64 (Windows Server 2022): install, `horcrux doctor`, and `horcrux build`.

## 4. Benchmark Validation

- [ ] Benchmark suite run on the reference hardware (see `docs/benchmark-methodology.md`).
- [ ] No performance regression > 10% relative to the previous release on any tracked workload.
- [ ] Benchmark results committed to `benchmarks/results/v2026.0401.0/`.
- [ ] Benchmark summary updated in `docs/benchmark-methodology.md`.

## 5. Release Artifacts

- [ ] Release artifacts built in `--release` mode from the tagged commit.
- [ ] Checksums (SHA-256) generated for all artifacts via `scripts/package-release.sh`.
- [ ] Artifact filenames follow the naming convention:
  `horcrux-<version>-<os>-<arch>.<ext>`.
- [ ] Artifacts uploaded to the GitHub release page.
- [ ] Checksum file (`horcrux-<version>-checksums.txt`) uploaded alongside artifacts.

## 6. Documentation

- [ ] `CHANGELOG.md` entry for `v2026.0401.0` is complete and accurate.
- [ ] `docs/project-status.md` updated with M6 completion.
- [ ] `README.md` roadmap updated to reflect M6 complete.
- [ ] `docs/quick-start.md` reviewed and accurate for the release.
- [ ] `docs/migration-guide.md` published and complete.
- [ ] `docs/troubleshooting.md` reviewed for accuracy.
- [ ] All documentation links checked (no broken references).

## 7. Release Notes

- [ ] Release notes drafted and reviewed by at least two maintainers.
- [ ] Known issues section is accurate.
- [ ] Upgrade guidance (from alpha) is clear and tested.
- [ ] Security fixes (if any) are described with CVE references.

## 8. Final Tag and Publication

- [ ] Release commit confirmed on the release branch: `git log --oneline -1`.
- [ ] Tag created: `git tag -a v2026.0401.0 -m "Horcrux v2026.0401.0"`.
- [ ] Tag pushed to GitHub: `git push origin v2026.0401.0`.
- [ ] GitHub Release page created with:
  - [ ] Release notes pasted in.
  - [ ] All artifacts attached.
  - [ ] "Latest release" flag set.
- [ ] Release announcement prepared (blog post, GitHub Discussions, social media).

## 9. Post-Release Checks (within 48 hours)

- [ ] Download and verify install from GitHub release page.
- [ ] `horcrux --version` returns `2026.0401.0`.
- [ ] Monitor issue tracker for immediate post-release regressions.
- [ ] Close the M6 milestone on GitHub.
- [ ] Tag M6 issue as complete with dated status entry.

---

**Release Manager:** _name_
**Sign-off Date:** _YYYY-MM-DD_
**Tag:** `v2026.0401.0`
