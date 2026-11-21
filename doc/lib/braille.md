# `cap:braille` — Unicode Braille 编解码、Grade-1 盲文转换与 2x4 终端点阵画布

纯 CP、零堆分配的 Unicode Braille Patterns（`U+2800..U+28FF`）标准库，提供 8 点位掩码 3 字节 UTF-8 编解码、Grade-1 英文盲文转换（自动处理大写指示符 `⠠` 与数字指示符 `⠼`）以及基于调用方缓冲区的 2x4 亚像素终端图形画布 `BrailleCanvas`。

## 导入

```cp
import {
    BRAILLE_BASE_CP,
    DOT1,
    DOT2,
    DOT3,
    DOT4,
    DOT5,
    DOT6,
    DOT7,
    DOT8,
    BRAILLE_NUMBER_SIGN,
    BRAILLE_CAPITAL_SIGN,
    dotBitForPixel,
    brailleToUtf8,
    utf8ToBraille,
    encodeBrailleMasks,
    decodeBrailleUtf8,
    asciiToBrailleMask,
    textToBrailleUtf8,
    BrailleCanvas,
} from "cap:braille";
```

## 核心 API

- **协议常量与位映射**：
  - `BRAILLE_BASE_CP: u32 = 0x2800`
  - `DOT1..DOT8: u8`（`0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80`）
  - `BRAILLE_NUMBER_SIGN: u8 = 0x3C`（`⠼`），`BRAILLE_CAPITAL_SIGN: u8 = 0x20`（`⠠`）
  - `dotBitForPixel(x: u8, y: u8): u8?` — 2x4 单元格坐标映射（`(0,0)->DOT1`..`(0,3)->DOT7`, `(1,0)->DOT4`..`(1,3)->DOT8`），越界返回 `null`。
- **UTF-8 3 字节零分配编解码**：
  - `brailleToUtf8(mask: u8): [3]u8` — 精确编码为 `[0xE2, 0xA0 | (mask >> 6), 0x80 | (mask & 0x3F)]`。
  - `utf8ToBraille(bytes: []const u8): u8?` — 严格校验 3 字节合法 UTF-8 且落在 `U+2800..U+28FF`。
  - `encodeBrailleMasks(masks: []const u8, dst: []u8): usize?` — 批量掩码编码为 UTF-8。
  - `decodeBrailleUtf8(utf8: []const u8, dst: []u8): usize?` — 批量 UTF-8 盲文解码为掩码。
- **Grade-1 英文盲文转换**：
  - `asciiToBrailleMask(ch: u8): u8?` — 支持 `a-z`/`A-Z`/`0-9`、空格及常用标点 `. , ; : ? ! ' -`。
  - `textToBrailleUtf8(text: []const u8, dst: []u8): usize?` — 转换 ASCII 文本为 Grade-1 盲文 UTF-8，自动在连续数字前插入 `⠼` 并在大写字母前插入 `⠠`。
- **2x4 亚像素点阵画布**：
  - `BrailleCanvas.init(cell_cols: usize, cell_rows: usize, cells: []u8): BrailleCanvas?`
  - 方法：`clear(*_)`、`pixelWidth(): usize`、`pixelHeight(): usize`、`setPixel(*_, x, y): bool`、`clearPixel(*_, x, y): bool`、`getPixel(x, y): bool`、`togglePixel(*_, x, y): bool`、`renderUtf8(out: []u8): usize?`。

## 示例

```cp
import std from "std";
import { BrailleCanvas, textToBrailleUtf8 } from "cap:braille";

fn renderDiagonal(out: []u8): usize? {
    let cells: [4]u8 = std.mem.zeroes([4]u8);
    let canvas = BrailleCanvas.init(2, 2, cells[0..]) ?? return null;
    * = canvas.setPixel(0, 0);
    * = canvas.setPixel(1, 1);
    * = canvas.setPixel(2, 2);
    * = canvas.setPixel(3, 3);
    return canvas.renderUtf8(out);
}
```
