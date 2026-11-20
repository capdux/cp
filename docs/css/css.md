# CP 标准库：工业级 CSS 引擎与样式表处理手册 (`cap:css`)

## 一、 架构总览与去 Bun 化纯正 CP 架构

`cap:css` 是 CP 语言官方内置的工业级 CSS 解析、词法分析、颜色空间与数学求值、选择器权重匹配、变量解析、厂商前缀/逻辑属性展开、CSS 嵌套展平与样式表压缩/美化标准库。整体架构吸收自 Bun 内置的 LightningCSS 引擎（`/data/dev/bun/src/css/`），已完成 **100% 去 Bun 化重构**——零 `@import("bun")`、零外部 JS/Bundler 耦合、零堆分配、全部 8 个核心 `.cp` 模块均可直接通过 `cpc check` 与 `cap test` 独立编译。

```text
┌────────────────────────────────────────────────────────────────────────┐
│         Layer 6: Stylesheet AST / Nesting / Printer & Minifier         │
│  CssStylesheet · CssRule · CssNesting.flatten() · CssStylesheetParser  │
│  [stylesheet.cp]                                                       │
├────────────────────────────────────────────────────────────────────────┤
│         Layer 5: Custom Properties (var()) & Prefixer / Logical        │
│  CssVariableMap · CssVarResolver · CssPrefixer · CssLogical            │
│  [variables.cp · prefixes.cp]                                          │
├────────────────────────────────────────────────────────────────────────┤
│         Layer 4: Rules / At-Rules / MediaQuery / Keyframes             │
│  CssProperty · CssRuleKind (15 种规则) · CssMediaQuery · CssKeyframes  │
│  [stylesheet.cp]                                                       │
├────────────────────────────────────────────────────────────────────────┤
│         Layer 3: Selectors Level 4 & W3C Specificity                   │
│  CssSelector · CssSelectorList · CssSpecificity                        │
│  [:is/:not/:has/:where 权重计算 · 6 大属性运算符 · 多类名精确匹配]        │
│  [selector.cp]                                                         │
├────────────────────────────────────────────────────────────────────────┤
│         Layer 2: Values Level 4 / Color 4 / Units / Calc()             │
│  CssColor (Hex/RGB/HSL/HWB/OKLAB/OKLCH/148 命名色/WCAG/Mix)             │
│  CssUnit (23 单位) · CssLength · CssCalc (calc/min/max/clamp 递归下降)   │
│  [color.cp · length.cp]                                                │
├────────────────────────────────────────────────────────────────────────┤
│         Layer 1: CSS Syntax Level 3 Tokenizer                          │
│  CssTokenizer · CssToken · CssTokenKind (25 种词法 Token)               │
│  [tokenizer.cp]                                                        │
└────────────────────────────────────────────────────────────────────────┘
```

### 模块文件清单（8 个纯正 CP 模块）

| 模块文件 | 上游谱系 (`// Source:`) | 核心职责 |
| :--- | :--- | :--- |
| `lib/css/index.cp` | `/data/dev/bun/src/css/css_parser.cp` | `cap:css` 统一对外导出出口 |
| `lib/css/tokenizer.cp` | `/data/dev/bun/src/css/css_parser.cp` | CSS Syntax Level 3 零分配流式词法扫描器（行/列追踪、引号/转义、`url()`、`/* */`、CDO/CDC） |
| `lib/css/color.cp` | `/data/dev/bun/src/css/values/color.cp` | CSS Color 4 解析与色域转换（Hex、RGB/RGBA、HSL/HSLA、HWB、OKLAB、OKLCH、148 命名色、WCAG 对比度） |
| `lib/css/length.cp` | `/data/dev/bun/src/css/values/length.cp` | 23 种 CSS 单位、`CssLength` 换算与递归下降 `CssCalc`（支持括号、`min`/`max`/`clamp`） |
| `lib/css/selector.cp` | `/data/dev/bun/src/css/selectors/parser.cp` | Selectors Level 4 解析、`:is`/`:not`/`:has`/`:where` 特异性计算、`CssSelectorList`、多类名与属性匹配 |
| `lib/css/variables.cp` | `/data/dev/bun/src/css/properties/custom.cp` | 自定义属性表 `CssVariableMap` 与防环递归 `CssVarResolver.resolve()` |
| `lib/css/prefixes.cp` | `/data/dev/bun/src/css/prefixes.cp` | `CssVendorPrefix`、`CssPrefixer` 厂商前缀展开与 `CssLogical` 逻辑属性转物理属性 |
| `lib/css/stylesheet.cp` | `/data/dev/bun/src/css/rules/rules.cp` | 15 种规则 AST、`CssMediaQuery`、`CssKeyframes`、`CssNesting.flatten`、`CssStylesheet`、压缩与美化输出 |

