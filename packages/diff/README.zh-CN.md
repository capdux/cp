[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# diff

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Myers O(ND) line & character diff engine with FNV-1a hash pre-filtering, GNU Unified Diff formatting, and lossless patch application for CP

## 安装与导入

```bash
cap add diff
```

```cp
import { fnv1a64 } from "diff";
```

## 核心导出 API

- `export enum DiffOp`
- `export fn fnv1a64(data: &[]u8): u64;`
- `export fn linesEqual(a: LineEntry, b: LineEntry): bool;`
- `export fn splitLines(text: &[]u8, out: []LineEntry): usize ! DiffError;`
- `export fn traceSlot(d: usize, k: i32): usize;`
- `export fn myersDiffEntries(old_entries: &[]LineEntry, new_entries: &[]LineEntry): LineDiffResult ! DiffError;`
- `export fn diffLines(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;`
- `export fn diffChars(old_text: &[]u8, new_text: &[]u8): LineDiffResult ! DiffError;`
- `export fn formatUnifiedDiff(old_name: &[]u8, new_name: &[]u8, old_text: &[]u8, new_text: &[]u8, context_lines: usize, out: []u8): &[]u8 ! DiffError;`
- `export fn applyLineDiff(old_text: &[]u8, diff_res: LineDiffResult, out: []u8): &[]u8 ! DiffError;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`diff@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`fast-diff@1.0.0`)，许可证：Apache-2.0
