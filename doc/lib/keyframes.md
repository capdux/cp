# `cap:keyframes` — 纯 CP 零分配 CSS `@keyframes` 解析、压缩与时间轴采样库

> 参考来源：`/data/dev/bun/src/css/rules/keyframes.cp`、`/data/dev/bun/docs/bundler/css.mdx` | 吸收日期：2026-11-22

100% 纯 CP、零外部依赖、零堆分配的 CSS Animations `@keyframes` 规则解析器、选择器规范化压缩器与时间轴关键帧采样引擎，吸收自 Bun 原生 CSS 引擎的 `rules/keyframes.cp` 模块。

## 导入方式

```cp
import {
    VendorPrefix,
    KeyframesNameKind,
    KeyframesName,
    SelectorKind,
    KeyframeSelector,
    KeyframeStop,
    ParsedKeyframes,
    KeyframeSegment,
    isReservedCssKeyword,
    parseKeyframesRule,
    sortStopsByOffset,
    sampleKeyframeSegment,
} from "cap:keyframes";
```

## API 契约

### 1. 动画名与选择器

- `VendorPrefix`：`none` (`@keyframes`), `webkit` (`@-webkit-keyframes`), `moz` (`@-moz-keyframes`), `o` (`@-o-keyframes`)
- `isReservedCssKeyword(name): bool`：检测 7 个 CSS 全局保留字（`none`, `initial`, `inherit`, `unset`, `default`, `revert`, `revert-layer`）
- `KeyframesName`：`parse(raw): KeyframesName?`、`format(minify, out: *[]u8): ([]const u8)?`
- `KeyframeSelector`：`parse(raw): KeyframeSelector?`、`offset(): f64`（`[0.0, 1.0]`）、`format(minify, out: *[]u8): ([]const u8)?`

### 2. `@keyframes` 规则解析与时间轴采样

- `KeyframeStop`：`{ offset: f64, selector_raw: []const u8, body_raw: []const u8 }`
- `ParsedKeyframes`：`{ vendor: VendorPrefix, name: KeyframesName, stop_count: usize }`
- `parseKeyframesRule(css_text, stops_out: *[]KeyframeStop): ParsedKeyframes?`
  零拷贝解析完整 `@keyframes` 块，支持 `/* ... */` 注释与逗号分隔多选择器（如 `0%, 100% { ... }`）展开。
- `sortStopsByOffset(stops: *[]KeyframeStop)`：原地按 `offset` 稳定升序排序。
- `KeyframeSegment`：`{ prev_idx: usize, next_idx: usize, local_t: f64 }`
- `sampleKeyframeSegment(stops: []const KeyframeStop, t: f64): KeyframeSegment?`：计算任意进度 `t ∈ [0.0, 1.0]` 所处的相邻关键帧区间及局部插值比例 `local_t`。

## 上游行为契约与边界注意事项

1. **CSS 全局保留字与引号压缩（对齐 Bun `KeyframesName`）**：
   - 未加引号的 `none`、`initial`、`inherit`、`unset`、`default`、`revert`、`revert-layer` 作为动画名在 CSS 规范中非法，`KeyframesName.parse` 严格拒绝；
   - 加引号的 `"none"` 合法且在 `format(minify = true)` 时强制保留引号；而普通合法标识符字符串 `"fade-in"` 在压缩模式下自动脱去引号输出 `fade-in`。
2. **选择器字节级最优压缩（对齐 Bun `KeyframeSelector.toCss`）**：
   - `minify = true` 时：`from`（4 字节）压缩为 `0%`（2 字节），`100%`（4 字节）压缩为 `to`（2 字节），与 LightningCSS / Bun 压缩策略完全一致。
3. **零分配与容错边界**：
   - 严格拒绝负百分比、`> 100%`、未闭合 `/*` 注释、未闭合 `{}` 与尾部多余字符；输出缓冲不足时安全返回 `null`。

## 示例

```cp
import { KeyframeStop, parseKeyframesRule, sortStopsByOffset, sampleKeyframeSegment } from "cap:keyframes";

fn demo() {
    const css = "@keyframes fade { from { opacity: 0; } 50% { opacity: 0.8; } to { opacity: 1; } }";
    let stops_buf: [8]KeyframeStop = [{ offset: 0.0, selector_raw: "", body_raw: "" }; 8];
    let stops: []KeyframeStop = stops_buf[0..];

    if (parsed := parseKeyframesRule(css, *stops)) {
        let active = stops[0..parsed.stop_count];
        sortStopsByOffset(*active);
        if (seg := sampleKeyframeSegment(active, 0.25)) {
            * = seg; // prev_idx=0 (0%), next_idx=1 (50%), local_t=0.5
        }
    }
}
```
