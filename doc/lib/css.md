# CP 标准库：工业级 CSS 引擎与样式表处理手册 (`cap:css`)

> 参考来源：`/data/dev/bun/docs/bundler/css.mdx`, `/data/dev/bun/src/css/` | 吸收日期：2026-11-22

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

---

## 四、 LightningCSS 工业级引擎子系统 (`lib/css/engine/`)

在保持既有 8 个对外基础模块零破坏的前提下，`cap:css` 新增 `lib/css/engine/` 工业级编译子系统，直迁自 `/data/dev/bun/src/css/`，已通过 `lib/css/index.cp` 统一导出至 `cap:css`。

### 新增模块清单

| 模块文件 | 上游谱系 (`// Source:`) | 核心能力 |
| :--- | :--- | :--- |
| `lib/css/engine/_utils.cp` | `/data/dev/bun/src/css/css_parser.cp` | 零分配公共扫描工具（`engAppendBytes`、`engFormatFloat`、`engSplitCommaTop`、`engSplitSpaceTop`、`engFindMatchingParen`） |
| `lib/css/engine/targets.cp` | `/data/dev/bun/src/css/targets.cp` | `Browsers` 24 位压缩版本矩阵（`major << 16 \| minor << 8 \| patch`）、`es2015`–`es2023` 与浏览器版本解析、`Feature`（27 项现代 CSS 特性兼容性判定）、`Features` 位掩码、`PrefixFlags` / `PrefixFeature` 与 `Targets.shouldCompile()` / `Targets.prefixes()` |
| `lib/css/engine/values.cp` | `/data/dev/bun/src/css/values/values.cp` | `Angle`（deg/rad/grad/turn）、`Time`（s/ms 智能最短压缩）、`Resolution`（dpi/dpcm/dppx/x）、`Ratio`、`AlphaValue`、`UrlValue`、`CssRect`、`Position`（1–4 值解析）、`EasingFunction`（含牛顿-二分 `eval(t)` 贝塞尔求值与 `steps()`）、`Gradient`（Linear/Radial/Conic 及双位置色标降级） |
| `lib/css/engine/properties.cp` | `/data/dev/bun/src/css/properties/properties.cp` | `RectShorthand`（`margin`/`padding`/`inset`/`border-*` 展开与最短合并）、`BorderShorthand`、`BorderRadius`（含 `/` 椭圆半径）、`BoxShadowList`、`TransformList` + `Matrix2D` 2D 矩阵合成、`FlexShorthand`、`TransitionItem`、`AnimationItem`、`DisplayValue`（内外双值最小化）、`FilterList` |
| `lib/css/engine/rules.cp` | `/data/dev/bun/src/css/rules/rules.cp` | `SupportsAst`（`@supports` 递归 `not`/`and`/`or`/`selector()` AST 解析与求值）、`MediaRangeQuery`（Media Queries Level 4 `(400px <= width < 900px)` → `(min-width: 400px) and (max-width: 899.98px)` 降级）、`ContainerQuery`、`LayerManager`（`@layer` 级联权重）、`FontFaceRule`、`PropertyRule`、`ScopeRule` |
| `lib/css/engine/compiler.cp` | `/data/dev/bun/src/css/printer.cp` | `CssCompileOptions`、`CssCompileStats`、`CssModulesHasher`（确定性类名作用域哈希与 `:global(...)` 保留）与 `CssCompiler.compile()`（Nesting 展平 + 未使用符号裁剪 + Media Range L4 降级 + 逻辑属性展开 + 简写压缩 + 靶向厂商前缀 + Minify/Pretty 输出） |
| `lib/css/engine/index.cp` | `/data/dev/bun/src/css/css_parser.cp` | 引擎子模块统一重导出出口 |


## 合并收敛子模块（第 1 批：12 个 CSS 属性与规则子模块）

为统一 CSS 领域出口，`cap:css` 已聚合吸收以下 12 个纯 CP 子模块（保留原有 `engine/` 全部导出零破坏；与 `engine/` 同名的独立子模块类型通过 `Css*` 聚合别名导出）：

| 子文件 | 原始来源 | 核心导出符号 |
| --- | --- | --- |
| `lib/css/animation.cp` | `bun/src/css/properties/{animation,transition}.cp` | `AnimationDirection`, `AnimationPlayState`, `AnimationFillMode`, `IterationCount`, `Animation`, `Transition` |
| `lib/css/atrules.cp` | `bun/src/css/rules/{keyframes,layer,counter_style,property}.cp` | `CssAtRuleError`, `isValidKeyframesIdent`, `TimelineRangeName`, `KeyframeSelectorKind`, `KeyframeSelector`, `LayerName`, `CounterSystem`, `PropertySyntaxKind`, `isValidCustomPropertyName`, `PropertyDescriptor` |
| `lib/css/background.cp` | `bun/src/css/properties/background.cp` | `CssBackgroundError`, `BackgroundRepeatKeyword`, `BackgroundRepeat`, `LengthPercentageAutoKind`, `LengthPercentageAuto`, `BackgroundSizeKind`, `BackgroundSize`, `BackgroundAttachment`, `BackgroundOrigin`, `BackgroundClip`, `BackgroundPosition`, `BackgroundLayer`, `Background` |
| `lib/css/border.cp` | `bun/src/css/properties/border.cp` | `LineStyle`, `BorderLengthUnit`, `BorderSideWidthKind`, `BorderSideWidth`, `BorderColorKind`, `BorderColorValue`, `Border`, `BorderWidth`, `CssBorderStyle`, `BorderColor`, `BorderLogicalWidth` |
| `lib/css/borderimage.cp` | `bun/src/css/properties/border_image.cp` | `BorderImageRepeatKeyword`, `BorderImageRepeat`, `SliceOffset`, `BorderImageSlice`, `WidthKind`, `BorderImageSideWidth`, `BorderImageWidth`, `OutsetKind`, `BorderImageOutsetValue`, `BorderImageOutset`, `BorderImage` |
| `lib/css/borderradius.cp` | `bun/src/css/properties/border_radius.cp` | `RadiusValue`, `CornerRadius`, `CssBorderRadius` |
| `lib/css/boxshadow.cp` | `bun/src/css/properties/{box_shadow,outline}.cp` | `OutlineStyle`, `ShadowLength`, `CssBoxShadow`（共享 `LineStyle`） |
| `lib/css/cascade.cp` | `bun/src/css/rules/{import,scope,viewport}.cp` | `ImportLayerKind`, `ImportRule`, `ScopePrelude`, `LayerStatement` |
| `lib/css/container.cp` | `bun/src/css/rules/container.cp` | `CssContainerError`, `ContainerType`, `ContainerName`, `ContainerSizeFeatureId`, `Orientation`, `CompareOp`, `ContainerSizeFeature`, `LogicOp`, `CssContainerQuery` |
| `lib/css/display.cp` | `bun/src/css/properties/display.cp` | `Visibility`, `DisplayKeyword`, `DisplayOutside`, `DisplayInside`, `DisplayPair`, `DisplayKind`, `Display` |
| `lib/css/easing.cp` | `bun/src/css/values/easing.cp` | `CssEasingError`, `CssStepPosition`, `CssEasingKind`, `CubicBezier`, `EasingSteps`, `CssEasingFunction` |
| `lib/css/effects.cp` | `bun/src/css/properties/effects.cp` | `CssFilterKind`, `FilterLengthUnit`, `FilterAngleUnit`, `DropShadow`, `Filter`, `CssFilterList` |

### 使用示例

```cp
import {
    Animation,
    Background,
    Border,
    CssBorderRadius,
    CssBoxShadow,
    Display,
    CssEasingFunction,
    CssFilterList,
} from "cap:css";

const anim = Animation.parse("300ms ease-in 50ms infinite alternate fadeIn").?;
const radius = CssBorderRadius.parse("8px 12px").?;
const shadow = CssBoxShadow.parse("inset 0 2px 6px rgba(0,0,0,0.2)").?;
const disp = Display.parse("inline flex").?;
```


