# CP 标准库：Zlib / Gzip 压缩子系统开发与使用手册 (`cap:pack`)

## 一、 架构总览与设计准则

`cap:pack`（在包管理器中兼容映射为 `cap:compress`）是 CP 语言官方内置的工业级无损数据压缩、解压与归档标准库。

```text
┌────────────────────────────────────────────────────────────────────────┐
│                        CP 用户层应用 (Application)                      │
├───────────────────────────────────┬────────────────────────────────────┤
│     单发极速 API (One-shot)       │    流式分块管道 (Streaming)        │
│  gzip / gunzip / deflate / inflate│ GzipStream / GunzipStream / Deflate│
├───────────────────────────────────┴────────────────────────────────────┤
│              GzipHeader (RFC 1952) 帧头/帧尾与校验和管理               │
├────────────────────────────────────────────────────────────────────────┤
│     NativeZlib (底层 C-FFI 容器) + ZStream 状态机 (@alignCast 堆安全)  │
├────────────────────────────────────────────────────────────────────────┤
│           vendor/zlib (zlib-ng 高性能纯静态库 libz.a，零 .so 依赖)      │
│           [AVX2 / AVX-512 / PCLMULQDQ / NEON 硬件向量极限吞吐]          │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 核心设计准则

1. **100% 独立纯静态内联（Zero Dynamic Dependency）**：
   - 彻底废除任何依赖宿主操作系统 `libz.so.1`（Linux）、`libz.dylib`（macOS）或 `zlib1.dll`（Windows）的动态链接方式；
   - 由 `cap` 构建工具链将 `vendor/zlib` 纯静态库（`libz.a`）直接内联入目标二进制文件，确保在无 C 运行环境的精简 Docker 容器中即开即用。
2. **多模式规范全覆盖**：
   - **RFC 1952 (Gzip)**：标准 10 字节魔数头（`0x1F, 0x8B`）、Deflate 载荷与 8 字节小端序校验尾（CRC32 + 原文尺寸 mod 2^32）；
   - **RFC 1950 (Zlib)**：标准 2 字节头与 Adler32 校验尾；
   - **RFC 1951 (DeflateRaw)**：无任何容器外壳的纯裸 Deflate 字节流，供 HTTP 协议分块传输或 ZIP 条目直通；
   - **Auto 智能嗅探**：解压时自动识别输入流是 Gzip 还是 Zlib，无需上层业务手动判断。
3. **动态倍增自愈内存保障（Dynamic Reallocation）**：
   - 解压阶段优先通过帧尾 `ISIZE` 启发式预估初始容量；遇到大尺寸未标明长度的流时，当输出缓冲区耗尽（`avail_out == 0`）时，自动触发几何倍增扩容（`cap * 2`），确保任意大尺寸载荷安全解压。
4. **堆安全指针稳定性**：
   - 底层 zlib 状态机在初始化时会在内部保存指向自身 `z_stream` 结构的指针。流式处理类内部采用 `@alloc` 结合 `@alignCast` 进行堆稳定分配，彻底消除由于结构体值拷贝导致的指针悬空。

---

## 二、 模块导入与快速入门

在 CP 项目中，直接通过内置标准库协议 `cap:pack` 导入：

```typescript
import {
    gzip,
    gunzip,
    deflate,
    inflate,
    deflateRaw,
    inflateRaw,
    GzipStream,
    GunzipStream,
} from "cap:pack";
```

### 1. 单发便捷压缩与解压 (One-shot API)

适用于内存中已具备完整缓冲区的快速处理场景：

```typescript
import { gzip, gunzip, deflate, inflate } from "cap:pack";

