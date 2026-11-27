[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# google-protobuf

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Protocol Buffers Wire Format encoder/decoder reusing unsigned-varint (ULEB128 + ZigZag) for CP

## 安装与导入

```bash
cap add google-protobuf
```

```cp
import { isValidWireType } from "google-protobuf";
```

## 核心导出 API

- `export fn isValidWireType(wt: u8): bool;`
- `export fn encodeTag(field_number: u32, wire_type: u8, out: []u8): usize;`
- `export fn decodeTag(buf: &[]u8): FieldTag;`
- `export fn encodeVarintField(field_number: u32, val: u64, out: []u8): usize;`
- `export fn encodeSint64Field(field_number: u32, val: i64, out: []u8): usize;`
- `export fn decodeVarintValue(buf: &[]u8, pos: usize): U64Decode;`
- `export fn decodeSint64Value(buf: &[]u8, pos: usize): Sint64Decode;`
- `export fn encodeFixed32Field(field_number: u32, val: u32, out: []u8): usize;`
- `export fn decodeFixed32(buf: &[]u8, pos: usize): u32;`
- `export fn encodeFixed64Field(field_number: u32, val: u64, out: []u8): usize;`
- `export fn decodeFixed64(buf: &[]u8, pos: usize): u64;`
- `export fn encodeBytesField(field_number: u32, payload: string, out: []u8): usize;`
- `export fn decodeBytesField(buf: &[]u8, pos: usize): BytesField;`
- `export fn skipField(buf: &[]u8, pos: usize, wire_type: u8): i64;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`google-protobuf@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`prost@1.0.0`)，许可证：MIT
- **上游生态**：`npm` (`pbf@1.0.0`)，许可证：MIT
