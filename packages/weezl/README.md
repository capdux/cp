# @cap/lzw

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:weezl` ([https://github.com/image-rs/lzw](https://github.com/image-rs/lzw))（原库名 `weezl` → CP 包名 `@cap/lzw`）

零堆分配的 CP GIF/TIFF/PDF 工业标准 LZW 字典无损压缩与解压引擎 — 洁净室 MIT 实现，对标 Rust `weezl`，支持 Clear(256) / EOI(257) 控制码、9..12 位可变码宽自增、4096 项静态字典自动清空与 `KwKwK` 自引用特例。

Zero-allocation GIF/TIFF/PDF LZW dictionary lossless compressor & decompressor for CP — clean-room MIT parity with Rust `weezl`.

## 安装 / Install

```bash
cap add @cap/lzw
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `CLEAR_CODE` (`256`) / `EOI_CODE` (`257`) | LZW 控制码常量 |
| `encode(src: []const u8, dst: []u8) -> i64` | LSB-first LZW 压缩（`dst` 不足返回 `-1`） |
| `decode(src: []const u8, dst: []u8) -> i64` | LSB-first LZW 解压（支持 `KwKwK` 与中途 Clear，非法返回 `-1`） |

## 使用示例 / Usage

```cp
import { encode, decode } from "@cap/lzw";

let comp: [256]u8 = undefined;
let out: [256]u8 = undefined;
const n = encode("TOBEORNOTTOBEORTOBEORNOT", comp[0..256]);
const m = decode(comp[0..(n as usize)], out[0..256]);
```

## License

MIT
