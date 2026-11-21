# `cap:rle` — 多标准零堆分配游程编码（RLE）编解码套件

纯 CP、零堆分配的多标准游程编码库，统一提供 Apple/TIFF/PDF/PSD PackBits、ZSoft PCX RLE、Truevision TGA 像素包 RLE（1/2/3/4B 像素）以及零字节游程编码（RLE0）的严格边界防护编解码实现。

## 导入

```cp
import {
    PACKBITS_NOP,
    PCX_RUN_MASK,
    PCX_MAX_RUN,
    TGA_RLE_FLAG,
    TGA_MAX_PACKET,
    ZERO_RLE_MAX_RUN,
    packbitsMaxEncodedSize,
    packbitsEncode,
    packbitsDecode,
    pcxEncode,
    pcxDecode,
    tgaRleEncode,
    tgaRleDecode,
    zeroRleEncode,
    zeroRleDecode,
} from "cap:rle";
```

## 核心 API

- **Apple / TIFF / PDF / PSD PackBits**：
  - `packbitsMaxEncodedSize(src_len: usize): usize`
  - `packbitsEncode(src: []const u8, dst: []u8): usize?`
  - `packbitsDecode(src: []const u8, dst: []u8): usize?`（自动跳过 `0x80` NOP 控制字节）
- **ZSoft PCX RLE**：
  - `pcxEncode(src: []const u8, dst: []u8): usize?`（`>= 0xC0` 或连续重复 `>= 2` 次编码为 `[0xC0 | count, byte]`，最大 count=63）
  - `pcxDecode(src: []const u8, dst: []u8): usize?`
- **Truevision TGA 像素包 RLE**（`bpp_bytes` 支持 `1, 2, 3, 4`）：
  - `tgaRleEncode(src: []const u8, bpp_bytes: usize, dst: []u8): usize?`
  - `tgaRleDecode(src: []const u8, bpp_bytes: usize, dst: []u8): usize?`
- **零字节游程编码（RLE0）**：
  - `zeroRleEncode(src: []const u8, dst: []u8): usize?`（连续 `0x00` 编码为 `[0x00, count]`，单次最多 255）
  - `zeroRleDecode(src: []const u8, dst: []u8): usize?`

## 示例

```cp
import { packbitsEncode, packbitsDecode } from "cap:rle";

fn roundtrip(input: []const u8, enc: []u8, dec: []u8): usize? {
    const enc_len = packbitsEncode(input, enc) ?? return null;
    return packbitsDecode(enc[0..enc_len], dec);
}
```
