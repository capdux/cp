# CP 标准库：色彩科学、WCAG 无障碍与终端 ANSI 配色引擎 (`cap:color` / `cap.color`)

## 一、 架构总览

`cap:color` 是 CP 语言官方内置的纯原生色彩科学、WCAG 2.1 无障碍对比度评估与终端 ANSI 配色标准库，面向 UI/浏览器渲染引擎、Design Token 调色板生成与 CLI 终端富文本主题系统打造。全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import { ... } from "cap:color"` 与 `import cap from "cap"; cap.color.*` 编译期按需惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/color/spaces.cp` | `// Source: cp/lib/color/spaces.cp (sRGB, HSL, HSV, OKLab & OKLCH Color Spaces)` | `ColorError`、`Rgba`、`Hsl`、`Hsv`、`Oklab`、`Oklch` 双向转换、`mixOklab` 与色彩变换 |
| `lib/color/wcag.cp` | `// Source: cp/lib/color/wcag.cp (WCAG 2.1 Relative Luminance, Contrast & Palette Generator)` | `Wcag`（相对亮度、对比度、AA/AAA 判定、最佳前景色）与 `Palette` 谐波调色板 |
| `lib/color/ansi.cp` | `// Source: cp/lib/color/ansi.cp (ANSI TrueColor, xterm-256 Quantization & Strip)` | `AnsiColorCodec`（24-bit TrueColor SGR、xterm-256 量化与 ANSI 序列剥离） |
| `lib/color/index.cp` | `// Source: cp/lib/color/index.cp (Unified Exports for cap:color)` | `cap:color` 与 `cap.color` 统一公开导出入口 |

---

## 二、 核心 API 速查

### 1. 色彩空间、Hex 解析与感知插值（`spaces.cp`）

- **`ColorError`**：`InvalidHex`、`OutOfMemory`
- **`Rgba`**：`r: u8`、`g: u8`、`b: u8`、`a: u8 = 255`
  - 构造与解析：`Rgba.rgb(r, g, b)`、`Rgba.rgba(r, g, b, a)`、`Rgba.parseHex(hex_str): Rgba ! ColorError`（支持 `#RGB` / `#RGBA` / `#RRGGBB` / `#RRGGBBAA`）
  - 格式化：`toHex(buf): string ! ColorError`（输出 `#rrggbb`，需 `buf.len >= 7`）、`toHexAlpha(buf): string ! ColorError`（输出 `#rrggbbaa`，需 `buf.len >= 9`）
  - 快捷转换与操作：`eql(other)`、`withAlpha(alpha)`、`toHsl()`、`toHsv()`、`toOklab()`、`toOklch()`
- **`Hsl`**（`h ∈ [0, 360)`，`s, l ∈ [0.0, 1.0]`，`a: u8`）与 **`Hsv`**（`h ∈ [0, 360)`，`s, v ∈ [0.0, 1.0]`，`a: u8`）：提供 `init`、`toRgba()`、`approxEql`
- **`Oklab`**（`l ∈ [0.0, 1.0]`，`a, b` 对立轴，`alpha: u8`）与 **`Oklch`**（`l, c, h ∈ [0, 360)`，`alpha: u8`）：提供 `init`、`toRgba()`、`toOklch()` / `toOklab()`、`approxEql`
- **精确双向转换函数**：
  - `rgbToHsl(c)` / `hslToRgb(hsl)`
  - `rgbToHsv(c)` / `hsvToRgb(hsv)`
  - `rgbToOklab(c)` / `oklabToRgb(lab)`（采用 Björn Ottosson 标准 LMS/OKLab 矩阵与 IEC 61966-2-1 sRGB 线性化）
  - `oklabToOklch(lab)` / `oklchToOklab(lch)`
  - `srgbToLinear(c: u8): f64` / `linearToSrgb(x: f64): u8`
- **感知插值与色彩调整**：
  - `mixOklab(a, b, t): Rgba`：在 OKLab 感知均匀空间对颜色与 Alpha 进行线性插值（`t ∈ [0.0, 1.0]`）
  - `rotateHue(c, deg): Rgba`：色相环旋转（度）
  - `lighten(c, amount): Rgba` / `darken(c, amount): Rgba` / `saturate(c, amount): Rgba`（`amount ∈ [0.0, 1.0]`）
  - `invert(c): Rgba`：RGB 通道反相（保留 Alpha）

### 2. WCAG 2.1 无障碍对比度与调色板（`wcag.cp`）

- **`Wcag`**：
  - `relativeLuminance(c: Rgba): f64`：WCAG 2.1 相对亮度 `L ∈ [0.0, 1.0]`（`0.2126 R + 0.7152 G + 0.0722 B`）
  - `contrastRatio(fg: Rgba, bg: Rgba): f64`：对比度 `(L1 + 0.05) / (L2 + 0.05) ∈ [1.0, 21.0]`（半透明前景自动与背景 Alpha 合成）
  - `meetsAA(fg, bg): bool`（`>= 4.5`）
  - `meetsAALarge(fg, bg): bool`（`>= 3.0`）
  - `meetsAAA(fg, bg): bool`（`>= 7.0`）
  - `bestTextColor(bg): Rgba`：在纯黑 `#000000` 与纯白 `#ffffff` 中自动择优高对比度前景文字色
- **`Palette`**：
  - `complementary(c): [2]Rgba`：`[c, c + 180°]` 互补色组
  - `triadic(c): [3]Rgba`：`[c, c + 120°, c + 240°]` 三等分色组
  - `analogous(c): [3]Rgba`：`[c - 30°, c, c + 30°]` 邻近谐波色组

### 3. 终端 ANSI TrueColor、xterm-256 量化与剥离（`ansi.cp`）

- **`AnsiColorCodec`**：
  - `fgTrueColor(alloc, c): ([]u8) ! ColorError`：生成 `\x1b[38;2;r;g;bm` 前景真彩色序列
  - `bgTrueColor(alloc, c): ([]u8) ! ColorError`：生成 `\x1b[48;2;r;g;bm` 背景真彩色序列
  - `rgbToAnsi256(c): u8`：按 RGB 欧氏距离在 `6x6x6` 立方体（`16..231`）与 24 级灰阶（`232..255`）中选取最优 xterm-256 索引
  - `stripAnsi(alloc, text): ([]u8) ! ColorError`：精确剥离 CSI/SGR、OSC 与双字节 ESC 转义序列并保留完整 UTF-8 文本

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import {
    Rgba,
    mixOklab,
    Wcag,
    Palette,
    AnsiColorCodec,
} from "cap:color";

pub fn demoColor(): !void {
    const alloc = std.heap.page_allocator;
    const brand = Rgba.parseHex("#3b82f6")?;
    const accent = Rgba.parseHex("#ec4899")?;

    const mid = mixOklab(brand, accent, 0.5);
    const text_fg = Wcag.bestTextColor(mid);
    const ratio = Wcag.contrastRatio(text_fg, mid);
    * = ratio;

    const triad = Palette.triadic(brand);
    * = triad;

    const fg_esc = AnsiColorCodec.fgTrueColor(alloc, brand)?;
    defer alloc.free(fg_esc);

    const code256 = cap.color.AnsiColorCodec.rgbToAnsi256(brand);
    * = code256;
}
```
