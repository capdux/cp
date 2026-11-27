[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `mime-types`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:mime-types` ([https://github.com/jshttp/mime-types](https://github.com/jshttp/mime-types))

Zero-heap MIME type, file extension, charset, `Content-Type` header, and HTTP compressibility resolver in pure CP (clean-room MIT counterpart to npm `mime-types` v2.1).

## Features

- **100% Zero Heap Allocation**: Pure slice/buffer operations with no dynamic memory allocations.
- **95+ Built-in MIME Types**: Covers web (`html`, `css`, `js`, `mjs`, `ts`, `json`, `xml`, `svg`, `wasm`), images (`png`, `jpg`, `webp`, `avif`, `ico`, `bmp`, `tiff`), documents (`pdf`, `rtf`, `doc`/`docx`, `xls`/`xlsx`, `ppt`/`pptx`), archives (`zip`, `gz`, `tar`, `br`, `zstd`, `7z`), audio/video (`mp3`, `wav`, `ogg`, `flac`, `mp4`, `webm`, `mov`, `m3u8`), fonts (`woff`, `woff2`, `ttf`, `otf`, `eot`), and developer/config formats (`yaml`, `toml`, `sh`, `py`, `rs`, `cp`, `zig`, `sql`, `graphql`).
- **Full `mime-types` API Parity**: `lookup`, `extension`, `charset`, `contentType`, `isCompressible`, and `parseMimeType`.

## Quick Start

```cp
import { lookup, extension, charset, contentType, isCompressible, parseMimeType } from "mime-types";

const m = lookup("/assets/app.JSON");      // "application/json"
const ext = extension("text/html; charset=utf-8"); // "html"
const cs = charset("application/ld+json"); // "UTF-8"

let buf: [128]u8 = undefined;
const n = contentType("html", &buf);       // "text/html; charset=utf-8"

const comp = isCompressible("image/svg+xml"); // true
const parts = parseMimeType("text/html; charset=\"utf-8\"");
```

## API

- `MimeParts { valid: bool, essence: string, type_part: string, subtype: string, charset_val: string }`
- `lookup(path_or_ext: string): string?`
- `extension(mime_type: string): string?`
- `charset(mime_type: string): string?`
- `contentType(type_or_ext: string, out: []u8): usize`
- `isCompressible(mime_type: string): bool`
- `parseMimeType(raw: string): MimeParts`

## License

MIT
