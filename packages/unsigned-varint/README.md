[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# unsigned-varint

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unsigned-varint` ([https://github.com/paritytech/unsigned-varint](https://github.com/paritytech/unsigned-varint))（原库名 `unsigned-varint` → CP 包名 `unsigned-varint`）

Zero-allocation 3-in-1 variable-length integer codec (ULEB128, SLEB128, Protobuf ZigZag, and QUIC RFC 9000 varint) for CP — clean-room MIT parity with Rust `unsigned-varint`, `leb128`, and `integer-encoding`.

零堆分配的 CP 变长整数 3 合 1 编解码引擎，统一实现 Unsigned/Signed LEB128（WASM / Protobuf / DWARF / libp2p）、ZigZag 有符号编码（Protobuf `sint64`）与 QUIC RFC 9000 2-bit 前缀变长整数，严格防 64-bit 溢出与截断流（纯 CP 洁净室 MIT 实现）。

## Install / 安装

```bash
cap add unsigned-varint
```

## API Reference / 接口一览

| Symbol | Signature | Description |
| :--- | :--- | :--- |
| `U64Decode` / `I64Decode` | `pub struct { ok: bool, value, bytes_read: usize }` | 零堆解码结果结构体 |
| `encodeUleb128` / `decodeUleb128` / `uleb128Size` | ULEB128 编解码与长度计算 | 严格拒绝第 10 字节 `> 0x01` 溢出与截断流 |
| `zigzagEncode` / `zigzagDecode` | `(i64) -> u64` / `(u64) -> i64` | 覆盖全 `i64` 范围（含 `i64::MIN` / `i64::MAX`） |
| `encodeSleb128` / `decodeSleb128` | SLEB128 有符号编解码 | 支持完整 64-bit 二补码符号扩展与溢出校验 |
| `encodeZigzagVarint` / `decodeZigzagVarint` | Protobuf `sint64` 编解码 | ZigZag + ULEB128 组合编解码 |
| `encodeQuicVarint` / `decodeQuicVarint` | QUIC RFC 9000 变长整数 | 1/2/4/8 字节大端变长整数（`0 .. 2^62-1`） |

## Usage / 使用示例

```cp
import { encodeUleb128, decodeUleb128, encodeQuicVarint, decodeQuicVarint } from "unsigned-varint";

let buf: [16]u8 = undefined;
const n = encodeUleb128(624485, buf[0..16]).?;
const u = decodeUleb128(buf[0..n]); // u.ok == true, u.value == 624485

const qn = encodeQuicVarint(15293, buf[0..16]).?; // 2 bytes: 0x7b, 0xbd
const qd = decodeQuicVarint(buf[0..qn]);          // qd.value == 15293
```

## License

MIT
