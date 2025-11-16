# CI/CD Workflows for Android Examples Repository

This document describes the CI/CD workflows for the `horcrux-android-examples` repository.

## Overview

The Android examples repository includes two primary CI workflows:

1. **Build All Examples** - Validates that all examples build successfully
2. **Regression Tests** - Comprehensive testing of Android build capabilities

These workflows ensure that:
- Examples remain functional with the latest Horcrux version
- Android build system changes don't break existing functionality
- Examples serve as reliable integration tests

## Workflow 1: Build All Examples

**File**: `.github/workflows/build-all-examples.yml`

### Purpose

Builds all example projects to verify they compile successfully with the current Horcrux version.

### Triggers

- Push to `main` branch
- Pull requests to `main` branch
- Manual workflow dispatch
- Called by main Horcrux repository on changes

### Workflow Configuration

```yaml
name: Build All Examples

on:
  push:
    branches: [main]
  pull_request:
    branches: [main]
  workflow_dispatch:
    inputs:
      horcrux_version:
        description: 'Horcrux version to test (commit SHA or tag)'
        required: false
        default: 'latest'
  repository_dispatch:
    types: [trigger-build]

env:
  ANDROID_SDK_VERSION: '34'
  ANDROID_NDK_VERSION: '26.1.10909125'
  BUILD_TOOLS_VERSION: '34.0.0'

jobs:
  setup:
    name: Setup Build Environment
    runs-on: ubuntu-latest
    outputs:
      horcrux-version: ${{ steps.horcrux.outputs.version }}
    steps:
      - name: Checkout examples repository
        uses: actions/checkout@v4

      - name: Install Horcrux
        id: horcrux
        run: |
          if [ "${{ github.event.inputs.horcrux_version }}" != "" ] && \
             [ "${{ github.event.inputs.horcrux_version }}" != "latest" ]; then
            HORCRUX_VERSION="${{ github.event.inputs.horcrux_version }}"
          else
            HORCRUX_VERSION="latest"
          fi
          
          # Install Horcrux (from main repository)
          git clone https://github.com/horcruxsys/horcrux.git /tmp/horcrux
          cd /tmp/horcrux
          
          if [ "$HORCRUX_VERSION" != "latest" ]; then
            git checkout "$HORCRUX_VERSION"
          fi
          
          mkdir build && cd build
          cmake .. -DCMAKE_BUILD_TYPE=Release -G Ninja
          cmake --build . -j$(nproc)
          sudo cmake --install .
          
          # Output version for other jobs
          INSTALLED_VERSION=$(horcrux-cli --version | cut -d' ' -f2)
          echo "version=$INSTALLED_VERSION" >> $GITHUB_OUTPUT
          echo "Installed Horcrux version: $INSTALLED_VERSION"

      - name: Setup Android SDK
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}
          ndk: ${{ env.ANDROID_NDK_VERSION }}

      - name: Verify Android toolchain
        run: |
          export ANDROID_HOME=$ANDROID_SDK_ROOT
          export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/${{ env.ANDROID_NDK_VERSION }}
          horcrux-cli doctor android

  build-basic-xml:
    name: Build Basic XML App
    needs: setup
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux
        with:
          version: ${{ needs.setup.outputs.horcrux-version }}

      - name: Setup Android SDK
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}

      - name: Build Debug Variant
        run: |
          cd basic-xml-app
          horcrux build //app:debug

      - name: Build Release Variant
        run: |
          cd basic-xml-app
          horcrux build //app:release

      - name: Upload APKs
        uses: actions/upload-artifact@v4
        with:
          name: basic-xml-app-apks
          path: basic-xml-app/build/outputs/apk/**/*.apk

  build-compose:
    name: Build Compose App
    needs: setup
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux
        with:
          version: ${{ needs.setup.outputs.horcrux-version }}

      - name: Setup Android SDK
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}

      - name: Build Compose Debug
        run: |
          cd compose-app
          horcrux build //app:composeDebug --enable-compose-metrics

      - name: Build Compose Release
        run: |
          cd compose-app
          horcrux build //app:composeRelease

      - name: Upload Compose Metrics
        uses: actions/upload-artifact@v4
        with:
          name: compose-metrics
          path: compose-app/build/compose-metrics/**/*

      - name: Upload APKs
        uses: actions/upload-artifact@v4
        with:
          name: compose-app-apks
          path: compose-app/build/outputs/apk/**/*.apk

  build-ndk:
    name: Build NDK App
    needs: setup
    runs-on: ubuntu-latest
    strategy:
      matrix:
        abi: [arm64-v8a, armeabi-v7a, x86, x86_64]
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux
        with:
          version: ${{ needs.setup.outputs.horcrux-version }}

      - name: Setup Android SDK & NDK
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}
          ndk: ${{ env.ANDROID_NDK_VERSION }}

      - name: Build NDK App for ${{ matrix.abi }}
        run: |
          cd ndk-app
          export ANDROID_NDK_ROOT=$ANDROID_SDK_ROOT/ndk/${{ env.ANDROID_NDK_VERSION }}
          horcrux build //app:ndk-release --abi ${{ matrix.abi }}

      - name: Verify Native Libraries
        run: |
          cd ndk-app
          file build/libs/lib/${{ matrix.abi }}/*.so
          nm -D build/libs/lib/${{ matrix.abi }}/*.so | head -20

      - name: Upload Native Libraries
        uses: actions/upload-artifact@v4
        with:
          name: ndk-app-${{ matrix.abi }}
          path: ndk-app/build/libs/lib/${{ matrix.abi }}/*.so

  build-flavors:
    name: Build Flavor App
    needs: setup
    runs-on: ubuntu-latest
    strategy:
      matrix:
        variant:
          - freeDebug
          - freeRelease
          - paidDebug
          - paidRelease
          - enterpriseDebug
          - enterpriseRelease
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux
        with:
          version: ${{ needs.setup.outputs.horcrux-version }}

      - name: Setup Android SDK
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}

      - name: Build ${{ matrix.variant }}
        run: |
          cd flavor-app
          horcrux build //app:${{ matrix.variant }}

      - name: Upload APK
        uses: actions/upload-artifact@v4
        with:
          name: flavor-app-${{ matrix.variant }}
          path: flavor-app/build/outputs/apk/**/*.apk

  build-multi-module:
    name: Build Multi-Module App
    needs: setup
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux
        with:
          version: ${{ needs.setup.outputs.horcrux-version }}

      - name: Setup Android SDK
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}

      - name: Build All Modules
        run: |
          cd multi-module-app
          # Build library modules first
          horcrux build //libraries/...
          # Build feature modules
          horcrux build //features/...
          # Build app module
          horcrux build //app:release

      - name: Verify Module Dependencies
        run: |
          cd multi-module-app
          horcrux query //app:dependencies

      - name: Upload APK
        uses: actions/upload-artifact@v4
        with:
          name: multi-module-app-apk
          path: multi-module-app/build/outputs/apk/**/*.apk

  summary:
    name: Build Summary
    needs: [build-basic-xml, build-compose, build-ndk, build-flavors, build-multi-module]
    runs-on: ubuntu-latest
    if: always()
    steps:
      - name: Check Build Status
        run: |
          echo "## Build Results" >> $GITHUB_STEP_SUMMARY
          echo "" >> $GITHUB_STEP_SUMMARY
          echo "| Example | Status |" >> $GITHUB_STEP_SUMMARY
          echo "|---------|--------|" >> $GITHUB_STEP_SUMMARY
          echo "| Basic XML App | ${{ needs.build-basic-xml.result }} |" >> $GITHUB_STEP_SUMMARY
          echo "| Compose App | ${{ needs.build-compose.result }} |" >> $GITHUB_STEP_SUMMARY
          echo "| NDK App | ${{ needs.build-ndk.result }} |" >> $GITHUB_STEP_SUMMARY
          echo "| Flavor App | ${{ needs.build-flavors.result }} |" >> $GITHUB_STEP_SUMMARY
          echo "| Multi-Module App | ${{ needs.build-multi-module.result }} |" >> $GITHUB_STEP_SUMMARY
          
          # Fail if any build failed
          if [ "${{ needs.build-basic-xml.result }}" != "success" ] || \
             [ "${{ needs.build-compose.result }}" != "success" ] || \
             [ "${{ needs.build-ndk.result }}" != "success" ] || \
             [ "${{ needs.build-flavors.result }}" != "success" ] || \
             [ "${{ needs.build-multi-module.result }}" != "success" ]; then
            echo "❌ One or more builds failed"
            exit 1
          fi
          
          echo "✅ All builds passed!"
```

