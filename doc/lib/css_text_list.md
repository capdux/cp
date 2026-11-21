# `cap:css_text_list` — CSS 文本排版、列表样式与视觉效果核心类型

> 溯源：从 `/data/dev/bun/src/css/properties/text.cp`、`list.cp` 与 `effects.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:css_text_list` 提供零分配、强类型的 CSS Text Level 3、CSS Lists Level 3、CSS Filter Effects 与 CSS Compositing 核心枚举与位标志结构体，所有枚举均提供 `fromStr(s: string): Self?` 与 `asStr(): string` 双向映射及语义查询方法。

---

## 快速导入

```cp
import {
    TextTransformCase,
    TextTransformOther,
    WhiteSpace,
    WordBreak,
    OverflowWrap,
    TextAlign,
    TextDecorationLine,
    Direction,
    ListStyleType,
    ListStylePosition,
    FilterKind,
    BlendMode,
    BoxShadowFlags,
} from "cap:css_text_list";
```

---

## API 参考

### 1. 文本排版 (`text.cp`)

| 类型 | 变体 / 字段 | 核心方法 |
| :--- | :--- | :--- |
| `TextTransformCase` | `none`, `uppercase`, `lowercase`, `capitalize` | `fromStr`, `asStr`, `isActive` |
| `TextTransformOther` | `full_width: bool`, `full_size_kana: bool` | `isEmpty`, `toU8`, `fromU8`, `eql` |
| `WhiteSpace` | `normal`, `pre`, `nowrap`, `pre_wrap`, `break_spaces`, `pre_line` | `fromStr`, `asStr`, `preservesSpaces`, `preservesNewlines`, `allowsWrap` |
| `WordBreak` | `normal`, `keep_all`, `break_all`, `break_word` | `fromStr`, `asStr`, `breaksAnywhere` |
| `OverflowWrap` | `normal`, `break_word`, `anywhere` | `fromStr`, `asStr`, `allowsEmergencyBreak` |
| `TextAlign` | `start`, `end`, `left`, `right`, `center`, `justify`, `match_parent`, `justify_all` | `fromStr`, `asStr`, `isLogical`, `isJustified` |
| `TextDecorationLine` | `underline`, `overline`, `line_through`, `blink`, `spelling_error`, `grammar_error` | `isNone`, `hasLine`, `toU8`, `fromU8`, `eql` |
| `Direction` | `ltr`, `rtl` | `fromStr`, `asStr`, `isRtl` |

### 2. 列表样式 (`list.cp`)

| 类型 | 变体 | 核心方法 |
| :--- | :--- | :--- |
| `ListStyleType` | `none`, `disc`, `circle`, `square`, `decimal`, `decimal_leading_zero`, `lower_roman`, `upper_roman`, `lower_alpha`, `upper_alpha`, `lower_latin`, `upper_latin`, `lower_greek`, `cjk_decimal` | `fromStr`, `asStr`, `isSymbolic`, `isNumeric`, `isAlphabetic`, `isRoman` |
| `ListStylePosition` | `inside`, `outside` | `fromStr`, `asStr`, `isInside` |

### 3. 视觉效果与混合模式 (`effects.cp`)

| 类型 | 变体 / 字段 | 核心方法 |
| :--- | :--- | :--- |
| `FilterKind` | `blur`, `brightness`, `contrast`, `grayscale`, `hue_rotate`, `invert`, `opacity`, `saturate`, `sepia`, `drop_shadow`, `url` | `fromStr`, `asStr`, `isColorMatrix` |
| `BlendMode` | `normal`, `multiply`, `screen`, `overlay`, `darken`, `lighten`, `color_dodge`, `color_burn`, `hard_light`, `soft_light`, `difference`, `exclusion`, `hue`, `saturation`, `color`, `luminosity` | `fromStr`, `asStr`, `isSeparable` |
| `BoxShadowFlags` | `inset`, `has_blur`, `has_spread`, `is_text_shadow` | `toU8`, `fromU8`, `isSimpleDrop`, `eql` |

---

## 使用示例

```cp
import { WhiteSpace, ListStyleType, BlendMode, BoxShadowFlags } from "cap:css_text_list";

fn inspectStyle() {
    if (ws := WhiteSpace.fromStr("pre-wrap")) {
        const keep_spaces = ws.preservesSpaces(); // true
        const can_wrap = ws.allowsWrap();         // true
        * = keep_spaces;
        * = can_wrap;
    }

    if (lt := ListStyleType.fromStr("lower-roman")) {
        const is_roman = lt.isRoman(); // true
        * = is_roman;
    }

    if (bm := BlendMode.fromStr("color-dodge")) {
        const separable = bm.isSeparable(); // true
        * = separable;
    }

    const flags = BoxShadowFlags{ has_blur: true };
    const simple = flags.isSimpleDrop(); // true
    * = simple;
}
```
