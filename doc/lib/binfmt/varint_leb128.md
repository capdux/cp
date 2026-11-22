# `cap:varint_leb128` — 纯 CP 零堆 LEB128 / SLEB128 / ZigZag 变长整数引擎

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

## 概述

`cap:varint_leb128`（亦支持 `cap.varint_leb128` 动态挂载）实现零堆分配的无符号 ULEB128（WASM/DWARF/Protobuf）、有符号 SLEB128 补码变长编码及 32/64 位 ZigZag 映射编解码，内置 10 字节溢出与截断 EOF 防护。

## 核心类型与 API

- `VarintError`: `UnexpectedEof`、`Overflow`、`BufferTooSmall`
- `DecodedU64` / `DecodedI64`: `{ value, bytes_read }`
- `encodeUleb128(val, out)` / `decodeUleb128(data)`
- `encodeSleb128(val, out)` / `decodeSleb128(data)`
- `zigzagEncode64(v)` / `zigzagDecode64(v)` / `zigzagEncode32(v)` / `zigzagDecode32(v)`

## 示例

```cp
import { encodeUleb128, decodeUleb128 } from "cap:binfmt";

let buf: [10]u8 = undefined;
const n = try encodeUleb128(624485, &buf);
const res = try decodeUleb128(buf[0..n]);
```
