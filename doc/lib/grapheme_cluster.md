# `cap:grapheme_cluster` — Unicode UAX #29 扩展字素簇边界判定与 UTF-8 迭代器

> 参考来源：/data/dev/bun/src/string/immutable/grapheme.cp | 吸收日期：2026-11-22

`cap:grapheme_cluster` 提供零外部依赖、零堆分配的 Unicode UAX #29 Extended Grapheme Cluster 切分、计数与切片引擎：

- 完整支持 GB3（`CR × LF`）、GB4/5（Control）、GB6–8（Hangul L/V/T/LV/LVT）、GB9/9a/9b（Extend/ZWJ/SpacingMark/Prepend）、GB11（`ExtPict Extend* ZWJ × ExtPict` 家庭 Emoji 序列）与 GB12/13（Regional Indicator 国旗配对）；
- 内置容错 UTF-8 解码器，遇到非法或截断 UTF-8 序列自动回退 `U+FFFD`，绝不崩溃。

---

## 快速导入

```cp
import {
    GraphemeBreakProperty,
    GraphemeBreakState,
    GraphemeIterator,
    DecodedCodepoint,
    classifyCodepoint,
    shouldBreak,
    decodeUtf8Step,
    count,
    sliceByGraphemes,
} from "cap:grapheme_cluster";
```

---

## API 参考

| 符号 / 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `GraphemeBreakProperty` | `Other`, `CR`, `LF`, `Control`, `Extend`, `ZWJ`, `RegionalIndicator`, `Prepend`, `SpacingMark`, `HangulL/V/T/LV/LVT`, `ExtendedPictographic` | UAX #29 码点断行属性分类枚举 |
| `classifyCodepoint` | `pub fn classifyCodepoint(cp: u21): GraphemeBreakProperty` | O(1) 码点属性分类 |
| `GraphemeBreakState` | `init()`, `reset(*_)` | 跨多码点规则状态机 |
| `shouldBreak` | `pub fn shouldBreak(state: *GraphemeBreakState, prev: u21, next: u21): bool` | 判定两码点间是否存在字素簇边界 |
| `decodeUtf8Step` | `pub fn decodeUtf8Step(bytes: []const u8): DecodedCodepoint` | 容错单步 UTF-8 解码 |
| `GraphemeIterator` | `init(s)`, `next(*_): ([]const u8)?`, `reset(*_)` | 零分配 UTF-8 字素簇迭代器 |
| `count` | `pub fn count(s: []const u8): usize` | 统计用户感知字符数 |
| `sliceByGraphemes` | `pub fn sliceByGraphemes(s: []const u8, start: usize, end: usize): []const u8` | 按字素簇下标区间零分配截取子串 |

---

## 使用示例

```cp
import { count, sliceByGraphemes, GraphemeIterator } from "cap:grapheme_cluster";

fn demo() {
    const text = "Hi👨‍👩‍👧‍👦🇨🇳!";
    const n = count(text); // 5 ("H", "i", "👨‍👩‍👧‍👦", "🇨🇳", "!")
    const emoji_only = sliceByGraphemes(text, 2, 4); // "👨‍👩‍👧‍👦🇨🇳"
    * = n;
    * = emoji_only;
}
```

## 上游行为契约与 Unicode 切分边界

1. **复合 Emoji 与国旗序列不撕裂（UAX #29 GB11/GB12/GB13）**：
   - 终端排版、编辑器光标移动与 UI 截断严禁按字节或单码点切分，`GraphemeIterator` 与 `sliceByGraphemes` 将 ZWJ 家庭 Emoji（如 `👨‍👩‍👧‍👦`）、肤色修饰符、组合音调及双 Regional Indicator 国旗（如 `🇨🇳`）完整保持为单一字素簇。
2. **CRLF 原子性（GB3）**：
   - Windows 换行符 `\r\n` 视为单个字素簇，不会在 `\r` 与 `\n` 之间错误断开。
3. **跨调用状态机重置**：
   - 直接调用底层 `shouldBreak(state, prev, next)` 连续扫描多段文本时，切换新文本段前须调用 `state.reset()` 清除残留的 `RegionalIndicator` 奇偶计数与 `ExtPict` ZWJ 状态。