## 合并收敛子模块（第 2 批：12 个 CSS 布局、字体、渐变、媒体与原语子模块）

第 2 批吸收以下 12 个纯 CP 子模块进入 `cap:css`（保留 `engine/` 与第 1 批既有导出零破坏，冲突符号通过聚合别名导出）：

| 子文件 | 原始来源 | 核心导出符号 |
| --- | --- | --- |
| `lib/css/flexgrid.cp` | `bun/src/css/properties/{flex,align,grid}.cp` | `CssLayoutError`, `FlexDirection`, `FlexWrap`, `FlexFlow`, `FlexBasisKind`, `FlexBasis`, `CssFlexShorthand`, `JustifyContent`, `AlignItems`, `AlignSelf`, `AlignContent`, `GridAutoFlow`, `TrackBreadthKind`, `TrackBreadth` |
| `lib/css/font.cp` | `bun/src/css/properties/font.cp` | `CssFontError`, `FontWeightKind`, `FontWeight`, `FontStyleKind`, `FontStyle`, `FontStretch`, `AbsoluteFontSize`, `RelativeFontSize`, `FontSizeKind`, `FontSize`, `LineHeightKind`, `LineHeight`, `FontVariantCaps`, `GenericFontFamily`, `FontFamilyItem`, `Font` |
| `lib/css/geometry.cp` | `bun/src/css/values/{ratio,rect}.cp`, `properties/overflow.cp` | `CssRatio`, `Rect`, `OverflowKeyword`, `Overflow`, `TextOverflow` |
| `lib/css/gradient.cp` | `bun/src/css/values/gradient.cp` | `MAX_STOPS`, `DirectionSide`, `LineDirection`, `RadialShape`, `RadialExtent`, `CssColorStop`, `LinearGradient`, `RadialGradient`, `ConicGradient`, `CssGradientKind`, `CssGradient` |
| `lib/css/imageset.cp` | `bun/src/css/values/image.cp` | `CssResolutionUnit`, `CssResolution`, `ImageSetOption`, `MimeSupportFn`, `ImageSet`, `CssImageKind`, `CssImage` |
| `lib/css/layoutctrl.cp` | `bun/src/css/properties/{overflow,position,contain}.cp` | `LayoutOverflowKeyword`, `LayoutOverflow`, `LayoutTextOverflow`, `LayoutPosition`, `LayoutContainerType`, `ContainFlags`, `ZIndex` |
| `lib/css/liststyle.cp` | `bun/src/css/properties/list.cp` | `PredefinedCounterStyle`, `ListStylePosition`, `MarkerSide`, `ListStyleTypeKind`, `ListStyleType`, `ListStyle` |
| `lib/css/masking.cp` | `bun/src/css/properties/masking.cp` | `GeometryBox`, `MaskOrigin`, `MaskClip`, `MaskMode`, `MaskType`, `MaskComposite`, `WebKitMaskComposite`, `WebKitMaskSourceType`, `MaskBorderMode`, `MaskBorderRepeat`, `FillRule`, `MASK_FLAG_*`, `MaskFeatureFlags`, `MaskConfig`, `MaskBorderConfig`, `getWebKitMaskPropertyName` |
| `lib/css/media.cp` | `bun/src/css/rules/media.cp` | `CssMediaError`, `MediaType`, `MediaQualifier`, `ColorScheme`, `ReducedMotion`, `HoverCapability`, `PointerCapability`, `MediaOrientation`, `MediaFeatureId`, `MediaCompareOp`, `MediaEnv`, `MediaFeature`, `MediaQuery`, `MediaQueryList` |
| `lib/css/pagerule.cp` | `bun/src/css/rules/page.cp`, `properties/position.cp` | `CssPageError`, `PagePseudoClass`, `PageMarginEdge`, `PageMarginBox`, `PageSelector`, `PagePosition` |
| `lib/css/position.cp` | `bun/src/css/values/position.cp` | `HorizontalSide`, `VerticalSide`, `PosUnit`, `PosLength`, `AxisPosKind`, `HorizontalPosition`, `VerticalPosition`, `CssPosition` |
| `lib/css/primitives.cp` | `bun/src/css/values/{alpha,percentage,url}.cp` | `Percentage`, `NumOrPctKind`, `NumberOrPercentage`, `CssAlphaValue`, `isAbsoluteCssUrl`, `isDataCssUrl` |


## 合并收敛子模块（第 3 批：最终清零 12 个 CSS 选择器、尺寸、SVG、文本、变换与单位子模块）

第 3 批完成全仓库 `lib/css_*` 清零，统一吸收至 `cap:css`（保留 `engine/` 与前两批既有导出零破坏，冲突符号通过聚合别名导出）：

| 子文件 | 原始来源 | 核心导出符号 |
| --- | --- | --- |
| `lib/css/selectorbuilder.cp` | `bun/src/css/selectors/builder.cp` | `Combinator`, `ComponentKind`, `SelectorComponent`, `Specificity`, `CombinatorEntry`, `BuiltSelector`, `SelectorBuilder` |
| `lib/css/sizing.cp` | `bun/src/css/properties/{size,margin_padding}.cp` | `BoxSizing`, `SizeVendorPrefix`, `SizeUnit`, `SizeLength`, `SizeKind`, `Size`, `MaxSizeKind`, `MaxSize`, `AspectRatio`, `InsetValue`, `BoxInsets`, `LogicalInsets` |
| `lib/css/supports.cp` | `bun/src/css/rules/supports.cp` | `SupportsKind`, `CssSupportsNode`, `SupportsTree` |
| `lib/css/svg.cp` | `bun/src/css/properties/svg.cp` | `MAX_DASH_ENTRIES`, `SVGPaintKind`, `SVGPaintFallbackKind`, `SVGPaint`, `StrokeLinecap`, `StrokeLinejoin`, `DashUnit`, `DashEntry`, `StrokeDasharray`, `PaintOrderItem`, `PaintOrder`, `ColorInterpolation`, `ColorRendering`, `ShapeRendering`, `TextRendering`, `ImageRendering` |
| `lib/css/syntax.cp` | `bun/src/css/values/syntax.cp` | `Multiplier`, `SyntaxType`, `SyntaxComponent`, `SyntaxString` |
| `lib/css/text.cp` | `bun/src/css/properties/text.cp` | `CssTextError`, `TextTransformCase`, `TEXT_TRANSFORM_*`, `TextTransform`, `WhiteSpace`, `WordBreak`, `LineBreak`, `Hyphens`, `OverflowWrap`, `TextAlign`, `DECO_LINE_*`, `TextDecorationLine`, `TextDecorationStyle`, `TextDecorationThicknessKind`, `TextDecorationThickness`, `TextDecoration` |
| `lib/css/textlist.cp` | `bun/src/css/properties/{text,list,effects}.cp` | `TextListTransformCase`, `TextTransformOther`, `TextListWhiteSpace`, `TextListWordBreak`, `TextListOverflowWrap`, `TextListTextAlign`, `Direction`, `TextListDecorationLine`, `TextListStyleType`, `TextListStylePosition`, `TextListFilterKind`, `BlendMode`, `BoxShadowFlags` |
| `lib/css/time.cp` | `bun/src/css/values/time.cp` | `CssTimeUnit`, `CssTime` |
| `lib/css/transform.cp` | `bun/src/css/properties/transform.cp` | `Matrix2d`, `Matrix3d`, `TransformStyle`, `BackfaceVisibility`, `TransformOrigin`, `CssTransformKind`, `Transform`, `CssTransformList` |
| `lib/css/transition.cp` | `bun/src/css/properties/transition.cp` | `CssTransitionError`, `TransitionBehavior`, `TransitionPropertyKind`, `TransitionProperty`, `TransitionTime`, `TransitionEasingKind`, `TransitionEasing`, `CssTransition`, `TransitionList` |
| `lib/css/ui.cp` | `bun/src/css/properties/ui.cp` | `UiColorScheme`, `Resize`, `UserSelect`, `CaretShape`, `Appearance`, `CursorKeyword` |
| `lib/css/units.cp` | `bun/src/css/values/{angle,resolution}.cp` | `UnitAngleUnit`, `UnitAngle`, `UnitResolutionUnit`, `UnitResolution` |
| `lib/css/dependencies.cp` | `bun/src/css/dependencies.cp` | `CssDependencyOptions`, `CssDependencyLocation`, `CssSourceRange`, `CssDependencyKind`, `CssDependency`, `formatDependencyPlaceholder`, `CssDependencyIter`（零分配 `@import` / `url(...)` 依赖提取、1-based 行列追踪、`supports(...)`/media 识别与确定性 16 字符占位符哈希） |
| `lib/css/diagnostics.cp` | `bun/src/css/error.cp` | `MAX_CSS_DIAGNOSTICS`, `CssDiagnosticSeverity`, `CssErrorLocation`, `CssParserErrorKind`（17 项解析错误码）, `CssPrinterErrorKind`（6 项 CSS Modules/打印错误码）, `CssDiagnosticDomain`, `CssDiagnostic`, `CssDiagnosticBag`（零分配诊断收集器与格式化） |
| `lib/css/propRule.cp` | `bun/src/css/rules/property.cp` | `MAX_PROPERTY_SYNTAX_COMPONENTS`, `CssPropertyRuleError`, `isValidDashedIdent`, `PropertySyntax`, `CssPropertyRule`（零堆分配 W3C CSS Properties and Values API Level 1 `@property` 自定义属性注册解析、`syntax`/`inherits`/`initial-value` 规范校验与美化/压缩序列化） |
| `lib/css/fontPalette.cp` | `bun/src/css/rules/font_palette_values.cp` | `MAX_PALETTE_OVERRIDES`, `CssFontPaletteError`, `BasePaletteKind`, `BasePalette`, `PaletteOverrideColor`, `CssFontPaletteValuesRule`（零堆分配 W3C CSS Fonts Level 4 `@font-palette-values` 彩色字体调色板解析、泛型字体族与 `currentColor` 拒绝、非负槽位覆写查询与格式化/压缩序列化） |

