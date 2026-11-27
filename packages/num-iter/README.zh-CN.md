[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# num-iter

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap stepped numeric range iterators for signed and unsigned 64-bit integers with overflow-safe inclusive/exclusive bounds (clean-room port of Rust num-iter)

## 安装与导入

```bash
cap add num-iter
```

```cp
import { nitrAbsI64 } from "num-iter";
```

## 核心导出 API

- `export fn nitrAbsI64(v: i64): u64;`
- `export fn nitrCheckedAddI64(a: i64, b: i64): i64?;`
- `export fn nitrDiffU64(hi: i64, lo: i64): u64;`
- `export fn nitrAdvanceSigned(state: i64, step: i64, n: usize): i64;`
- `export fn rangeStep(start: i64, stop: i64, step: i64): RangeStep;`
- `export fn rangeStepInclusive(start: i64, stop: i64, step: i64): RangeStepInclusive;`
- `export fn rangeStepU64(start: u64, stop: u64, step: u64): RangeStepU64;`
- `export fn collectRangeStep(r: &mut RangeStep, out: []i64): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`num-iter@1.0.0`)，许可证：MIT OR Apache-2.0
