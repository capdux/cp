# `cap:html_asset_scanner` — 零堆分配 HTML 资源引用扫描器

> 参考来源：`/data/dev/bun/docs/bundler/html-static.mdx`、`/data/dev/bun/src/bundler/HTMLScanner.cp` | 吸收日期：2026-11-22

`cap:html_asset_scanner` 提供纯 CP、零外部依赖、零堆分配的 HTML 资源引用扫描与 `srcset` 解析能力，返回每个资源在原始 HTML 缓冲区中的精确字节偏移 `[start_offset, end_offset)`，便于上游打包器（Bundler）或静态站点生成器执行原位 URL 重写与依赖图提取。

## 核心类型

### `AssetKind`

```cp
pub enum AssetKind: u8 {
    script = 0,
    stylesheet = 1,
    image = 2,
    media = 3,
    font = 4,
    icon = 5,
    manifest = 6,
    worker = 7,

    fn asStr(): string
}
```

### `AssetRef`

```cp
pub struct AssetRef {
    kind: AssetKind = .image,
    tag: []const u8 = "",
    attr: []const u8 = "",
    url: []const u8 = "",
    start_offset: usize = 0,
    end_offset: usize = 0,
}
```

- `url` 为零拷贝切片，恒满足 `html[start_offset..end_offset] == url`。

### `SrcsetCandidate`

```cp
pub struct SrcsetCandidate {
    url: []const u8 = "",
    descriptor: []const u8 = "",
}
```

## 核心函数与扫描器

### `HtmlAssetScanner.scan`

```cp
pub struct HtmlAssetScanner {
    static fn scan(html: []const u8, out: []AssetRef): usize
}
```

- 扫描规则：
  - `<script src="...">` → `.script`
  - `<link href="...">` 按 `rel` / `as` / `type` 分类：
    - `rel~="stylesheet"` / `as="style"` → `.stylesheet`
    - `rel~="modulepreload"` / `as="script"` → `.script`
    - `as="font"` / `type="font/..."` → `.font`
    - `as="image"` → `.image`
    - `as="video"` / `as="audio"` → `.media`
    - `as="worker"` → `.worker`
    - `rel~="manifest"` → `.manifest`
    - `rel~="icon"` / `rel~="apple-touch-icon"` → `.icon`
  - `<img src="..." srcset="...">` → `.image`
  - `<video src="..." poster="...">` → `src` 为 `.media`，`poster` 为 `.image`
  - `<audio src="...">` → `.media`
  - `<source src="..." srcset="...">` → `src` 为 `.media`，`srcset` 为 `.image`
- 自动跳过 HTML 注释 `<!-- ... -->`、`<!DOCTYPE ...>`、处理指令、闭合标签，以及 `<script>...</script>` / `<style>...</style>` 原始文本块中的伪标签。
- 支持大小写不敏感标签/属性名，以及双引号、单引号、无引号属性值。
- 输出受限于调用方传入的 `out` 容量，不会越界或触发堆分配。

### `parseSrcset`

```cp
pub fn parseSrcset(srcset: []const u8, out: []SrcsetCandidate): usize
```

- 按 HTML Living Standard 解析 `srcset` 候选项列表（如 `small.jpg 480w, retina.jpg 2x`），保留 URL 内部逗号（如 CDN 参数 `w_400,q_80/img.jpg`）并提取宽度/像素密度描述符。

## 使用示例

```cp
import { HtmlAssetScanner, AssetRef, SrcsetCandidate, parseSrcset } from "cap:html_asset_scanner";

const html = "<link rel=\"stylesheet\" href=\"/app.css\"><img src=\"/hero.png\" srcset=\"/hero@2x.png 2x\"><script src=\"/main.js\"></script>";
let refs: [8]AssetRef = [AssetRef{}; 8];
const n = HtmlAssetScanner.scan(html, refs[0..]);

// refs[0]: kind=.stylesheet, url="/app.css"
// refs[1]: kind=.image,      url="/hero.png"
// refs[2]: kind=.image,      url="/hero@2x.png 2x" (attr="srcset")
// refs[3]: kind=.script,     url="/main.js"
```

## 上游行为契约与边界注意事项

1. **精确字节偏移与原位 URL 重写（对齐 Bun HTML Bundler）**：
   - 每条 `AssetRef` 保证 `html[start_offset..end_offset] == url`，打包器可直接按偏移降序或流式拼接将相对路径（如 `./app.ts`、`./logo.png`）重写为带内容哈希的构建产物路径，无需构建完整 DOM 树。
2. **伪标签防误报机制**：
   - 扫描器严格跳过 `<!-- ... -->` 注释、`<!DOCTYPE ...>`、闭合标签以及 `<script>...</script>` / `<style>...</style>` 原始文本块内部出现的字符串（例如 JS 模板字符串里的 `"<img src='...'>"`），防止误提取非真实 DOM 资源。
3. **`srcset` CDN 参数逗号兼容与零分配上限**：
   - `parseSrcset` 遵循 HTML Living Standard 区分“URL 内部逗号（如 Cloudinary `w_400,q_80/img.jpg`）”与“候选项分隔逗号”；
   - `HtmlAssetScanner.scan` 与 `parseSrcset` 均严格受限于调用方 `out` 切片容量，永不越界或触发堆分配。
