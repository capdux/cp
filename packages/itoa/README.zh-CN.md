[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# itoa

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Fast zero-heap two-digits-per-step integer-to-ASCII formatter (clean-room CP implementation of crates.io itoa)

## 安装与导入

```bash
cap add itoa
```

```cp
import { formatU64 } from "itoa";
```

## 核心导出 API

- `export fn formatU64(val: u64, out: []u8): usize;`
- `export fn formatI64(val: i64, out: []u8): usize;`
- `export fn formatU32(val: u32, out: []u8): usize;`
- `export fn formatI32(val: i32, out: []u8): usize;`
- `export fn formatUsize(val: usize, out: []u8): usize;`
- `export fn formatHexU64(val: u64, uppercase: bool, out: []u8): usize;`
- `export fn formatRadixU64(val: u64, radix: u8, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`itoa@1.0.0`)，许可证：MIT
