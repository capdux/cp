[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# memchr

面向 CP 的零堆分配 16-lane 硬件 SIMD 字节与子串搜索引擎（对齐 Rust `memchr` v2）。

底层采用 `@Vector(16, u8)` 配合 `@splat` 与 `@reduce(.Or, ...)`，单次循环并行扫描 16 字节，提供正向/反向 1~3 字节查找、字节频次统计、批量偏移提取与子串匹配（`memmem`）。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **crates.io** |
| 原包名与版本 | `memchr` v2（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/BurntSushi/memchr` |
| 开源协议 | MIT（上游：MIT OR Unlicense） |

从 Rust 迁移至 CP 时保持原包名，零学习成本：

```rust
// Rust (crates.io: memchr)
use memchr::{memchr, memchr2, memrchr, memmem};
let pos = memchr(b'\n', haystack);
let sub = memmem::find(haystack, b"needle");
```

```cp
// CP (cap: memchr) — 16-lane SIMD，零堆分配
import { memchr, memchr2, memrchr, memmemFind } from "memchr";
const pos = memchr(0x0A, &haystack);        // 返回 i64 偏移，未命中为 -1
const sub = memmemFind(&haystack, &needle); // 返回 i64 偏移，未命中为 -1
```

## 安装

```bash
cap add memchr
```

## 实战示例

### 基础用法：16-lane SIMD 单/多字节与子串查找

```cp
import { memchr, memchr2, memchr3, memrchr, memmemFind, memmemRfind } from "memchr";

const data: []const u8 = "GET /api/v1/users HTTP/1.1\r\nHost: example.com\r\n\r\n";
const needle: []const u8 = "HTTP/1.1";

// 正向 16-lane SIMD 字节扫描
const sp = memchr(' ' as u8, &data);                 // 3
const delim = memchr2('\r' as u8, '\n' as u8, &data); // 26
const any3 = memchr3(':' as u8, '/' as u8, '?' as u8, &data); // 4

// 反向 16-lane SIMD 查找最后一个斜杠
const lastSlash = memrchr('/' as u8, &data);         // 22

// 正向 / 反向子串查找
const protoPos = memmemFind(&data, &needle);         // 18
const lastProto = memmemRfind(&data, &needle);       // 18
```

### 进阶用法：批量偏移提取与频次统计

```cp
import { memchrCount, memchrFindAll, countByte } from "memchr";

const csv: []const u8 = "id,name,role,region,status\n1,alice,admin,us,ok\n";

// 单遍 SIMD 统计目标字节总数
const commas = memchrCount(',' as u8, &csv); // 8
const newlines = countByte('\n' as u8, &csv); // 2

// 将所有逗号偏移写入栈上数组（零堆分配，自动按 out_offsets.len 截断保护）
let offsets: [16]usize = undefined;
const found = memchrFindAll(',' as u8, &csv, offsets[0..16]);
// offsets[0..found] 即为全部命中字节索引
```

## API 速查表

| 函数 | 签名 | 说明 |
| --- | --- | --- |
| `memchr` | `fn(needle: u8, haystack: &[]u8): i64` | 正向查找 `needle`，未找到返回 `-1`。 |
| `memchr2` | `fn(n1: u8, n2: u8, haystack: &[]u8): i64` | 正向查找 `n1` 或 `n2`，未找到返回 `-1`。 |
| `memchr3` | `fn(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64` | 正向查找 `n1`/`n2`/`n3`，未找到返回 `-1`。 |
| `memrchr` | `fn(needle: u8, haystack: &[]u8): i64` | 反向查找最后一个 `needle`，未找到返回 `-1`。 |
| `memrchr2` | `fn(n1: u8, n2: u8, haystack: &[]u8): i64` | 反向查找最后一个 `n1` 或 `n2`，未找到返回 `-1`。 |
| `memrchr3` | `fn(n1: u8, n2: u8, n3: u8, haystack: &[]u8): i64` | 反向查找最后一个 `n1`/`n2`/`n3`，未找到返回 `-1`。 |
| `memchrCount` | `fn(needle: u8, haystack: &[]u8): usize` | 统计 `needle` 出现次数。 |
| `countByte` | `fn(needle: u8, haystack: &[]u8): usize` | 高速字节计数（供 `bytecount` 等包复用）。 |
| `memchrFindAll` | `fn(needle: u8, haystack: &[]u8, out_offsets: []usize): usize` | 将命中偏移写入 `out_offsets`，返回写入条数。 |
| `memmemFind` | `fn(haystack: &[]u8, needle: &[]u8): i64` | 正向子串搜索，未找到返回 `-1`。 |
| `memmemRfind` | `fn(haystack: &[]u8, needle: &[]u8): i64` | 反向子串搜索，未找到返回 `-1`。 |

## 性能特性

- **16-lane 硬件 SIMD**：主循环每次处理 16 字节（`@Vector(16, u8)`），不足 16 字节尾部自动标量兜底。
- **零堆分配**：所有接口均直接操作只读字节切片 `&[]u8`。

## 协议

MIT — 对齐 Rust `memchr` v2 的纯 CP 净室实现。
