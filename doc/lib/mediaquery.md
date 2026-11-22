# `cap:mediaquery` — 纯 CP 零分配 CSS Media Queries Level 4 解析与求值库

> 参考来源：`/data/dev/bun/src/css/media_query.cp`、`/data/dev/bun/docs/bundler/css.mdx` | 吸收日期：2026-11-22

100% 纯 CP、零外部依赖、零堆分配的 CSS Media Queries Level 4（`@media`）解析器、规范化器与设备视口求值引擎，吸收自 Bun 原生 CSS 引擎的 `media_query.cp` 模块。

## 导入方式

```cp
import {
    MediaType,
    Qualifier,
    MediaComparison,
    ColorScheme,
    ReducedMotion,
    Orientation,
    HoverKind,
    PointerKind,
    MediaEnv,
    MediaQuerySummary,
    parseLengthPx,
    parseResolutionDppx,
    parseRatio,
    evalMediaQueryList,
    parseMediaQuery,
    canonicalizeMediaQuery,
} from "cap:mediaquery";
```

## API 契约

### 1. 设备与视口上下文

- `MediaType` (`all`, `screen`, `print`, `unknown`)、`Qualifier` (`none`, `only`, `not`)、`MediaComparison` (`equal`, `gt`, `ge`, `lt`, `le`，提供 `opposite()` 与 `eval(lhs, rhs)`)
- `ColorScheme` (`light`, `dark`)、`ReducedMotion` (`no_preference`, `reduce`)、`Orientation` (`portrait`, `landscape`)、`HoverKind` (`none`, `hover`)、`PointerKind` (`none`, `coarse`, `fine`)
- `MediaEnv`：`media_type`, `width_px`, `height_px`, `root_font_size_px`, `dppx`, `color_bits`, `color_scheme`, `reduced_motion`, `hover`, `pointer`
  - 预设工厂：`MediaEnv.desktop()`（`1440x900, 1dppx, hover/fine`）、`MediaEnv.mobile()`（`390x844, 3dppx, none/coarse`）
  - 辅助方法：`orientation(): Orientation`（`height >= width` 为 `portrait`，否则 `landscape`）、`isValid(): bool`

### 2. 单位解析原语

- `parseLengthPx(raw, root_font_px): f64?`：解析 `px`, `em`, `rem`, `cm`, `mm`, `in`, `pt`, `pc` 与无单位 `0`
- `parseResolutionDppx(raw): f64?`：解析 `dppx`, `x`, `dpi` (`96dpi = 1dppx`), `dpcm`
- `parseRatio(raw): f64?`：解析 `16/9`、`16 / 9` 或正小数 `1.5`（拦截分母 `<= 0`）

### 3. 媒体查询解析、求值与规范化

- `evalMediaQueryList(input, env): bool?`
  求值逗号分隔的 Media Query List（OR 语义）；支持传统 `min-*`/`max-*` 冒号语法、MQ4 单边/双边区间语法（`(768px <= width < 1200px)`）、布尔上下文特性（`(color)`, `(hover)`）、离散特性（`prefers-color-scheme`, `prefers-reduced-motion`, `orientation`, `hover`, `pointer`）以及嵌套 `not`/`and`/`or` 条件。
- `parseMediaQuery(input): MediaQuerySummary?`
  解析单条媒体查询结构（`qualifier`, `media_type`, `has_condition`, `feature_count`），提供 `alwaysMatches(): bool`。
- `canonicalizeMediaQuery(input, out: *[]u8): ([]const u8)?`
  零分配规范化媒体查询文本（小写化、规整空白与逗号/冒号间距，空查询规范化为 `"all"`）。

## 上游行为契约与边界注意事项

1. **禁止同级混用 `and` 与 `or`（W3C MQ4 规范）**：
   - 与 Bun `media_query.cp` 一致，同一括号层级内不允许直接混用 `and` 与 `or`（如 `(width >= 600px) and (hover) or (print)` 会被拒绝返回 `null`），必须使用括号显式分组（如 `((width >= 600px) and (hover)) or (orientation: portrait)`）。
2. **双边区间比较方向一致性**：
   - `(400px <= width <= 1200px)` 与 `(1200px >= width > 400px)` 合法；方向相反的 `(400px < width > 200px)` 被严格拦截返回 `null`。
3. **`not` 修饰符作用域与递归深度保护**：
   - `not screen and (color)` 中的 `not` 否定**整条**媒体查询（等价于 `!(screen && color)`），而 `not (color)` 仅否定括号条件；
   - 嵌套括号深度硬限制为 `32` 层，防畸形深层嵌套触发栈溢出。

## 示例

```cp
import { MediaEnv, ColorScheme, evalMediaQueryList } from "cap:mediaquery";

fn demo() {
    let env = MediaEnv.desktop();
    env.color_scheme = ColorScheme.dark;

    if (ok := evalMediaQueryList("screen and (768px <= width <= 1600px) and (prefers-color-scheme: dark)", env)) {
        * = ok; // true
    }
}
```
