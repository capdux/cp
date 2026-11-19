# CP 标准库：全栈压缩、归档与校验和子系统开发与使用手册 (`cap:pack`)

## 一、 架构总览与设计哲学

`cap:pack`（在包管理器中同时兼容映射为 `cap:compress`）是 CP 语言官方内置的**工业级无损数据压缩、流式解压、零拷贝归档（Tar / PKZIP）与完整性校验和（CRC32 / Adler32）**全栈标准库。模块完整吸收自 Bun（`/data/dev/bun/src/zlib/`、`/data/dev/bun/src/zstd/`、`/data/dev/bun/src/brotli/`、`/data/dev/bun/src/libarchive/`），并针对 CP 强类型、确定性内存安全与零动态链接依赖进行了深度重构。

```text
┌────────────────────────────────────────────────────────────────────────────────────────┐
│                              CP 业务与运行时层 (Application)                            │
├───────────────────────────┬─────────────────────────────┬──────────────────────────────┤
│   多算法无损压缩引擎       │     零拷贝归档容器引擎       │     完整性校验和与协商引擎    │
│  • Gzip / Zlib / Deflate  │  • POSIX.1-1988 USTAR Tar   │  • IEEE 802.3 CRC-32         │
│  • Zstandard (RFC 8478)   │    TarHeader / TarReader    │  • RFC 1950 Adler-32         │
│  • Brotli (RFC 7932)      │  • PKZIP Local / Central    │  • CompressionFormat 协商    │
│  • RleCodec 游程编码      │    ZipEocd / ZipReader      │  • CompressBoundEstimator    │
├───────────────────────────┴─────────────────────────────┴──────────────────────────────┤
│             流式分块管道与帧描述符零拷贝视图 (GzipStream / ZstdFrameHeader / BrotliHeader)│
├────────────────────────────────────────────────────────────────────────────────────────┤
│     底层静态内联引擎 (vendor/zlib libz.a + _codecs.cp / _archive.cp + Zig std.compress)│
└────────────────────────────────────────────────────────────────────────────────────────┘
```

### 核心设计准则

1. **零动态库依赖（Zero `.so` / `.dylib` Dependency）**：
   - Zlib / Gzip / Deflate 由 `vendor/zlib` 高性能静态库（`libz.a`）直接内联入产物二进制，不依赖宿主操作系统的 `libz.so.1`。
   - 压缩上界计算（`CompressBoundEstimator`）、帧头解析（`GzipHeader`、`ZstdFrameHeader`、`BrotliHeader`）、归档解析（`TarReader`、`ZipReader`）与校验和（`Crc32`、`Adler32`）均为 100% 纯 CP 原生算术实现，零外部 FFI 开销。
2. **零拷贝归档遍历（Zero-Copy Archive Views）**：
   - `TarReader` 与 `ZipReader` 直接在内存切片上通过游标推进并返回零拷贝视图（`TarEntryView`、`ZipEntryView`），不触发任何堆分配或二次内存拷贝。
3. **HTTP 内容编码原生协同**：
   - `CompressionFormat.fromAcceptEncoding()` 原生支持按 `zstd > br > gzip > deflate` 优先级自动协商最优 HTTP `Accept-Encoding` 算法。

---

## 二、 模块文件拓扑与来源映射

| 模块文件 | 核心职责 | 上游吸收来源 |
| :--- | :--- | :--- |
| `lib/pack/index.cp` | `cap:pack` / `cap:compress` 统一导出门面 | `/data/dev/bun/src/zlib/` & `/data/dev/bun/src/libarchive/` |
| `lib/pack/gzip.cp` | RFC 1952 Gzip / RFC 1950 Zlib / RFC 1951 Raw Deflate 单发与流式引擎 | `/data/dev/bun/src/zlib/zlib.cp` |
| `lib/pack/zstd.cp` | RFC 8478 Zstandard 帧头解析（`ZstdFrameHeader`）、压缩上界与编解码接口 | `/data/dev/bun/src/zstd/zstd.cp` |
| `lib/pack/brotli.cp` | RFC 7932 Brotli 模式枚举（`BrotliMode`）、窗口位数解析（`BrotliHeader`）与上界 | `/data/dev/bun/src/brotli/brotli.cp` |
| `lib/pack/tar.cp` | POSIX.1-1988 USTAR 512 字节块编解码（`TarHeader`）与零拷贝读取器（`TarReader`） | `/data/dev/bun/src/libarchive/libarchive.cp` |
| `lib/pack/zip.cp` | PKZIP Local Header / Central Directory / EOCD 反向扫描与零拷贝读取器（`ZipReader`） | `/data/dev/bun/src/libarchive/` |
| `lib/pack/checksum.cp` | IEEE 802.3 `Crc32` 与 RFC 1950 `Adler32` 流式增量及单次校验器 | `/data/dev/bun/src/zlib/` & IEEE 802.3 / RFC 1950 specs |
| `lib/pack/_codecs.cp` | Zlib / Zstd / Brotli C-FFI 声明、状态枚举与 `CompressBoundEstimator` | `/data/dev/bun/src/zlib/zlib.cp`, `brotli.cp`, `zstd.cp` |
| `lib/pack/_archive.cp` | Libarchive C-FFI 类型、格式常量与状态码绑定 | `/data/dev/bun/src/libarchive/` |
| `lib/pack/native.cp` | 基于 Zig `std.compress` 的原生纯静态解压桥接器 | `/data/dev/bun/src/zlib/` & Zig `std.compress` |

