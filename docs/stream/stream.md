# CP 标准库：统一流式抽象门面手册 (`cap:stream`)

## 一、 架构总览与设计准则

`cap:stream` 是 CP 语言面向流式数据处理场景的标准门面模块（Facade），直接与底层 `cap:io` 共享零开销的 Trait 契约与实现（无任何中间适配器包装损耗），确保通过 `cap:stream` 与 `cap:io` 导入的 `Reader` / `Writer` / `BufferReader` / `Pipe` 等类型在编译期完全同一、可无缝互操作。设计吸收自 Bun 统一流管道体系（`/data/dev/bun/src/io/pipes.cp`）。

```text
┌─────────────────────────────────────────────────────────────────────────┐
│                      cap:stream 统一流式编程门面                         │
├──────────────────────┬─────────────────────────┬────────────────────────┤
│ 核心 Trait 契约       │  内存与聚合缓冲流        │  限额守卫与管道传输     │
│ Reader / Writer      │  BufferReader           │  copyStream<R, W>      │
│ Closer / Seeker      │  BufferWriter (4KB)     │  copyN<R, W>           │
│ IOError / SeekOrigin │  BufferedReader<R>(256B)│  MaxBuf / MaxBufKind   │
│                      │  BufferedWriter<W>(256B)│  Pipe / PipeReader /   │
│                      │                         │  PipeWriter            │
└──────────────────────┴─────────────────────────┴────────────────────────┘
                      ▼ 零开销直接重导出 (Zero-cost Re-export)
                             lib/io/ (cap:io)
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 上游谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/stream.cp` | `// Source: /data/dev/bun/src/io/pipes.cp` | 统一重导出 `IOError`、`SeekOrigin`、`Reader`、`Writer`、`Closer`、`Seeker`、`BufferReader`、`BufferWriter`、`BufferedReader`、`BufferedWriter`、`copyStream`、`copyN`、`MaxBuf`、`MaxBufKind`、`PipeReader`、`PipeWriter`、`Pipe` |

---

## 二、 核心组合模式

1. **Trait 多态管线（`copyStream` / `copyN`）**：
   任何实现 `Reader` 的数据源（`BufferReader`、`BufferedReader<R>`、`PipeReader` 或用户自定义类）均可通过编译期单态化泛型函数 `copyStream(r, w)` / `copyN(r, w, limit)` 零虚表开销地泵送至任意 `Writer`。
2. **缓冲叠加装饰器（`BufferedReader<R>` / `BufferedWriter<W>`）**：
   在细粒度逐字节解析或高频小块写入场景中，用 256 字节栈内联缓存批量聚合读写操作，显著降低底层调用频次。
3. **背压与内存上限保护（`MaxBuf`）**：
   在处理外部输入流或子进程管道输出时，将 `MaxBuf` 与读取循环联动，超限时立刻返回 `err.BufferFull` 并中止继续分配。

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
    copyStream,
    copyN,
    MaxBuf,
    MaxBufKind,
    Pipe,
} from "cap:stream";

pub fn demoStreamPipeline() {
    // 1. 内存流 + 限长拷贝 + 全量泵送
    const payload = "stream-header:stream-body";
    let src = new BufferReader(payload[0..payload.len]);

    let hdr_out = new BufferWriter();
    const h_len = copyN(src, hdr_out, 13) catch 0; // "stream-header"
    * = h_len;

    * = src.seek(1, SeekOrigin.Current) catch 0;   // 跳过 ':'
    let body_out = new BufferWriter();
    const b_len = copyStream(src, body_out) catch 0; // "stream-body"
    * = b_len;

    // 2. BufferedWriter 聚合输出 + MaxBuf 限额守卫
    let guard = MaxBuf.withKind(64, MaxBufKind.Custom);
    let sink = new BufferWriter();
    let bw = new BufferedWriter<BufferWriter>(sink);

    const chunk = "aggregated-packet";
    if (guard.onReadBytes(chunk.len) catch false) {
        * = bw.write(chunk) catch 0;
        bw.flush() catch {};
    }

    // 3. 原生 OS Pipe 跨端流式传输
    let pipe = Pipe.create() catch unreachable;
    * = pipe.writer.write("pipe-ok") catch 0;
    let buf: [16]u8 = [0 as u8; 16];
    const n = pipe.reader.read(buf[0..16]) catch 0;
    * = n;
    pipe.writer.close();
    pipe.reader.close();
}
```

---

## 四、 与 Bun / Node.js API 对照表

| 功能特性 | Bun (`ReadableStream` / `Bun.ArrayBufferSink`) | Node.js (`node:stream`) | CP 标准库 (`cap:stream`) |
| :--- | :--- | :--- | :--- |
| **流式接口契约** | WHATWG Streams / Zig Reader & Writer | `Readable` / `Writable` / `Duplex` | `Reader` / `Writer` / `Closer` / `Seeker` |
| **管道拷贝** | `readable.pipeTo(writable)` | `stream.pipeline(src, dst)` | `copyStream(r, w)` / `copyN(r, w, limit)` |
| **缓冲聚合** | `ArrayBufferSink` | `highWaterMark` 内部缓冲 | `BufferedReader<R>` / `BufferedWriter<W>` |
| **流大小上限保护** | `MaxBuf` | `maxBuffer` 选项 | `MaxBuf` (`MaxBufKind.Stdout/Stderr/Custom`) |
| **进程间匿名管道** | `PipeReader` / `PipeWriter` | `child_process` stdio pipes | `Pipe.create()` |