### `@font-palette-values` 彩色字体调色板示例（`fontPalette.cp`）

```typescript
import { CssFontPaletteValuesRule } from "cap:css";

const palette = CssFontPaletteValuesRule.parse(
    "@font-palette-values --brand { font-family: \"Bungee Spice\"; base-palette: 1; override-colors: 0 #ff6b6b, 1 rgb(25, 113, 194); }"
)?;
// palette.findOverride(0) == "#ff6b6b", palette.findOverride(1) == "rgb(25, 113, 194)"
let buf: [256]u8 = undefined;
const minified = palette.toCssMinified(buf[0..])?;
```

### `@property` 自定义属性注册规则示例（`propRule.cp`）

```typescript
import { CssPropertyRule, PropertySyntax, isValidDashedIdent } from "cap:css";

const rule = CssPropertyRule.parse(
    "@property --brand-accent { syntax: '<color>'; inherits: true; initial-value: #3b82f6; }"
)?;
// rule.name == "--brand-accent", rule.inherits == true, rule.acceptsValue("oklch(0.7 0.15 240)") == true
let buf: [256]u8 = undefined;
const minified = rule.toCssMinified(buf[0..])?;
// "@property --brand-accent{syntax:\"<color>\";inherits:true;initial-value:#3b82f6}"
```

### CSS Modules 命名模式、哈希与 `composes` 解析（`cssModules.cp`）

> 归并自原 `lib/cssmodules/`（Bun `src/css/css_modules.cp`）。

- 核心导出：`CssModuleConfig`, `PatternKind`, `PatternSegment`, `Pattern`, `ReferenceKind`, `CssModuleRef`, `isValidCssIdent`, `extractFileStemSanitized`, `computeModuleHash`, `scopeClassName`, `parseComposesValue`。

### `@keyframes` 规则解析、排序与时间线采样（`keyframes.cp`）

> 归并自原 `lib/keyframes/`（Bun `src/css/rules/keyframes.cp`）；为避免与 `atrules.cp` 既有 `KeyframeSelector` 冲突，`KeyframeSelector` 在主包中别名为 `CssKeyframeRuleSelector`。

- 核心导出：`VendorPrefix`, `KeyframesNameKind`, `KeyframesName`, `SelectorKind`, `CssKeyframeRuleSelector`, `KeyframeStop`, `ParsedKeyframes`, `KeyframeSegment`, `isReservedCssKeyword`, `parseKeyframesRule`, `sortStopsByOffset`, `sampleKeyframeSegment`。

> 注：原 `lib/mediaquery/mediaquery.cp` 因与 `lib/css/media.cp` 存在 Media Queries Level 4 功能重叠，按参考归档规范保留为 `lib/css/media_query_.cp`（不从 `cap:css` 重复导出）。

### `@custom-media` 规则解析、序列化与媒体查询展开（`customMedia.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/custom_media.cp`，零堆分配支持 W3C Media Queries Level 5 `@custom-media` 声明、`true`/`false` 布尔媒体值、级联覆盖注册表与 `(--custom-name)` 媒体查询展开。

- 核心导出：`CUSTOM_MEDIA_MAX_RULES` (`64`)、`CustomMediaError`、`CustomMediaValueKind`（`query` / `always_true` / `always_false`）、`CustomMediaRule`（`parse` / `toCss`）、`CustomMediaRegistry`（`init` / `register` / `lookup` / `resolveMediaQuery`）。

```typescript
import { CustomMediaRule, CustomMediaRegistry } from "cap:css";

let reg = CustomMediaRegistry.init();
* = reg.register(CustomMediaRule.parse("@custom-media --narrow (max-width: 30em);")?)?;
let out: [128]u8 = [0; 128];
const n = reg.resolveMediaQuery("screen and (--narrow)", out[0..])?;
// out[0..n] == "screen and (max-width: 30em)"
```

### `@namespace` 规则解析、序列化与命名空间注册表（`nsRule.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/namespace.cp`，零堆分配支持 W3C CSS Namespaces Level 3 默认/前缀命名空间声明（字符串与 `url(...)` 双语法）、规范序列化及选择器前缀解析。

- 核心导出：`CSS_NAMESPACE_MAX_RULES` (`32`)、`CSS_HTML_NS`、`CSS_SVG_NS`、`CSS_MATHML_NS`、`CssNamespaceError`、`NamespaceRule`（`parse` / `isDefault` / `toCss`）、`NamespaceRegistry`（`init` / `register` / `lookupPrefix` / `resolveSelectorPrefix`）。

```typescript
import { NamespaceRule, NamespaceRegistry, CSS_SVG_NS } from "cap:css";

let reg = NamespaceRegistry.init();
* = reg.register(NamespaceRule.parse("@namespace svg url(\"http://www.w3.org/2000/svg\");")?)?;
const uri = reg.resolveSelectorPrefix("svg") ?? ""; // CSS_SVG_NS
```

### `@scope` 规则解析、序列化与样式表提取（`scopeRule.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/scope.cp`，零堆分配支持 W3C CSS Cascading and Inheritance Level 6 `@scope [(<scope-start>)]? [to (<scope-end>)]? { <body> }` 规则解析（隐式/单根/仅下限/甜甜圈作用域）、嵌套括号与属性引号安全选择器拆分、美化与压缩 CSS 序列化及样式表批量提取。

- 核心导出：`CSS_SCOPE_MAX_SELECTORS` (`16`)、`CSS_SCOPE_MAX_RULES` (`32`)、`CssScopeError`、`ScopeSelectorList`（`parse` / `count` / `at` / `eql` / `writeCss`）、`ScopeRule`（`parse` / `isImplicit` / `isDonut` / `hasStart` / `hasEnd` / `startCount` / `endCount` / `eql` / `toCss` / `toCssMinified`）、`ScopeRuleList`（`extractFromStylesheet` / `count` / `at`）。

```typescript
import { ScopeRule } from "cap:css";

const rule = ScopeRule.parse("@scope (.card, .panel) to (.card-footer) { a { color: blue; } }")?;
let buf: [256]u8 = [0; 256];
const n = rule.toCssMinified(buf[0..])?;
// buf[0..n] == "@scope(.card,.panel) to (.card-footer){a { color: blue; }}"
```

