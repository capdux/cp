// Source: cp/doc/lib/hpack.md
# `cap:hpack` — RFC 7541 HTTP/2 HPACK 头部压缩编解码器

纯 CP、零 C 依赖的 RFC 7541 HTTP/2 HPACK 头部压缩与解压缩引擎，完整实现 Appendix A 61 项静态表、Appendix B 256+EOS 规范 Huffman 编解码、有界 FIFO 动态表及全部四种头部字段表示形式。

## 导入方式

```cp
import {
    HpackError,
    HeaderField,
    IndexingMode,
    DynamicTable,
    DecodedHeaders,
    HpackEncoder,
    HpackDecoder,
    huffmanEncode,
    huffmanDecode,
    encodeInt,
    decodeInt,
} from "cap:hpack";

// 或动态命名空间挂载：
import cap from "cap";
// cap.hpack.HpackEncoder / cap.hpack.HpackDecoder
```

## 核心类型与错误集

### `HpackError`
- `InvalidVarint`：前缀位数不在 `1..8` 合法范围
- `VarintOverflow`：变长前缀整数超出 `u64` 表示范围
- `IndexOutOfBounds`：索引为 `0` 或超出静态表与当前动态表总长度
- `InvalidHuffman`：包含 EOS 符号、非全 `1` 填充或超长填充（`> 7` bits）
- `TableSizeExceeded`：对端发送的动态表尺寸更新超过 `max_allowed_table_size`，或出现在头部字段之后
- `TruncatedInput`：输入字节流提前截断
- `OutOfMemory`：内存分配失败

### `HeaderField` & `IndexingMode`
- `HeaderField { name: []const u8, value: []const u8 }`
- `IndexingMode`：
  - `Incremental`：增量索引（`01xxxxxx`），写入动态表
  - `WithoutIndexing`：不索引（`0000xxxx`），不写入动态表
  - `NeverIndexed`：永不索引（`0001xxxx`），用于敏感头（如 `authorization` / `cookie`）保护

## 静态表与动态表 (`lib/hpack/table.cp`)
- `STATIC_TABLE_LEN`（61）、`ENTRY_OVERHEAD`（32）
- `getStaticEntry(index: usize): HeaderField?`：查询 RFC 7541 Appendix A 静态表（1..61）
- `DynamicTable`：
  - `init(alloc, max_size)` / `deinit(*_)`
  - `insert(*_, name, value): ! HpackError`：每条目占用 `name.len + value.len + 32` 字节；超限自动从尾部驱逐最旧条目；单条目超过 `max_size` 时按 RFC 7541 Section 4.4 清空动态表且不报错
  - `setMaxSize(*_, new_max_size)` / `clear(*_)` / `len()` / `currentSize()` / `maxSize()`

## Huffman 编解码 (`lib/hpack/huffman.cp`)
- `huffmanEncodedLength(input: []const u8): usize`
- `huffmanEncode(alloc, input: []const u8): ([]u8) ! HpackError`
- `huffmanDecode(alloc, input: []const u8): ([]u8) ! HpackError`：严格校验 EOS 与 trailing padding

## 编解码器 (`lib/hpack/codec.cp`)
- `HpackEncoder`：
  - `init(alloc, max_table_size)` / `deinit(*_)`
  - `setHuffman(*_, enabled: bool)` / `setMaxTableSize(*_, new_size: usize)`
  - `encodeHeader(*_, out: *std.ArrayList(u8), name, value, mode): ! HpackError`
  - `encode(*_, headers: []const HeaderField): ([]u8) ! HpackError`
  - `encodeWithModes(*_, headers: []const HeaderField, modes: []const IndexingMode): ([]u8) ! HpackError`
- `HpackDecoder`：
  - `init(alloc, max_allowed_table_size)` / `deinit(*_)`
  - `setMaxAllowedTableSize(*_, limit: usize)`
  - `decode(*_, block: []const u8): DecodedHeaders ! HpackError`（使用完毕后调用 `decoded.deinit()` 释放内存）
