[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# zigzag

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 32-bit/64-bit ZigZag bijection and ZigZag-Varint streaming codec (Protobuf/Avro/Parquet/CBOR) in pure CP

## 安装与导入

```bash
cap add zigzag
```

```cp
import { encodeI32 } from "zigzag";
```

## 核心导出 API

- `export fn encodeI32(n: i32): u32;`
- `export fn decodeI32(u: u32): i32;`
- `export fn encodeI64(n: i64): u64;`
- `export fn decodeI64(u: u64): i64;`
- `export fn encodeZigzagVarint(n: i64, out: []u8): usize;`
- `export fn decodeZigzagVarint(data: &[]u8, start: usize): VarintResult;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`zigzag@1.0.0`)，许可证：MIT