### CSS `<ident>` / `<custom-ident>` / `<dashed-ident>` 与 CSSOM 标识符转义（`ident.cp`）

> 吸收自 `/data/dev/bun/src/css/values/ident.cp`，零堆分配支持 W3C CSS Values Level 4 `<ident>`、`<custom-ident>`（自动拒绝 6 大 CSS-wide 全局关键字）、`<dashed-ident>`、CSS Modules `DashedIdentReference`（`--var from "./tokens.css" | global`）与 W3C CSSOM `serializeCssIdentifier` 转义序列化。

- 核心导出：`CssIdentError`、`isCssWideKeyword`、`serializeCssIdentifier`、`CssIdent`、`CustomIdent`、`DashedIdent`、`CssIdentFromKind`、`DashedIdentReference`。

```typescript
import { CustomIdent, DashedIdentReference, serializeCssIdentifier } from "cap:css";

const ci = CustomIdent.parse("slide-in")?;
const ref_mod = DashedIdentReference.parse("--brand-color from \"./tokens.css\"")?;
let buf: [64]u8 = [0; 64];
const n = serializeCssIdentifier("1a:b", buf[0..])?;
// buf[0..n] == "\\31 a\\:b"
```

### CSS `url(...)` 解析、路径分类与最短化序列化（`urlVal.cp`）

> 吸收自 `/data/dev/bun/src/css/values/url.cp`，零堆分配实现 W3C CSS Values Level 4 `url(...)` 解析、RFC 3986 / WHATWG `isAbsolute()` 分类（相对路径、根相对 `/`、片段 `#`、`data:`、绝对 scheme），以及对齐 Bun 的最短化压缩序列化（在带引号与反斜杠转义无引号形式间自动择短）。

- 核心导出：`CssUrlError`、`CssUrlKind`（`Relative` / `RootRelative` / `Fragment` / `Data` / `AbsoluteScheme`）、`CssUrlValue`（`parse` / `kind` / `isAbsolute` / `isRelative` / `isFragment` / `isDataUrl` / `needsQuoting` / `eql` / `toCss` / `toCssMinified`）。

```typescript
import { CssUrlValue } from "cap:css";

const u = CssUrlValue.parse("url(\"./assets/logo.svg\")")?;
let buf: [64]u8 = [0; 64];
const n = u.toCssMinified(buf[0..])?;
// buf[0..n] == "url(./assets/logo.svg)"
```

### CSS 盒模型四边简写（`CssBoxRect`）与二维简写（`CssSize2D`）（`boxRect.cp`）

> 吸收自 `/data/dev/bun/src/css/values/rect.cp`，零堆分配实现 CSS 1/2/3/4 边盒模型简写（`margin` / `padding` / `inset` / `border-width`）与 1/2 值属性（`gap` / `overflow` / `border-spacing`）的函数安全解析（保护 `calc(...)`/`var(...)` 内部空白）与最短化折叠序列化。

- 核心导出：`CssBoxRectError`、`CssBoxRect`（`all` / `symmetric` / `parse` / `minimalComponentCount` / `isUniform` / `isSymmetric` / `eql` / `toCss` / `toCssExpanded`）、`CssSize2D`（`both` / `parse` / `isUniform` / `eql` / `toCss`）。

```typescript
import { CssBoxRect } from "cap:css";

const r = CssBoxRect.parse("10px 20px 10px 20px")?;
let buf: [64]u8 = [0; 64];
const n = r.toCss(buf[0..])?;
// buf[0..n] == "10px 20px"
```

### CSS `@layer` 级联层规则解析、序列化与优先级注册表（`layerRule.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/layer.cp`，零堆分配实现 W3C CSS Cascading and Inheritance Level 5 `@layer` 语句声明（`@layer reset, base, theme.dark;`）、命名/匿名 `@layer` 块规则（`@layer framework.layout { ... }` / `@layer { ... }`）、样式表批量提取（`CssLayerRuleList`）与级联优先级排序注册表（`CssLayerOrderRegistry`）。

- 核心导出：`CSS_LAYER_MAX_SEGMENTS` (`16`)、`CSS_LAYER_MAX_STATEMENT_NAMES` (`32`)、`CSS_LAYER_MAX_RULES` (`32`)、`CSS_LAYER_MAX_REGISTRY` (`64`)、`CssLayerError`、`CssLayerName`（`parse` / `count` / `at` / `eql` / `hash` / `isSublayerOf` / `isSameOrSublayerOf` / `parent` / `writeCss` / `toCss`）、`CssLayerStatementRule`（`parse` / `count` / `at` / `isEmpty` / `eql` / `toCss` / `toCssMinified`）、`CssLayerBlockRule`（`parse` / `isAnonymous` / `hasName` / `eql` / `toCss` / `toCssMinified`）、`CssLayerRuleKind`、`CssLayerRule`、`CssLayerRuleList`（`extractFromStylesheet`）、`CssLayerOrderRegistry`（`init` / `registerName` / `registerAnonymous` / `registerRule` / `registerRuleList` / `priorityOf` / `comparePriority`）。

```typescript
import { CssLayerRuleList, CssLayerOrderRegistry, CssLayerName } from "cap:css";

const rules = CssLayerRuleList.extractFromStylesheet(
    "@layer reset, theme.base, utilities; @layer reset { * { margin: 0; } } @layer theme.dark { body { color: #eee; } }"
)?;
let reg = CssLayerOrderRegistry.init();
* = reg.registerRuleList(rules)?;
const cmp = reg.comparePriority(CssLayerName.parse("reset")?, CssLayerName.parse("utilities")?);
// cmp == -1（reset 优先级低于 utilities）
```

### CSS Grid Level 2 轨道列表与模板区域解析/校验（`grid.cp`）

> 吸收自 `/data/dev/bun/src/css/properties/grid.cp`，零堆分配实现 W3C CSS Grid Layout Level 2 `grid-template-columns` / `grid-template-rows`（`none`、`<track-size>`、`minmax()`、`fit-content()`、`repeat(N | auto-fill | auto-fit, ...)`、命名网格线 `[line-a line-b]`）与 `grid-template-areas`（多行模板、`.`/`...` 空单元格、列数一致性与实心矩形校验）。

- 核心导出：`CSS_GRID_MAX_TRACKS` (`32`)、`CSS_GRID_MAX_LINE_NAMES` (`8`)、`CSS_GRID_MAX_AREA_CELLS` (`128`)、`CssGridError`、`CssGridTrackBreadthKind`、`CssGridTrackBreadth`、`CssGridTrackSizeKind`、`CssGridTrackSize`、`CssGridRepeatCountKind`、`CssGridRepeatCount`、`CssGridLineNameSet`、`CssGridTrackRepeat`、`CssGridTrackItemKind`、`CssGridTrackItem`、`CssGridTrackSizing`（`parse` / `toCss` / `toCssMinified`）、`CssGridTemplateAreas`（`parse` / `cellAt` / `validateRectangles` / `toCss`）。
- 规范防线：`minmax(min, max)` 的 `min` 位置严格禁止 `<flex>`（如 `1fr`）；`repeat()` 仅允许正整数、`auto-fill` 或 `auto-fit`；`grid-template-areas` 自动拦截行列不齐与非矩形（L 形/分裂）命名区域。

```cp
import { CssGridTrackSizing, CssGridTemplateAreas } from "cap:css";

const cols = CssGridTrackSizing.parse("[sidebar-start] 240px [sidebar-end main-start] repeat(auto-fit, minmax(200px, 1fr)) [main-end]")?;
const areas = CssGridTemplateAreas.parse("\"header header\" \"nav main\" \"footer ...\"")?;
let buf: [256]u8 = [0; 256];
const n1 = cols.toCssMinified(buf[0..])?;
const n2 = areas.toCss(buf[0..])?;
* = n1;
* = n2;
```

