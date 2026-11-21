# `cap:mime_sniff` — WHATWG MIME Sniffing 魔数嗅探与文本/二进制判定引擎

> 来源：吸收自 `/data/dev/bun/src/http/MimeType.zig`（WHATWG MIME Sniffing）

`cap:mime_sniff` 提供纯 CP、零外部依赖、零堆分配的文件魔数 MIME 嗅探、WHATWG 二进制字节判定与 UTF-8/UTF-16 文本内容识别能力。

## 核心函数

```cp
pub fn isBinaryDataByte(b: u8): bool
pub fn isTextData(buf: []const u8): bool
pub fn sniffImageMime(buf: []const u8): ([]const u8)?
pub fn sniffArchiveOrExecMime(buf: []const u8): ([]const u8)?
pub fn sniffAudioVideoMime(buf: []const u8): ([]const u8)?
pub fn sniffMime(buf: []const u8): []const u8
```

- `isBinaryDataByte`：遵循 WHATWG MIME Sniffing §3 二进制字节表（`0x00..0x08`、`0x0B`、`0x0E..0x1A`、`0x1C..0x1F`、`0x7F`）。
- `isTextData`：检查前 512 字节是否不含二进制控制字节且为合法 UTF-8（支持 UTF-8/UTF-16 BOM）。
- `sniffImageMime`：识别 PNG、JPEG、GIF87a/GIF89a、WebP、BMP、ICO 与 SVG（含可选 `<?xml ...?>` 前缀）。
- `sniffArchiveOrExecMime`：识别 PDF、ZIP、GZIP、ZSTD、BZIP2、XZ、WASM、ELF 与 SQLite 3。
- `sniffAudioVideoMime`：识别 WAV、OGG、FLAC、MP3（ID3 与 MPEG 帧同步）、MP4（`ftyp`）与 WebM（EBML）。
- `sniffMime`：按 图像 → 音视频 → 归档/可执行 → HTML/XML/JSON 文本探测 → `text/plain; charset=utf-8` / `application/octet-stream` 顺序综合嗅探。

## 使用示例

```cp
import { sniffMime, isTextData } from "cap:mime_sniff";

const m1 = sniffMime("\x89PNG\r\n\x1A\n..."); // "image/png"
const m2 = sniffMime("{\"ok\":true}");       // "application/json"
const m3 = sniffMime("hello world");         // "text/plain; charset=utf-8"
```
