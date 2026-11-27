[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查](README_AGENT.md)

# split2

面向 CP 的零堆分配跨分片流式行切分器（支持 LF `\n` 与 CRLF `\r\n`）及无状态零拷贝行切分函数（对齐 npm `split2`）。

内置固定 4096 字节栈缓冲区，无缝处理跨任意 TCP / 文件读取分片截断的行（包括 `\r` 与 `\n` 分别位于相邻分片边界的场景），并支持 `max_line_len` 溢出防护。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| 源生态平台 | **npm** |
| 原包名与版本 | `split2`（CP 生态版 `v1.0.0`） |
| 上游仓库 | `https://github.com/mcollina/split2` |
| 开源协议 | ISC |

从 TypeScript / Node.js 迁移至 CP 保持原包名，零成本切换：

```ts
// TypeScript / Node.js (npm: split2)
import split2 from "split2";
stream.pipe(split2()).on("data", (line) => handleLine(line));
```

```cp
// CP (cap: split2) — 零堆分配流式与零拷贝行切分
import { LineSplitter, splitLines } from "split2";
let sp = LineSplitter.new();
const n = sp.pushChunk(&chunk, outBuf[0..4096], starts[0..64], lens[0..64]);
```

## 安装

```bash
cap add split2
```

## 实战示例

### 基础用法：跨分片流式行切分（自动处理跨边界 CRLF）

```cp
import { LineSplitter } from "split2";

let sp = LineSplitter.new();
let outBuf: [1024]u8 = undefined;
let starts: [16]usize = undefined;
let lens: [16]usize = undefined;

// 分片 1 末尾停在 '\r'（CRLF 被拆在两个分片中）
const c1: []const u8 = "hello\nworld\r";
const n1 = sp.pushChunk(&c1, outBuf[0..1024], starts[0..16], lens[0..16]);
// n1 == 1 -> 第 0 行为 outBuf[starts[0] .. starts[0] + lens[0]] == "hello"

// 分片 2 以 '\n' 开头，且末尾有一行未带换行符的尾部数据
const c2: []const u8 = "\nlast-line";
const n2 = sp.pushChunk(&c2, outBuf[0..1024], starts[0..16], lens[0..16]);
// n2 == 1 -> 第 0 行为 "world"（自动剥离 '\r'）

// 流结束（EOF）时冲刷剩余半行
let tail: [256]u8 = undefined;
const tailLen = sp.flush(tail[0..256]);
// tail[0..tailLen] == "last-line"
```

### 进阶与安全用法：无状态零拷贝切分、跳过空行与超长行溢出防护

```cp
import { LineSplitter, splitLines } from "split2";

// 1. 无状态零拷贝切分为 string 切片数组（兼容 LF / CRLF）
let slices: [8]string = undefined;
const count = splitLines("a\r\nb\nc", slices[0..8]);
// count == 3 -> slices[0] == "a", slices[1] == "b", slices[2] == "c"

// 2. 限制单行最大长度并跳过空行，防止网络流无换行符导致内存耗尽
let guarded = LineSplitter.withMaxLen(128);
guarded.skip_empty = true;
// 超长部分安全截断并置位 guarded.overflowed() == true（调用 guarded.reset() 复位）
```

## API 速查表

| 导出符号 | 签名 | 说明 |
| --- | --- | --- |
| `LineSplitter.new` | `static fn(): LineSplitter` | 创建默认 4096 字节上限（`SPLIT2_BUF_CAP`）的流式切分器。 |
| `LineSplitter.withMaxLen` | `static fn(max_len: usize): LineSplitter` | 创建自定义单行上限（最大钳位至 4096）的切分器。 |
| `LineSplitter.pushChunk` | `fn(*_, chunk: &[]u8, out_lines_buf: []u8, out_starts: []usize, out_lens: []usize): usize` | 喂入分片，将完整行打包写入 `out_lines_buf`，返回完整行数。 |
| `LineSplitter.flush` | `fn(*_, out_line: []u8): usize` | EOF 时冲刷缓冲区残余半行，返回写入字节数。 |
| `LineSplitter.pendingLen` | `fn(): usize` | 查询当前缓冲的未闭合半行字节数。 |
| `LineSplitter.overflowed` | `fn(): bool` | 查询是否发生过超长行截断。 |
| `LineSplitter.reset` | `fn(*_)` | 清空半行缓冲并重置溢出标志。 |
| `splitLines` | `fn(input: string, out_slices: []string): usize` | 无状态零拷贝行切分，写入 `out_slices` 并返回行数。 |

## 安全与限制

- **有界内存防护**：固定 4096 字节栈上缓冲，彻底避免缺少换行符的畸形流导致 OOM。
- **零堆分配**：流式与无状态接口均零堆分配。

## 协议

ISC — 受 npm `split2` 启发的纯 CP 净室实现。
