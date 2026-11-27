[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/jest-docblock

Zero-heap top-of-file docblock (`/** @pragma val */`) extractor, parser, and formatter in pure CP.

## API

- `DocPragma { key: string, value: string }`
- `extractDocblock(code: string): string`
- `parseDocblock(docblock: string, out: []DocPragma): usize`
- `printDocblock(pragmas: []const DocPragma, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`jest-docblock@29.7.0`)
- **Upstream Repository**: https://github.com/jestjs/jest
- **License**: MIT (clean-room CP implementation)