---

## 三、 六大核心组件详解与代码示例

### 1. Zlib / Gzip / Deflate 压缩与流式管道

> 📘 **专项深度文档**：关于 Zlib / Gzip 状态机内部细节、`windowBits` 驱动机制与扩容策略，请同时参阅同目录下的专项手册 [`docs/pack/zlib.md`](./zlib.md)。

支持 **RFC 1952 (Gzip)**、**RFC 1950 (Zlib)** 与 **RFC 1951 (Raw Deflate)** 三种标准容器格式，提供单发极速 API 与分块流式管道（`GzipStream` / `GunzipStream` / `DeflateStream` / `InflateStream`）：

```typescript
import {
    gzip,
    gunzip,
    deflate,
    inflate,
    deflateRaw,
    inflateRaw,
    GzipHeader,
    GzipStream,
    GunzipStream,
} from "cap:pack";

pub fn demoZlibAndGzip() {
    const raw = "High-performance CP Gzip & Zlib compression engine.";

    // 1. 单发 Gzip 压缩与解压 (RFC 1952)
    const gz_bytes = gzip(raw, 6);
    if (hdr := GzipHeader.parse(gz_bytes)) {
        @print("Gzip OS flag: {d}, mtime: {d}\n", hdr.os, hdr.mtime);
    }
    const restored_gz = gunzip(gz_bytes).?;

    // 2. 单发 Zlib Deflate (RFC 1950) 与 Raw Deflate (RFC 1951)
    const zlib_bytes = deflate(raw, 6);
    const restored_zlib = inflate(zlib_bytes).?;

    const raw_deflate = deflateRaw(raw, 6);
    const restored_raw = inflateRaw(raw_deflate).?;

    // 3. 分块流式 Gzip 压缩与解压管道
    let comp = GzipStream.create(6).?;
    const c1 = comp.write("Chunk #1: Streaming payload ");
    const c2 = comp.write("Chunk #2: Final payload.");
    const c_end = comp.finish();

    let decomp = GunzipStream.create().?;
    const d1 = decomp.write(c1);
    const d2 = decomp.write(c2);
    const d3 = decomp.write(c_end);
    const d_end = decomp.finish();
}
```

---

### 2. Zstandard (`Zstd` / `ZstdFrameHeader` — RFC 8478)

`lib/pack/zstd.cp` 提供 RFC 8478 标准帧头解析器 `ZstdFrameHeader`（支持魔数校验、Skippable 可跳过帧识别、单段模式 `single_segment`、滑动窗口大小 `window_size`、字典 ID `dictionary_id`、校验和标志 `content_checksum` 以及 1/2/4/8 字节可变长原始尺寸 `content_size` 提取）与 `Zstd` 门面：

```typescript
import {
    Zstd,
    ZstdFrameHeader,
    ZSTD_MAGIC,
    ZSTD_DEFAULT_CLEVEL,
} from "cap:pack";

pub fn demoZstd() {
    // 1. 压缩上界预估（纯算术，零 C 依赖）
    const max_len = Zstd.bound(4096);
    const ffi_bound = Zstd.compressBound(4096);
    @print("4KB 数据 Zstd 压缩缓冲区上界: {d} 字节\n", max_len);

    // 2. 构造或接收 RFC 8478 Zstandard 帧头并执行零拷贝元数据解析
    // 帧结构: Magic(28 B5 2F FD) + FHD(0x64: SingleSegment=1, Checksum=1, FCS_Flag=1 -> 2字节FCS) + FCS(0x0100 -> 256+256=512)
    let frame: [7]u8 = [0x28, 0xB5, 0x2F, 0xFD, 0x64, 0x00, 0x01];

    if (ZstdFrameHeader.isZstdHeader(frame[0..7])) {
        if (hdr := ZstdFrameHeader.parse(frame[0..7])) {
            @print("Zstd 帧头长度: {d}, 原始解压后尺寸: {d}\n", hdr.header_size, hdr.content_size);
        }
    }

    // 3. 快速提取解压后体积（用于预分配内存）
    const decomp_size = Zstd.findDecompressedSize(frame[0..7]);
}
```

