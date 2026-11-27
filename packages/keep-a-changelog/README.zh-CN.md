[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# keep-a-changelog

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Keep a Changelog 1.1.0 parser, release lookup, section counter, and header formatter (keep-a-changelog / changelog-parser / Rust parse-changelog parity) for CP

## 安装与导入

```bash
cap add keep-a-changelog
```

```cp
import { isSpace } from "keep-a-changelog";
```

## 核心导出 API

- `export enum ChangeKind`
- `export fn isSpace(c: u8): bool;`
- `export fn toLowerAscii(c: u8): u8;`
- `export fn trim(s: string): string;`
- `export fn eqIgnoreCase(a: string, b: string): bool;`
- `export fn containsIgnoreCase(hay: string, needle_lower: string): bool;`
- `export fn isRefLinkDef(trimmed: string): bool;`
- `export fn classifySection(heading: string): ChangeKind;`
- `export fn parseReleaseHeading(rest_raw: string): ReleaseInfo;`
- `export fn isTopLevelBullet(line: string): bool;`
- `export fn parseChangelog(md: string): ParsedChangelog;`
- `export fn findRelease(log: ParsedChangelog, version: string): ReleaseInfo?;`
- `export fn latestRelease(log: ParsedChangelog): ReleaseInfo?;`
- `export fn pushStr(out: []u8, pos: usize, s: string): usize;`
- `export fn formatReleaseHeader(version: string, date: string, yanked: bool, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`keep-a-changelog@1.0.0`)，许可证：MIT
