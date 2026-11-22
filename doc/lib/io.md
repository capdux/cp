# CP 标准库：统一流式 I/O、非阻塞管道与文件流手册 (`cap:io`)

> 参考来源：`/data/dev/bun/docs/runtime/file-io.mdx`, `/data/dev/bun/src/io/` | 吸收日期：2026-11-22

## 一、 架构总览与设计准则

`cap:io` 是 CP 语言官方内置的统一流式 I/O 标准库，基于 `Reader` / `Writer` / `Closer` / `Seeker` 四大解耦 Trait 契约，完整移植 Bun `src/io/` 的生产级流式与非阻塞状态机能力：包含 `IoSource` / `IoStatus` / `FileType` / `PollResult`、原生匿名与非阻塞管道（`Pipe.createNonblocking()` / `PipeReader` / `PipeWriter` 的 `EAGAIN` 与 `BrokenPipe` 状态机）、面向文件偏移与零拷贝的流式封装（`FileReader` / `FileWriter` / `openForWriting`）、防资源耗尽守卫 `MaxBuf`，以及增强型 `BufferedReader<R>` / `BufferedWriter<W>` 与组合流装饰器。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                       CP 统一流式 Trait 契约层                           │
│     Reader (read)  |  Writer (write)  |  Closer (close)  | Seeker (seek)│
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 内存流与增强型缓冲    │  非阻塞管道与流源探测    │  文件流与安全限额       │
│ BufferReader/Writer  │  IoSource / FileType    │  FileReader (pread/zcp)│
│ BufferedReader<R>    │  Pipe.createNonblocking │  FileWriter (pwrite)   │
│   readLine/readUntil │  PipeReader.attemptRead │    openForWriting      │
│   readUntilDelimiter │    EAGAIN -> WouldBlock │  MaxBuf / unlimited()  │
│   shift/fill/clear   │  PipeWriter.attemptWrite│    onReadBytesStatus   │
│ BufferedWriter<W>    │    EPIPE -> BrokenPipe  │  Console / Stdio       │
│ ChunkedBuffer / Copy │    pollReadable/Writable│                        │
└──────────────────────┴─────────────────────────┴────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/io/traits.cp` | `// Source: /data/dev/bun/src/io/pipes.cp` | `IOError` 错误集（含 `WouldBlock`/`BrokenPipe`/`AccessDenied`）、`SeekOrigin` 与 `Reader`/`Writer`/`Closer`/`Seeker` |
| `lib/io/source.cp` | `// Source: /data/dev/bun/src/io/source.cp & /data/dev/bun/src/io/pipes.cp` | `IoStatus`、`ReadState`、`WriteStatus`、`PollState`、`FileType`、`PollResult` 与 `IoSource`（`detectFileType`/`pollReadable`/`pollWritable`/`setNonblocking`） |
| `lib/io/maxBuf.cp` | `// Source: /data/dev/bun/src/io/MaxBuf.cp` | `MaxBufKind`、`MaxBuf`（`unlimited()`、`consumed()`、`getLimit()`、`onReadBytes()`、`onReadBytesStatus()`、`reset()`、`isExceeded()`） |
| `lib/io/pipe.cp` | `// Source: /data/dev/bun/src/io/PipeReader.cp & /data/dev/bun/src/io/PipeWriter.cp` | `Pipe.create()`、`Pipe.createNonblocking()`、`PipeReader`（`attemptRead`/`readToEnd`/`attachMaxBuf`/`pollReadable`）、`PipeWriter`（`attemptWrite`/`writeAll`/`pollWritable`/`isBrokenPipe`） |
| `lib/io/fileStream.cp` | `// Source: /data/dev/bun/src/io/openForWriting.cp & /data/dev/bun/src/sys/File.cp` | `FileReader`（`open`/`pread`/`seek`/`size`/`remaining`/`readToEnd`/`copyFileRangeTo`/`sendfileTo`）、`FileWriter`（`open`/`create`/`openAppend`/`openForWriting`/`pwrite`/`writeAll`/`sync`/`fdatasync`/`ftruncate`） |
| `lib/io/bufferStream.cp` | `// Source: /data/dev/bun/src/io/pipes.cp` | `BufferReader`、`BufferWriter`、`BufferedReader<R>`（含 `readUntilDelimiter`/`unreadSlice`/`shift`/`fill`/`clear`）、`BufferedWriter<W>`、`LimitedReader<R>`、`MultiReader<R>`、`TeeReader<R, W>`、`DiscardWriter`、`CountingWriter<W>`、`ChunkedBuffer`、`copyStream` / `copyN` |
| `lib/io/stdio.cp` | `// Source: /data/dev/bun/src/io/source.cp` | `Console`、`Stdio`、`print` / `println` / `eprint` / `eprintln` |
| `lib/io/index.cp` | `// Source: /data/dev/bun/src/io/io.cp` | `cap:io` 统一公开导出入口 |