### CSS `@import` 规则解析、条件组合与依赖提取（`importRule.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/import.cp`，零堆分配实现 W3C CSS Cascading and Inheritance Level 5 `@import` 规则解析器、组合条件模型（`layer` / `layer(<layer-name>)`、`supports(<cond> | <declaration>)`、`<media-query-list>`）、带 bundler 占位符改写的规范序列化器与样式表前置 `@import` 批量提取器。

- 核心导出：`CSS_IMPORT_MAX_RULES` (`64`)、`CSS_IMPORT_NO_RECORD` (`0xFFFF_FFFF`)、`CssImportError`、`CssImportLayerKind`、`CssImportLayer`（`none` / `anonymous` / `named` / `isPresent` / `isAnonymous` / `eql` / `toCss`）、`CssImportSupportsKind`、`CssImportSupports`（`none` / `parse` / `isPresent` / `eql` / `toCss`）、`CssImportConditions`（`empty` / `isEmpty` / `hasAnonymousLayer` / `layersEql` / `supportsEql` / `eql` / `toCss`）、`CssImportRule`（`fromUrl` / `fromUrlAndImportRecordIdx` / `fromConditionsAndUrl` / `parse` / `hasConditions` / `hasImportRecord` / `isExternalUrl` / `eql` / `toCss` / `toCssWithPlaceholder`）、`CssImportRuleList`（`extractFromStylesheet` / `count` / `get`、`ignored_after_rules`）。
- 规范防线：支持 `"..."` / `'...'` / `url("...")` / `url('...')` / `url(...)` 目标格式；强制校验 `layer` 必须位于 `supports(...)` 之前（违序返回 `DuplicateCondition`）；`CssImportRuleList.extractFromStylesheet()` 严格遵循 CSS Syntax 规范，仅采集位于 `@charset` 与前置 `@layer` 语句之后、首个普通样式规则之前的 `@import`，并自动统计被规范忽略的迟到 `@import` 数量。

```cp
import { CssImportRule, CssImportRuleList } from "cap:css";

const rule = CssImportRule.parse(
    "@import url(\"./theme.css\") layer(framework.theme) supports(display: grid) screen and (min-width: 768px);"
)?;
let buf: [256]u8 = [0; 256];
const n = rule.toCss(buf[0..])?;
// "@import \"./theme.css\" layer(framework.theme) supports(display: grid) screen and (min-width: 768px);"

const list = CssImportRuleList.extractFromStylesheet(
    "@charset \"utf-8\"; @layer reset, theme; @import \"./reset.css\" layer(reset); body { color: #111; } @import \"./late.css\";"
)?;
// list.count() == 1, list.ignored_after_rules == 1
* = n;
```

### CSS `@counter-style` 规则解析、校验与实时计数器渲染（`counter.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/counter_style.cp`，零堆分配实现 W3C CSS Counter Styles Level 3 `@counter-style` 全套描述符解析器（`system`、`symbols`、`additive-symbols`、`negative`、`prefix`、`suffix`、`range`、`pad`、`fallback`、`speak-as`）、规范序列化器与实时计数器/列表标记格式化引擎。

- 核心导出：`CSS_COUNTER_MAX_SYMBOLS` (`32`)、`CSS_COUNTER_MAX_RANGES` (`8`)、`CssCounterStyleError`、`CssCounterSystemKind`、`CssCounterSystem`、`CssAdditiveTuple`、`CssCounterRange`、`CssCounterSpeakAs`、`CssCounterStyleRule`（`initEmpty` / `parse` / `isInRange` / `formatCounter` / `formatMarker` / `toCss` / `eql`）。
- 算法覆盖：完整支持 `cyclic`、`fixed <integer>`、`symbolic`、`alphabetic`（Excel 列名式双射进制）、`numeric`（任意进制）、`additive`（贪心降序加权，如罗马数字 `upper-roman`）以及 `negative` 前后缀、`pad` 最小宽度补齐（含 UTF-8 码点计数与负号计宽）和 `prefix`/`suffix` 标记拼装。
- 规范防线：禁止 `none` 及 CSS-wide 关键字作为 `<counter-style-name>`；`numeric`/`alphabetic` 强制至少 2 个符号；`additive-symbols` 强制权重非负且严格降序；`extends` 禁止声明 `symbols`/`additive-symbols`；超界数值返回 `OutOfRange` 以提示调用方切入 `fallback` 样式。

```cp
import { CssCounterStyleRule } from "cap:css";

const roman = CssCounterStyleRule.parse(
    "@counter-style upper-roman { system: additive; range: 1 3999; additive-symbols: 1000 M, 900 CM, 500 D, 400 CD, 100 C, 90 XC, 50 L, 40 XL, 10 X, 9 IX, 5 V, 4 IV, 1 I; suffix: \". \"; }"
)?;
let buf: [64]u8 = [0; 64];
const n = roman.formatMarker(2026, buf[0..])?;
// "MMXXVI. "
* = n;
```

### CSS `@starting-style` 入场过渡规则与嵌套提升（`startStyle.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/starting_style.cp`，零堆分配实现 W3C CSS Transitions Level 2 `@starting-style` 规则解析、块内声明/子规则拆解、压缩与规范序列化、嵌套父选择器提升（`liftForParent`）与样式表批量提取。

- 核心导出：`CSS_STARTING_STYLE_MAX_ENTRIES` (`32`)、`CSS_STARTING_STYLE_MAX_RULES` (`32`)、`CssStartingStyleError`、`CssStartingStyleEntryKind`、`CssStartingStyleEntry`、`CssStartingStyleRule`（`empty` / `parse` / `isEmpty` / `hasDirectDeclarations` / `count` / `get` / `eql` / `toCss` / `toCssMinified` / `liftForParent`）、`CssStartingStyleRuleList`（`extractFromStylesheet` / `count` / `get`）。
- 规范防线与嵌套降级：`@starting-style` 强制要求空 prelude（禁止 `@starting-style foo { ... }`），严格拦截内部非法 `@import` / `@charset` / `@namespace`；配合 `/data/dev/bun/docs/bundler/css.mdx` 的 CSS Nesting 展平管线，`liftForParent(".popover", buf)` 可将嵌套在选择器内部的 `@starting-style { opacity: 0; &:hover { ... } }` 展平提升为顶层 `@starting-style { .popover { opacity: 0; } .popover:hover { ... } }`。

```cp
import { CssStartingStyleRule } from "cap:css";

const nested = CssStartingStyleRule.parse("@starting-style { opacity: 0; transform: translateY(8px); }")?;
let buf: [256]u8 = [0; 256];
const n = nested.liftForParent("dialog[open]", buf[0..])?;
// "@starting-style { dialog[open] { opacity: 0; transform: translateY(8px); } }"
* = n;
```

### CSS 逻辑属性 12 分组映射与 LTR/RTL 双向降级（`logical.cp`）

> 吸收自 `/data/dev/bun/src/css/logical.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`，零堆分配实现 CSS Logical Properties & Values Level 1 全部 12 大分组（`border_color`/`border_style`/`border_width`/`border_radius`/`margin`/`scroll_margin`/`padding`/`scroll_padding`/`inset`/`size`/`min_size`/`max_size`）的分类、LTR/RTL 物理属性映射及 `:dir(ltr)`/`:dir(rtl)` 自动降级规则生成。

- 核心导出：`CssLogicalError`、`LogicalPropertyCategory`（`physical`/`logical`）、`LogicalPropertyGroup`（12 组）、`WritingDirection`（`ltr`/`rtl`）、`LogicalPhysicalPair`（`isDirectionDependent`/`physicalFor`/`secondPhysicalFor`）、`classifyLogicalProperty`、`logicalGroupOf`、`mapLogicalToPhysical`、`formatLogicalFallbackRule`。
- 降级语义：方向无关属性（如 `block-size`、`margin-block-start`）以及对称简写（如 `margin-inline: 16px`）直接降级为单块物理声明；方向相关属性（如 `margin-inline-start`、`border-start-end-radius`）及非对称 inline 轴双值（如 `margin-inline: 8px 16px`）自动生成 `:dir(ltr)` 与 `:dir(rtl)` 双分支降级规则。

