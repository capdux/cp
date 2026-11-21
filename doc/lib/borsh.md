# `cap:borsh` — 纯 CP Borsh 确定性二进制编解码器（零堆分配）

## 概述

`cap:borsh` 提供纯 CP、零外部依赖、零堆分配的 Borsh（Binary Object Representation Serializer for Hashing）确定性二进制编解码器，严格遵循 Borsh 规范：小端固定宽度整型、4 字节 LE `u32` 变长前缀、布尔与 `Option` 标签仅允许 `0`/`1`、以及 IEEE-754 `f32`/`f64` 严格拒绝 NaN。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `BorshWriter` | 零分配写入器：`init(buf: []u8)`、`written(): []const u8`、`remaining(): usize` |
| 标量写入 | `writeBool`, `writeU8`, `writeI8`, `writeU16`, `writeI16`, `writeU32`, `writeI32`, `writeU64`, `writeI64`, `writeF32`（拒 NaN）, `writeF64`（拒 NaN） |
| 变长与容器写入 | `writeBytes`, `writeString`, `writeFixedBytes`, `writeOptionTag`, `writeEnumTag`, `writeSeqLen` |
| `BorshReader` | 零拷贝只读游标：`init(data: []const u8)`、`remaining(): usize`、`isEof(): bool` |
| 标量读取 | `readBool`（仅允 `0`/`1`）, `readU8`, `readI8`, `readU16`, `readI16`, `readU32`, `readI32`, `readU64`, `readI64`, `readF32`（拒 NaN）, `readF64`（拒 NaN） |
| 变长与容器读取 | `readBytes`, `readString`, `readFixedBytes(n)`, `readOptionTag`, `readEnumTag`, `readSeqLen` |

## 示例

```cp
import { BorshWriter, BorshReader } from "cap:borsh";

fn roundtrip(buf: []u8): u64 {
    let w = BorshWriter.init(buf);
    if (!w.writeU64(42)) return 0;
    if (!w.writeString("borsh")) return 0;

    let r = BorshReader.init(w.written());
    const id = r.readU64() ?? return 0;
    return id;
}
```
