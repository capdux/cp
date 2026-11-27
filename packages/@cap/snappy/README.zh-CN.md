[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# @cap/snappy

Google Snappy 1.2.1 fast compression/decompression library (Kafka/Parquet/LevelDB/RocksDB) for CP

| 元信息 | 值 |
| --- | --- |
| 包名 | `@cap/snappy` |
| 版本 | `1.2.1` |
| 许可证 | `BSD-3-Clause` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add @cap/snappy
```

```cp
import {
    SnappyError,
    snappy_compress,
    snappy_uncompress,
    snappy_max_compressed_length,
    snappy_uncompressed_length,
    snappy_validate_compressed_buffer,
    maxCompressedLength,
    uncompressedLength,
} from "@cap/snappy";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `error` | `SnappyError` | `InvalidInput, BufferTooSmall` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `snappy_compress` | `(input: [*]&u8!, input_length: usize, compressed: [*]u8!, compressed_length: *usize!): i32` | 调用 `snappy_compress` 并返回 `i32`。 |
| `snappy_uncompress` | `(compressed: [*]&u8!, compressed_length: usize, uncompressed: [*]u8!, uncompressed_length: *usize!): i32` | 调用 `snappy_uncompress` 并返回 `i32`。 |
| `snappy_max_compressed_length` | `(source_length: usize): usize` | 调用 `snappy_max_compressed_length` 并返回 `usize`。 |
| `snappy_uncompressed_length` | `(compressed: [*]&u8!, compressed_length: usize, result: *usize!): i32` | 调用 `snappy_uncompressed_length` 并返回 `i32`。 |
| `snappy_validate_compressed_buffer` | `(compressed: [*]&u8!, compressed_length: usize): i32` | 调用 `snappy_validate_compressed_buffer` 并返回 `i32`。 |
| `maxCompressedLength` | `(source_len: usize): usize` | 调用 `maxCompressedLength` 并返回 `usize`。 |
| `uncompressedLength` | `(compressed: &[]u8): !usize` | 调用 `uncompressedLength` 并返回 `!usize`。 |
| `compressInto` | `(src: &[]u8, dst: []u8): !&[]u8` | 调用 `compressInto` 并返回 `!&[]u8`。 |
| `uncompressInto` | `(compressed: &[]u8, dst: []u8): !&[]u8` | 调用 `uncompressInto` 并返回 `!&[]u8`。 |
| `validateCompressedBuffer` | `(compressed: &[]u8): bool` | 校验输入合法性，返回 `bool`。 |

## 最小调用示例

```cp
import { validateCompressedBuffer } from "@cap/snappy";

pub fn main() {
    let mut out_buf: [256]u8 = undefined;
    let result = validateCompressedBuffer(out_buf[0..256]);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **零开销原生绑定**：通过 CP FFI 直接调用底层原生 C 库，无 GC 停顿与动态运行时桥接开销。
- **调用方自有输出缓冲区**：接受 `[]u8` 缓冲区参数的接口直接就地写入，内存上限完全确定。
- **显式错误联合与可选类型**：采用 `!T` 与 `T?` 替代运行时异常，错误传播清晰可控。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
