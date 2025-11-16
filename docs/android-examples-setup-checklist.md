# Android Examples Repository Setup Checklist

This checklist guides the creation of the `horcrux-android-examples` repository as specified in the issue.

## Repository Creation

- [ ] Create new repository: `horcruxsys/horcrux-android-examples`
- [ ] Set repository description: "Production-ready Android example projects for the Horcrux build system"
- [ ] Add topics: `android`, `build-system`, `horcrux`, `examples`, `jetpack-compose`, `ndk`, `kotlin`
- [ ] Set license to MIT (same as main repository)
- [ ] Initialize with README from `docs/horcrux-android-examples-README.md`

## Example Project 1: Basic XML App

- [ ] Create `basic-xml-app/` directory
- [ ] Implement simple Android app with:
  - [ ] Main Activity with XML layout
  - [ ] String resources (English + one other language)
  - [ ] Material Design components
  - [ ] Navigation to second activity
  - [ ] Resource qualifiers (drawable-hdpi, drawable-xhdpi, etc.)
  - [ ] Debug and release build types
- [ ] Create `horcrux.yaml` configuration
- [ ] Add ProGuard/R8 rules for release build
- [ ] Write comprehensive README.md
- [ ] Add unit tests (JUnit)
- [ ] Add instrumentation tests (Espresso)
- [ ] Add parallel Gradle configuration in `gradle-reference/`
- [ ] Verify build: `horcrux build //app:debug`
- [ ] Verify build: `horcrux build //app:release`
- [ ] Generate and test APK installation

## Example Project 2: Compose App

- [ ] Create `compose-app/` directory
- [ ] Implement Jetpack Compose app with:
  - [ ] Material 3 theming
  - [ ] Multiple composable screens
  - [ ] Compose navigation
  - [ ] State management patterns
  - [ ] Animation examples
  - [ ] Preview functions
  - [ ] ViewModel integration
- [ ] Create `horcrux.yaml` with Compose compiler config
- [ ] Enable Compose metrics and reports
- [ ] Write comprehensive README.md
- [ ] Add unit tests for ViewModels
- [ ] Add Compose UI tests
- [ ] Add parallel Gradle configuration in `gradle-reference/`
- [ ] Verify build: `horcrux build //app:composeDebug --enable-compose-metrics`
- [ ] Verify build: `horcrux build //app:composeRelease`
- [ ] Review Compose metrics output

## Example Project 3: NDK App

- [ ] Create `ndk-app/` directory
- [ ] Implement Android app with native code:
  - [ ] Java/Kotlin Activity
  - [ ] JNI bindings
  - [ ] C++ native library with:
    - [ ] Mathematical functions
    - [ ] String manipulation
    - [ ] Image processing demo
  - [ ] Native thread examples
- [ ] Create `horcrux.yaml` with NDK configuration
- [ ] Configure multi-ABI builds
- [ ] Write comprehensive README.md
- [ ] Add C++ unit tests
- [ ] Add JNI integration tests
- [ ] Add parallel Gradle configuration with CMake in `gradle-reference/`
- [ ] Verify build for all ABIs: `horcrux build //app:ndk-debug --all-abis`
- [ ] Verify each ABI: arm64-v8a, armeabi-v7a, x86, x86_64
- [ ] Test native library loading on device/emulator

## Example Project 4: Flavor App

- [ ] Create `flavor-app/` directory
- [ ] Implement multi-flavor Android app with:
  - [ ] 3 product flavors: free, paid, enterprise
  - [ ] 2 flavor dimensions: tier, api-level
  - [ ] 3 build types: debug, release, staging
  - [ ] Flavor-specific:
    - [ ] Resources (icons, strings, colors)
    - [ ] Source code (different features)
    - [ ] Dependencies (analytics in paid only, etc.)
    - [ ] Application ID suffixes
  - [ ] Manifest merging examples
- [ ] Create `horcrux.yaml` with variant matrix
- [ ] Write comprehensive README.md
- [ ] Document all 12 variants
- [ ] Add tests per variant
- [ ] Add parallel Gradle configuration in `gradle-reference/`
- [ ] Verify: `horcrux query //app:variants` (list all)
- [ ] Verify build for each variant:
  - [ ] `horcrux build //app:freeDebug`
  - [ ] `horcrux build //app:paidRelease`
  - [ ] `horcrux build //app:enterpriseStaging`
  - [ ] Test other variants...
- [ ] Verify variant-specific resources in output

## Example Project 5: Multi-Module App

- [ ] Create `multi-module-app/` directory with structure:
  - [ ] `app/` - Main application module
  - [ ] `features/feature-home/` - Home feature module
  - [ ] `features/feature-profile/` - Profile feature module
  - [ ] `features/feature-settings/` - Settings feature module
  - [ ] `libraries/lib-network/` - Network library (Retrofit/OkHttp)
  - [ ] `libraries/lib-database/` - Database library (Room)
  - [ ] `libraries/lib-analytics/` - Analytics library
  - [ ] `shared/shared-ui/` - Shared UI components
  - [ ] `shared/shared-models/` - Shared data models
  - [ ] `shared/shared-utils/` - Shared utilities