---

## 二、 核心对外 API 详解

### 1. CSS Tokenizer (`CssTokenizer`, `CssToken`, `CssTokenKind`)

| API 签名 | 说明 |
| :--- | :--- |
| `CssTokenizer.init(src): CssTokenizer` | 从源码切片初始化零分配词法扫描器 |
| `tz.nextToken(*_): CssToken` | 读取下一个 Token（包含 `Whitespace` 与 `Comment`） |
| `tz.nextNonTrivia(*_): CssToken` | 自动跳过空白与注释读取下一个有效 Token |
| `tz.tokenizeAll(*_, out_tokens, skip_trivia): usize` | 批量填充 `CssToken` 切片并返回 Token 数量 |
| `tok.isTrivia(): bool` / `tok.eqlIdent(target): bool` | 判断是否为空白/注释，或大小写不敏感匹配标识符 |

### 2. CSS Color Level 4 (`CssColor`)

支持 `#rgb` / `#rgba` / `#rrggbb` / `#rrggbbaa`、传统逗号与现代空格斜杠 `rgb()`/`rgba()`/`hsl()`/`hsla()`/`hwb()`/`oklab()`/`oklch()`（支持 `deg`/`rad`/`turn`/`grad` 角度）及全部 **148 个 W3C 标准命名色**（二分查找）。

| API 签名 | 说明 |
| :--- | :--- |
| `CssColor.parse(input): CssColor?` | 解析任意 Hex / RGB / HSL / HWB / OKLAB / OKLCH / 命名色 |
| `CssColor.rgb(r, g, b)` / `CssColor.rgba(r, g, b, a)` | 直接构造 sRGB 颜色对象 |
| `CssColor.fromHsl(h, s, l, a)` / `fromHwb(h, w, b, a)` | HSL / HWB 转 sRGB |
| `CssColor.fromOklab(l, a, b, alpha)` / `fromOklch(l, c, h, a)` | OKLAB / OKLCH 感知均匀色域转换至 sRGB |
| `color.withAlpha(a)` / `lighten(pct)` / `darken(pct)` | 透明度替换与 HSL 亮度增减 |
| `color.toHex(buf)` / `toRgbString(buf)` / `toHsl(&h, &s, &l)` | 格式化输出与 HSL 反算 |
| `color.luminance()` / `contrastRatio(other)` / `mix(other, w)` | WCAG 相对亮度、对比度与线性混色 |

### 3. CSS Values, Units & `calc()` (`CssUnit`, `CssLength`, `CssCalc`)

- `CssUnit` 支持 23 种单位：`Px`、`Rem`、`Em`、`Vw`、`Vh`、`Vmin`、`Vmax`、`Percent`、`Pt`、`Cm`、`Mm`、`In`、`Pc`、`Q`、`Ch`、`Ex`、`Deg`、`Rad`、`Turn`、`Grad`、`Sec`、`Ms`、`Unitless`。

| API 签名 | 说明 |
| :--- | :--- |
| `CssUnit.fromSuffix(sfx): CssUnit?` / `unit.suffix()` | 单位后缀双向解析与格式化 |
| `CssLength.parse(raw): CssLength?` | 解析长度/角度/时间数值（`0` 自动归一化为 `0px`） |
| `len.toPx(root_font_px, vw, vh): f64` | 换算为标准像素 `px` |
| `len.toMs(): f64` / `len.toDeg(): f64` | 时间转毫秒 / 角度转角度制 |
| `len.scale(factor)` / `len.eql(other)` / `len.format(buf)` | 缩放、比较与零堆分配格式化 |
| `CssCalc.evalPx(expr, root_font_px, vw, vh): f64?` | 递归下降求值 `calc()`、`min()`、`max()`、`clamp()`、嵌套括号与一元正负号（除零返回 `null`） |

### 4. Selectors Level 4 & Specificity (`CssSelector`, `CssSelectorList`, `CssSpecificity`)