```cp
import { formatLogicalFallbackRule } from "cap:css";

let buf: [256]u8 = [0; 256];
const n = formatLogicalFallbackRule(".card", "margin-inline-start", "16px", buf[0..])?;
// ":dir(ltr) .card { margin-left: 16px; } :dir(rtl) .card { margin-right: 16px; }"
* = n;
```

### CSS `@font-face` 规则解析与 `<urange>` 问号压缩（`fontFace.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/font_face.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`，零堆分配实现 `@font-face` 描述符解析（`font-family`/`src`/`font-weight`/`font-style`/`font-display`/`unicode-range`）、`url(...) format(...) tech(...)` 与 `local(...)` 来源解析，以及 W3C `<urange>` 问号通配最优压缩（如 `U+0000-00FF` → `U+??`、`U+4000-4FFF` → `U+4???`）。

- 核心导出：`CSS_UNICODE_MAX_CODEPOINT` (`0x10FFFF`)、`CSS_FONT_FACE_MAX_SOURCES` (`16`)、`CSS_FONT_FACE_MAX_RANGES` (`32`)、`CssFontFaceError`、`CssFontFormat`、`CssFontDisplay`、`CssFontSourceKind`、`CssUnicodeRange`（`parse`/`contains`/`eql`/`toCss`）、`CssFontFaceSource`、`CssFontFaceRule`（`parse`/`sourceAt`/`rangeAt`/`matchesCodePoint`/`toCss`）。

```cp
import { CssFontFaceRule, CssUnicodeRange } from "cap:css";

const r = CssUnicodeRange.parse("U+0000-00FF")?;
let ubuf: [32]u8 = [0; 32];
const un = r.toCss(ubuf[0..])?; // "U+??"

const ff = CssFontFaceRule.parse(
    "@font-face { font-family: 'Inter'; src: local('Inter'), url('./inter.woff2') format('woff2') tech(variations); font-display: swap; unicode-range: U+0000-00FF, U+4E00-4FFF; }"
)?;
const hit = ff.matchesCodePoint(0x4E2D); // true
* = un;
* = hit;
```

### CSS 声明块解析、去重压缩与序列化（`declBlock.cp`）

> 吸收自 `/data/dev/bun/src/css/declaration.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`，采用普通声明与 `!important` 声明双桶分离布局，零堆分配实现词法感知解析、`error_recovery` 容错恢复、级联安全去重压缩（`minifyDeduplicate`）、`propertyIdHash` 规则合并签名以及美化/压缩双模式序列化。

- 核心导出：`CSS_DECL_MAX_NORMAL` (`64`)、`CSS_DECL_MAX_IMPORTANT` (`32`)、`CssDeclBlockError`、`CssDeclItem`（`eqlName`/`eql`）、`CssDeclParseOptions`、`CssDeclarationBlock`（`init`/`parse`/`push`/`isEmpty`/`len`/`normalSlice`/`importantSlice`/`getEffective`/`minifyDeduplicate`/`propertyIdHash`/`toCss`/`toCssBlock`）。

```cp
import { CssDeclarationBlock, CssDeclParseOptions } from "cap:css";

let blk = CssDeclarationBlock.parse(
    "{ color: red; background: url('a;b:c.png'); color: blue !important; }",
    CssDeclParseOptions{},
)?;
const removed = blk.minifyDeduplicate(); // 移除被 !important 覆盖的 color: red (removed == 1)
let out: [128]u8 = [0; 128];
const n = blk.toCssBlock(out[0..], true)?;
// out[0..n] == "{background:url('a;b:c.png');color:blue!important}"
* = removed;
* = n;
```

### CSS 属性渐进降级上下文（`handlerCtx.cp`）

> 吸收自 `/data/dev/bun/src/css/context.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`，零堆分配追踪声明语法上下文（`StyleRule`/`Keyframes`/`StyleAttribute`）、聚合 `@supports` 条件回退桶、生成 `:dir(ltr)`/`:dir(rtl)` 逻辑属性降级规则与 `@media (prefers-color-scheme: dark)` 深色主题降级规则，并支持 CSS Modules 未使用符号表继承与死规则剪枝。

- 核心导出：`CSS_CTX_MAX_SUPPORTS` (`16`)、`CSS_CTX_MAX_UNUSED_SYMBOLS` (`32`)、`CssHandlerContextError`、`CssDeclarationContext`（`allowsLogicalLowering`/`allowsAuxiliaryRules`）、`CssSupportsFallbackBucket`、`CssPropertyHandlerContext`（`init`/`child`/`shouldCompileLogical`/`markUnusedSymbol`/`isSymbolUnused`/`addConditionalProperty`/`addLogicalRule`/`addDarkRule`/`hasAdditionalRules`/`reset`/`formatSupportsRules`/`formatAdditionalRules`）。

```cp
import { CssDeclarationContext, CssPropertyHandlerContext } from "cap:css";

let ctx = CssPropertyHandlerContext.init(CssDeclarationContext.StyleRule);
* = ctx.addConditionalProperty("(color: oklch(0% 0 0))", "color", "oklch(0.7 0.15 240)")?;
* = ctx.addLogicalRule("margin-left", "1rem", "margin-right", "1rem")?;
* = ctx.addDarkRule("background", "#111")?;

let sbuf: [256]u8 = [0; 256];
const sn = ctx.formatSupportsRules(".card", sbuf[0..], true)?;
// "@supports (color: oklch(0% 0 0)){.card{color:oklch(0.7 0.15 240)}}"
let abuf: [256]u8 = [0; 256];
const an = ctx.formatAdditionalRules(".card", abuf[0..], true)?;
// ".card:dir(ltr){margin-left:1rem}.card:dir(rtl){margin-right:1rem}@media (prefers-color-scheme:dark){.card{background:#111}}"
* = sn;
* = an;
```

### CSS `@viewport` 规则解析、校验与多前缀降级输出（`viewport.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/viewport.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`，零堆分配实现 W3C CSS Device Adaptation Level 1 `@viewport` 与供应商前缀变体（`@-ms-viewport` / `@-webkit-viewport` / `@-o-viewport` / `@-moz-viewport`）的解析、标准描述符语义校验、`width`/`height` 简写展开、级联安全去重、美化/压缩序列化、多前缀降级输出及样式表批量提取。

- 核心导出：`CSS_VIEWPORT_MAX_DECLS` (`32`)、`CSS_VIEWPORT_MAX_RULES` (`16`)、`CssViewportError`、`CssViewportVendorPrefix`（`asStr`/`atRuleName`/`isPrefixed`/`fromAtKeyword`）、`CssViewportDescriptorKind`（`fromName`/`canonicalName`/`isShorthand`）、`CssViewportUserZoom`、`CssViewportOrientation`、`CssViewportFit`、`CssViewportMinMaxPair`（`parseShorthand`/`eql`）、`CssViewportDeclaration`（`validate`/`eql`）、`CssViewportRule`（`init`/`parse`/`push`/`count`/`isEmpty`/`at`/`find`/`effectiveOrientation`/`effectiveUserZoom`/`effectiveViewportFit`/`expandWidthShorthand`/`expandHeightShorthand`/`deduplicate`/`eql`/`toCss`/`toCssMinified`/`toCssWithPrefixes`）、`CssViewportRuleList`（`extractFromStylesheet`/`count`/`at`）。

```cp
import { CssViewportRule, CssViewportVendorPrefix } from "cap:css";

const rule = CssViewportRule.parse(
    "@-ms-viewport { width: device-width; zoom: 1.0; orientation: portrait; viewport-fit: cover; }"
)?;
const prefixes = [CssViewportVendorPrefix.Ms, CssViewportVendorPrefix.None];
let out: [256]u8 = [0; 256];
const n = rule.toCssWithPrefixes(out[0..], prefixes[0..], true)?;
// "@-ms-viewport{width:device-width;zoom:1.0;orientation:portrait;viewport-fit:cover}@viewport{width:device-width;zoom:1.0;orientation:portrait;viewport-fit:cover}"
* = n;
```