### Key Features

- **Parallel Builds**: Examples build in parallel for faster CI
- **Matrix Strategy**: NDK and flavor variants use matrix builds
- **Artifact Upload**: APKs and libraries uploaded for verification
- **Build Summary**: Clear summary of all build results
- **Version Control**: Can test specific Horcrux versions

## Workflow 2: Regression Tests

**File**: `.github/workflows/regression-tests.yml`

### Purpose

Comprehensive regression testing of Android build capabilities including:
- Incremental builds
- Cache verification
- Build reproducibility
- Performance benchmarks

### Triggers

- Scheduled: Daily at 2 AM UTC
- Manual workflow dispatch
- Repository dispatch from main Horcrux repo
- Tagged releases

### Workflow Configuration

```yaml
name: Android Regression Tests

on:
  schedule:
    - cron: '0 2 * * *'  # Daily at 2 AM UTC
  workflow_dispatch:
    inputs:
      horcrux_version:
        description: 'Horcrux version to test'
        required: false
        default: 'latest'
  repository_dispatch:
    types: [regression-test]

env:
  ANDROID_SDK_VERSION: '34'
  ANDROID_NDK_VERSION: '26.1.10909125'
  BUILD_TOOLS_VERSION: '34.0.0'

jobs:
  clean-build-test:
    name: Clean Build Test
    runs-on: ubuntu-latest
    strategy:
      matrix:
        example: [basic-xml-app, compose-app, ndk-app, flavor-app, multi-module-app]
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux

      - name: Setup Android
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}
          ndk: ${{ env.ANDROID_NDK_VERSION }}

      - name: Clean Build
        run: |
          cd ${{ matrix.example }}
          horcrux clean
          time horcrux build //...

  incremental-build-test:
    name: Incremental Build Test
    runs-on: ubuntu-latest
    strategy:
      matrix:
        example: [basic-xml-app, compose-app, multi-module-app]
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux

      - name: Setup Android
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}

      - name: Initial Build
        run: |
          cd ${{ matrix.example }}
          horcrux build //...

      - name: Touch Source File
        run: |
          cd ${{ matrix.example }}
          touch src/main/java/com/horcrux/example/MainActivity.java || \
          touch src/main/kotlin/com/horcrux/example/MainActivity.kt

      - name: Incremental Build (should be fast)
        run: |
          cd ${{ matrix.example }}
          START=$(date +%s)
          horcrux build //...
          END=$(date +%s)
          DURATION=$((END - START))
          echo "Incremental build took: ${DURATION}s"
          # Incremental build should be under 10 seconds
          if [ $DURATION -gt 10 ]; then
            echo "❌ Incremental build too slow: ${DURATION}s"
            exit 1
          fi

  cache-test:
    name: Cache Verification Test
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux

      - name: Setup Android
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}

      - name: First Build
        run: |
          cd basic-xml-app
          horcrux build //app:debug

      - name: Clean Build Directory
        run: |
          cd basic-xml-app
          rm -rf build/

      - name: Second Build (should use cache)
        run: |
          cd basic-xml-app
          START=$(date +%s)
          horcrux build //app:debug
          END=$(date +%s)
          DURATION=$((END - START))
          echo "Cached build took: ${DURATION}s"
          # Cached build should be under 5 seconds
          if [ $DURATION -gt 5 ]; then
            echo "❌ Cache not working properly: ${DURATION}s"
            exit 1
          fi

  reproducibility-test:
    name: Build Reproducibility Test
    runs-on: ubuntu-latest
    strategy:
      matrix:
        example: [basic-xml-app, compose-app]
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux

      - name: Setup Android
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}

      - name: First Build
        run: |
          cd ${{ matrix.example }}
          horcrux build //app:release
          cp build/outputs/apk/release/app-release.apk /tmp/build1.apk

      - name: Clean
        run: |
          cd ${{ matrix.example }}
          horcrux clean

      - name: Second Build
        run: |
          cd ${{ matrix.example }}
          horcrux build //app:release
          cp build/outputs/apk/release/app-release.apk /tmp/build2.apk

      - name: Compare Builds
        run: |
          if ! diff /tmp/build1.apk /tmp/build2.apk; then
            echo "❌ Builds are not reproducible!"
            echo "Build 1 hash: $(sha256sum /tmp/build1.apk)"
            echo "Build 2 hash: $(sha256sum /tmp/build2.apk)"
            exit 1
          fi
          echo "✅ Builds are reproducible"

  performance-benchmark:
    name: Build Performance Benchmark
    runs-on: ubuntu-latest
    steps:
      - name: Checkout
        uses: actions/checkout@v4

      - name: Install Horcrux
        uses: ./.github/actions/install-horcrux

      - name: Setup Android
        uses: android-actions/setup-android@v3
        with:
          api-level: ${{ env.ANDROID_SDK_VERSION }}
          build-tools: ${{ env.BUILD_TOOLS_VERSION }}
          ndk: ${{ env.ANDROID_NDK_VERSION }}

      - name: Benchmark All Examples
        run: |
          ./scripts/benchmark-builds.sh > benchmark-results.txt

      - name: Upload Benchmark Results
        uses: actions/upload-artifact@v4
        with:
          name: benchmark-results
          path: benchmark-results.txt

      - name: Comment Benchmark Results
        if: github.event_name == 'pull_request'
        uses: actions/github-script@v7
        with:
          script: |
            const fs = require('fs');
            const results = fs.readFileSync('benchmark-results.txt', 'utf8');
            github.rest.issues.createComment({
              issue_number: context.issue.number,
              owner: context.repo.owner,
              repo: context.repo.repo,
              body: '## Build Performance Benchmark\n\n```\n' + results + '\n```'
            });

  summary:
    name: Regression Test Summary
    needs: [clean-build-test, incremental-build-test, cache-test, reproducibility-test, performance-benchmark]
    runs-on: ubuntu-latest
    if: always()
    steps:
      - name: Generate Summary
        run: |
          echo "## Regression Test Results" >> $GITHUB_STEP_SUMMARY
          echo "" >> $GITHUB_STEP_SUMMARY
          echo "| Test | Status |" >> $GITHUB_STEP_SUMMARY
          echo "|------|--------|" >> $GITHUB_STEP_SUMMARY
          echo "| Clean Build | ${{ needs.clean-build-test.result }} |" >> $GITHUB_STEP_SUMMARY
          echo "| Incremental Build | ${{ needs.incremental-build-test.result }} |" >> $GITHUB_STEP_SUMMARY
          echo "| Cache Verification | ${{ needs.cache-test.result }} |" >> $GITHUB_STEP_SUMMARY
          echo "| Build Reproducibility | ${{ needs.reproducibility-test.result }} |" >> $GITHUB_STEP_SUMMARY
          echo "| Performance Benchmark | ${{ needs.performance-benchmark.result }} |" >> $GITHUB_STEP_SUMMARY
```

