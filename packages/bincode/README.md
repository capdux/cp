[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/bincode

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:bincode` ([https://github.com/bincode-org/bincode](https://github.com/bincode-org/bincode))

零堆分配的 CP 紧凑二进制序列化与零拷贝反序列化引擎 — 洁净室 MIT 实现，对标 Rust crates.io #381 `bincode` v2（3.91 亿+ 总下载），专为高性能 RPC 与持久化打造。

Zero-allocation bincode v2 compact binary serializer & zero-copy decoder for CP — clean-room MIT parity with Rust `bincode` v2 (391M+ downloads).

## 安装 / Install

```bash
cap add @cap/bincode
```

## API 参考 / API Reference

- **`BincodeEncoder`**：`init(out)`, `asSlice()`, `encodeBool`, `encodeU64` (v2 varint), `encodeI64` (ZigZag + varint), `encodeStr`, `encodeOptStr`, `encodeFixedU32`, `encodeFixedU64`
- **`BincodeDecoder`**：`init(data)`, `remaining()`, `atEnd()`, `decodeBool`, `decodeU64`, `decodeI64`, `decodeStr`（零拷贝）, `decodeOptStr` (`OptStrResult`), `decodeFixedU32`, `decodeFixedU64`

## 使用示例 / Usage

```cp
import { BincodeEncoder, BincodeDecoder } from "@cap/bincode";

let buf: [128]u8 = undefined;
let enc = BincodeEncoder.init(buf[0..128]);
* = enc.encodeU64(42 as u64);
* = enc.encodeI64(-100);
* = enc.encodeStr("cap-rpc");

let decoder = BincodeDecoder.init(enc.asSlice());
const id = decoder.decodeU64().?;     // 42
const delta = decoder.decodeI64().?;  // -100
const tag = decoder.decodeStr().?;    // "cap-rpc"
```

## License

MIT
