[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/browserslist-query

Zero-heap Browserslist target query clause parser and version matcher in pure CP.

## API

- `BrowserOp` enum (`Ge`, `Gt`, `Le`, `Lt`, `Eq`, `LastN`, `Defaults`, `Dead`, `Unknown`)
- `BrowserClause { negated: bool, is_and: bool, browser: string, op: BrowserOp, version_major: u32, version_minor: u32, count: u32 }`
- `parseBrowserClause(clause: string): BrowserClause`
- `matchBrowserVersion(clause: BrowserClause, browser: string, major: u32, minor: u32, latest_major: u32): bool`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`browserslist@4.24.4`)
- **Upstream Repository**: https://github.com/browserslist/browserslist
- **License**: MIT (clean-room CP implementation)
