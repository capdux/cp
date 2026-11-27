[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/keep-changelog

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:keep-a-changelog` ([https://github.com/oscarotero/keep-a-changelog](https://github.com/oscarotero/keep-a-changelog))（原库名 `keep-a-changelog` → CP 包名 `@cap/keep-changelog`）

零堆分配的 Keep a Changelog 1.1.0 规范解析、版本检索与发布标题生成引擎 — 洁净室 MIT 实现，对标 npm `keep-a-changelog` / `changelog-parser` 与 Rust `parse-changelog`。

Zero-allocation Keep a Changelog 1.1.0 parser, release lookup, section counter, and header formatter for CP — clean-room MIT parity with npm `keep-a-changelog`, `changelog-parser`, and Rust `parse-changelog`.

## 特性 / Features

- **Keep a Changelog 1.1.0 全兼容 / Full 1.1.0 Spec**：精准解析 `## [Unreleased]`、`## [1.2.0] - 2026-11-26`、`## [1.1.0] - 2026-10-01 [YANKED]` 及内联链接标题，自动在底部 `[1.2.0]: https://...` 参考链接处截断正文。
- **变更分类统计 / Section Counters**：自动识别 `### Added`、`### Changed`、`### Deprecated`、`### Removed`、`### Fixed`、`### Security` 并统计各分类下的 `- ` / `* ` 顶层条目数。
- **零堆分配 / Zero Heap Allocation**：所有解析字段直接零拷贝切片自原始 Markdown 文本。

## 安装 / Install

```bash
cap add @cap/keep-changelog
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `ChangeKind` | 枚举：`Added` / `Changed` / `Deprecated` / `Removed` / `Fixed` / `Security` / `Other` |
| `ReleaseInfo` | `{ version, date, is_unreleased, is_yanked, body, added_count, changed_count, fixed_count, security_count }` |
| `ParsedChangelog` | `{ releases[32], count }` + `releaseAt(i)` |
| `parseChangelog(md)` | 解析完整 Keep a Changelog 文档 |
| `findRelease(log, version)` | 按版本号或 `"Unreleased"` 查找发布记录 |
| `latestRelease(log)` | 获取最新非 Unreleased 发布记录 |
| `classifySection(heading)` | 将 `###` 分类标题映射为 `ChangeKind` |
| `formatReleaseHeader(version, date, yanked, out)` | 零分配生成标准 `## [1.2.0] - 2026-11-26` 标题 |

## 使用示例 / Usage

```cp
import { parseChangelog, findRelease, latestRelease, formatReleaseHeader } from "@cap/keep-changelog";

const log = parseChangelog(changelog_md);
const latest = latestRelease(log).?;
// latest.version == "1.2.0", latest.date == "2026-11-26", latest.added_count == 2

let buf: [128]u8 = undefined;
const hdr = formatReleaseHeader("1.3.0", "2026-12-01", false, buf[0..128]);
// => "## [1.3.0] - 2026-12-01"
```

## License

MIT