---

## 二、 核心增强能力详解

### 1. 非阻塞管道状态机 (`Pipe` / `PipeReader` / `PipeWriter`)

- `Pipe.createNonblocking()` 底层调用 Linux `pipe2(O_CLOEXEC | O_NONBLOCK)` 原子创建非阻塞双端管道。
- `PipeReader.attemptRead(buf)` 返回 `PipeReadResult { status: IoStatus, bytes: usize, errno: i32 }`：
  - `EINTR` 自动重试；
  - `EAGAIN`/`EWOULDBLOCK` 返回 `IoStatus.WouldBlock`；
  - `0` 字节标记 `eof_reached` 并返回 `IoStatus.EndOfFile`；
  - 绑定 `MaxBuf` 后超限自动返回 `IoStatus.BufferFull`。
- `PipeWriter.attemptWrite(data)` 与 `writeAll(data)` 支持部分写偏移续传、`EAGAIN` -> `IoStatus.WouldBlock` 与 `EPIPE` -> `IoStatus.BrokenPipe` 状态标记。

### 2. 流式文件读写与零拷贝 (`FileReader` / `FileWriter`)

- `FileReader.copyFileRangeTo(dest_fd, len)` 与 `sendfileTo(dest_fd, len)` 直接通过内核零拷贝接口传输文件区间并自动推进读偏移。
- `FileWriter.openForWriting(path, mode, force_sync, out_nonblocking)` 对应 Bun `openForWriting` 策略，打开文件并基于文件类型与 TTY 检测自动配置非阻塞模式。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    IoStatus, MaxBuf, Pipe, FileReader, FileWriter, BufferReader, BufferedReader,
} from "cap:io";

pub fn demoPhaseAIo(): !void {
    let p = try Pipe.createNonblocking();
    defer p.close();

    let mb = new MaxBuf(1024);
    p.reader.attachMaxBuf(mb);

    let buf: [64]u8 = undefined;
    const empty_res = p.reader.attemptRead(buf[0..64]);
    // empty_res.status == IoStatus.WouldBlock

    * = p.writer.attemptWrite("ping");
    const ok_res = p.reader.attemptRead(buf[0..64]);
    * = ok_res.bytes; // 4

    let br = new BufferedReader<BufferReader>(new BufferReader("a\nb\nc"));
    br.shift();
    let line_buf: [32]u8 = undefined;
    * = try br.readUntilDelimiter('\n', line_buf[0..32]);
}
```

---

## 上游 Bun I/O 行为契约与边界注意事项

1. **非阻塞管道状态机（`EAGAIN` / `EPIPE`）**：
   - `Pipe.createNonblocking()` 创建的 `PipeReader` / `PipeWriter` 遵循 Bun `src/io/PipeReader.cp` 契约：缓冲区暂无数据时 `attemptRead` 返回 `IoStatus.WouldBlock`（不视为错误）；对端关闭后写入返回 `BrokenPipe`，可通过 `pollReadable` / `pollWritable` 配合事件循环无阻塞驱动。
2. **防资源耗尽限额（`MaxBuf`）**：
   - 读取不可信网络流、子进程输出或未知大小文件时，务必通过 `attachMaxBuf(new MaxBuf(limit))` 绑定上限；超过限额时 `onReadBytesStatus` 立即中止读取，避免无限流耗尽内存。
3. **短读/短写与 `BufferedWriter` 刷盘**：
   - 底层 `Reader.read` 允许返回少于请求长度的字节数（返回 `0` 表示 EOF），循环聚合请使用 `copyStream` / `copyN` / `PipeReader.readToEnd`；使用 `BufferedWriter` 时在关闭流前必须调用 `flush()` 避免尾部缓冲数据丢失。
4. **标准流与惰性文件引用（参考 `/data/dev/bun/docs/runtime/file-io.mdx`）**：
   - `Stdio` / `Console` 封装标准输入输出与错误流；面向文件偏移的 `FileReader` (`pread`) 与 `FileWriter` (`pwrite`) 支持定位读写且不影响内核共享文件偏移指针。
5. **增量字节汇聚器与流式批刷盘（`arrayBufferSink.cp`，吸收自 `/data/dev/bun/src/runtime/webcore/ArrayBufferSink.cp` & `/data/dev/bun/docs/runtime/streams.mdx`）**：
   - `ArrayBufferSink` 提供 8KB 固定零堆分配缓冲区（`ARRAY_BUFFER_SINK_CAP = 8192`），支持 `write`（UTF-8/二进制）、`writeLatin1`（ISO-8859-1 转 UTF-8）、`writeUtf16`（含代理对解码与孤立代理转 `U+FFFD`）；
   - **流式刷盘模式（`stream: true`）**：按 `highWaterMark` 触发 `shouldFlush()` 后调用 `flush(out)` 导出当前分块并归零内部缓冲继续复用，避免频繁碎片化 `socket.write()`；
   - **终态契约（`end(out)`）**：调用 `end()` 后标记终态，后续写入返回 `ArrayBufferSinkError.SinkEnded` 直至显式调用 `start(opts)` 重置。

```cp
import { ArrayBufferSink, ArrayBufferSinkOptions } from "cap:io";

