[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# radsort

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation O(N) 8-bit LSD radix sort for u32, i32, u64, i64, and IEEE-754 f64 with pass skipping for CP

## 安装与导入

```bash
cap add radsort
```

```cp
import { i32ToSortable } from "radsort";
```

## 核心导出 API

- `export fn i32ToSortable(v: i32): u32;`
- `export fn i64ToSortable(v: i64): u64;`
- `export fn f64ToSortable(v: f64): u64;`
- `export fn sortU32(data: []u32, scratch: []u32): bool;`
- `export fn sortI32(data: []i32, scratch: []i32): bool;`
- `export fn sortU64(data: []u64, scratch: []u64): bool;`
- `export fn sortI64(data: []i64, scratch: []i64): bool;`
- `export fn sortF64(data: []f64, scratch: []f64): bool;`
- `export fn isSortedI64(data: &[]i64): bool;`
- `export fn isSortedU64(data: &[]u64): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`radsort@1.0.0`)，许可证：MIT
