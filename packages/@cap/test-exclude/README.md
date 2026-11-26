# @cap/test-exclude

Zero-heap test and coverage file path glob filter engine in pure CP.

## API

- `ExcludeConfig { includes: []const string, excludes: []const string, extensions: []const string }`
- `matchSimpleGlob(pattern: string, path: string): bool`
- `shouldInstrument(rel_path: string, cfg: ExcludeConfig): bool`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`test-exclude@7.0.1`)
- **Upstream Repository**: https://github.com/istanbuljs/test-exclude
- **License**: ISC (clean-room CP implementation)
