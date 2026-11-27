[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# zerocopy

适用于 CP 语言的零堆分配、零拷贝前缀/后缀字节切片读写器、端序整数封装与对齐校验切片转换库，对齐 Google Rust [`zerocopy`](https://github.com/google/zerocopy) crate（MIT 协议，CP v1.0.0，底层复用 `bytemuck`）。

## 安装与导入

```bash
cap add zerocopy
```

```cp
import {
    ByteCursor,
    ByteWriterCursor,
    U16LE,
    U32BE,
    readU32BeFromPrefix,
    readFromPrefix,
    sliceAsBytesU32,
} from "zerocopy";
```

## 核心 API 参考

### 游标读写器

| 类型 | 方法 | 说明 |
| :--- | :--- | :--- |
| `ByteCursor` | `new(total_len)`, `remaining()`, `reset()`, `seek(offset): bool`, `skip(n): bool` | 带偏移状态的二进制顺序解码游标，不持有底层缓冲区所有权。 |
| `ByteCursor`（读取） | `peekU8(&[]u8): u8?`, `nextU8([]u8): u8?`, `nextU16Le/Be([]u8): u16?`, `nextU32Le/Be([]u8): u32?`, `nextU64Le/Be([]u8): u64?`, `nextI32Le([]u8): i32?`, `nextF32Le(&[]u8): f32?`, `nextF64Le(&[]u8): f64?`, `nextBytes(&[]u8, n): (&[]u8)?` | 带边界检查的顺序读取；剩余字节不足时返回 `null`。 |
| `ByteWriterCursor` | `new(cap_len)`, `written()`, `remaining()`, `reset()`, `putU8`, `putU16Le/Be`, `putU32Le/Be`, `putU64Le/Be`, `putI32Le`, `putF32Le`, `putF64Le`, `putBytes(out: []u8, src: &[]u8): bool` | 带边界检查的顺序二进制写入器，缓冲区不足时返回 `false`。 |

### 端序无关对齐安全封装类型

全部 6 种类型（`U16LE`、`U16BE`、`U32LE`、`U32BE`、`U64LE`、`U64BE`）内部以 `[N]u8`（1 字节对齐）存储，提供统一接口：

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `new(v)` | `(T): Self` | 按指定大小端序编码整数 `v`。 |
| `fromBytes(buf)` | `(&[]u8): Self?` | 无对齐要求地读取前 `N` 字节；长度不足返回 `null`。 |
| `get()` / `set(v)` | `(): T` / `(T)` | 解码读取或更新存储的整数值。 |
| `toBytes()` | `(): [N]u8` | 返回底层原始字节数组。 |

### 前缀 / 后缀零拷贝切分与对齐转换

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `readFromPrefix` / `readFromSuffix` | `(buf: &[]u8, n: usize): SplitBytes?` | 零拷贝切分 `buf` 为 `{ head: &[]u8, tail: &[]u8 }`。 |
| `readU16LeFromPrefix` / `readU16BeFromPrefix` | `(buf: &[]u8): PrefixU16?` | 从头部读取 `u16` 并返回 `{ value, rest: &[]u8 }`。 |
| `readU32LeFromPrefix` / `readU32BeFromPrefix` | `(buf: &[]u8): PrefixU32?` | 从头部读取 `u32` 并返回 `{ value, rest: &[]u8 }`。 |
| `readU64LeFromPrefix` / `readU64BeFromPrefix` | `(buf: &[]u8): PrefixU64?` | 从头部读取 `u64` 并返回 `{ value, rest: &[]u8 }`。 |
| `readU16LeFromSuffix` / `readU32LeFromSuffix` / `readU64LeFromSuffix` | `(buf: &[]u8): PrefixU*?` | 从尾部读取小端整数并在 `rest` 中返回剩余前缀切片。 |
| `writeToPrefix` / `writeToSuffix` | `(dst: []u8, src: &[]u8): ([]u8)?` | 向目标切片头部/尾部写入 `src`，返回剩余可写切片。 |
| `isAlignedSlice` | `(byte_len, elem_size, addr, align_req): bool` | 借助 `bytemuck.checkCastLayout` 校验长度整除与地址对齐。 |
| `refFromBytesU16` / `U32` / `U64` | `(buf: &[]u8): u16? \| u32? \| u64?` | 对齐与长度严格校验的直接指针加载；不满足返回 `null`。 |
| `refFromBytesSliceU32` | `(buf: &[]u8): (&[]u32)?` | 当地址 4 字节对齐且长度为 4 的倍数时零拷贝转为 `&[]u32`。 |
| `sliceAsBytesU16` / `U32` / `U64` | `(src: &[]u*): &[]u8` | 整数切片到只读字节切片的零拷贝视图转换。 |

## 代码示例

### 1. 零拷贝协议包头链式解码

```cp
import { readU16BeFromPrefix, readU32BeFromPrefix, readFromPrefix } from "zerocopy";

fn parsePacket(pkt: &[]u8): bool {
    let ver = readU16BeFromPrefix(pkt) ?? return false;
    let payload_len = readU32BeFromPrefix(ver.rest) ?? return false;
    let split = readFromPrefix(payload_len.rest, payload_len.value as usize) ?? return false;
    // split.head 为零拷贝 payload 只读视图，split.tail 为剩余字节
    return ver.value == 1 && split.head.len == (payload_len.value as usize);
}
```

### 2. 使用 `ByteWriterCursor` 与 `ByteCursor` 顺序编解码

```cp
import { ByteWriterCursor, ByteCursor } from "zerocopy";

fn cursorRoundTrip() {
    let buf: [32]u8 = undefined;
    let mut w = ByteWriterCursor.new(32);
    w.putU32Le(buf[0..32], 0xDEADBEEF);
    w.putF64Le(buf[0..32], 3.14159);

    let mut r = ByteCursor.new(w.written());
    let magic = r.nextU32Le(buf[0..32]);
    let pi = r.nextF64Le(&buf[0..32]);
}
```

### 3. 无对齐要求的端序封装（`U32BE`）

```cp
import { U32BE } from "zerocopy";

fn endianExample() {
    let net_val = U32BE.new(8080);
    let raw = net_val.toBytes(); // [0x00, 0x00, 0x1F, 0x90]
    let parsed = U32BE.fromBytes(&raw);
    // parsed.?.get() == 8080
}
```

## 与源包差异与零拷贝安全亮点

- **显式强类型替代 Derive 宏**：用显式类型化函数与端序封装（`U16LE`..`U64BE`）替代 Rust 的 `#[derive(FromBytes, IntoBytes)]`，编译速度极快且签名直观。
- **杜绝未对齐未定义行为**：`refFromBytesU32`、`refFromBytesSliceU32` 在运行时显式校验指针地址对齐与长度整除性，不满足直接返回 `null`。
- **全程零拷贝切片**：`readFromPrefix`、`readFromSuffix`、`nextBytes` 与 `sliceAsBytes*` 均返回只读借用视图，零内存拷贝与零堆分配。
