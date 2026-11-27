[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# unsigned-varint

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation 3-in-1 variable-length integer codec (ULEB128, SLEB128, Protobuf ZigZag, and QUIC RFC 9000 varint) for CP

## 安装与导入

```bash
cap add unsigned-varint
```

```cp
import { i64ToBits } from "unsigned-varint";
```

## 核心导出 API

- `export fn i64ToBits(v: i64): u64;`
- `export fn bitsToI64(u: u64): i64;`
- `export fn uleb128Size(v: u64): usize;`
- `export fn encodeUleb128(v: u64, out: []u8): usize?;`
- `export fn decodeUleb128(data: string): U64Decode;`
- `export fn zigzagEncode(v: i64): u64;`
- `export fn zigzagDecode(u: u64): i64;`
- `export fn encodeSleb128(v: i64, out: []u8): usize?;`
- `export fn decodeSleb128(data: string): I64Decode;`
- `export fn encodeZigzagVarint(v: i64, out: []u8): usize?;`
- `export fn decodeZigzagVarint(data: string): I64Decode;`
- `export fn encodeQuicVarint(v: u64, out: []u8): usize?;`
- `export fn decodeQuicVarint(data: string): U64Decode;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`unsigned-varint@1.0.0`)，许可证：MIT
