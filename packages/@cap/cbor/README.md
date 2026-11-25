# `@cap/cbor`

工业级、纯 CP 实现、零外部依赖、零堆分配的 **CBOR (RFC 8949)** 编解码库，严格遵循 [RFC 8949](https://www.rfc-editor.org/rfc/rfc8949.html) 规范并对标 Rust `minicbor` / `ciborium` crate 语义。

## 核心特性

- **100% 纯 CP & 零外部依赖**：无需 C/FFI、无需 `unsafe`、无需外部静态/动态库，全平台零配置编译。
- **零堆分配 & 零拷贝解码**：`CborEncoder` 直接写入调用方提供的固定字节切片 `[]u8`；`CborDecoder` 直接从输入切片 `[]const u8` 零拷贝切出 `Bytes` 与 `Text` 载荷视图。
- **严格 RFC 8949 规范最短编码（Canonical Shortest Encoding）**：
  - Major 0 / Major 1 自动按 `0..23`（1B）、`1B`（`ai=24`）、`2B`（`ai=25`）、`4B`（`ai=26`）、`8B`（`ai=27`）选择最短大端序编码；
  - 负整数遵循 `-1 - n` 语义，无溢出完整覆盖 `i64` 最小值（`-9223372036854775808`）；
  - Major 2（字节串）、Major 3（UTF-8 文本）、Major 4（数组）、Major 5（Map）、Major 6（语义 Tag）、Major 7（`false`/`true`/`null`/`undefined` 及 IEEE-754 `f32`/`f64`）。
- **异常边界防护与原子游标**：
  - `peekMajor()` / `peekKind()` 在读取前即完成类型探测与保留头校验；
  - `skipValue()` 内置 `MAX_CBOR_DEPTH = 128` 深度护栏、畸形大长度预检与失败自动回滚游标机制，对截断、非法保留位（`ai = 28..30`）、不定长流（`ai = 31`）与超深嵌套安全返回错误，绝不 panic。

## 常量与类型

| 符号 | 类型 / 定义 | 说明 |
| :--- | :--- | :--- |
| `MAX_CBOR_DEPTH` | `usize = 128` | `skipValue()` 最大嵌套递归深度护栏 |
| `CborError` | `err { BufferTooSmall, UnexpectedEof, TypeMismatch, InvalidHeader, Overflow, UnsupportedIndefinite }` | 编解码统一错误集 |
| `CborMajor` | `enum: u8 { Uint = 0, NegInt = 1, Bytes = 2, Text = 3, Array = 4, Map = 5, Tag = 6, Simple = 7 }` | RFC 8949 八大主类型 |
| `CborValueKind` | `enum { Uint, NegInt, Bytes, Text, Array, Map, Tag, Bool, Null, Undefined, Float16, Float32, Float64, Simple }` | 细粒度值类型标签 |

## API 文档

### 编码器 `CborEncoder`

- **`CborEncoder.init(buf: []u8): CborEncoder`**  
  在调用方提供的固定缓冲区 `buf` 上初始化编码器。
- **`encoder.written(): []const u8`**  
  返回已写入的只读字节切片视图。
- **`encoder.remaining(): usize`**  
  返回缓冲区剩余可写字节数。
- **`encoder.reset()`**  
  重置写游标为 `0`，复用缓冲区。
- **`encoder.writeUint(val: u64): void ! CborError`**  
  以 Major 0 规范最短大端序写入无符号整数（0..23 / 1B / 2B / 4B / 8B）。
- **`encoder.writeInt(val: i64): void ! CborError`**  
  写入有符号整数（非负值走 Major 0；负值走 Major 1 `-1 - n`，完整支持 `i64` 最小值）。
- **`encoder.writeBytes(data: []const u8): void ! CborError`**  
  写入 Major 2 字节串（空间不足时原子失败，不推进游标）。
- **`encoder.writeText(text: []const u8): void ! CborError`**  
  写入 Major 3 UTF-8 文本串（空间不足时原子失败，不推进游标）。
- **`encoder.writeArrayHeader(count: u64): void ! CborError`**  
  写入 Major 4 数组头（元素数量）。
- **`encoder.writeMapHeader(count: u64): void ! CborError`**  
  写入 Major 5 Map 头（键值对数量）。
- **`encoder.writeTag(tag: u64): void ! CborError`**  
  写入 Major 6 语义标签（如 Tag 1 表示 Unix Epoch 时间戳）。
- **`encoder.writeBool(val: bool): void ! CborError`**  
  写入布尔值（`false` = `0xF4`，`true` = `0xF5`）。
- **`encoder.writeNull(): void ! CborError`** / **`encoder.writeUndefined(): void ! CborError`**  
  写入 `null`（`0xF6`）或 `undefined`（`0xF7`）。
- **`encoder.writeF32(val: f32): void ! CborError`** / **`encoder.writeF64(val: f64): void ! CborError`**  
  写入 IEEE-754 大端序单精度（`0xFA` + 4B）或双精度（`0xFB` + 8B）浮点数。

### 解码器 `CborDecoder`

- **`CborDecoder.init(data: []const u8): CborDecoder`**  
  在只读字节切片 `data` 上初始化零拷贝解码器。
- **`decoder.remaining(): usize`**  
  返回剩余未读字节数。
- **`decoder.peekMajor(): CborMajor ! CborError`**  
  窥视当前游标处的主类型，不推进游标；对非法保留头返回 `InvalidHeader`，对不定长头返回 `UnsupportedIndefinite`。
- **`decoder.peekKind(): CborValueKind ! CborError`**  
  窥视当前游标处的细粒度值类型，不推进游标。
- **`decoder.readUint(): u64 ! CborError`**  
  读取 Major 0 无符号整数。
- **`decoder.readInt(): i64 ! CborError`**  
  读取 Major 0 / Major 1 有符号整数（超出 `i64` 范围返回 `Overflow` 并保持游标不变）。
- **`decoder.readBytes(): []const u8 ! CborError`**  
  零拷贝读取 Major 2 字节串切片。
- **`decoder.readText(): []const u8 ! CborError`**  
  零拷贝读取 Major 3 UTF-8 文本切片。
- **`decoder.readArrayHeader(): u64 ! CborError`**  
  读取 Major 4 数组元素数量。
- **`decoder.readMapHeader(): u64 ! CborError`**  
  读取 Major 5 Map 键值对数量。
- **`decoder.readTag(): u64 ! CborError`**  
  读取 Major 6 语义标签编号。
- **`decoder.readBool(): bool ! CborError`**  
  读取布尔值。
- **`decoder.readNull(): void ! CborError`** / **`decoder.readUndefined(): void ! CborError`**  
  读取 `null` 或 `undefined`。
- **`decoder.readF32(): f32 ! CborError`** / **`decoder.readF64(): f64 ! CborError`**  
  读取 IEEE-754 大端序单精度 / 双精度浮点数。
- **`decoder.skipValue(): void ! CborError`**  
  递归跳过当前任意标量、字节串、文本、Tag 或嵌套 Array/Map；受 `MAX_CBOR_DEPTH = 128` 深度护栏与畸形大长度预检保护，失败时自动回滚游标。

## 使用示例

```typescript
import {
    CborEncoder,
    CborDecoder,
    CborMajor,
} from "@cap/cbor";
import { print } from "cap:io";

pub fn main() {
    let buf: [256]u8 = [0; 256];
    let encoder = CborEncoder.init(buf[0..]);

    // 编码一个 Map: { "ts": 1(1363896240), "meta": [true, -1000], "name": "cp" }
    encoder.writeMapHeader(3) catch unreachable;
    encoder.writeText("ts") catch unreachable;
    encoder.writeTag(1) catch unreachable;
    encoder.writeUint(1363896240) catch unreachable;
    encoder.writeText("meta") catch unreachable;
    encoder.writeArrayHeader(2) catch unreachable;
    encoder.writeBool(true) catch unreachable;
    encoder.writeInt(-1000) catch unreachable;
    encoder.writeText("name") catch unreachable;
    encoder.writeText("cp") catch unreachable;

    // 零拷贝解码并跳过 "meta" 嵌套数组
    let reader = CborDecoder.init(encoder.written());
    const map_len = reader.readMapHeader() catch unreachable;
    let i: u64 = 0;
    while (i < map_len) : (i += 1) {
        const key = reader.readText() catch unreachable;
        if (key.len == 2 && key[0] == 't' && key[1] == 's') {
            const tag = reader.readTag() catch unreachable;
            const epoch = reader.readUint() catch unreachable;
            print("ts tag={} epoch={}\n", tag, epoch);
        } else if (key.len == 4 && key[0] == 'm') {
            reader.skipValue() catch unreachable;
        } else if ((reader.peekMajor() catch unreachable) == CborMajor.Text) {
            const name = reader.readText() catch unreachable;
            print("name len={}\n", name.len);
        }
    }
}
```
