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
| `lib/css/property_rule.cp` | `bun/src/css/rules/property.cp` | `MAX_PROPERTY_SYNTAX_COMPONENTS`, `CssPropertyRuleError`, `isValidDashedIdent`, `PropertySyntax`, `CssPropertyRule`（零堆分配 W3C CSS Properties and Values API Level 1 `@property` 自定义属性注册解析、`syntax`/`inherits`/`initial-value` 规范校验与美化/压缩序列化） |
| `lib/css/font_palette_values.cp` | `bun/src/css/rules/font_palette_values.cp` | `MAX_PALETTE_OVERRIDES`, `CssFontPaletteError`, `BasePaletteKind`, `BasePalette`, `PaletteOverrideColor`, `CssFontPaletteValuesRule`（零堆分配 W3C CSS Fonts Level 4 `@font-palette-values` 彩色字体调色板解析、泛型字体族与 `currentColor` 拒绝、非负槽位覆写查询与格式化/压缩序列化） |

### `@font-palette-values` 彩色字体调色板示例（`font_palette_values.cp`）

```typescript
import { CssFontPaletteValuesRule } from "cap:css";

const palette = CssFontPaletteValuesRule.parse(
    "@font-palette-values --brand { font-family: \"Bungee Spice\"; base-palette: 1; override-colors: 0 #ff6b6b, 1 rgb(25, 113, 194); }"
)?;
// palette.findOverride(0) == "#ff6b6b", palette.findOverride(1) == "rgb(25, 113, 194)"
let buf: [256]u8 = undefined;
const minified = palette.toCssMinified(buf[0..])?;
```

### `@property` 自定义属性注册规则示例（`property_rule.cp`）

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

### CSS Modules 命名模式、哈希与 `composes` 解析（`css_modules.cp`）

> 归并自原 `lib/cssmodules/`（Bun `src/css/css_modules.cp`）。

- 核心导出：`CssModuleConfig`, `PatternKind`, `PatternSegment`, `Pattern`, `ReferenceKind`, `CssModuleRef`, `isValidCssIdent`, `extractFileStemSanitized`, `computeModuleHash`, `scopeClassName`, `parseComposesValue`。

### `@keyframes` 规则解析、排序与时间线采样（`keyframes.cp`）

> 归并自原 `lib/keyframes/`（Bun `src/css/rules/keyframes.cp`）；为避免与 `atrules.cp` 既有 `KeyframeSelector` 冲突，`KeyframeSelector` 在主包中别名为 `CssKeyframeRuleSelector`。

- 核心导出：`VendorPrefix`, `KeyframesNameKind`, `KeyframesName`, `SelectorKind`, `CssKeyframeRuleSelector`, `KeyframeStop`, `ParsedKeyframes`, `KeyframeSegment`, `isReservedCssKeyword`, `parseKeyframesRule`, `sortStopsByOffset`, `sampleKeyframeSegment`。

> 注：原 `lib/mediaquery/mediaquery.cp` 因与 `lib/css/media.cp` 存在 Media Queries Level 4 功能重叠，按参考归档规范保留为 `lib/css/media_query_.cp`（不从 `cap:css` 重复导出）。

### `@custom-media` 规则解析、序列化与媒体查询展开（`custom_media.cp`）

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

### `@namespace` 规则解析、序列化与命名空间注册表（`namespace_rule.cp`）

> 吸收自 `/data/dev/bun/src/css/rules/namespace.cp`，零堆分配支持 W3C CSS Namespaces Level 3 默认/前缀命名空间声明（字符串与 `url(...)` 双语法）、规范序列化及选择器前缀解析。

- 核心导出：`CSS_NAMESPACE_MAX_RULES` (`32`)、`CSS_HTML_NS`、`CSS_SVG_NS`、`CSS_MATHML_NS`、`CssNamespaceError`、`NamespaceRule`（`parse` / `isDefault` / `toCss`）、`NamespaceRegistry`（`init` / `register` / `lookupPrefix` / `resolveSelectorPrefix`）。

```typescript
import { NamespaceRule, NamespaceRegistry, CSS_SVG_NS } from "cap:css";

let reg = NamespaceRegistry.init();
* = reg.register(NamespaceRule.parse("@namespace svg url(\"http://www.w3.org/2000/svg\");")?)?;
const uri = reg.resolveSelectorPrefix("svg") ?? ""; // CSS_SVG_NS
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
