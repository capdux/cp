# `cap:postcard` — 纯 CP Postcard 紧凑二进制编解码与 COBS 帧定界库

## 概述

`cap:postcard` 提供纯 CP、零外部依赖、零堆分配的 Postcard 嵌入式/`no_std` 紧凑二进制编解码器，涵盖无符号 LEB128 Varint、ZigZag 有符号整数编码、IEEE-754 小端浮点、严格位宽溢出与布尔校验，以及串口/流式传输标配的 COBS（Consistent Overhead Byte Stuffing）帧定界编解码。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `zigzagEncodeI16`/`DecodeI16`、`zigzagEncodeI32`/`DecodeI32`、`zigzagEncodeI64`/`DecodeI64` | 零溢出有符号 ZigZag 编解码 |
| `PostcardWriter` | 零分配写入器（空间不足原子回滚返回 `false`）：`init(buf)`、`written()`、`remaining()`、`writeBool`、`writeU8`/`I8`、`writeVarU16`/`U32`/`U64`、`writeVarI16`/`I32`/`I64`、`writeF32`/`F64`、`writeBytes`/`writeString`、`writeOptionTag`、`writeVariantIdx`、`writeSeqLen` |
| `PostcardReader` | 零拷贝只读游标：`init(data)`、`remaining()`、`isEof()`、`readBool`（仅 `0`/`1`）、`readU8`/`I8`、`readVarU16`/`U32`/`U64`（严检 3B/5B/10B 位宽上限）、`readVarI16`/`I32`/`I64`、`readF32`/`F64`、`readBytes`/`readString`、`readOptionTag`、`readVariantIdx`、`readSeqLen` |
| `cobsEncode(src, dst): usize?` | COBS 编码（输出不含 `0x00`，**不**自动附加末尾 `0x00` 哨兵） |
| `cobsEncodeFramed(src, dst): usize?` | COBS 编码并在末尾写入 `0x00` 帧定界哨兵 |
| `cobsDecode(src, dst): usize?` | COBS 解码（严格拒绝输入中的非法 `0x00` 与越界 code 跳转） |

## 示例

```cp
import { PostcardWriter, PostcardReader, cobsEncodeFramed, cobsDecode } from "cap:postcard";

fn sendPacket(wire: []u8, framed: []u8): usize {
    let w = PostcardWriter.init(wire);
    if (!w.writeVarI32(-128)) return 0;
    if (!w.writeString("telemetry")) return 0;
    return cobsEncodeFramed(w.written(), framed) ?? 0;
}
```