pub fn main()! {
    let raw = "Hello CP! High-performance industrial Gzip compression.";

    // 1. 标准 Gzip 压缩 (RFC 1952)
    let compressed_gz = gzip(raw, 6); // 默认级别 6 (1-9)
    @print("Gzip 压缩前: {d} 字节, 压缩后: {d} 字节\n", raw.len, compressed_gz.len);

    // 2. Gzip 解压 (自动校验帧头与 CRC32 帧尾)
    let decompressed_gz = gunzip(compressed_gz);
    if (decompressed_gz != null) {
        @print("Gzip 解压产物: {s}\n", decompressed_gz.?);
    }

    // 3. 标准 Zlib Deflate 压缩 (RFC 1950)
    let compressed_zlib = deflate(raw, 6);
    let decompressed_zlib = inflate(compressed_zlib);
    if (decompressed_zlib != null) {
        @print("Zlib 解压产物: {s}\n", decompressed_zlib.?);
    }
}
```

---

## 三、 分块流式处理管道 (Streaming Chunked Pipeline)

当处理超大文件、网络代理转发或 HTTP Chunked 响应体时，全量一次性读入内存会产生巨大的内存开销。流式分块管道允许按块写入（Chunked Write），边压边出。

### 1. 分块流式压缩 (`GzipStream` / `DeflateStream`)

```typescript
import { GzipStream } from "cap:pack";

pub fn compressFileInChunks(reader: any) {
    // 创建流式压缩器 (默认压缩级别 6)
    let stream_opt = GzipStream.create(6);
    if (stream_opt == null) {
        @print("创建压缩流失败\n");
        return;
    }
    let compressor = stream_opt.?;

    // 模拟持续接收数据块
    let chunk1 = "Part 1 of incoming large data stream... ";
    let chunk2 = "Part 2 with more payload bytes... ";

    let out1 = compressor.write(chunk1);
    let out2 = compressor.write(chunk2);

    // 完成流式压缩，刷出剩余数据与 8 字节校验尾，并释放底层状态机
    let out_final = compressor.finish();

    // 最终压缩产物为: out1 + out2 + out_final
}
```

### 2. 分块流式解压 (`GunzipStream` / `InflateStream`)

```typescript
import { GunzipStream } from "cap:pack";

pub fn decompressStream(received_chunks: [][]const u8) {
    // 创建流式解压器 (支持 Auto 智能嗅探 Zlib/Gzip)
    let stream_opt = GunzipStream.create();
    if (stream_opt == null) {
        @print("创建解压流失败\n");
        return;
    }
    let decompressor = stream_opt.?;

    let i: usize = 0;
    while (i < received_chunks.len) {
        let piece = decompressor.write(received_chunks[i]);
        if (piece.len > 0) {
            // 将解压产物投递至下游消费者
            processOutput(piece);
        }
        i += 1;
    }

    // 刷空解压缓冲并关闭流
    let trailing = decompressor.finish();
    if (trailing.len > 0) {
        processOutput(trailing);
    }
}
```

### 3. 通用底层流式类 (`ZlibStreamCompressor` / `ZlibStreamDecompressor`)

若需精细定制 `windowBits`（如裸 Deflate 或非标准窗口大小），可直接调用通用流式类：

```typescript
import {
    ZlibStreamCompressor,
    ZlibStreamDecompressor,
    WINDOW_BITS_RAW,
    WINDOW_BITS_ZLIB,
    WINDOW_BITS_GZIP,
    WINDOW_BITS_AUTO,
} from "cap:pack";

// 创建裸 Deflate 流 (RFC 1951，无头无尾)
let raw_comp = ZlibStreamCompressor.create(WINDOW_BITS_RAW, 9);
let raw_decomp = ZlibStreamDecompressor.create(WINDOW_BITS_RAW);
```

---

## 四、 RFC 1952 帧头与帧尾解析器 (`GzipHeader`)

`GzipHeader` 提供无需启动完整解压器即可直接解析、校验 Gzip 二进制头尾的轻量级工具：

```typescript
import { GzipHeader, GZIP_MAGIC_ID1, GZIP_MAGIC_ID2 } from "cap:pack";

