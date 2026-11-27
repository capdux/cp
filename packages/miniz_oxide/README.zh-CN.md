[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# miniz_oxide

适用于 CP 语言的高性能无损 DEFLATE / zlib 压缩、校验和（CRC-32 / Adler-32）及单文件 ZIP 归档引擎，对齐 Rust [`miniz_oxide`](https://github.com/Frommi/miniz_oxide) crate，底层封装原生 `miniz` 3.0.2（MIT 协议，CP v3.0.2）。

同时提供 RAII 自动释放的动态缓冲区（`MinizBuf`，离开作用域时由 `__cp_deinit` 自动回收）与零堆分配切片编解码接口（`compressToSlice` / `decompressToSlice`），并提供带输出上限的解压接口（`decompressToVecWithLimit`）防止异常膨胀耗尽资源。

## 安装与导入

```bash
cap add miniz_oxide
```

```cp
import {
    MinizError,
    MinizBuf,
    compressToVec,
    compressToVecZlib,
    decompressToVec,
    decompressToVecZlib,
    decompressToVecWithLimit,
    compressToSlice,
    decompressToSlice,
    crc32,
    adler32,
} from "miniz_oxide";
```

## 核心 API 参考

### 错误类型与 RAII 缓冲区

| 符号 | 签名 / 成员 | 说明 |
| :--- | :--- | :--- |
| `MinizError` | `ZipCreateFailed`, `ZipExtractFailed`, `OutOfMemory`, `CompressFailed`, `DecompressFailed`, `BufferTooSmall`, `InvalidLevel`, `OutputLimitExceeded` | 所有可失败函数（`!T`）返回的错误联合集合。 |
| `MinizBuf` | `raw: [*]u8!?`, `len: usize` | 持有底层 C 堆缓冲区的 RAII 封装，作用域结束自动调用 `__cp_deinit` 释放。 |
| `MinizBuf.slice()` | `(): &[]u8` | 返回缓冲区内容的零拷贝只读切片视图。 |
| `MinizBuf.zipSingleFile` | `(entry_name: string, payload: &[]u8): !MinizBuf` | 在内存中创建包含单个文件条目的 ZIP 归档。 |
| `MinizBuf.extractFile` | `(zip_bytes: &[]u8, entry_name: string): !MinizBuf` | 从内存 ZIP 归档中按文件名提取条目内容。 |

### 压缩与解压函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `compressToVec` / `compress_to_vec` | `(input: &[]u8, level: u8): !MinizBuf` | Raw DEFLATE 压缩（`level` 取值 `0..=10`），返回 RAII `MinizBuf`。 |
| `compressToVecZlib` / `compress_to_vec_zlib` | `(input: &[]u8, level: u8): !MinizBuf` | 带 zlib 头部的 DEFLATE 压缩（`level` 取值 `0..=10`）。 |
| `decompressToVec` / `decompress_to_vec` | `(input: &[]u8): !MinizBuf` | Raw DEFLATE 解压至 RAII `MinizBuf`。 |
| `decompressToVecZlib` / `decompress_to_vec_zlib` | `(input: &[]u8): !MinizBuf` | Zlib 格式解压。 |
| `decompressToVecWithLimit` | `(input: &[]u8, max_size: usize): !MinizBuf` | 带最大字节数限制的 Raw DEFLATE 解压（超限返回 `OutputLimitExceeded`）。 |
| `decompressToVecZlibWithLimit` | `(input: &[]u8, max_size: usize): !MinizBuf` | 带最大字节数限制的 Zlib 解压。 |
| `compressToSlice` / `compressToSliceZlib` | `(input: &[]u8, out: []u8, level: u8): !usize` | 零堆分配压缩，直接写入调用方 `out` 切片，返回写入字节数。 |
| `decompressToSlice` / `decompressToSliceZlib` | `(input: &[]u8, out: []u8): !usize` | 零堆分配解压，直接写入调用方 `out` 切片，返回写入字节数。 |

### 校验和与工具函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `versionString` | `(): string` | 返回底层 `miniz` 版本字符串。 |
| `compressBound` | `(source_len: usize): usize` | 计算 `source_len` 字节输入在最坏情况下的压缩输出上界。 |
| `crc32` / `crc32Update` | `(&[]u8): u32` / `(u32, &[]u8): u32` | 单次与增量 IEEE CRC-32 校验和。 |
| `adler32` / `adler32Update` | `(&[]u8): u32` / `(u32, &[]u8): u32` | 单次与增量 Adler-32 校验和。 |
| `makeDeflateFlags` | `(level: u8, write_zlib_header: bool): !i32` | 校验 `level`（`0..=10`）并生成 `tdefl` 压缩标志位。 |

包同时导出底层 C FFI 原语（`cp_miniz_*`、`tdefl_*`、`tinfl_*`、`mz_*`）供高级流式管线调用。

## 代码示例

### 1. Zlib 压缩与限流解压（RAII `MinizBuf`）

```cp
import { compressToVecZlib, decompressToVecZlibWithLimit } from "miniz_oxide";

fn zlibRoundTrip(data: &[]u8): !usize {
    let compressed = compressToVecZlib(data, 6)?;
    let restored = decompressToVecZlibWithLimit(compressed.slice(), 1024 * 1024)?;
    // compressed 与 restored 在函数退出时自动 RAII 释放
    return restored.slice().len;
}
```

### 2. 零堆分配切片编解码

```cp
import { compressToSlice, decompressToSlice } from "miniz_oxide";

fn stackDeflate(input: &[]u8): !usize {
    let comp_buf: [512]u8 = undefined;
    let clen = compressToSlice(input, comp_buf[0..512], 6)?;

    let out_buf: [512]u8 = undefined;
    let dlen = decompressToSlice(&comp_buf[0..clen], out_buf[0..512])?;
    return dlen;
}
```

### 3. 内存单文件 ZIP 打包与提取

```cp
import { MinizBuf, crc32 } from "miniz_oxide";

fn zipExample(payload: &[]u8): !u32 {
    let archive = MinizBuf.zipSingleFile("hello.txt", payload)?;
    let extracted = MinizBuf.extractFile(archive.slice(), "hello.txt")?;
    return crc32(extracted.slice());
}
```

## 与源包差异与安全亮点

- **确定性 RAII 内存回收**：`MinizBuf` 实现 `__cp_deinit`，在正常返回或 `?` 错误早退时自动释放底层 C 堆内存，零泄漏。
- **资源耗尽防护**：处理外部不可信压缩流时可通过 `decompressToVecWithLimit` / `decompressToVecZlibWithLimit` 设定硬性解压大小上限（超限返回 `MinizError.OutputLimitExceeded`）。
- **双风格命名兼容**：同时提供 CP 惯用驼峰命名（`compressToVec`）与 Rust 原版蛇形别名（`compress_to_vec`）。