### CSS 未知 / 扩展 `@-rule` 通用保留与提取（`unknown.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/unknown.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`，零堆分配实现 W3C CSS Syntax Level 3 未知及工具链扩展 `@-rule`（如 `@tailwind`、`@theme`、`@utility`、`@position-try`、`@view-transition`、`@-moz-document`）的语句/块双形态解析、括号安全 prelude 词法切分、标准/扩展分类、带源码行列号追踪的样式表批量提取及美化/压缩序列化。

- 核心导出：`CSS_UNKNOWN_AT_MAX_RULES` (`32`)、`CSS_UNKNOWN_AT_MAX_PRELUDE_TOKENS` (`32`)、`CssUnknownAtRuleError`、`isStandardKnownAtRule`、`CssUnknownAtRuleKind`（`Statement`/`Block`）、`CssUnknownPreludeTokens`（`parse`/`count`/`at`）、`CssUnknownAtRule`（`statement`/`withBlock`/`parse`/`kind`/`hasBlock`/`hasPrelude`/`isVendorPrefixed`/`isKnownStandard`/`preludeTokens`/`eql`/`toCss`/`toCssMinified`）、`CssUnknownAtRuleList`（`extractUnknownFromStylesheet`/`extractAllAtRulesFromStylesheet`/`count`/`at`）。

```cp
import { CssUnknownAtRule, CssUnknownAtRuleList } from "cap:css";

const rule = CssUnknownAtRule.parse("@position-try --flip-block { top: anchor(bottom); }")?;
let out: [128]u8 = [0; 128];
const n = rule.toCssMinified(out[0..])?;
// "@position-try --flip-block{top: anchor(bottom);}"

const list = CssUnknownAtRuleList.extractUnknownFromStylesheet(
    "@import \"./base.css\";\n@tailwind utilities;\n@theme { --color-brand: #3b82f6; }"
)?;
// list.count() == 2 (自动跳过已知标准 @import，仅提取 @tailwind 与 @theme)
* = n;
* = list;
```

### CSS `@-moz-document` 条件文档规则解析与 URL 匹配（`document.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/document.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`，零堆分配实现 W3C CSS Conditional Rules Level 3 `@-moz-document` 规则解析（完整支持 `url()` / `url-prefix()` / `domain()` / `media-document()` / `regexp()`）、对齐 Firefox/Bun 规范的严格无参 `url-prefix()` 模式（`parseStrict`）、域名子域边界安全的 URL 实时匹配器、美化/压缩序列化及样式表批量提取。

- 核心导出：`CSS_DOCUMENT_MAX_CONDITIONS` (`16`)、`CSS_DOCUMENT_MAX_RULES` (`16`)、`CssDocumentRuleError`、`CssDocumentMatchKind`（`Url`/`UrlPrefix`/`Domain`/`MediaDocument`/`Regexp`）、`CssDocumentCondition`（`isEmptyUrlPrefix`/`matchesUrl`/`eql`/`writeCss`）、`CssMozDocumentRule`（`urlPrefixEmpty`/`parse`/`parseStrict`/`conditionCount`/`conditionAt`/`isFirefoxHack`/`matchesUrl`/`eql`/`toCss`/`toCssMinified`）、`CssMozDocumentRuleList`（`extractFromStylesheet`/`count`/`at`）。

```cp
import { CssMozDocumentRule } from "cap:css";

const strict_rule = CssMozDocumentRule.parseStrict("@-moz-document url-prefix() { .ff { gap: 4px; } }")?;
const hit = strict_rule.matchesUrl("https://example.com/app"); // true
let out: [128]u8 = [0; 128];
const n = strict_rule.toCssMinified(out[0..])?;
// "@-moz-document url-prefix(){.ff { gap: 4px; }}"
* = hit;
* = n;
```

### CSS Tailwind 指令（`@tailwind` / `@apply`）解析与级联诊断（`tailwind.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/tailwind.cp` 与 `/data/dev/bun/docs/bundler/css.mdx`，零堆分配实现 Tailwind `@tailwind <base|components|utilities|variants>;` 指令解析与序列化、`@apply` 实用类列表解析（支持方括号任意值如 `bg-[#1da1f2]` 与末尾 `!important`）、样式表批量提取（含行列号追踪）、重复层检测及标准级联顺序校验（`base < components < utilities < variants`）。

- 核心导出：`CSS_TAILWIND_MAX_RULES` (`32`)、`CSS_TAILWIND_MAX_APPLY_CLASSES` (`32`)、`CssTailwindError`、`CssTailwindStyleName`（`Base`/`Components`/`Utilities`/`Variants` + `fromStr`/`asStr`/`cascadeOrder`）、`CssTailwindAtRule`（`of`/`parse`/`cascadeOrder`/`eql`/`toCss`）、`CssTailwindApplyRule`（`parse`/`count`/`at`/`contains`/`toCss`）、`CssTailwindDirectiveList`（`extractFromStylesheet`/`count`/`at`/`hasLayer`/`hasDuplicateLayers`/`isCanonicalOrder`）。

```cp
import { CssTailwindDirectiveList, CssTailwindApplyRule } from "cap:css";

const list = CssTailwindDirectiveList.extractFromStylesheet(
    "@tailwind base;\n@tailwind components;\n@tailwind utilities;\n"
)?;
const ok_order = list.isCanonicalOrder() && !list.hasDuplicateLayers(); // true
const applied = CssTailwindApplyRule.parse("@apply flex items-center bg-[#1da1f2] !important;")?;
* = ok_order;
* = applied;
```

### CSS Values & Units Level 4 数学函数与常量求值引擎（`mathFunc.cp`）

> 吸收自 `/data/dev/bun/src/css/values/calc.cp`，零堆分配实现 W3C CSS Values & Units Level 4 全部 21 种数学函数（`calc`/`min`/`max`/`clamp`/`round`/`mod`/`rem`/`abs`/`sign`/`sin`/`cos`/`tan`/`asin`/`acos`/`atan`/`atan2`/`pow`/`sqrt`/`hypot`/`log`/`exp`）、5 种数学常量（`pi`/`e`/`infinity`/`-infinity`/`NaN`）与 4 种角度单位（`deg`/`rad`/`grad`/`turn`）的递归下降求值与编译期常量折叠。

- 核心导出：`CSS_MATH_MAX_DEPTH` (`32`)、`CSS_MATH_MAX_ARGS` (`16`)、`CSS_MATH_PI`、`CSS_MATH_E`、`CssMathFuncError`、`CssMathRoundingStrategy`（`Nearest`/`Up`/`Down`/`ToZero`）、`CssMathConstant`（`E`/`Pi`/`Infinity`/`NegInfinity`/`NaN`）、`CssMathFuncKind`（21 种函数 + `parse`/`asStr`/`isTrig`/`isStepped`/`isExponential`）、`cssMathRound`、`cssMathMod`、`cssMathRem`、`cssMathSign`、`cssMathClamp`、`cssMathHypot`、`cssMathLog`、`cssMathAngleToRad`、`evalCssMathExpr`。

```cp
import { evalCssMathExpr, cssMathMod, cssMathRem } from "cap:css";

const v1 = evalCssMathExpr("calc(round(up, hypot(3, 4) * 2 + 1, 5) + abs(-3))")?; // 18.0
const v2 = evalCssMathExpr("sin(90deg) + cos(0.5turn)")?;                         // 0.0
const m = cssMathMod(-18.0, 5.0)?;                                                // 2.0 (与 divisor 同号)
const r = cssMathRem(-18.0, 5.0)?;                                                // -3.0 (与 dividend 同号)
* = v1;
* = v2;
* = m;
* = r;
```

---

## 上游 Bun (LightningCSS) 行为契约与边界注意事项

1. **引擎血统与默认浏览器降级基线**：
   - 参考 `/data/dev/bun/docs/bundler/css.mdx`，Bun CSS 引擎源自 LightningCSS 的原生移植，默认面向 ES2020 / Chrome 87+ / Safari 14+ / Firefox 78+ / Edge 88+ 执行语法降级与厂商前缀注入；`cap:css` 在 `CssPrefixer`、`CssLogical` 与 `CssNesting.flatten()` 中完整保留了这一降级管线。
