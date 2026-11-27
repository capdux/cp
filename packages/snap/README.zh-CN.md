[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent Quick Ref](README_AGENT.md)

# snap

> **源包溯源**：Clean-Room MIT 纯 CP 实现，对标 [`crates.io:snap`](https://github.com/BurntSushi/rust-snappy)（BSD-3-Clause）| CP 包版本：`1.0.0` | 零外部依赖

零堆分配的 Google Snappy Raw Block 无损压缩与解压引擎。完整支持 varint32 原文长度头、Literal / Copy-1 / Copy-2 / Copy-4 标签以及重叠 RLE 回指解压。

## 零成本迁移对照（Rust → CP）

直接沿用原包名 `"snap"`，无需改名即可完成迁移：

| Rust (`crates.io:snap::raw`) | CP (`snap`) |
| :--- | :--- |
| `use snap::raw::{Encoder, Decoder, max_compress_len, decompress_len};` | `import { compress, decompress, maxCompressedLen, uncompressedLen } from "snap";` |
| `max_compress_len(src.len())` | `maxCompressedLen(src.len)` |
| `Encoder::new().compress(src, dst)` | `compress(src, dst)` |
| `decompress_len(compressed)` | `uncompressedLen(compressed)` |
| `Decoder::new().decompress(compressed, out)` | `decompress(compressed, out)` |

## 核心 API

| 签名 | 说明 |
| :--- | :--- |
| `maxCompressedLen(input_len: usize): usize` | 计算最坏情况下的压缩输出上限（`32 + len + len / 6`）。 |
| `uncompressedLen(src: &[]u8): i64` | 解析 varint32 原文长度头，格式错误返回 `-1`。 |
| `compress(src: &[]u8, dst: []u8): i64` | 将 `src` 压缩写入 `dst`，成功返回压缩字节数，缓冲不足或失败返回 `-1`。 |
| `decompress(src: &[]u8, dst: []u8): i64` | 将 Snappy 块 `src` 解压写入 `dst`，成功返回解压字节数，数据损坏或缓冲不足返回 `-1`。 |

## 实战示例

### 基础压缩与解压往返

```cp
import { compress, decompress, maxCompressedLen, uncompressedLen } from "snap";

fn demoRoundTrip() {
    const input = "hello snappy hello snappy hello snappy hello snappy!";
    let compressed: [128]u8 = undefined;

    const c_len = compress(input, compressed[0..]);
    if (c_len > 0) {
        const expected = uncompressedLen(compressed[0..(c_len as usize)]);
        let restored: [128]u8 = undefined;
        const d_len = decompress(compressed[0..(c_len as usize)], restored[0..]);
        // expected == input.len as i64 && d_len == expected
        * = expected; * = d_len;
    }
    * = maxCompressedLen(input.len);
}
```

### 异常输入与缓冲区不足防护

```cp
import { compress, decompress } from "snap";

fn demoErrorHandling() {
    const payload = "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa";
    let tiny: [4]u8 = undefined;
    // 目标缓冲不足 -> 安全返回 -1
    const rc = compress(payload, tiny[0..]);
    // rc == -1

    const bad_block = [0xFF as u8, 0xFF as u8, 0xFF as u8, 0xFF as u8, 0xFF as u8, 0xFF as u8];
    let out: [64]u8 = undefined;
    // 畸形 varint / 截断块 -> 安全返回 -1，不触发 panic
    const rd = decompress(bad_block[0..], out[0..]);
    // rd == -1
    * = rc; * = rd;
}
```

## 性能与安全亮点

- **零堆分配**：4096 槽位哈希表（`let table: [4096]u16 = undefined`）直接在栈上分配，零堆开销且跳过无用清零。
- **`@memcpy` 批量拷贝加速**：Literal 块与非重叠 Copy-1/2/4 均通过 `@memcpy` 块拷贝完成；重叠 RLE（`offset < copy_len`）自动走逐字节回指扩展。
- **严格边界与畸形块校验**：每个标签均校验 varint32、非零偏移、历史窗口边界与目标容量，任何损坏输入一律返回 `-1`。
- **标准线格式兼容**：与 Google Snappy 及 Rust `snap::raw` 二进制完全互通。

## 许可协议

MIT
