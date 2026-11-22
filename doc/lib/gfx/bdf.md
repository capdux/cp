# `cap:bdf` — 零分配 Adobe BDF 2.1/2.2 点阵字体解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:gfx`，导入请使用 `import { ... } from "cap:gfx"`。

> **模块路径**：`lib/bdf/`（入口 `lib/bdf/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 Adobe BDF 2.1 / 2.2 全局字体头、`STARTPROPERTIES`..`ENDPROPERTIES` 属性迭代、`STARTCHAR`..`ENDCHAR` 字形迭代、MSB-first 点阵位图像素解码与码点查找。

---

## 一、导入方式

```cp
import {
    BdfBBox,
    BdfHeader,
    BdfProperty,
    BdfGlyph,
    BdfPropIter,
    BdfGlyphIter,
    isBdf,
    parseBdfHeader,
    bdfGlyphPixel,
    findBdfGlyph,
} from "cap:gfx";
```

---

## 二、核心数据结构

| 类型 | 字段 | 说明 |
| :--- | :--- | :--- |
| `BdfBBox` | `width: i32`, `height: i32`, `x_off: i32`, `y_off: i32` | 字体全局或单字形边界框 |
| `BdfHeader` | `version`, `font_name`, `point_size`, `x_res`, `y_res`, `bbox`, `declared_chars`, `ascent`, `descent`, `chars_offset` | BDF 字体全局头（自动提取 `FONT_ASCENT` / `FONT_DESCENT`） |
| `BdfProperty` | `name: []const u8`, `value: []const u8` | 字体属性键值对（自动剥离带引号字符串的首尾引号） |
| `BdfGlyph` | `name`, `encoding: i32`, `dwidth_x`, `dwidth_y`, `bbox`, `bitmap_slice: []const u8` | 零拷贝字形（`encoding` 支持 `-1` 未编码字形） |

---

## 三、核心 API

- `isBdf(data: []const u8): bool` — 校验首行是否以 `"STARTFONT "` 开头。
- `parseBdfHeader(data: []const u8): BdfHeader?` — 解析全局字体头直至 `CHARS`（或首个 `STARTCHAR`）。
- `BdfPropIter.init(data)` / `next(*_): BdfProperty?` — 迭代 `STARTPROPERTIES`..`ENDPROPERTIES` 全部属性。
- `BdfGlyphIter.init(data)` / `initFromHeader(data, hdr)` / `next(*_): BdfGlyph?` — 零拷贝迭代所有完整闭合的 `STARTCHAR`..`ENDCHAR` 字形。
- `bdfGlyphPixel(glyph: BdfGlyph, row: u32, col: u32): bool?` — 按每行 `((width + 7) / 8) * 2` 个 hex 字符 MSB-first 精准解码单个像素；越界或畸形行返回 `null`。
- `findBdfGlyph(data: []const u8, codepoint: i32): BdfGlyph?` — 按 `ENCODING` 码点查找字形。

---

## 四、使用示例

```cp
import { parseBdfHeader, findBdfGlyph, bdfGlyphPixel } from "cap:gfx";

fn inspectBdf(font_data: []const u8) {
    if (hdr := parseBdfHeader(font_data)) {
        * = hdr.declared_chars;
    }
    if (g := findBdfGlyph(font_data, 65)) { // 'A'
        if (lit := bdfGlyphPixel(g, 0, 2)) {
            * = lit;
        }
    }
}
```