---

### 3. Brotli (`Brotli` / `BrotliMode` / `BrotliHeader` — RFC 7932)

`lib/pack/brotli.cp` 吸收自 Bun `src/brotli/brotli.cp`，提供 RFC 7932 压缩模式枚举 `BrotliMode`（`Generic`、`Text`、`Font`）、流式状态机枚举 `BrotliStreamState`、编解码参数容器 `BrotliEncoderOptions` / `BrotliDecoderOptions`、WBITS 滑动窗口帧头解析器 `BrotliHeader` 以及 `Brotli` 压缩上界计算器：

```typescript
import {
    Brotli,
    BrotliMode,
    BrotliHeader,
    BrotliEncoderOptions,
    BrotliDecoderOptions,
    BROTLI_DEFAULT_QUALITY,
    BROTLI_DEFAULT_WINDOW,
} from "cap:pack";

pub fn demoBrotli() {
    // 1. 预估 Brotli 压缩输出缓冲区上界
    const bound_len = Brotli.bound(8192);
    const comp_bound = Brotli.compressBound(8192);

    // 2. 校验质量等级 [0..11] 与窗口位数 [10..24] / Large Window [10..30]
    const ok_q = Brotli.isValidQuality(BROTLI_DEFAULT_QUALITY);
    const ok_w = Brotli.isValidWindowBits(BROTLI_DEFAULT_WINDOW, false);
    const ok_large_w = Brotli.isValidWindowBits(28, true);

    // 3. 配置 UTF-8 文本模式或 WOFF2 字体优化模式
    const enc_opts = BrotliEncoderOptions{
        mode: BrotliMode.Text,
        quality: 9,
        lgwin: 22,
    };

    // 4. 解析 RFC 7932 Section 9.1 Brotli 帧头首字节 WBITS
    // 例如首字节 bit 0 == 0 代表 WBITS = 16
    let sample_br: [2]u8 = [0x06, 0x00];
    if (bhdr := BrotliHeader.parse(sample_br[0..2])) {
        @print("Brotli 窗口位数 WBITS = {d}, is_last = {}\n", bhdr.window_bits, bhdr.is_last);
    }
}
```

---

### 4. POSIX.1-1988 USTAR Tar 归档引擎 (`TarHeader` / `TarReader` / `TarEntryView`)

`lib/pack/tar.cp` 提供符合 POSIX.1-1988 USTAR 规范的 512 字节块编解码器 `TarHeader` 与内存零拷贝归档迭代器 `TarReader`：

- 自动计算与验证 8 字节八进制 ASCII 头部校验和（将偏移 `148..156` 视为 ASCII 空格 `' '`）；
- 自动按 512 字节边界对齐推进文件载荷，支持常规文件（`'0'`）、目录（`'5'`）、符号链接（`'2'`）等类型判别。

```typescript
import {
    TarHeader,
    TarReader,
    TarTypeFlag,
    TAR_BLOCK_SIZE,
} from "cap:pack";

pub fn demoTarArchive() {
    // 分配 3 个 512 字节块：1 个文件头块 + 1 个数据块 + 1 个全零结束块
    let archive: [1536]u8 = [0; 1536];
    const content = "package = \"cap:pack\"\nversion = \"1.0.0\"\n";

    // 1. 编码 512 字节 USTAR 头部
    const encoded_ok = TarHeader.encodeHeader(
        archive[0..512],
        "cap.toml",
        content.len as u64,
        0o644,
        1700000000,
        TarTypeFlag.Regular as u8,
    );

    // 2. 写入文件内容至第 2 个块 (512..512+content.len)
    @memcpy(archive[512 .. 512 + content.len], content);

    // 3. 使用 TarReader 零拷贝遍历归档条目
    let reader = TarReader.init(archive[0..1536]);
    while (entry := reader.next()) {
        @print("Tar 条目: {s}, 大小: {d}, 常规文件: {}\n",
            entry.name, entry.size, entry.isRegularFile());
        // entry.data 是直接指向 archive 内部切片的零拷贝视图
    }
}
```

---

