[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# miniz_oxide

面向 CP 的原生无损 DEFLATE / zlib 压缩解压、带输出上限防护的解压、CRC32/Adler32 校验以及单文件 ZIP 打包/解包库，底层由 `miniz 3.0.2`（`-O3`）驱动并提供 `MinizBuf` 自动 RAII 内存释放。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [crates.io/crates/miniz_oxide](https://crates.io/crates/miniz_oxide) |
| **上游原包名** | `miniz_oxide`（CP 版本 `3.0.2`，底层 `miniz 3.0.2`） |
| **上游仓库** | https://github.com/Frommi/miniz_oxide |
| **许可证** | MIT |

从 Rust 迁移到 CP 保持原包名不变，同时提供驼峰命名与 Rust `snake_case` 别名：

```rust
// Rust
use miniz_oxide::deflate::compress_to_vec_zlib;
use miniz_oxide::inflate::decompress_to_vec_zlib;
```

```cp
// CP —— 保持原包名，原生 C (-O3) 性能 + MinizBuf 自动释放
import {
    compressToVecZlib, decompressToVecZlib,
    compress_to_vec_zlib, decompress_to_vec_zlib,
} from "miniz_oxide";
```

## 安装

```bash
cap add miniz_oxide
```

## 实战示例

### 1. DEFLATE 与 Zlib 压缩/解压（`*ToVec` 与 `*WithLimit`）

```cp
import {
    compressToVec, decompressToVec,
    compressToVecZlib, decompressToVecZlib,
    decompressToVecWithLimit, decompressToVecZlibWithLimit,
    MinizBuf,
} from "miniz_oxide";

fn demoZlib(payload: &[]u8): !void {
    // 压缩级别 0..10；返回的 MinizBuf 在离开作用域时通过 __cp_deinit 自动释放
    let compressed: MinizBuf = compressToVecZlib(payload, 6)?;

    // 带输出上限的解压，防止异常压缩比导致的资源耗尽
    let restored: MinizBuf = decompressToVecZlibWithLimit(compressed.slice(), 1024 * 1024)?;
    let raw_bytes: &[]u8 = restored.slice();

    // 原始 DEFLATE（不含 zlib 头尾）
    let raw_comp = compressToVec(payload, 6)?;
    let raw_back = decompressToVecWithLimit(raw_comp.slice(), 1024 * 1024)?;
}
```

### 2. 零堆切片到切片压缩与校验和（`compressToSlice*` / `decompressToSlice*`）

```cp
import {
    compressBound, compressToSliceZlib, decompressToSliceZlib,
    crc32, crc32Update, adler32, adler32Update, versionString,
} from "miniz_oxide";

fn demoZeroHeapSlice(input: &[]u8): !void {
    let bound = compressBound(input.len);
    mut comp_buf: [1024]u8 = [0; 1024];
    mut out_buf: [1024]u8 = [0; 1024];

    let comp_len = compressToSliceZlib(input, &mut comp_buf, 6)?;
    let orig_len = decompressToSliceZlib(&comp_buf[0..comp_len], &mut out_buf)?;

    // 增量校验和计算
    let c1 = crc32(input);
    let c2 = crc32Update(c1, input);
    let a1 = adler32(input);
    let a2 = adler32Update(a1, input);
    let ver = versionString();
}
```

### 3. 单文件 ZIP 打包与提取（`MinizBuf.zipSingleFile` / `extractFile`）

```cp
import { MinizBuf } from "miniz_oxide";

fn demoZip(content: &[]u8): !void {
    let archive = MinizBuf.zipSingleFile("hello.txt", content)?;
    let extracted = MinizBuf.extractFile(archive.slice(), "hello.txt")?;
    let data: &[]u8 = extracted.slice();
}
```

## 性能与安全亮点

- **原生 C 引擎（`miniz 3.0.2`，`-O3`）**：预编译静态库直连，提供极高吞吐的 DEFLATE/zlib/ZIP 编解码性能。
- **确定性 RAII 释放**：`MinizBuf.__cp_deinit` 自动调用 `cp_miniz_free` 释放底层堆缓冲区，错误早退也零泄漏。
- **零堆切片模式**：`compressToSlice*` 与 `decompressToSlice*` 完全复用调用方栈缓冲区，空间不足时返回 `MinizError.BufferTooSmall`。
- **资源耗尽防护**：`decompressToVecWithLimit` 与 `decompressToVecZlibWithLimit` 严格限制最大输出字节数，超限时返回 `MinizError.OutputLimitExceeded`。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
