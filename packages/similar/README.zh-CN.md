[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# similar

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation line-level LCS diff, unified diff formatter, and similarity ratio engine for CP (clean-room MIT, inspired by Rust similar v2).

## 安装与导入

```bash
cap add similar
```

```cp
import { smdSplitLines } from "similar";
```

## 核心导出 API

- `export fn smdSplitLines(text: string, out_lines: []SmdLine): usize;`
- `export fn smdLineEq(a: string, la: SmdLine, b: string, lb: SmdLine): bool;`
- `export fn smdEmitOp(ops_out: []DiffOp, stats: &mut DiffStats, tag: u8, old_idx: usize, new_idx: usize);`
- `export fn smdWriteBytes(out: []u8, pos: usize, src: string, start: usize, end: usize): usize;`
- `export fn smdWriteStr(out: []u8, pos: usize, s: string): usize;`
- `export fn smdWriteByte(out: []u8, pos: usize, b: u8): usize;`
- `export fn smdWriteUsize(out: []u8, pos: usize, val: usize): usize;`
- `export fn diffLines(old_text: string, new_text: string, ops_out: []DiffOp): DiffStats;`
- `export fn hasChanges(old_text: string, new_text: string): bool;`
- `export fn similarityRatio(old_text: string, new_text: string): u32;`
- `export fn formatUnifiedDiff(old_name: string, new_name: string, old_text: string, new_text: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`similar@1.0.0`)，许可证：MIT
