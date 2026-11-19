# CP 标准库：统一流式 I/O、缓冲管线与原生管道手册 (`cap:io`)

## 一、 架构总览与设计准则

`cap:io` 是 CP 语言官方内置的统一流式 I/O 标准库，基于 `Reader` / `Writer` / `Closer` / `Seeker` 四大解耦 Trait 契约，构建了零堆分配的内存缓冲流（`BufferReader` / `BufferWriter`）、256 字节聚合缓存装饰器（`BufferedReader<R>` / `BufferedWriter<W>`）、流式拷贝引擎（`copyStream` / `copyN`）、防资源耗尽的 `MaxBuf` 字节限额守卫、OS 原生匿名管道（`Pipe` / `PipeReader` / `PipeWriter`）以及兼容独立模式与 BEAM 调度模式的标准控制台输出（`Console` / `Stdio`）。核心设计吸收自 Bun 原生 I/O 子系统（`/data/dev/bun/src/io/`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 统一流式 Trait 契约层                           │
│     Reader (read)  |  Writer (write)  |  Closer (close)  | Seeker (seek)│
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 内存切片与聚合缓冲流   │  OS 原生管道与限额守卫   │  控制台与终端交互       │
│ BufferReader         │  Pipe.create()          │  Console / console     │
│ BufferWriter (4KB)   │    PipeReader (fd)      │  Stdio / stdio         │
│ BufferedReader<R>    │    PipeWriter (fd)      │  print / println       │
│ BufferedWriter<W>    │  MaxBuf (防内存耗尽守卫)  │  eprint / eprintln     │
│ copyStream / copyN   │                         │                        │
└──────────────────────┴─────────────────────────┴────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

`lib/io/` 下全部 6 个源文件第 1 行均严格标记上游 Bun 源码谱系：

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/io/traits.cp` | `// Source: /data/dev/bun/src/io/pipes.cp` | `IOError` 错误集、`SeekOrigin` 枚举及 `Reader` / `Writer` / `Closer` / `Seeker` 核心 Trait |
| `lib/io/buffer_stream.cp` | `// Source: /data/dev/bun/src/io/pipes.cp` | `BufferReader`、`BufferWriter`、`BufferedReader<R>`、`BufferedWriter<W>` 以及泛型 `copyStream` / `copyN` |
| `lib/io/max_buf.cp` | `// Source: /data/dev/bun/src/io/MaxBuf.cp` | `MaxBufKind` 与 `MaxBuf` 最大读取字节限制守卫（防止超大输出耗尽内存） |
| `lib/io/pipe.cp` | `// Source: /data/dev/bun/src/io/PipeReader.cp & /data/dev/bun/src/io/PipeWriter.cp` | 基于 `std.c.pipe/read/write/close` 的 `PipeReader`、`PipeWriter` 与 `Pipe.create()` 工厂 |
| `lib/io/stdio.cp` | `// Source: /data/dev/bun/src/io/source.cp` | `Console`、`Stdio`、`print` / `println` / `eprint` / `eprintln`（自动区分 `@is_nest` BEAM 与原生终端模式） |
| `lib/io/index.cp` | `// Source: /data/dev/bun/src/io/io.cp` | `cap:io` 统一公开导出入口 |

---

## 二、 核心 API 与设计要点

### 1. 错误集与 Trait 契约 (`traits.cp`)

- **`IOError`**：`EndOfStream`、`UnexpectedEof`、`Closed`、`BufferFull`、`ShortWrite`、`InvalidSeek`。
- **`SeekOrigin`**：`Start`（从头定位）、`Current`（相对当前游标）、`End`（相对末尾定位）。
- **四大 Trait**：
  - `Reader`：`fn read(*_, buf: *[]u8): usize!`（返回 `0` 表示 EOF）
  - `Writer`：`fn write(*_, buf: []const u8): usize!`
  - `Closer`：`fn close(*_)`
  - `Seeker`：`fn seek(*_, offset: i64, origin: SeekOrigin): u64!`

### 2. 内存流与泛型缓冲装饰器 (`buffer_stream.cp`)

- **`BufferReader`**：直接包装 `[]const u8` 切片，零拷贝支持 `read`、`seek`、`remaining()`、`reset()`；越界寻址返回 `err.InvalidSeek`。
- **`BufferWriter`**：内置 4096 字节固定缓冲区，支持 `write`、`bytes()`、`len()`、`reset()`；空间耗尽时返回 `err.BufferFull`。
- **`BufferedReader<R: Reader>`**：内置 256 字节读缓存，提供高频单字节 `readByte()` 与分块 `read()`，读至 EOF 时 `readByte()` 返回 `err.EndOfStream`。
- **`BufferedWriter<W: Writer>`**：内置 256 字节写缓存，写满自动刷出，并提供显式 `flush()`。
- **泛型管线函数**：
  - `copyStream<R: Reader, W: Writer>(r: *R, w: *W): usize!`：以 256 字节分块全量传输直至 EOF；
  - `copyN<R: Reader, W: Writer>(r: *R, w: *W, limit: usize): usize!`：精确复制最多 `limit` 字节。

