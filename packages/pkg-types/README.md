[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/pkg-types

Zero-heap `package.json` / `cap.json` manifest field parser and package name validator in pure CP.

## API

- `PkgManifest { name: string, version: string, license: string, main: string, module: string, entry: string, is_private: bool, valid: bool }`
- `PkgNameParts { scope: string, pkg_name: string, has_scope: bool }`
- `parsePkgManifest(json_text: string): PkgManifest`
- `isValidPkgName(name: string): bool`
- `splitPkgScope(name: string): PkgNameParts`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`pkg-types@1.3.1`)
- **Upstream Repository**: https://github.com/unjs/pkg-types
- **License**: MIT (clean-room CP implementation)
