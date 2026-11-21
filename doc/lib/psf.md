# `cap:psf` — Linux Console PSF1 & PSF2 点阵字体零分配解析库

> **导入方式**：`import { PsfVersion, detectPsfVersion, isPsf, parsePsfHeader, psfGlyphBitmap, psfGlyphPixel, findPsfGlyphByCodepoint } from "cap:psf";`  
> **内存模型**：100% 纯 CP 零堆分配，字形位图零拷贝指向原始字体缓冲区。

---

## 核心常量与类型

- **PSF1**：`PSF1_MAGIC0 = 0x36`, `PSF1_MAGIC1 = 0x04`, `PSF1_HEADER_SIZE = 4`, `PSF1_MODE512`, `PSF1_MODEHASTAB`, `PSF1_MODEHASSEQ`, `PSF1_SEPARATOR = 0xFFFF`, `PSF1_STARTSEQ = 0xFFFE`
- **PSF2**：`PSF2_MAGIC = 0x864AB572`, `PSF2_HEADER_SIZE = 32`, `PSF2_HAS_UNICODE_TABLE`, `PSF2_SEPARATOR = 0xFF`, `PSF2_STARTSEQ = 0xFE`
- **`PsfVersion: u8`**：`unknown = 0`, `psf1 = 1`, `psf2 = 2`
- **`PsfHeader`**：`version`, `header_size`, `glyph_count`, `bytes_per_glyph`, `width`, `height`, `has_unicode_table`, `glyphs_offset`, `unicode_table_offset`

---

## API 列表

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `detectPsfVersion` / `isPsf` | `fn detectPsfVersion(data): PsfVersion` | 探测 PSF1 / PSF2 魔数 |
| `parsePsfHeader` | `fn parsePsfHeader(data): PsfHeader?` | 统一解析 PSF1/PSF2 头并校验位图尺寸、行步幅与截断边界 |
| `psfGlyphBitmap` | `fn psfGlyphBitmap(data, header, glyph_idx): ([]const u8)?` | 零拷贝获取字形原始位图切片 |
| `psfGlyphPixel` | `fn psfGlyphPixel(data, header, glyph_idx, row, col): bool?` | 按每行 `(width + 7) / 8` 字节 MSB-first 读取单像素 |
| `findPsfGlyphByCodepoint` | `fn findPsfGlyphByCodepoint(data, header, codepoint): u32?` | 在 PSF1 UCS-2 / PSF2 UTF-8 Unicode 表中查找码点（自动跳过 `STARTSEQ` 组合序列） |

---

## 使用示例

```typescript
import { parsePsfHeader, findPsfGlyphByCodepoint, psfGlyphPixel } from "cap:psf";

fn renderGlyphA(font_bytes: []const u8) {
    if (hdr := parsePsfHeader(font_bytes)) {
        const idx = findPsfGlyphByCodepoint(font_bytes, hdr, 0x41) ?? 0x41;
        if (on := psfGlyphPixel(font_bytes, hdr, idx, 0, 0)) {
            * = on;
        }
    }
}
```