let sink = ArrayBufferSink.init();
sink.start(ArrayBufferSinkOptions { stream: true, high_water_mark: 1024 });
* = sink.write("hel")?;
let chunk: [64]u8 = [0; 64];
const res = sink.flush(chunk[0..])?; // res.bytes_flushed == 3, 内部缓冲自动清零复用
* = res;
```
6. **子进程静态缓冲区非阻塞管道写入状态机（`staticPipeWriter.cp`，吸收自 `/data/dev/bun/src/runtime/api/bun/subprocess/StaticPipeWriter.cp`）**：
   - 导出：`StaticPipeState`（`Idle` / `Writing` / `Closed` / `Errored`）、`StaticPipeWriteStatus`（`Pending` / `Drained` / `EndOfFile`）、`StaticPipeWriterError`（`AlreadyStarted` / `NotWriting` / `InvalidState`）、`StaticPipeStep`、`StaticPipeWriter`（`init`/`ref`/`deref`/`getBuffer`/`remaining`/`totalBytes`/`progressPermille`/`isTerminal`/`start`/`onWrite`/`drainToBuf`/`onError`/`close`）；
   - 面向子进程 `stdin` 静态切片投喂：通过 `onWrite(amount, status)` 零拷贝推进未发送切片（自动钳位防止越界），写完或收到 `EndOfFile` 时自动关闭并 `detach` 数据源，支持 `drainToBuf` 确定性分片拉取。

```cp
import { StaticPipeWriter } from "cap:io";

let w = StaticPipeWriter.init("hello pipe");
* = w.start()?;
let chunk: [5]u8 = [0; 5];
const n1 = w.drainToBuf(chunk[0..])?; // 5 ("hello")
const n2 = w.drainToBuf(chunk[0..])?; // 5 (" pipe"), 自动转入 Closed
* = n1; * = n2;
```
7. **子进程输出流七态读取状态机（`subprocessReadable.cp`，吸收自 `/data/dev/bun/src/runtime/api/bun/subprocess/Readable.cp`）**：
   - 导出：`SUBPROCESS_READABLE_BUF_CAP` (`4096`)、`SUBPROCESS_PIPE_OVERHEAD_BYTES` (`64`)、`SubprocessReadableKind`（`Fd` / `Memfd` / `Pipe` / `Inherit` / `Ignore` / `Closed` / `Buffer`）、`SubprocessReadableError`（`NotPipe` / `BufferOverflow` / `AlreadyClosed`）、`SubprocessReadable`（`initInherit`/`initIgnore`/`initFd`/`initMemfd`/`initPipe`/`initBuffer`/`memoryCost`/`hasPendingActivity`/`ref`/`unref`/`appendPipeChunk`/`finishPipeToBuffer`/`slice`/`close`/`isClosed`）；
   - 完整还原 Bun 子进程 `stdout`/`stderr` 读取生命周期：活跃管道态（`Pipe`）持续 `appendPipeChunk` 并在 EOF 时通过 `finishPipeToBuffer()` 零分配收敛为只读 `Buffer` 态，自动释放 `pending_activity` 与事件循环 `ref`，同时提供精确 `memoryCost()` 账本统计与 `max_bytes` 溢出防护。

```cp
import { SubprocessReadable, SubprocessReadableKind } from "cap:io";

let r = SubprocessReadable.initPipe(7, 1024);
* = r.appendPipeChunk("out: ")?;
* = r.appendPipeChunk("ok")?;
const data = r.finishPipeToBuffer()?; // "out: ok", r.kind == SubprocessReadableKind.Buffer
* = data;
```

---

## 收敛子模块深度文档

- [stream.md](io/stream.md)
