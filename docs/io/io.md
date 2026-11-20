# CP 标准库：统一流式 I/O、缓冲管线与原生管道手册 (`cap:io`)

## 一、 架构总览与设计准则

`cap:io` 是 CP 语言官方内置的统一流式 I/O 标准库，基于 `Reader` / `Writer` / `Closer` / `Seeker` 四大解耦 Trait 契约，构建了内存缓冲流（`BufferReader` / `BufferWriter` / `ChunkedBuffer`）、增强型 4KB 缓冲装饰器（`BufferedReader<R>` / `BufferedWriter<W>`）、流式组合器（`LimitedReader` / `MultiReader` / `TeeReader` / `DiscardWriter` / `CountingWriter`）、流式拷贝引擎（`copyStream` / `copyN`）、防资源耗尽的 `MaxBuf` 字节限额守卫、OS 原生匿名管道（`Pipe` / `PipeReader` / `PipeWriter`）以及兼容独立模式与 BEAM 调度模式的标准控制台输出（`Console` / `Stdio`）。核心设计吸收自 Bun 原生 I/O 子系统（`/data/dev/bun/src/io/`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 统一流式 Trait 契约层                           │
│     Reader (read)  |  Writer (write)  |  Closer (close)  | Seeker (seek)│
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 内存流与动态缓冲      │  增强型缓冲与组合装饰器  │  OS 管道与限额守卫      │
│ BufferReader         │  BufferedReader<R>      │  Pipe.create()         │
│ BufferWriter (4KB)   │    readLine / readUntil │    PipeReader / Writer │
│ ChunkedBuffer        │    peek / discard       │  MaxBuf (防内存耗尽)    │
│   动态按块扩容       │    readFull             │  Console / Stdio       │
│                      │  BufferedWriter<W>      │  print / println       │
│                      │    刷盘阈值 & 大块直通   │                        │
│                      │  LimitedReader / Multi  │                        │
│                      │  TeeReader / Counting   │                        │
│                      │  DiscardWriter          │                        │
└──────────────────────┴─────────────────────────┴────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/io/traits.cp` | `// Source: /data/dev/bun/src/io/pipes.cp` | `IOError` 错误集、`SeekOrigin` 枚举及 `Reader` / `Writer` / `Closer` / `Seeker` 核心 Trait |
| `lib/io/buffer_stream.cp` | `// Source: /data/dev/bun/src/io/pipes.cp` | `BufferReader`、`BufferWriter`、`BufferedReader<R>`、`BufferedWriter<W>`、`LimitedReader<R>`、`MultiReader<R>`、`TeeReader<R, W>`、`DiscardWriter`、`CountingWriter<W>`、`ChunkedBuffer`、`copyStream` / `copyN` |
| `lib/io/max_buf.cp` | `// Source: /data/dev/bun/src/io/MaxBuf.cp` | `MaxBufKind` 与 `MaxBuf` 最大读取字节限制守卫 |
| `lib/io/pipe.cp` | `// Source: /data/dev/bun/src/io/PipeReader.cp & /data/dev/bun/src/io/PipeWriter.cp` | `PipeReader`、`PipeWriter` 与 `Pipe.create()` |
| `lib/io/stdio.cp` | `// Source: /data/dev/bun/src/io/source.cp` | `Console`、`Stdio`、`print` / `println` / `eprint` / `eprintln` |
| `lib/io/index.cp` | `// Source: /data/dev/bun/src/io/io.cp` | `cap:io` 统一公开导出入口 |

---

## 二、 核心 API 与增强能力详解

### 1. 增强型 `BufferedReader<R: Reader>` (`buffer_stream.cp`)

- 内置 `4096` 字节读缓存，支持：
  - `readByte(): u8!` / `read(buf: *[]u8): usize!`（大块 `>= 4096` 缓存空时自动直通底层）
  - `readFull(buf: *[]u8): usize!`：必须读满 `buf.len` 字节，提前遭遇 EOF 返回 `err.UnexpectedEof`
  - `peek(n: usize): ([]const u8)!` & `peekByte(): u8!`：预读缓存数据但不推进游标
  - `discard(n: usize): usize!`：高效跳过指定字节数
  - `readUntil(delim: u8, out: *[]u8): usize!` & `readUntilAlloc(allocator, delim): ([]u8)!`：读至分隔符（包含分隔符）
  - `readLine(out: *[]u8): ([]const u8)?!` & `readLineAlloc(allocator): ([]u8)?!`：按行读取并自动剥离 `\n` / `\r\n`，EOF 时返回 `null`
  - `buffered(): usize`：返回当前缓存内未读字节数