| API 签名 | 说明 |
| :--- | :--- |
| `CssSelector.parse(sel): CssSelector` | 解析复杂选择器，支持 `:where()`（零权重）、`:is()`/`:not()`/`:has()`（取最高参数权重）及多类名/属性运算符 |
| `CssSelectorList.parse(sel_list): CssSelectorList` | 解析逗号分隔的选择器列表，提供 `maxSpecificity()` 与 `matchesElement()` |
| `spec.score()` / `spec.compare(other)` / `spec.add(other)` | 打包权重与 W3C 三元组字典序比较 |
| `selector.matchesTag(tag)` / `matchesElement(tag, id, classes)` | 标签、ID 与全量多类名精确匹配 |
| `selector.matchesAttribute(attr_name, attr_val): bool` | 校验 `=`、`~=`、`|=`、`^=`、`$`=`、`*=` 及存在性属性约束 |

### 5. CSS Variables, Prefixing & Logical Properties (`CssVariableMap`, `CssVarResolver`, `CssPrefixer`, `CssLogical`)

| API 签名 | 说明 |
| :--- | :--- |
| `CssVariableMap.init()` / `set` / `get` / `has` / `remove` | 零分配自定义属性注册表（最多 64 个 `--*` 变量） |
| `CssVarResolver.resolve(val, vars, out_buf)` | 递归展开 `var(--name, fallback)`，内置 16 层防环保护 |
| `CssPrefixer.prefixesForProperty(name)` / `expand(...)` | 为 `user-select`、`backdrop-filter`、`appearance` 等生成 `-webkit-`/`-moz-`/`-ms-` 前缀声明 |
| `CssLogical.isLogicalProperty(name)` / `expand(...)` | 将 `margin-inline`、`padding-block`、`inline-size`、`inset-inline-start` 等逻辑属性展开为物理属性 |

### 6. Rules, Nesting, Stylesheet AST, Printer & Minifier

| API 签名 | 说明 |
| :--- | :--- |
| `CssProperty.parse(decl): CssProperty?` | 解析声明、`!important` 与 `--custom-prop` |
| `CssMediaQuery.parse(query)` / `matches(vw, is_dark)` / `matchesFull(...)` | 解析并评估 `min/max-width`、`min/max-height`、`prefers-color-scheme`、`prefers-reduced-motion`、`orientation` 与 `not` 取反 |
| `CssKeyframes.parse(rule_text): CssKeyframes?` | 解析 `@keyframes` 与各关键帧步骤声明 |
| `CssNesting.flatten(css_text, out_buf): []const u8` | 将 CSS Nesting（`&`、嵌套子选择器、嵌套 `@media`）展平为标准扁平 CSS |
| `CssStylesheet.parse(css_text): CssStylesheet` | 高层样式表 AST 容器，提供 `findRuleBySelector`、`collectVariables`、`format` |
| `CssStylesheetParser.parseRule` / `parseStylesheet` / `minify` / `formatPretty` | 字符串安全解析、注释移除压缩与 2 空格缩进美化输出 |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    CssTokenizer, CssColor, CssLength, CssCalc,
    CssSelector, CssSelectorList,
    CssVariableMap, CssVarResolver, CssPrefixer, CssLogical,
    CssNesting, CssStylesheet, CssStylesheetParser,
} from "cap:css";

pub fn demoCssEngine() {
    // 1. 颜色、calc() 与选择器权重
    const accent = CssColor.parse("oklch(62.8% 0.258 29.23deg)").?.lighten(10.0);
    const width_px = CssCalc.evalPx("clamp(16px, (100vw - 2rem) / 2, 640px)", 16.0, 1200.0, 800.0).?;
    const sel = CssSelector.parse("ul#nav > li.item:not(.disabled)");

    // 2. 变量解析与 CSS Nesting 展平
    let vars = CssVariableMap.init();
    * = vars.set("--accent", "#2563eb");
    let resolved_buf: [128]u8 = [0; 128];
    const bg = CssVarResolver.resolve("var(--btn-bg, var(--accent))", vars, resolved_buf[0..128]);

    let flat_buf: [512]u8 = [0; 512];
    const flat = CssNesting.flatten(".card { color: #111; &:hover { color: #2563eb; } }", flat_buf[0..512]);

    // 3. 样式表解析、压缩与美化
    const sheet = CssStylesheet.parse(flat);
    let out_buf: [512]u8 = [0; 512];
    const minified = CssStylesheetParser.minify(flat, out_buf[0..512]);
    _ = accent;
    _ = width_px;
    _ = sel;
    _ = bg;
    _ = sheet;
    _ = minified;
}
```
