# Release Process

This document describes the Horcrux release branch strategy, version policy,
freeze windows, and blocker severity rubric.

## Versioning Policy

Horcrux uses **CalVer** with the format `YYYY.MMDD.PATCH`:

| Segment | Meaning |
|---------|---------|
| `YYYY` | Four-digit release year |
| `MMDD` | Zero-padded month and day of the planned release |
| `PATCH` | Patch increment within the same calendar day (`0`, `1`, …) |

Examples: `2026.0401.0`, `2026.0401.1` (same-day hotfix), `2026.0715.0`.

### Support Policy

| Release type | Support window |
|--------------|----------------|
| Stable (`YYYY.MMDD.0`) | 12 months of security and critical fixes |
| Patch (`YYYY.MMDD.N`, N > 0) | Superseded by the next patch in the same series |
| Pre-release (`-alpha`, `-rc.N`) | No support guarantee |

## Branch Strategy

```
main  ─────────────────────────────────────────────▶ integration
        │                               │
        ├─ release/2026.0401  ─────────►┤  release branch
        │        │                       │
        │        ├─ rc.1 tag             │
        │        ├─ rc.2 tag             │
        │        └─ v2026.0401.0 tag ───►│
        │                                │
        └─ hotfix/2026.0401.1 ──────────►│  post-release hotfix
```

### Branch Naming

| Branch | Purpose |
|--------|---------|
| `main` | Trunk; always green, never force-pushed |
| `release/YYYY.MMDD` | Stabilisation branch cut from `main` |
| `hotfix/YYYY.MMDD.N` | Urgent post-release fixes; branched from the release tag |

### Cutting a Release Branch

```bash
# Cut the release branch from main
git checkout main && git pull
git checkout -b release/2026.0401
git push -u origin release/2026.0401
```

Only **blocker-severity fixes** (see below) are merged into a release branch
after it is cut. All other work continues on `main`.

## Freeze Windows

| Window | Trigger | Allowed merges |
|--------|---------|----------------|
| Feature freeze | Branch cut | Blocker fixes only |
| Code freeze | RC.1 tag | P0 critical blockers only |
| Release freeze | Final RC tag | No code changes; docs and release notes only |

## Release Candidate Process

1. Cut the release branch and push `rc.1` tag.
2. Run full CI matrix on the release branch.
3. Enable sanitizer and static-analysis gates (AddressSanitizer, UBSan).
4. Execute smoke tests on all supported platforms (Linux x86-64, macOS arm64, Windows x64).
5. If blockers are found, fix on release branch, increment RC counter, and re-run.
6. When CI is green and no open blockers remain, tag the final release.

```bash
# Tag a release candidate
git tag -a v2026.0401.0-rc.1 -m "Release candidate 1 for v2026.0401.0"
git push origin v2026.0401.0-rc.1

# Tag the final release
git tag -a v2026.0401.0 -m "Horcrux v2026.0401.0"
git push origin v2026.0401.0
```

## Blocker Severity Rubric

Use this rubric to classify issues against the release gate. Only **P0** and
**P1** issues can block a release. All other issues are deferred.

| Severity | Label | Definition | Example |
|----------|-------|------------|---------|
| P0 — Critical | `severity:p0` | Data loss, silent correctness failure, security vulnerability, complete build failure on a supported platform | Cache returns stale artifact silently |
| P1 — Blocker | `severity:p1` | Feature regression against the previous release; no acceptable workaround | `horcrux build` exits non-zero on a clean project |
| P2 — Major | `severity:p2` | Significant usability regression; workaround exists | Error message is unhelpful; command behaves inconsistently |
| P3 — Minor | `severity:p3` | Cosmetic or edge-case issue; no user impact on the critical path | Typo in help text; non-fatal warning in log |

### Go / No-Go Decision

The release manager runs the go/no-go check immediately before tagging:

1. Zero open P0 issues on the release branch.
2. Zero open P1 issues on the release branch.
3. Full CI matrix green for at least two consecutive RC runs.
4. All smoke tests pass on all supported platforms.
5. Release checklist (see `docs/release-checklist.md`) is fully signed off.

## Backport Policy

Fixes are backported to the active stable release branch when:

- The issue is P0 or P1 severity.
- The fix applies cleanly (or with minimal conflict resolution).
- The fix has been validated on `main` first.

Backports are submitted as pull requests targeting the release branch and
require at least one additional reviewer approval.

## Hotfix Policy

For critical (P0) issues discovered post-release:

```bash
git checkout v2026.0401.0
git checkout -b hotfix/2026.0401.1
# ... apply fix ...
git tag -a v2026.0401.1 -m "Horcrux v2026.0401.1"
git push origin hotfix/2026.0401.1 v2026.0401.1
```

Hotfix releases increment the `PATCH` segment and include only the minimal fix.

## Changelog Maintenance

The `CHANGELOG.md` in the repository root is the authoritative release log.
Every release must include a changelog entry before the tag is pushed. See
[Keep a Changelog](https://keepachangelog.com/en/1.1.0/) for format guidance.

## Related Documents

- [Release Checklist](release-checklist.md)
- [CI Documentation](ci.md)
- [Benchmark Methodology](benchmark-methodology.md)