- [ ] Implement clear module boundaries
- [ ] Set up dependency injection (Hilt/Koin)
- [ ] Create root `horcrux.yaml` with all modules
- [ ] Write comprehensive README.md
- [ ] Add per-module tests
- [ ] Add integration tests
- [ ] Add parallel Gradle multi-project setup in `gradle-reference/`
- [ ] Verify: `horcrux build //libraries/...` (all library modules)
- [ ] Verify: `horcrux build //features/...` (all feature modules)
- [ ] Verify: `horcrux build //app:release` (full app)
- [ ] Verify: `horcrux query //app:dependencies` (dependency tree)
- [ ] Test incremental builds (change one module, rebuild)

## Documentation

- [ ] Create `docs/` directory with:
  - [ ] `migration-guide.md` - Step-by-step Gradle to Horcrux migration
  - [ ] `best-practices.md` - Android build best practices with Horcrux
  - [ ] `ci-integration.md` - CI/CD setup guide
  - [ ] `troubleshooting.md` - Common issues and solutions
- [ ] Create main README.md (use template from this repo)
- [ ] Create CONTRIBUTING.md
- [ ] Create LICENSE (MIT)
- [ ] Create .gitignore for Android projects

## CI/CD Setup

- [ ] Create `.github/workflows/` directory
- [ ] Create `build-all-examples.yml` workflow:
  - [ ] Job for basic-xml-app (debug + release)
  - [ ] Job for compose-app (debug + release + metrics)
  - [ ] Job for ndk-app (all ABIs)
  - [ ] Job for flavor-app (all variants)
  - [ ] Job for multi-module-app (full build)
  - [ ] Artifact upload for APKs
  - [ ] Build summary generation
- [ ] Create `regression-tests.yml` workflow:
  - [ ] Clean build tests
  - [ ] Incremental build tests
  - [ ] Cache verification tests
  - [ ] Build reproducibility tests
  - [ ] Performance benchmarks
- [ ] Create reusable action: `.github/actions/install-horcrux/`
- [ ] Test workflows locally with `act`
- [ ] Enable workflow on push to main
- [ ] Test workflow with pull request

## Scripts

- [ ] Create `scripts/` directory with:
  - [ ] `build-all.sh` - Build all examples sequentially
  - [ ] `test-all.sh` - Run all tests
  - [ ] `verify-ci.sh` - Verify CI configuration locally
  - [ ] `benchmark-builds.sh` - Benchmark all example builds
  - [ ] `clean-all.sh` - Clean all build outputs
- [ ] Make all scripts executable: `chmod +x scripts/*.sh`
- [ ] Test each script locally

## Integration with Main Repository

- [ ] In main horcrux repo, add reference to examples in README.md ✅
- [ ] In main horcrux repo, update examples/README.md ✅
- [ ] In main horcrux repo, create docs/android-examples.md ✅
- [ ] In main horcrux repo, update CONTRIBUTING.md ✅
- [ ] In main horcrux repo, create android-integration workflow:
  - [ ] Trigger examples build on Android code changes
  - [ ] Wait for examples build to complete
  - [ ] Report status back to PR
- [ ] Set up repository secrets:
  - [ ] `EXAMPLES_REPO_TOKEN` for cross-repo workflow triggers
  - [ ] `SLACK_WEBHOOK` for build notifications (optional)

## Testing & Validation

- [ ] Clone repository fresh and follow README
- [ ] Build each example from scratch
- [ ] Verify all examples build successfully
- [ ] Install APKs on real device/emulator
- [ ] Test basic functionality of each app
- [ ] Verify incremental builds are fast
- [ ] Verify cache works correctly
- [ ] Run full CI locally
- [ ] Test cross-platform (Linux, macOS if possible)

## Release & Announcement

- [ ] Tag initial release as v0.1.0
- [ ] Create GitHub release with release notes
- [ ] Announce in main repository discussions
- [ ] Update main repository README with link
- [ ] Share in Android developer communities (optional)
- [ ] Write blog post about examples (optional)

## Maintenance Plan

- [ ] Set up Dependabot for dependency updates
- [ ] Schedule monthly review of Android SDK/tools versions
- [ ] Plan quarterly updates for new Android features
- [ ] Monitor main Horcrux repo for breaking changes
- [ ] Update examples when new Horcrux features are added

## Acceptance Criteria Verification

From the original issue, verify:

- [x] Examples repository documented and specified
- [ ] Basic XML app example created and builds successfully
- [ ] Compose app example created and builds successfully
- [ ] NDK app example created and builds successfully
- [ ] Flavor app example created and builds successfully
- [ ] Multi-module app example created and builds successfully
- [ ] All examples used in CI for Android regression tests
- [ ] CI workflows created and tested
- [ ] Documentation complete and comprehensive
- [ ] Integration with main repository set up

## Notes

- All examples should be production-ready, not toy applications
- Code should follow Android best practices
- Include comprehensive documentation in each example
- Each example should be independently buildable
- Examples serve dual purpose: learning and testing
- Keep dependencies minimal but realistic
- Update examples when Android best practices evolve

## Timeline Estimate

- Repository setup: 1 day
- Basic XML App: 2 days
- Compose App: 3 days
- NDK App: 3 days
- Flavor App: 2 days
- Multi-Module App: 4 days
- Documentation: 2 days
- CI/CD setup: 2 days
- Testing & validation: 2 days
- **Total: ~3 weeks of focused development**

---

**Status**: Documentation phase complete ✅  
**Next Steps**: Create horcrux-android-examples repository and implement examples

For questions or issues, see [docs/android-examples.md](android-examples.md) or open an issue in the main Horcrux repository.
