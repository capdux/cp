# @cap/memmem

Zero-allocation byte-substring search engine with precompiled BMH + KMP `Finder`, reverse search `rfind`, and non-overlapping `count` for CP — clean-room MIT parity with Rust `memchr::memmem` (1.82B downloads on crates.io).

纯 CP 零堆分配字节子串搜索引擎，结合 Boyer-Moore-Horspool 坏字符快跳与 KMP 最坏 O(N+M) 线性防退化算法，支持二进制零字节帧搜索、反向查找与不重叠计数。

## Install / 安装

```bash
cap add @cap/memmem
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `find` | `(haystack: []const u8, needle: []const u8) -> i64` | First match index (`0` on empty needle, `-1` if absent) |
| `rfind` | `(haystack: []const u8, needle: []const u8) -> i64` | Last match index (`haystack.len` on empty needle, `-1` if absent) |
| `contains` | `(haystack: []const u8, needle: []const u8) -> bool` | Substring existence check |
| `count` | `(haystack: []const u8, needle: []const u8) -> usize` | Non-overlapping occurrence count |
| `Finder` | `init(needle)`, `findIn(h)`, `findFrom(h, start)`, `countIn(h)`, `needleLen()` | Precompiled BMH + KMP searcher (up to 256-byte needle) |

## Usage / 使用示例

```cp
import { Finder, find, rfind, count } from "@cap/memmem";

const idx = find("hello world world", "world");   // 6
const last = rfind("hello world world", "world"); // 12
const n = count("aaaaa", "aa");                   // 2
```