### 5. PKZIP 归档与零拷贝读取器 (`ZipLocalHeader` / `ZipCentralHeader` / `ZipEocd` / `ZipReader`)

`lib/pack/zip.cp` 实现了完整的 PKZIP 二进制规范结构：

- **`ZipLocalHeader`** (`0x04034b50`, 30 字节)：支持单步编码带 CRC-32 校验的完整本地文件条目（`encodeEntry`）与解析；
- **`ZipCentralHeader`** (`0x02014b50`, 46 字节)：中央目录文件头编解码，记录条目属性与本地头部偏移 `local_header_offset`；
- **`ZipEocd`** (`0x06054b50`, 22 字节)：中央目录结束记录，支持 `findInBuffer()` 从缓冲区末尾向后扫描定位 EOCD；
- **`ZipReader` & `ZipEntryView`**：零拷贝顺序遍历 ZIP 条目并内置 `entry.verifyCrc()` 完整性校验。

```typescript
import {
    ZipLocalHeader,
    ZipCentralHeader,
    ZipEocd,
    ZipReader,
    Crc32,
    ZIP_METHOD_STORE,
} from "cap:pack";

pub fn demoZipArchive() {
    let zip_buf: [512]u8 = [0; 512];
    const file_name = "hello.txt";
    const payload = "Hello PKZIP Zero-Copy Archive!";

    // 1. 编码 Local File Entry (30 字节头 + 文件名 + 数据载荷)
    const entry_len = ZipLocalHeader.encodeEntry(zip_buf[0..256], file_name, payload);

    // 2. 编码 Central Directory Header (46 字节 + 文件名)
    const cd_offset = entry_len;
    const chdr = ZipCentralHeader{
        compression_method: ZIP_METHOD_STORE,
        crc32: Crc32.checksum(payload),
        compressed_size: payload.len as u32,
        uncompressed_size: payload.len as u32,
        name_len: file_name.len as u16,
        local_header_offset: 0,
    };
    const cd_hdr_len = chdr.write(zip_buf[cd_offset .. cd_offset + 46]);
    @memcpy(zip_buf[cd_offset + 46 .. cd_offset + 46 + file_name.len], file_name);
    const cd_total_len = cd_hdr_len + file_name.len;

    // 3. 编码 22 字节 End of Central Directory (EOCD)
    const eocd_offset = cd_offset + cd_total_len;
    const eocd = ZipEocd{
        entries_on_disk: 1,
        total_entries: 1,
        central_dir_size: cd_total_len as u32,
        central_dir_offset: cd_offset as u32,
    };
    const eocd_len = eocd.write(zip_buf[eocd_offset .. eocd_offset + 22]);
    const total_zip_len = eocd_offset + eocd_len;

    // 4. 从二进制流尾部反向查找 EOCD
    if (found_eocd := ZipEocd.findInBuffer(zip_buf[0..total_zip_len])) {
        @print("ZIP 归档总条目数: {d}, 中央目录偏移: {d}\n",
            found_eocd.total_entries, found_eocd.central_dir_offset);
    }

    // 5. 使用 ZipReader 零拷贝读取条目并校验 CRC32
    let reader = ZipReader.init(zip_buf[0..total_zip_len]);
    if (entry := reader.next()) {
        @print("读取文件: {s}, 内容: {s}, CRC校验通过: {}\n",
            entry.name, entry.data, entry.verifyCrc());
    }
}
```

---

### 6. 校验和引擎 (`Crc32` / `Adler32`)

`lib/pack/checksum.cp` 提供两种工业标准校验和算法的**单发极速计算**与**流式分块增量更新**：

- **`Crc32`**：IEEE 802.3 标准多项式 `0xEDB88320`（用于 Gzip 帧尾、PKZIP 条目校验、以太网帧校验）。标准向量 `"123456789"` 输出 `0xCBF43926`。
- **`Adler32`**：RFC 1950 Zlib 标准模数 `65521`（用于 Zlib Deflate 帧尾校验）。标准向量 `"Wikipedia"` 输出 `0x11E60398`。

```typescript
import { Crc32, Adler32 } from "cap:pack";

pub fn demoChecksums() {
    // 1. 单次计算
    const crc_val = Crc32.checksum("123456789");       // 0xCBF43926
    const adler_val = Adler32.checksum("Wikipedia");   // 0x11E60398

    // 2. 流式增量更新与重置复用
    let crc_stream = Crc32.init();
    crc_stream.update("1234");
    crc_stream.update("56789");
    const crc_final = crc_stream.final();              // 0xCBF43926
    crc_stream.reset();

    let adler_stream = Adler32.init();
    adler_stream.update("Wiki");
    adler_stream.update("pedia");
    const adler_final = adler_stream.final();          // 0x11E60398
}
```