pub fn inspectGzip(buffer: []const u8) {
    // 1. 魔数嗅探
    if (buffer.len < 10 || buffer[0] != GZIP_MAGIC_ID1 || buffer[1] != GZIP_MAGIC_ID2) {
        @print("不是有效的 Gzip 格式文件\n");
        return;
    }

    // 2. 解析 10 字节标准头
    let header_opt = GzipHeader.parse(buffer);
    if (header_opt != null) {
        let hdr = header_opt.?;
        @print("Flags: 0x{x}, 修改时间: {d}, 操作系统: {d}\n", hdr.flags, hdr.mtime, hdr.os);
    }

    // 3. 校验末尾 8 字节 Trailer 与解压后原文是否吻合
    if (buffer.len >= 18) {
        let trailer = buffer[buffer.len - 8 .. buffer.len];
        let is_valid = GzipHeader.verifyTrailer(trailer, decompressed_payload);
        @print("Trailer CRC32 与长度校验: {}\n", is_valid);
    }
}
```

---

## 五、 面向对象便携包装矩阵

为满足面向对象设计习惯，`cap:pack` 提供对应格式的静态包装类：

```typescript
import {
    Gzip,
    Gunzip,
    DeflateCodec,
    InflateCodec,
    DeflateRawCodec,
} from "cap:pack";

// 等价于 gzip(src, level) 与 gunzip(src)
let gz_data = Gzip.compress(src, 6);
let gz_raw = Gunzip.decompress(gz_data);

// 等价于 deflate(src, level) 与 inflate(src)
let zlib_data = DeflateCodec.compress(src, 6);
let zlib_raw = InflateCodec.decompress(zlib_data);

// 裸流编解码
let raw_data = DeflateRawCodec.compress(src, 6);
let raw_orig = DeflateRawCodec.decompress(raw_data);
```

---

## 六、 WindowBits 参数驱动矩阵

底层由 `NativeZlib.deflateInit2_` 与 `NativeZlib.inflateInit2_` 驱动，核心参数含义如下：

| 常量名称 | 取值 | 模式 | 描述 |
| :--- | :---: | :--- | :--- |
| `WINDOW_BITS_RAW` | `-15` | **RFC 1951** 裸流 | 不产生任何头尾校验；解压时不校验任何校验和 |
| `WINDOW_BITS_ZLIB` | `15` | **RFC 1950** Zlib | 输出 2 字节头与 4 字节 Adler32 校验尾 |
| `WINDOW_BITS_GZIP` | `31` (15 + 16) | **RFC 1952** Gzip | 自动生成 10 字节标准头与 8 字节 CRC32/ISIZE 尾 |
| `WINDOW_BITS_AUTO` | `47` (15 + 32) | **自动嗅探** | 仅用于解压端，根据输入首字节自动切换 Zlib 或 Gzip |

---

## 七、 与 Node.js / Bun API 映射指南

| 功能需求 | Node.js (`node:zlib`) | Bun (`bun`) | CP 语言 (`cap:pack`) |
| :--- | :--- | :--- | :--- |
| **Gzip 压缩** | `zlib.gzipSync(buf)` | `bun.gzipSync(buf)` | `gzip(buf)` / `Gzip.compress(buf)` |
| **Gzip 解压** | `zlib.gunzipSync(buf)` | `bun.gunzipSync(buf)` | `gunzip(buf)` / `Gunzip.decompress(buf)` |
| **Zlib 压缩** | `zlib.deflateSync(buf)` | `bun.deflateSync(buf)` | `deflate(buf)` / `DeflateCodec.compress(buf)` |
| **Zlib 解压** | `zlib.inflateSync(buf)` | `bun.inflateSync(buf)` | `inflate(buf)` / `InflateCodec.decompress(buf)` |
| **裸 Deflate 压缩**| `zlib.deflateRawSync(buf)` | `bun.deflateRawSync(buf)` | `deflateRaw(buf)` / `DeflateRawCodec.compress(buf)` |
| **裸 Deflate 解压**| `zlib.inflateRawSync(buf)` | `bun.inflateRawSync(buf)` | `inflateRaw(buf)` / `DeflateRawCodec.decompress(buf)` |
| **流式分块压缩** | `new zlib.Gzip()` | `new ZlibCompressorArrayList` | `GzipStream.create()` / `ZlibStreamCompressor.create()` |
| **流式分块解压** | `new zlib.Gunzip()` | `new NewZlibReader(writer)` | `GunzipStream.create()` / `ZlibStreamDecompressor.create()` |
