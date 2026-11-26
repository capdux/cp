# @cap/debugid

Zero-heap Sentry/Breakpad/PDB `DebugId` (128-bit UUID + appendix age) parser and formatter in pure CP.

## API

- `parseDebugId(s: string): DebugId` — parses Breakpad (`33..40` hex) and canonical hyphenated UUID (`uuid[-appendix]`) debug IDs.
- `formatBreakpad(id: DebugId, out: []u8): usize` — formats uppercase Breakpad ID.
- `formatCanonical(id: DebugId, out: []u8): usize` — formats lowercase Sentry canonical ID.

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`debugid@0.8.0`)
- **Upstream Repository**: https://github.com/getsentry/rust-debugid
- **License**: Apache-2.0 (clean-room CP implementation)
