[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# borsh

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation canonical little-endian Borsh binary serializer and deserializer for CP

## 安装与导入

```bash
cap add borsh
```

```cp
import { i32ToBits } from "borsh";
```

## 核心导出 API

- `export fn i32ToBits(v: i32): u32;`
- `export fn bitsToI32(u: u32): i32;`
- `export fn i64ToBits(v: i64): u64;`
- `export fn bitsToI64(u: u64): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`borsh@1.0.0`)，许可证：MIT