### 3. 资源限额守卫 (`MaxBuf`) 与原生管道 (`Pipe`)

- **`MaxBuf`**：每次收到数据块调用 `onReadBytes(bytes)` 扣减剩余配额；超限时返回 `err.BufferFull` 并置 `isExceeded() == true`，支持 `remaining()`、`reset()` 及 `MaxBuf.withKind(limit, kind)` 分类标记（`Stdout` / `Stderr` / `Custom`）。
- **`Pipe`**：调用 `Pipe.create()` 创建一对连通的内核匿名管道（`reader: PipeReader`、`writer: PipeWriter`），读写均满足 `Reader` / `Writer` / `Closer` 契约，关闭后再次操作安全返回 `err.Closed`。

---

## 三、 可运行代码示例

```typescript
import {
    SeekOrigin,
    BufferReader,
    BufferWriter,
    BufferedReader,
    BufferedWriter,
    copyStream,
    copyN,
    MaxBuf,
    MaxBufKind,
    Pipe,
} from "cap:io";

pub fn demoIoSubsystem() {
    // 1. BufferReader + Seek + BufferWriter + copyStream / copyN
    let r = new BufferReader("CP zero-copy I/O stream pipeline");
    let w = new BufferWriter();
    const total = copyStream(r, w) catch 0;
    * = total; // 32

    * = r.seek(3, SeekOrigin.Start) catch 0;
    let w_part = new BufferWriter();
    const partial = copyN(r, w_part, 9) catch 0; // "zero-copy"
    * = partial;

    // 2. BufferedReader / BufferedWriter 聚合缓存
    let raw_r = new BufferReader("ABCD");
    let br = new BufferedReader<BufferReader>(raw_r);
    const ch0 = br.readByte() catch 0; // 'A' (65)
    * = ch0;

    let raw_w = new BufferWriter();
    let bw = new BufferedWriter<BufferWriter>(raw_w);
    * = bw.write("buffered chunk") catch 0;
    bw.flush() catch {};

    // 3. MaxBuf 缓冲区字节限额守卫
    let guard = MaxBuf.withKind(1024, MaxBufKind.Stdout);
    const ok1 = guard.onReadBytes(512) catch false;   // true, remaining == 512
    const ok2 = guard.onReadBytes(600) catch false;   // false (err.BufferFull)
    * = ok1;
    * = ok2;

    // 4. OS 原生匿名管道 IPC
    let pipe = Pipe.create() catch unreachable;
    * = pipe.writer.write("ping-pipe") catch 0;
    let recv: [32]u8 = [0 as u8; 32];
    const n = pipe.reader.read(recv[0..32]) catch 0;
    * = n; // 9
    pipe.writer.close();
    pipe.reader.close();
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`src/io/` / `Bun.ArrayBufferSink`) | Node.js (`node:stream`) | CP 标准库 (`cap:io`) |
| :--- | :--- | :--- | :--- |
| **统一流接口** | Zig `Reader` / `Writer` + `pipes.cp` | `stream.Readable` / `stream.Writable` | `Reader` / `Writer` / `Closer` / `Seeker` Trait |
| **内存读写流** | `BufferReader` / `ArrayBufferSink` | `Readable.from(buf)` / `PassThrough` | `BufferReader` / `BufferWriter` |
| **缓冲装饰器** | `BufferedReader` / `BufferedWriter` | `highWaterMark` 内部队列 | `BufferedReader<R>` / `BufferedWriter<W>` |
| **流式传输与限长复制** | `pipe()` / `Bun.write(dst, src)` | `stream.pipeline()` | `copyStream(r, w)` / `copyN(r, w, limit)` |
| **输出限额防护** | `MaxBuf` (`src/io/MaxBuf.cp`) | `child_process` `maxBuffer` | `MaxBuf` (`onReadBytes` / `isExceeded`) |
| **匿名管道 IPC** | `PipeReader` / `PipeWriter` | `child.stdout` / `child.stdin` | `Pipe.create()` -> `PipeReader` / `PipeWriter` |
| **控制台输出** | `console.log` / `process.stdout` | `console.log` / `console.error` | `console.log` / `print` / `println` / `eprintln` |
