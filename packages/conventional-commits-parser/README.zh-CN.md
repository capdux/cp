[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# conventional-commits-parser

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Conventional Commits 1.0.0 parser, formatter, and Semver bump recommender for CP

## 安装与导入

```bash
cap add conventional-commits-parser
```

```cp
import { isTypeChar } from "conventional-commits-parser";
```

## 核心导出 API

- `export enum SemverBump`
- `export fn isTypeChar(c: u8): bool;`
- `export fn isFooterTokenChar(c: u8): bool;`
- `export fn ccIsHspace(c: u8): bool;`
- `export fn isLineBlank(s: string, start: usize, end: usize): bool;`
- `export fn trimLineEnd(s: string, start: usize, end: usize): usize;`
- `export fn trimBlockEdges(s: string, start: usize, end: usize): string;`
- `export fn matchFooterLine(msg: string, line_start: usize, line_end: usize): FooterLineMatch;`
- `export fn parseCommit(msg: string): ParsedCommit;`
- `export fn isValidHeader(header: string): bool;`
- `export fn recommendedBump(commit: ParsedCommit): SemverBump;`
- `export fn bumpRank(b: SemverBump): u8;`
- `export fn highestBump(a: SemverBump, b: SemverBump): SemverBump;`
- `export fn appendByte(out: []u8, pos: usize, c: u8): usize;`
- `export fn formatCommitHeader(commit: ParsedCommit, out: []u8): string;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`conventional-commits-parser@1.0.0`)，许可证：MIT