---

## 四、 核心参数与常量速查矩阵

| 类别 | 符号 / 常量 | 取值 / 签名 | 说明 |
| :--- | :--- | :--- | :--- |
| **Zlib 窗口模式** | `WINDOW_BITS_RAW` | `-15` | RFC 1951 裸 Deflate 流（无头尾） |
| **Zlib 窗口模式** | `WINDOW_BITS_ZLIB` | `15` | RFC 1950 标准 Zlib 封装（2 字节头 + Adler32 尾） |
| **Zlib 窗口模式** | `WINDOW_BITS_GZIP` | `31` | RFC 1952 标准 Gzip 封装（10 字节头 + CRC32/ISIZE 尾） |
| **Zlib 窗口模式** | `WINDOW_BITS_AUTO` | `47` | 解压端自动探测 Gzip 或 Zlib 格式 |
| **Zstd 参数** | `ZSTD_MAGIC` | `0xFD2FB528` | RFC 8478 标准帧魔数（小端序 `28 B5 2F FD`） |
| **Zstd 参数** | `ZSTD_DEFAULT_CLEVEL` | `3` (范围 `1..22`) | Zstandard 默认压缩级别 |
| **Brotli 参数** | `BrotliMode` | `Generic(0)`, `Text(1)`, `Font(2)` | RFC 7932 编码器上下文优化模式 |
| **Brotli 参数** | `BROTLI_DEFAULT_QUALITY`| `11` (范围 `0..11`) | Brotli 默认最高压缩质量 |
| **Brotli 参数** | `BROTLI_DEFAULT_WINDOW` | `22` (范围 `10..24` / `30`) | Brotli 默认滑动窗口位数 (4MB) |
| **Tar 常量** | `TAR_BLOCK_SIZE` | `512` | POSIX USTAR 标准扇区块字节数 |
| **PKZIP 签名** | `ZIP_LOCAL_HEADER_SIG` | `0x04034b50` | 30 字节 Local File Header 魔数签名 |
| **PKZIP 签名** | `ZIP_CENTRAL_DIR_SIG` | `0x02014b50` | 46 字节 Central Directory Header 魔数签名 |
| **PKZIP 签名** | `ZIP_EOCD_SIG` | `0x06054b50` | 22 字节 End of Central Directory 魔数签名 |

---

## 五、 与 Bun / Node.js 生态全面对照表

| 能力维度 | Node.js (`node:zlib` / 生态包) | Bun (`bun` / 内置 Zig 模块) | CP 标准库 (`cap:pack`) |
| :--- | :--- | :--- | :--- |
| **Gzip 单发编解码** | `zlib.gzipSync` / `gunzipSync` | `Bun.gzipSync` / `gunzipSync` | `gzip(src, level)` / `gunzip(src)` |
| **Zlib 单发编解码** | `zlib.deflateSync` / `inflateSync` | `Bun.deflateSync` / `inflateSync` | `deflate(src, level)` / `inflate(src)` |
| **裸 Deflate 编解码** | `zlib.deflateRawSync` / `inflateRawSync` | `Bun.deflateRawSync` / `inflateRawSync` | `deflateRaw(src)` / `inflateRaw(src)` |
| **流式分块压缩/解压** | `zlib.createGzip()` / `createGunzip()` | `ZlibCompressorArrayList` / `NewZlibReader` | `GzipStream.create()` / `GunzipStream.create()` |
| **Zstd 帧解析与边界** | `zlib.zstdCompressSync` (Node 22+) | `src/zstd/zstd.cp` (`ZSTD_*`) | `Zstd.bound()` / `ZstdFrameHeader.parse()` |
| **Brotli 模式与帧解析** | `zlib.brotliCompressSync` | `src/brotli/brotli.cp` | `Brotli.bound()` / `BrotliMode` / `BrotliHeader` |
| **Tar 归档解析与构建** | 需第三方 `npm:tar` | `Bun.Archive` (`src/libarchive/`) | `TarHeader.encodeHeader()` / `TarReader` (零拷贝) |
| **ZIP 归档解析与构建** | 需第三方 `npm:adm-zip` / `yauzl` | `src/libarchive/` | `ZipLocalHeader` / `ZipEocd.findInBuffer()` / `ZipReader` |
| **CRC32 / Adler32** | `zlib.crc32(data)` | `Bun.hash.crc32` / `adler32` | `Crc32.checksum()` / `Adler32.checksum()` + 增量流 |