### Key Tests

1. **Clean Build Test**: Verifies all examples build from scratch
2. **Incremental Build Test**: Validates fast incremental compilation
3. **Cache Test**: Ensures build cache works correctly
4. **Reproducibility Test**: Verifies deterministic builds
5. **Performance Benchmark**: Tracks build performance over time

## Integration with Main Repository

The main Horcrux repository can trigger these workflows:

**In `horcruxsys/horcrux/.github/workflows/android-integration.yml`:**

```yaml
name: Android Integration Tests

on:
  pull_request:
    paths:
      - 'src/android/**'
      - 'src/core/android_*'
      - 'docs/android-*.md'
  push:
    branches: [main, production]
    paths:
      - 'src/android/**'
      - 'src/core/android_*'

jobs:
  trigger-examples:
    runs-on: ubuntu-latest
    steps:
      - name: Trigger Android Examples Build
        uses: actions/github-script@v7
        with:
          github-token: ${{ secrets.EXAMPLES_REPO_TOKEN }}
          script: |
            await github.rest.repos.createDispatchEvent({
              owner: 'horcruxsys',
              repo: 'horcrux-android-examples',
              event_type: 'trigger-build',
              client_payload: {
                horcrux_sha: context.sha,
                horcrux_ref: context.ref
              }
            });
            
      - name: Wait for Examples Build
        uses: fountainhead/action-wait-for-check@v1.1.0
        with:
          token: ${{ secrets.EXAMPLES_REPO_TOKEN }}
          checkName: Build All Examples
          repo: horcrux-android-examples
          owner: horcruxsys
          timeoutSeconds: 1800
```

