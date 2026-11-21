# `cap:css_image_set` — CSS Images 4 `<image>` 与 `image-set()` 解析与选图库

> 来源：吸收自 `/data/dev/bun/src/css/values/image.cp`

`cap:css_image_set` 提供纯 CP、零外部依赖、零堆分配的 CSS `<resolution>`、`image-set()` / `-webkit-image-set()` 及统一 `<image>` 解析、分辨率归一化、DPR 最优选图与规范化序列化能力。

## 核心类型

### `ResolutionUnit` & `Resolution`

```cp
pub enum ResolutionUnit: u8 {
    dppx = 0,
    x = 1,
    dpi = 2,
    dpcm = 3,

    fn asStr(): string
}

pub struct Resolution {
    value: f32 = 1.0,
    unit: ResolutionUnit = .x,

    static fn parse(input: []const u8): Resolution?
    fn toDppx(): f32
    fn normalized(): Resolution
    fn toCss(out: []u8): []const u8
}
```

- 单位换算：`96dpi == 1dppx`、`1dpcm == 2.54 / 96 dppx`、`x == dppx`。
- `normalized()` 与 `toCss(out)` 将 `dppx` 与 `x` 统一规范化为 `x`（兼容 Safari 与标准 `image-set()` 语法）。

### `ImageSetOption`

```cp
pub struct ImageSetOption {
    url: []const u8 = "",
    resolution: Resolution = Resolution{ value: 1.0, unit: .x },
    mime_type: ([]const u8)? = null,

    static fn parse(input: []const u8): ImageSetOption?
    fn toCss(out: []u8, is_prefixed: bool): []const u8
}
```

- 支持 `url(...)`（双引号、单引号、无引号）及裸字符串 `"a.webp"`。
- 分辨率与 `type("image/avif")` 均为可选，且二者出现顺序不限；缺省分辨率默认为 `1x`。

### `ImageSet`

```cp
pub type MimeSupportFn = fn ([]const u8): bool;

pub struct ImageSet {
    options: [16]ImageSetOption = [ImageSetOption{}; 16],
    len: usize = 0,
    is_webkit_prefixed: bool = false,

    static fn parse(input: []const u8): ImageSet?
    fn selectBest(dpr: f32, supported_mime_fn: MimeSupportFn?): ImageSetOption?
    fn toCss(out: []u8): []const u8
}
```

- 支持 `image-set(...)` 与 `-webkit-image-set(...)`，最多 16 项候选项。
- `selectBest(dpr, supported_mime_fn)`：
  1. 若候选项带有 `mime_type` 且传入了 `supported_mime_fn`，仅保留受支持的 MIME 类型；
  2. 优先选取满足 `toDppx() >= dpr` 的最小分辨率项；
  3. 若所有受支持候选项均低于 `dpr`，回退至最大可用分辨率项。

### `CssImageKind` & `CssImage`

```cp
pub enum CssImageKind: u8 {
    none = 0,
    url = 1,
    image_set = 2,
    gradient_raw = 3,

    fn asStr(): string
}

pub struct CssImage {
    kind: CssImageKind = .none,
    url: []const u8 = "",
    image_set: ImageSet = ImageSet{},
    gradient_raw: []const u8 = "",

    static fn parse(input: []const u8): CssImage?
    fn toCss(out: []u8): []const u8
}
```

## 使用示例

```cp
import { ImageSet, CssImage } from "cap:css_image_set";

fn supportsWebpAndPng(mime: []const u8): bool {
    return std.mem.eql(u8, mime, "image/webp") || std.mem.eql(u8, mime, "image/png");
}

const iset = ImageSet.parse(
    "image-set(\"hero.avif\" 2x type(\"image/avif\"), \"hero.webp\" 2x type(\"image/webp\"), \"hero.png\" 1x)"
).?;
const best = iset.selectBest(2.0, supportsWebpAndPng).?;
// best.url == "hero.webp"

let buf: [256]u8 = [0; 256];
const css_out = iset.toCss(buf[0..]);
```
