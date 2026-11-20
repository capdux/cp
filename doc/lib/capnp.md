# `cap:capnp` — Cap'n Proto 零拷贝二进制编解码库

## 概述

`cap:capnp`（`lib/capnp/`）提供纯 CP 实现的工业级 Cap'n Proto 二进制分段流帧（Segmented Message Framing）、64-bit 指针编解码（Struct / List / Far / Double-Far）以及 `TraversalLimit` 防放大攻击保护。所有读取器（`MessageReader` / `StructReader` / `ListReader`）均直接零拷贝切片映射原始字节缓冲区。

## 核心类型

| 类型 | 说明 |
| --- | --- |
| `CapnpError` | 错误集合：`InvalidFrame`、`TruncatedFrame`、`TooManySegments`、`OutOfBounds`、`InvalidPointer`、`NullPointer`、`TraversalLimitExceeded`、`NestingLimitExceeded`、`InvalidText`、`InvalidFarPointer`、`UnsupportedElementSize` 等 |
| `PointerKind` | 指针类型枚举：`Struct (0)`、`List (1)`、`Far (2)`、`Other (3)` |
| `ElementSize` | 列表元素大小枚举：`Void`、`Bit`、`Byte`、`TwoBytes`、`FourBytes`、`EightBytes`、`Pointer`、`Composite` |
| `TraversalLimit` | 遍历预算与嵌套深度控制器（默认 64 MiB 字数预算、`nesting_limit = 64`） |
| `MessageBuilder` / `MessageReader` | 多 Segment 流帧编码器与零拷贝解码器 |
| `StructBuilder` / `StructReader` | 结构体读写器（支持标量、XOR 默认值编码、`Text` / `Data`、跨段 `Far Pointer`） |
| `ListBuilder` / `ListReader` | 列表读写器（支持 `Bit`、`Byte`、`TwoBytes`、`FourBytes`、`EightBytes`、`Pointer`、`Composite`） |

## API 速查

- **流帧构建与解析**：
  - `MessageBuilder.init(alloc)` / `_.deinit()` / `_.addSegment(words)` / `_.initRoot(data_words, ptr_words)` / `_.toBytes(alloc)`
  - `MessageReader.init(alloc, frame_bytes, limit)` / `MessageReader.initDefault(alloc, frame_bytes)` / `_.deinit()` / `_.getRoot()`
- **结构体读写**：
  - 标量：`readBool` / `readU8` / `readU16` / `readU32` / `readI32` / `readU64` / `readF64` 及对应 `write*`
  - 默认值 XOR：`readU32OrDefault(idx, def)` / `writeU32OrDefault(idx, val, def)` 等
  - 指针：`initStruct` / `initStructInSegment` / `initList` / `initCompositeList` / `writeText` / `writeData`、`readStruct` / `readList` / `readText` / `readData`
- **列表读写**：
  - `len()`、`getBool` / `setBool`、`getU32` / `setU32`、`getStruct`、`getText` / `setText`、`initStruct`

## 示例

```cp
import { MessageBuilder, MessageReader } from "cap:capnp";

let builder = MessageBuilder.init(alloc);
defer builder.deinit();

let root = try builder.initRoot(1, 1);
root.writeU32OrDefault(0, 42, 100);
try root.writeText(0, "hello capnp");

const bytes = try builder.toBytes(alloc);
defer alloc.free(bytes);

let reader = try MessageReader.initDefault(alloc, bytes);
defer reader.deinit();

const r_root = try reader.getRoot();
const val = r_root.readU32OrDefault(0, 100); // 42
const txt = try r_root.readText(0);         // "hello capnp"
```
