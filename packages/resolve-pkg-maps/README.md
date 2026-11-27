[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/resolve-pkg-maps

Zero-heap `package.json` `"exports"` subpath wildcard and condition resolver in pure CP.

## API

- `ExportRule { pattern: string, condition: string, target: string }`
- `matchSubpathPattern(pattern: string, subpath: string, matched_star: []u8): i64`
- `resolveExportTarget(rules: []const ExportRule, subpath: string, conditions: []const string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`resolve-pkg-maps@1.0.0`)
- **Upstream Repository**: https://github.com/privatenumber/resolve-pkg-maps
- **License**: MIT (clean-room CP implementation)
