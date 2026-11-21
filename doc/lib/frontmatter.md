# `cap:frontmatter` — Markdown YAML/TOML Frontmatter 零分配提取器

> 来源：吸收自 `/data/dev/bun/src/md/parser.zig`（Markdown YAML/TOML Frontmatter Extractor）

`cap:frontmatter` 提供纯 CP、零外部依赖、零堆分配的 Markdown Frontmatter 头块切分与键值查询能力，支持 YAML（`---` / `...`）与 TOML（`+++`）两种主流格式。

## 核心类型与函数

### `FrontmatterKind` & `FrontmatterSplit`

```cp
pub enum FrontmatterKind: u8 {
    none = 0,
    yaml = 1,
    toml = 2,

    fn asStr(): string
}

pub struct FrontmatterSplit {
    kind: FrontmatterKind = .none,
    raw_meta: []const u8 = "",
    body: []const u8 = "",
}
```

### `splitFrontmatter`

```cp
pub fn splitFrontmatter(doc: []const u8): FrontmatterSplit
```

- 自动剥离可选 UTF-8 BOM 并兼容 LF / CRLF 行尾。
- 首行 `---` 搭配闭合 `---` 或 `...` 识别为 `.yaml`；首行 `+++` 搭配闭合 `+++` 识别为 `.toml`。
- 无头块或头块未闭合时返回 `kind = .none`、`raw_meta = ""`、`body = doc`。

### `getFrontmatterField`

```cp
pub fn getFrontmatterField(raw_meta: []const u8, kind: FrontmatterKind, key: []const u8): ([]const u8)?
```

- 支持 YAML `key: value` 与 TOML `key = value`。
- 自动忽略 `#` 注释行、剥离首尾单/双引号与行尾 `# comment`（YAML 保留值内非空白前的 `#`，如颜色 `#ff00ff`）。

## 使用示例

```cp
import { splitFrontmatter, getFrontmatterField } from "cap:frontmatter";

const doc = "---\ntitle: \"Hello CP\"\ndraft: false\n---\n# Heading\n";
const sp = splitFrontmatter(doc);
// sp.kind == .yaml, sp.body == "# Heading\n"
// getFrontmatterField(sp.raw_meta, sp.kind, "title").? == "Hello CP"
```
