# CP 标准库：MessagePack 紧凑二进制编解码引擎 (`cap:msgpack`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:binfmt`，导入请使用 `import { ... } from "cap:binfmt"`。

## 一、 架构总览与设计准则

`cap:msgpack` 是 CP 语言官方内置的纯 CP **MessagePack 二进制序列化与反序列化引擎**。**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:binfmt"`，也支持 `import cap from "cap"; cap.msgpack.*` 动态懒挂载调用。

### 核心特性

- **全格式覆盖与自动最小编码**：支持 positive/negative fixint、`uint8/16/32/64`、`int8/16/32/64`、`float32/64`、`nil`、`bool`、`fixstr`/`str8/16/32`、`bin8/16/32`、`fixarray`/`array16/32`、`fixmap`/`map16/32` 以及 `fixext1/2/4/8/16` & `ext8/16/32` 扩展类型。
- **零拷贝解码（Zero-Copy）**：解码输出的 `str`、`bin`、`ext.data` 直接切片引用原始输入缓冲区，无需额外内存拷贝；仅 `array` 和 `map` 容器切片通过传入的 `Allocator` 分配，调用 `value.deinit(allocator)` 即可递归释放。
- **安全边界与资源耗尽防护**：默认 `DEFAULT_MAX_DEPTH = 64` 嵌套深度限制，严格拦截截断报文（`UnexpectedEof`）、保留格式标签 `0xc1`（`InvalidFormatTag`）、尾随多余字节（`TrailingBytes`）以及长度越界。

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/msgpack/msgpack.cp` | `// Source: cp/lib/msgpack/msgpack.cp (...)` | `MsgPackError`、`MsgExt`、`MsgMapEntry`、`MsgValue`、`MsgPackWriter`、`MsgPackReader`、`encode`/`decode`/`decodeWithMaxDepth` |
| `lib/msgpack/index.cp` | `// Source: cp/lib/msgpack/index.cp (...)` | `cap:msgpack` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与数据模型

- **`MsgPackError`**：`UnexpectedEof`、`InvalidFormatTag`、`MaxDepthExceeded`、`TrailingBytes`、`IntegerOverflow`、`OutOfMemory`。
- **`MsgExt`**：`{ ext_type: i8, data: []const u8 }`，静态构造 `MsgExt.init(ext_type, data)`。
- **`MsgMapEntry`**：`{ key: MsgValue, value: MsgValue }`，静态构造 `MsgMapEntry.init(key, value)`。
- **`MsgValue`**：带载荷联合枚举：
  - 变体：`nil`、`boolean(bool)`、`int(i64)`、`uint(u64)`、`float(f64)`、`str([]const u8)`、`bin([]const u8)`、`array([]MsgValue)`、`map([]MsgMapEntry)`、`ext(MsgExt)`；
  - 工厂方法：`makeNil()`、`makeBool(v)`、`makeInt(v)`、`makeUint(v)`、`makeFloat(v)`、`makeStr(v)`、`makeBin(v)`、`makeArray(items)`、`makeMap(entries)`、`makeExt(ext_type, data)`；
  - 查询与比较：`isNil()`、`asBool()`、`asInt()`、`asUint()`、`asFloat()`、`asStr()`、`asBin()`、`asArray()`、`asMap()`、`asExt()`、`get(key_str)`、`eql(other)`；
  - 内存释放：`deinit(allocator)` 递归释放 `array` 与 `map` 容器切片。

### 2. 流式编码器 (`MsgPackWriter`) 与 `encode`

- **`MsgPackWriter.init(allocator)`** / **`deinit()`** / **`bytes()`** / **`toOwnedSlice()`**：
  - 标量与容器流式写入：`writeNil()`、`writeBool(v)`、`writeInt(v)`、`writeUint(v)`、`writeFloat(v)`、`writeFloat32(v)`、`writeStr(s)`、`writeBin(b)`、`writeArrayHeader(len)`、`writeMapHeader(len)`、`writeExt(ext_type, data)`；
  - **`encodeValue(value)`**：递归编码任意 `MsgValue`。
- **`encode(allocator, value): ([]u8)!`**：一步完成 `MsgValue` 到紧凑二进制字节切片的编码。

### 3. 解码器 (`MsgPackReader`) 与 `decode`

- **`MsgPackReader.init(allocator, data)`** / **`initWithMaxDepth(allocator, data, max_depth)`**：
  - **`readValue()`** / **`readValueAtDepth(depth)`** / **`remaining()`** / **`isAtEnd()`**。
- **`decode(allocator, bytes): MsgValue!`** / **`decodeWithMaxDepth(allocator, bytes, max_depth): MsgValue!`**:
  - 全量解码并严格校验无尾随字节（否则返回 `err.TrailingBytes`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { MsgValue, MsgMapEntry, MsgPackWriter, encode, decode } from "cap:binfmt";
import cap from "cap";

pub fn demoMsgPack()! {
    const alloc = std.heap.page_allocator;

    // 1. 流式编码
    let writer = MsgPackWriter.init(alloc);
    defer writer.deinit();
    writer.writeMapHeader(2)?;
    writer.writeStr("agent")?;
    writer.writeStr("cp-worker")?;
    writer.writeStr("ext")?;
    writer.writeExt(-1, "\x01\x02\x03\x04")?;
    const raw = writer.bytes();

    // 2. 零拷贝解码与 cap.msgpack 动态挂载
    let val = cap.msgpack.decode(alloc, raw)?;
    defer val.deinit(alloc);

    const agent = val.get("agent").?.asStr().?;
    const ext = val.get("ext").?.asExt().?;
    * = agent;
    * = ext;
}
```
