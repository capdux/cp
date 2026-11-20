# CP 标准库：RFC 8949 CBOR 确定性编解码引擎 (`cap:cbor`)

## 一、 架构总览与设计准则

`cap:cbor` 是 CP 语言官方内置的高性能紧凑二进制序列化标准库，提供完整的 **RFC 8949 CBOR** 与 **Section 4.2 确定性规范编码（Deterministic Canonical Encoding）** 能力（同时兼容 `BinValue`/`BinDocument` 与 `MsgPack`）。**零外部依赖、零 `std.posix`、零 `@import("bun")`**，既支持 `import { ... } from "cap:cbor"`，也支持 `import cap from "cap"; cap.cbor.*` 动态懒挂载调用。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                    CP RFC 8949 CBOR 编解码引擎 (cap:cbor)                │
├───────────────────────┬─────────────────────────┬───────────────────────┤
│ 强类型 CBOR AST       │ 确定性编码 (Sec 4.2)    │ 安全解码与深度防护     │
│ CborValue / CborTag   │ encode / encodeCanonical│ decode / decodeWithMax│
│ CborMapEntry / Error  │ 最短整数/Float16/32/64  │ UTF-8/AI28-30/Trailing│
│ deinit(alloc) 零泄漏  │ Map 键按编码字节序排序  │ MaxDepthExceeded 防护  │
└───────────────────────┴─────────────────────────┴───────────────────────┘
```

### 模块文件拓扑

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/cbor/value.cp` | `// Source: cp/lib/cbor/value.cp (Shared Compact Binary Value AST for CBOR & MessagePack)` | `CborError`、`CborTag`、`CborMapEntry`、`CborValue`（含 `deinit(allocator)` 与 `eql(other)`）以及 `BinValue`/`BinDocument` |
| `lib/cbor/cbor.cp` | `// Source: cp/lib/cbor/cbor.cp (RFC 8949 CBOR Encoder & Decoder)` | `encode`、`encodeCanonical`、`decode`、`decodeWithMaxDepth`（Major 0-7、Float16/32/64、Tag、UTF-8 校验、64 层深度防护） |
| `lib/cbor/msgpack.cp` | `// Source: cp/lib/cbor/msgpack.cp (MessagePack Binary Encoder & Decoder)` | MessagePack 全格式编解码（兼容导出） |
| `lib/cbor/index.cp` | `// Source: cp/lib/cbor/index.cp (Unified Exports for cap:cbor)` | `cap:cbor` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误与 AST 类型（`value.cp`）

- **`CborError`**：
  - `UnexpectedEof`、`InvalidAdditionalInfo`、`InvalidBreak`、`UnsupportedIndefinite`、`MaxDepthExceeded`、`TrailingBytes`、`InvalidUtf8`、`IntegerOverflow`、`OutOfMemory`。
- **`CborMapEntry`**：`{ key: CborValue, value: CborValue }`，支持任意 `CborValue` 作为 Map 键。
- **`CborTag`**：`{ tag: u64, value: *CborValue }`，承载 CBOR Major 6 语义标签。
- **`CborValue`**（Tagged Union）：
  - 变体：`unsigned: u64`、`negative: i64`、`bytes: []const u8`、`text: []const u8`、`array: []CborValue`、`map: []CborMapEntry`、`tag: CborTag`、`boolean: bool`、`null_val`、`undefined_val`、`float: f64`。
  - 方法：`deinit(allocator)` 递归释放解码树内存；`eql(other)` 进行深度结构比较（支持 `NaN` 相等）。

### 2. RFC 8949 编解码与确定性编码（`cbor.cp`）

- **`encode(allocator, value): ([]u8)!`**：按 RFC 8949 最短整数与最短 IEEE 754 浮点（Float16/Float32/Float64）编码全部 Major 0-7 类型。
- **`encodeCanonical(allocator, value): ([]u8)!`**：遵循 **RFC 8949 Section 4.2 确定性编码规范**（适用于 WebAuthn / COSE 签名）：
  - 最小整数与长度前缀编码、最短无损浮点编码（NaN 规范化为 `0xF9 0x7E 0x00`）；
  - 全部 Map 键递归按**规范 CBOR 编码后的字节序列字典升序**（`std.mem.order`）排列。
- **`decode(allocator, bytes): CborValue!`** / **`decodeWithMaxDepth(allocator, bytes, max_depth): CborValue!`**：
  - 完整解码 Major 0-7，校验 UTF-8（`InvalidUtf8`）、拒绝保留 Additional Info 28..30（`InvalidAdditionalInfo`）、孤立 Break `0xFF`（`InvalidBreak`）、不定长流（`UnsupportedIndefinite`）、`i64` 负整数溢出（`IntegerOverflow`）、尾随垃圾字节（`TrailingBytes`）及嵌套深度超限（`MaxDepthExceeded`）。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { CborValue, CborMapEntry, encodeCanonical, decode } from "cap:cbor";
import cap from "cap";

pub fn demoCanonicalCbor()! {
    const alloc = std.heap.page_allocator;

    let entries: [2]CborMapEntry = [
        CborMapEntry{ key: CborValue.text("long_key"), value: CborValue.unsigned(100) },
        CborMapEntry{ key: CborValue.unsigned(1), value: CborValue.text("short_int_key") },
    ];
    const val = CborValue.map(&entries);

    // 确定性编码：整数键 1 (0x01) 自动排在文本键 "long_key" (0x68...) 之前
    const bytes = encodeCanonical(alloc, val)?;
    defer alloc.free(bytes);

    let decoded = cap.cbor.decode(alloc, bytes)?;
    defer decoded.deinit(alloc);
    * = decoded.eql(val);
}
```