2. **CSS 嵌套展平（`CssNesting.flatten()`）语义**：
   - 支持选择器内嵌套子选择器以及嵌套 `@media` 等条件规则；展平后自动将父选择器注入子规则并将嵌套 `@media` 提升至顶层，兼容不支持原生 CSS Nesting 的运行环境。
3. **Selectors Level 4 特异性规则（`CssSpecificity`）**：
   - `:where(...)` 特异性恒为 `(0, 0, 0)`；`:is(...)`、`:not(...)`、`:has(...)` 取参数列表中特异性最高的选择器作为整体特异性。
4. **CSS Color 4 与数学求值边界**：
   - `CssColor` 支持 `oklab()` / `oklch()` / `hwb()` / `hsl()` / `rgb()` / 148 命名色与色域转换；`CssCalc` 递归下降求值器遵循规范要求：`+` 与 `-` 运算符两侧必须留白（`calc(100% - 16px)`），支持 `min()`、`max()`、`clamp()` 嵌套。
5. **CSS Modules 作用域与 `composes` 约束（参考 `/data/dev/bun/docs/bundler/css.mdx`）**：
   - `.module.css` 文件自动启用局部作用域，类名与 `@keyframes` 动画名会被改写为带模块哈希的唯一标识符，避免全局命名冲突。
   - `composes` 声明**必须**位于规则块内所有普通 CSS 属性声明之前，且**仅允许**用于单个类选择器（如 `.button`）；在 ID 选择器（`#button`）或逗号复合选择器（`.a, .b`）中使用 `composes` 属于非法语法。
   - 跨文件组合（`composes: bg from "./background.module.css"`）时，应避免被组合类之间定义相互冲突的同名属性（CSS Modules 规范将跨文件同名属性冲突定义为未定义行为）。
6. **`@layer` 级联层声明顺序与最佳实践**：
   - 推荐在入口样式表顶部使用单条语句规则（如 `@layer reset, base, components, utilities;`）显式锁定层顺序；后续重复出现的同名 `@layer` 块或语句不会改变其首次声明时的优先级。
   - 点分嵌套层名（如 `framework.theme.dark`）的 `.` 两侧严禁空白；同一块规则的 prelude 不允许逗号列表（`@layer a, b { ... }` 为非法语法）。
7. **`@import` 条件顺序、外链保留与打包占位符改写（参考 `/data/dev/bun/docs/bundler/css.mdx`）**：
   - W3C CSS Cascading Level 5 规定 `@import` 必须位于样式表头部（仅允许出现在 `@charset` 与前置 `@layer` 语句之后、任何普通规则之前）；出现在普通规则之后的 `@import` 必须被忽略（`CssImportRuleList.ignored_after_rules` 可用于静态诊断告警）。
   - `@import` 条件严格遵循 `layer` → `supports(...)` → `<media-query-list>` 顺序；打包器在内联本地样式表时应通过 `isExternalUrl()` 跳过 `http://` / `https://` / `//` / `data:` 外链资源，并使用 `toCssWithPlaceholder()` 生成稳定的依赖占位符。
8. **声明块 `!important` 双桶分离与自定义属性大小写规则（`declBlock.cp`）**：
   - 对齐 LightningCSS/Bun 的 `DeclarationBlock` 布局，普通声明与 `!important` 声明分别存储在独立数组中，序列化时先输出普通声明再输出 `!important` 声明；压缩模式下省略块内末条声明的分号与冒号后空格；
   - 标准 CSS 属性名按 ASCII 大小写无关比对（`Color` 与 `color` 视为同一属性），而 `--` 开头的 CSS 自定义属性（CSS Variables）严格区分大小写（`--Theme` 与 `--theme` 互不覆盖）。
9. **`@viewport` 供应商前缀降级与描述符边界约束（`viewport.cp`，参考 `/data/dev/bun/docs/bundler/css.mdx`）**：
   - 遵循 Bun CSS Bundler 的默认浏览器目标与厂商前缀注入策略：`@viewport` 规则块不接受任何 prelude token（`@viewport screen { ... }` 属于非法语法），标准描述符仅接受规范定义的值域（`user-zoom` 仅限 `zoom | fixed`，`orientation` 仅限 `auto | portrait | landscape`，`viewport-fit` 仅限 `auto | contain | cover`，`zoom`/`min-zoom`/`max-zoom` 禁止负值）；
   - `width`/`height` 简写按 W3C 规范接收 1~2 个 `<viewport-length>` 值（单值同时展开为 `min-*` 与 `max-*`，双值分别对应 `min-*` 与 `max-*`）；针对旧版 Edge/IE 兼容场景，推荐使用 `toCssWithPrefixes()` 同时输出 `@-ms-viewport` 与标准 `@viewport`。
10. **未知与工具链扩展 `@-rule` 保留透传契约（`unknown.cp`，参考 `/data/dev/bun/docs/bundler/css.mdx`）**：
    - 现代 CSS 生态与 Bun CSS Bundler 插件（如 Tailwind v4 的 `@tailwind` / `@theme` / `@utility` / `@custom-variant`，以及 CSS Anchor Positioning `@position-try` / `@view-transition`）依赖解析器无损保留未知 `@-rule` 的原始 prelude 与块内容；
    - 词法边界：语句型未知规则必须以顶层 `;` 终止，块型规则的 `{...}` 及内部 `()`/`[]` 必须严格平衡；序列化时 at-keyword 统一规范为小写 ASCII，prelude 与块内容保持原样不破坏下游插件语法。
11. **`@-moz-document` 严格模式与域名匹配安全边界（`document.cp`）**：
    - 仅 Firefox 曾实现该规则且现代版本出于隐私限制仅保留无参 `@-moz-document url-prefix()`（用作 Firefox 特性检测）；与 Bun/LightningCSS 保持一致，默认拒绝非 `-moz` 前缀（如 `@document` / `@-webkit-document` 返回 `UnsupportedPrefix`），生产打包推荐使用 `parseStrict()` 强制约束；
    - `domain("example.com")` 匹配器严格校验点号边界，匹配 `example.com` 与 `sub.example.com`，绝不误匹配 `evil-example.com`；自动剥离 URL 中的 `userinfo@` 与 `:port`。
12. **Tailwind `@tailwind` / `@apply` 指令级联与插件透传契约（`tailwind.cp`，参考 `/data/dev/bun/docs/bundler/css.mdx`）**：
    - `@tailwind` 仅允许 `base` / `components` / `utilities` / `variants` 四种标准层名且严禁附带块 `{...}`；推荐按 `base -> components -> utilities -> variants` 严格递增顺序声明并通过 `isCanonicalOrder()` 与 `hasDuplicateLayers()` 在打包预处理阶段执行静态诊断；
    - `@apply` 的 `!important` 修饰符必须位于实用类列表末尾，方括号任意值（如 `grid-cols-[1fr_2fr]`）内部括号须严格平衡。
13. **CSS Math Level 4 编译期求值与降级边界（`mathFunc.cp`，参考 `/data/dev/bun/docs/bundler/css.mdx`）**：
    - 当 `round()`、`mod()`、`rem()`、`sin()`/`cos()`/`tan()`、`pow()`/`sqrt()`/`hypot()`/`log()`/`exp()` 仅含静态数值、数学常量（`pi`/`e`）或已知角度单位（`deg`/`rad`/`grad`/`turn`）时，打包器应在构建期直接求值折叠为静态数值，以兼容未原生支持 CSS Math Level 4 的旧浏览器（Chrome < 111 / Safari < 15.4）；
    - W3C 关键语义约束：`mod(a, b)` 结果与除数 `b` 同号，而 `rem(a, b)` 结果与被除数 `a` 同号；`clamp(min, val, max)` 等价于 `max(min, min(val, max))`，当 `min > max` 时恒由 `min` 胜出；`round()` 首参可省略策略关键字（默认 `nearest`），当首参为数学常量（如 `round(pi, 1)`）时必须识别为被取整值而非非法策略。