## Local Testing

Test CI workflows locally using [act](https://github.com/nektos/act):

```bash
# Install act
brew install act

# Run build workflow
act -j build-basic-xml

# Run regression tests
act -j clean-build-test

# Run with specific Horcrux version
act -j build-all --input horcrux_version=v0.1.0
```

## Monitoring and Alerts

### Build Notifications

Configure Slack notifications for build failures:

```yaml
- name: Notify on Failure
  if: failure()
  uses: slackapi/slack-github-action@v1
  with:
    webhook-url: ${{ secrets.SLACK_WEBHOOK }}
    payload: |
      {
        "text": "❌ Android example build failed: ${{ github.workflow }}",
        "blocks": [
          {
            "type": "section",
            "text": {
              "type": "mrkdwn",
              "text": "*Build Failed:* ${{ github.workflow }}\n*Example:* ${{ matrix.example }}\n*Commit:* ${{ github.sha }}"
            }
          }
        ]
      }
```

### Performance Tracking

Track build performance over time:

```bash
# scripts/track-performance.sh
#!/bin/bash
echo "$(date +%Y-%m-%d),$(cat benchmark-results.txt)" >> performance-history.csv
git add performance-history.csv
git commit -m "Update performance metrics"
```

## Conclusion

These CI workflows ensure that:
- All Android examples remain functional
- Build system changes are validated against real projects
- Performance regressions are detected early
- Examples serve as reliable integration tests

For questions or issues with CI, see the [CI Integration Guide](ci-integration.md) or open an issue in the main Horcrux repository.
