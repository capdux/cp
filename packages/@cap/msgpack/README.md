# `@cap/msgpack`

工业级、纯 CP 实现、零外部依赖、零堆分配的 **MessagePack** 编解码库，严格遵循官方 [MessagePack 规范](https://github.com/msgpack/msgpack/blob/master/spec.md) 并对标 Rust `rmp` crate 语义。

## 核心特性

- **100% 纯 CP & 零外部依赖**：无需 C/FFI、无需 `unsafe`、无需外部静态/动态库，全平台零配置编译。
- **零堆分配 & 零拷贝解码**：`MsgPackEncoder` 直接写入固定字节切片 `[]u8`；`MsgPackDecoder` 直接从输入切片 `[]const u8` 零拷贝切出 `Str`、`Bin` 与 `Ext` 载荷视图。
- **完整 MessagePack 规范与大端序编码**：
  - 自动选择最紧凑整数编码（positive fixint / negative fixint / uint8..64 / int8..64）
  - IEEE-754 `f32` / `f64`、`nil`、`bool`
  - `str`（fixstr / str8 / str16 / str32）、`bin`（bin8 / bin16 / bin32）
  - `array`（fixarray / array16 / array32）、`map`（fixmap / map16 / map32）
  - `ext`（fixext 1/2/4/8/16 + ext8 / ext16 / ext32）
- **异常边界防护**：内置 `peekType()` 类型探测与带深度护栏（`MAX_SKIP_DEPTH = 128`）的递归 `skipValue()`，对截断、非法保留字节 `0xC1`、越界长度与超深嵌套安全返回错误，不 panic、不越界，且解码失败时不破坏游标位置。

## 常量与类型

| 符号 | 类型 / 定义 | 说明 |
| :--- | :--- | :--- |
| `MAX_SKIP_DEPTH` | `usize = 128` | `skipValue()` 最大嵌套递归深度护栏 |
| `MsgPackError` | `err { BufferTooSmall, UnexpectedEof, TypeMismatch, InvalidMarker, Overflow }` | 编解码统一错误集 |
| `MsgPackType` | `enum { Nil, Bool, Int, Uint, Float32, Float64, Str, Bin, Array, Map, Ext }` | MessagePack 值类型标签 |
| `MsgPackExt` | `struct { type_code: i8; data: []const u8; }` | 扩展类型零拷贝视图 |

## API 文档

### 编码器 `MsgPackEncoder`

- **`MsgPackEncoder.init(buf: []u8): MsgPackEncoder`**  
  在调用方提供的固定缓冲区 `buf` 上初始化编码器。
- **`encoder.written(): []const u8`**  
  返回已写入的只读字节切片视图。
- **`encoder.remaining(): usize`**  
  返回缓冲区剩余可写字节数。
- **`encoder.reset()`**  
  重置写游标为 `0`，复用缓冲区。
- **`encoder.writeNil(): void ! MsgPackError`**  
  写入 `nil`（`0xC0`）。
- **`encoder.writeBool(val: bool): void ! MsgPackError`**  
  写入布尔值（`0xC2` / `0xC3`）。
- **`encoder.writeUint(val: u64): void ! MsgPackError`**  
  自动选择最紧凑无符号整数格式（positive fixint / uint8 / uint16 / uint32 / uint64）写入。
- **`encoder.writeInt(val: i64): void ! MsgPackError`**  
  自动选择最紧凑有符号整数格式（非负值走 `writeUint`；负值走 negative fixint / int8 / int16 / int32 / int64）写入。
- **`encoder.writeF32(val: f32): void ! MsgPackError`** / **`encoder.writeF64(val: f64): void ! MsgPackError`**  
  以 IEEE-754 大端序写入单精度 / 双精度浮点数。
- **`encoder.writeStr(s: []const u8): void ! MsgPackError`**  
  自动选择 fixstr / str8 / str16 / str32 写入字符串字节。
- **`encoder.writeBin(data: []const u8): void ! MsgPackError`**  
  自动选择 bin8 / bin16 / bin32 写入二进制载荷。
- **`encoder.writeArrayHeader(count: u32): void ! MsgPackError`**  
  自动选择 fixarray / array16 / array32 写入数组头。
- **`encoder.writeMapHeader(count: u32): void ! MsgPackError`**  
  自动选择 fixmap / map16 / map32 写入 Map 头（键值对数量）。
- **`encoder.writeExt(type_code: i8, data: []const u8): void ! MsgPackError`**  
  自动选择 fixext 1/2/4/8/16 或 ext8 / ext16 / ext32 写入扩展类型。

### 解码器 `MsgPackDecoder`

- **`MsgPackDecoder.init(data: []const u8): MsgPackDecoder`**  
  在只读字节切片 `data` 上初始化零拷贝解码器。
- **`decoder.remaining(): usize`**  
  返回剩余未读字节数。
- **`decoder.peekType(): MsgPackType ! MsgPackError`**  
  窥视当前游标处的值类型，不推进游标。遇到 `0xC1` 返回 `InvalidMarker`，空输入返回 `UnexpectedEof`。
- **`decoder.readNil(): void ! MsgPackError`**  
  读取 `nil`。
- **`decoder.readBool(): bool ! MsgPackError`**  
  读取布尔值。
- **`decoder.readUint(): u64 ! MsgPackError`**  
  读取无符号整数（兼容非负有符号整数编码；负值返回 `Overflow`）。
- **`decoder.readInt(): i64 ! MsgPackError`**  
  读取有符号整数（兼容无符号整数编码；超过 `i64` 最大值返回 `Overflow`）。
- **`decoder.readF32(): f32 ! MsgPackError`** / **`decoder.readF64(): f64 ! MsgPackError`**  
  读取大端序 IEEE-754 单精度 / 双精度浮点数。
- **`decoder.readStr(): []const u8 ! MsgPackError`**  
  零拷贝读取字符串切片。
- **`decoder.readBin(): []const u8 ! MsgPackError`**  
  零拷贝读取二进制切片。
- **`decoder.readArrayHeader(): u32 ! MsgPackError`**  
  读取数组元素数量。
- **`decoder.readMapHeader(): u32 ! MsgPackError`**  
  读取 Map 键值对数量。
- **`decoder.readExt(): MsgPackExt ! MsgPackError`**  
  零拷贝读取扩展类型（`type_code` 与 `data` 视图）。
- **`decoder.skipValue(): void ! MsgPackError`**  
  递归跳过当前任意标量、字符串、二进制、扩展或嵌套数组/Map 值；受 `MAX_SKIP_DEPTH` 深度护栏保护，失败时自动回滚游标。

## 使用示例

```typescript
import {
    MsgPackEncoder,
    MsgPackDecoder,
    MsgPackType,
} from "@cap/msgpack";
import { print } from "cap:io";

pub fn main() {
    let buf: [256]u8 = [0; 256];
    let encoder = MsgPackEncoder.init(buf[0..]);

    // 编码一个 Map: { "id": 42, "meta": [true, -100], "name": "cp" }
    encoder.writeMapHeader(3) catch unreachable;
    encoder.writeStr("id") catch unreachable;
    encoder.writeUint(42) catch unreachable;
    encoder.writeStr("meta") catch unreachable;
    encoder.writeArrayHeader(2) catch unreachable;
    encoder.writeBool(true) catch unreachable;
    encoder.writeInt(-100) catch unreachable;
    encoder.writeStr("name") catch unreachable;
    encoder.writeStr("cp") catch unreachable;

    // 零拷贝解码并跳过 "meta" 嵌套数组
    let decoder = MsgPackDecoder.init(encoder.written());
    const map_len = decoder.readMapHeader() catch unreachable;
    let i: u32 = 0;
    while (i < map_len) : (i += 1) {
        const key = decoder.readStr() catch unreachable;
        if (key.len == 4 && key[0] == 'm') {
            decoder.skipValue() catch unreachable;
        } else if (decoder.peekType() catch unreachable == MsgPackType.Uint) {
            const id = decoder.readUint() catch unreachable;
            * = id;
        } else {
            const name = decoder.readStr() catch unreachable;
            print(name);
        }
    }
}
```
