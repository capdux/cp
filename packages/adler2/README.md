[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `@cap/adler32`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:adler2` ([https://github.com/oyvindln/adler2](https://github.com/oyvindln/adler2))（原库名 `adler2` → CP 包名 `@cap/adler32`）

Zero-heap RFC 1950 Adler-32 checksum engine in pure CP, modeled after Rust's `adler2` crate. Includes deferred-modulo chunked streaming (`NMAX = 5552`), O(1) Rsync-style rolling window updates (`roll`), and distributed shard merging (`combineAdler32`).

零堆分配纯 CP RFC 1950 Adler-32 校验和引擎，对标 Rust `adler2`（8.3 亿+下载），支持分块延迟取模流式更新、O(1) 滑动窗口滚动校验（Rsync 增量同步）与分布式分片合并 `combineAdler32`。

## Installation

```bash
cap add @cap/adler32
```

## API

| Function / Method | Description |
| --- | --- |
| `adler32(data)` | One-shot RFC 1950 Adler-32 checksum |
| `Adler32.init()` / `fromValue(v)` | Initialize streaming hasher |
| `update(data)` / `updateByte(b)` | Feed bytes with `NMAX = 5552` deferred modulo |
| `roll(window_len, out_byte, in_byte)` | O(1) rolling window checksum update |
| `finalize()` / `reset()` | Retrieve `u32` digest or reset to `1` |
| `combineAdler32(a1, a2, len2)` | Combine checksums of two concatenated buffers |

## License

MIT