### 2. 增强型 `BufferedWriter<W: Writer>` (`buffer_stream.cp`)

- `init(writer: *W)`：默认 4KB 缓存与 `4096` 字节刷盘阈值；
- `withThreshold(writer: *W, threshold: usize)`：自定义自动刷盘阈值（如 `64` / `512`），写满阈值即刻自动 `flush()`；
- **大块数据零拷贝写直通**：当写入长度 `data.len >= flush_threshold` 时，先 `flush()` 已有缓存，再绕过中间拷贝直接写入底层 `writer`；
- `writeByte(b)`、`flush()`、`buffered()`、`available()`。

### 3. 组合流装饰器与动态 `ChunkedBuffer`

- **`LimitedReader<R>`**：限长读取装饰器，最多读取 `limit` 字节后返回 EOF（`0`），提供 `bytesRemaining()` 与 `isExhausted()`；
- **`MultiReader<R>`**：按序串联多个同构 `Reader`，前一个 EOF 后自动切换至下一个；
- **`TeeReader<R, W>`**：读取的同时将所有读出字节镜像写入目标 `Writer`（典型用于流式边读边算哈希/写日志）；
- **`DiscardWriter`**：零开销丢弃写入内容并统计 `bytesDiscarded()`；
- **`CountingWriter<W>`**：包装底层 `Writer` 并精确统计 `count()`，支持 `resetCount()`；
- **`ChunkedBuffer`**：基于 `std.mem.Allocator` 的动态按块（默认 4096 字节）可扩展字节流缓冲，完整实现 `Reader` + `Writer` + `Seeker`，支持 `write` / `writeByte` / `read` / `readByte` / `peek` / `seek` / `bytes` / `unreadBytes` / `chunkCount` / `reset` / `clear` / `toOwnedSlice`。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    SeekOrigin,
    BufferReader,
    BufferWriter,
    BufferedReader,
    BufferedWriter,
    LimitedReader,
    MultiReader,
    TeeReader,
    DiscardWriter,
    CountingWriter,
    ChunkedBuffer,
} from "cap:io";

pub fn demoEnhancedIo()! {
    const alloc = std.heap.page_allocator;

    // 1. ChunkedBuffer 动态扩容 + BufferedReader 按行/预读/精确读
    let cb = new ChunkedBuffer(alloc);
    defer cb.deinit();
    * = try cb.write("HEADER\r\nline1: alpha\nline2: beta");

    let br = new BufferedReader<ChunkedBuffer>(cb);
    const head_peek = try br.peek(6); // "HEADER"（不推进游标）
    * = head_peek;
    * = try br.discard(8); // 跳过 "HEADER\r\n"

    let line_buf: [128]u8 = undefined;
    const l1 = try br.readLine(line_buf[0..128]); // "line1: alpha"
    * = l1;

    // 2. BufferedWriter 自定义阈值与写直通 + CountingWriter
    let dst = new ChunkedBuffer(alloc);
    defer dst.deinit();
    let bw = new BufferedWriter<ChunkedBuffer>(dst);
    bw.setThreshold(16);
    * = try bw.write("small");
    // 仍停留在缓存中，写入 >= 16 字节将触发先刷盘再直通
    * = try bw.write("0123456789ABCDEF0123");
    try bw.flush();

    // 3. LimitedReader + TeeReader + CountingWriter + DiscardWriter
    let src = new BufferReader("hello world!");
    let lim = new LimitedReader<BufferReader>(src, 5);
    let tee_dst = new BufferWriter();
    let tee = new TeeReader<BufferReader, BufferWriter>(new BufferReader("hello"), tee_dst);
    let out: [16]u8 = undefined;
    const n = try tee.read(out[0..16]); // 5 ("hello")
    * = n;
    * = lim.bytesRemaining();
    * = tee_dst.bytes(); // "hello"
}
```
