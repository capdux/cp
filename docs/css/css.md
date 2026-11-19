# CP 标准库：工业级 CSS 引擎与样式表处理手册 (`cap:css`)

## 一、 架构总览与 100 模块分层设计

`cap:css` 是 CP 语言官方内置的工业级 CSS 解析、值计算、选择器权重求值与样式表压缩标准库，完整对照并吸收自 Bun 内置的 LightningCSS 架构引擎（`/data/dev/bun/src/css/`，全仓共 **100 个** `.cp` 源码模块）。对外通过 `lib/css/index.cp` 暴露零堆分配、纯现代 CP 的高层 API 门面，同时保留底层完整 5 层引擎模块谱系以供深度扩展与上游同步。

```text
┌────────────────────────────────────────────────────────────────────────┐
│              Layer 5: Printer / Minifier & CSS Modules                 │
│  CssRule.formatRule(minify) · CssStylesheetParser.minify()             │
│  [底层谱系: printer.cp · sourcemap.cp · css_modules.cp · targets.cp]    │
├────────────────────────────────────────────────────────────────────────┤
│         Layer 4: Rules / At-Rules / MediaQuery / Keyframes             │
│  CssRuleKind · CssProperty · CssMediaQuery · CssKeyframes · CssRule    │
│  [底层谱系: rules/ (22 模块: style/media/keyframes/container/layer...)] │
├────────────────────────────────────────────────────────────────────────┤
│             Layer 3: Selectors Level 4 & W3C Specificity               │
│  CssSelector.parse() · CssSpecificity (ids/classes/elements)           │
│  matchesTag() · matchesElement()                                       │
│  [底层谱系: selectors/ (3 模块: parser.cp · selector.cp · builder.cp)]  │
├────────────────────────────────────────────────────────────────────────┤
│          Layer 2: Values Level 4 / Color 4 / Units / Calc()            │
│  CssColor (Hex/RGB/HSL/OKLCH/Named/WCAG/Mix)                           │
│  CssUnit (18 单位) · CssLength (toPx/toMs/toDeg) · CssCalc.evalPx()     │
│  [底层谱系: values/ (22 模块) + properties/ (31 属性处理器模块)]        │
├────────────────────────────────────────────────────────────────────────┤
│               Layer 1: Tokenizer / Parser & Diagnostics                │
│  CssStylesheetParser.parseRule() · parseStylesheet()                   │
│  [底层谱系: css_parser.cp · declaration.cp · media_query.cp · error.cp] │
└────────────────────────────────────────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源（全 100 模块）

`lib/css/` 下全部 100 个 `.cp` 文件第 1 行均已严格校准为 `// Source: /data/dev/bun/src/css/...`：

| 分层与目录 | 模块数 | 代表文件 | 上游谱系路径 | 核心职责 |
| :--- | :---: | :--- | :--- | :--- |
| **对外公共门面** | 5 | `index.cp`, `color.cp`, `length.cp`, `selector.cp`, `stylesheet.cp` | `/data/dev/bun/src/css/{css_parser,values/color,values/length,selectors/parser,rules/rules}.cp` | `cap:css` 零堆分配、零 Bun 耦合的纯现代 CP 公共 API |
| **核心引擎与管线** | 17 | `css_parser.cp`, `declaration.cp`, `media_query.cp`, `printer.cp`, `css_modules.cp`, `targets.cp`, `compat.cp`, `prefixes.cp`, `logical.cp`, `sourcemap.cp`, `context.cp`, `dependencies.cp`, `error.cp`, `generics.cp`, `small_list.cp` | `/data/dev/bun/src/css/*.cp` | 词法/语法解析框架、浏览器兼容目标、厂商前缀、逻辑属性、CSS Modules 与 SourceMap |
| **属性处理器 (`properties/`)** | 31 | `align.cp`, `animation.cp`, `background.cp`, `border.cp`, `border_radius.cp`, `box_shadow.cp`, `display.cp`, `flex.cp`, `font.cp`, `grid.cp`, `margin_padding.cp`, `position.cp`, `text.cp`, `transform.cp`, `transition.cp`, `custom.cp` 等 | `/data/dev/bun/src/css/properties/*.cp` | 覆盖 Flexbox、Grid、动画、渐变背景、边框阴影、自定义属性与前缀降级 |
| **规则引擎 (`rules/`)** | 22 | `rules.cp`, `style.cp`, `media.cp`, `keyframes.cp`, `container.cp`, `layer.cp`, `nesting.cp`, `scope.cp`, `supports.cp`, `font_face.cp`, `import.cp`, `property.cp`, `starting_style.cp`, `tailwind.cp` 等 | `/data/dev/bun/src/css/rules/*.cp` | 完整 CSS At-Rules 语法树与嵌套规则展开 |
| **选择器 (`selectors/`)** | 3 | `parser.cp`, `selector.cp`, `builder.cp` | `/data/dev/bun/src/css/selectors/*.cp` | CSS Selectors Level 4 词法、选择器列表与组合器构建 |
| **值与单位 (`values/`)** | 22 | `color.cp`, `color_generated.cp`, `length.cp`, `calc.cp`, `angle.cp`, `time.cp`, `percentage.cp`, `gradient.cp`, `easing.cp`, `image.cp`, `position.cp`, `ratio.cp`, `resolution.cp`, `url.cp` 等 | `/data/dev/bun/src/css/values/*.cp` | 颜色空间、长度/角度/时间/分辨率/比例、数学表达式与渐变图像值 |

---

## 二、 核心对外 API 详解

### 1. CSS Color Level 4 (`CssColor`)

支持 `#rgb` / `#rgba` / `#rrggbb` / `#rrggbbaa`、传统逗号 `rgb()`/`rgba()`/`hsl()`/`hsla()`、现代空格斜杠语法 `rgb(255 128 0 / 50%)`、感知均匀色域 `oklch(62.8% 0.258 29.23deg)` 及标准命名色（`rebeccapurple`、`cornflowerblue`、`transparent` 等）。

| API 签名 | 说明 |
| :--- | :--- |
| `CssColor.parse(input): CssColor?` | 解析任意 Hex / RGB(A) / HSL(A) / OKLCH / 命名色字符串 |
| `CssColor.rgb(r, g, b)` / `CssColor.rgba(r, g, b, a)` | 直接构造 sRGB 颜色对象 |
| `CssColor.fromHsl(h_deg, s_pct, l_pct, a): CssColor` | HSL 色域转 sRGB |
| `CssColor.fromOklch(l, c, h_deg, a): CssColor` | OKLCH 经 OKLab 与线性 RGB 矩阵转换至 sRGB，同时保留 `oklch_l`/`c`/`h` 元数据 |
| `color.toHex(out_buf): []const u8` | 格式化为 `#rrggbb`（不透明）或 `#rrggbbaa`（半透明） |
| `color.toRgbString(out_buf): []const u8` | 格式化为 `rgb(r, g, b)` 或 `rgba(r, g, b, a)` |
| `color.luminance(): f64` | 计算 WCAG 相对亮度（`[0.0, 1.0]`） |
| `color.contrastRatio(other): f64` | 计算 WCAG 无障碍对比度（`[1.0, 21.0]`，黑白为 `21.0`） |
| `color.mix(other, weight): CssColor` | sRGB 空间线性混色（`weight` ∈ `[0.0, 1.0]`） |
| `color.eql(other): bool` | RGBA 四通道精确比较 |

### 2. CSS Values, Units & `calc()` (`CssUnit`, `CssLength`, `CssCalc`)

- `CssUnit` 支持 18 种单位：`Px`、`Rem`、`Em`、`Vw`、`Vh`、`Vmin`、`Vmax`、`Percent`、`Pt`、`Cm`、`Mm`、`In`、`Deg`、`Rad`、`Turn`、`Sec`、`Ms`、`Unitless`。

| API 签名 | 说明 |
| :--- | :--- |
| `CssUnit.fromSuffix(sfx): CssUnit?` / `unit.suffix()` | 单位后缀双向解析与格式化（大小写不敏感） |
| `CssLength.parse(raw): CssLength?` | 解析带单位或无单位数值（`0` 自动归一化为 `0px`） |
| `len.toPx(root_font_px, viewport_w, viewport_h): f64` | 将绝对/相对/视口长度单位统一换算为标准像素 `px` |
| `len.toMs(): f64` | 将时间单位（`s`/`ms`）换算为毫秒 |
| `len.toDeg(): f64` | 将角度单位（`deg`/`rad`/`turn`）换算为角度制 |
| `len.format(out_buf): []const u8` | 零堆分配格式化输出（整数省略小数，浮点保留 2 位） |
| `CssCalc.evalPx(expr, root_font_px, vw, vh): f64?` | 两遍扫描求值 `calc(...)`，严格遵循 `*`/`/` 高于 `+`/`-` 优先级并拦截除零错误 |

### 3. CSS Selectors Level 4 & Specificity (`CssSelector`, `CssSpecificity`)

| API 签名 | 说明 |
| :--- | :--- |
| `CssSelector.parse(sel): CssSelector` | 解析复合/复杂选择器（支持 ` `、`>`、`+`、`~`、标签、`*`、`#id`、`.class`、`[attr=val]`、伪类、`:where()` 零权重特例及 `::pseudo-element`） |
| `spec.score(): u32` | 返回打包权重 `(ids << 16) \| (classes << 8) \| elements` |
| `spec.compare(other): i32` | 按 W3C 三元组字典序比较优先级（`-1` / `0` / `1`） |
| `selector.matchesTag(tag): bool` | 大小写不敏感标签匹配（空标签或 `*` 匹配全部） |
| `selector.matchesElement(tag, id, class_list): bool` | 综合校验元素标签名、ID 与空格分隔的类名列表 |

### 4. Rules, `@media`, `@keyframes`, Stylesheet Parser & Minifier

| API 签名 | 说明 |
| :--- | :--- |
| `CssProperty.parse(decl): CssProperty?` | 解析单条声明，识别 `!important` 与 `--custom-prop` |
| `prop.isCustomProperty(): bool` / `prop.format(buf, minify)` | 判断 CSS 变量并按压缩或美化模式序列化 |
| `CssMediaQuery.parse(query): CssMediaQuery` | 解析 `screen`/`print` 媒体类型、`min-width`/`max-width` 及 `prefers-color-scheme: dark` |
| `media.matches(viewport_w, is_dark): bool` | 根据当前视口宽度与深色模式状态求值媒体查询 |
| `CssKeyframes.parse(rule_text): CssKeyframes?` | 解析 `@keyframes` 动画名与 `from`/`50%`/`to` 关键帧声明集合 |
| `CssStylesheetParser.parseRule(rule_str): CssRule?` | 解析单条规则（Style、`@import`、`@media`、`@keyframes`、`@supports`、`@layer`、`@container`、`@font-face`） |
| `CssStylesheetParser.parseStylesheet(css_text, out_rules): usize` | 解析整份样式表（自动跳过 `/* ... */` 注释），返回解析规则数 |
| `rule.getProperty(name): ([]const u8)?` / `rule.formatRule(buf, minify)` | 按名称检索声明值或序列化整条规则 |
| `CssStylesheetParser.minify(css_text, out_buf): []const u8` | 单遍流式移除注释、折叠空白、消除标点冗余空格与 `}` 前末尾分号 |

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import {
    CssColor, CssUnit, CssLength, CssCalc,
    CssSpecificity, CssSelector,
    CssProperty, CssRuleKind, CssMediaQuery, CssKeyframes, CssRule, CssStylesheetParser,
} from "cap:css";

pub fn demoCssEngine() {
    // 1. 颜色解析、OKLCH 转换、混色与 WCAG 对比度
    const brand = CssColor.parse("oklch(62.8% 0.258 29.23deg)").?;
    const surface = CssColor.parse("#ffffff").?;
    const mixed = brand.mix(surface, 0.5);
    const contrast = surface.contrastRatio(CssColor.parse("#111827").?);
    let hex_buf: [16]u8 = [0; 16];
    const mixed_hex = mixed.toHex(hex_buf[0..16]);
    _ = contrast;
    _ = mixed_hex;

    // 2. 单位转换与 calc() 求值
    const width_px = CssCalc.evalPx("calc(100vw - 2 * 1.5rem)", 16.0, 1440.0, 900.0).?; // 1392.0
    const anim_len = CssLength.parse("0.35s").?;
    const anim_ms: f64 = anim_len.toMs(); // 350.0
    _ = width_px;
    _ = anim_ms;

    // 3. 选择器权重与元素匹配
    const sel = CssSelector.parse("nav#top > a.btn[data-state=\"active\"]:hover::after");
    const matched = sel.matchesElement("a", "", "btn primary"); // true
    const score = sel.specificity.score();
    _ = matched;
    _ = score;

    // 4. 样式表解析、媒体查询求值与压缩输出
    const css = "/* theme */ @media screen and (min-width: 768px) and (prefers-color-scheme: dark) { .hero { color: #fff; } } .hero { padding: 1rem !important; }";
    let rules: [8]CssRule = std.mem.zeroes([8]CssRule);
    const n = CssStylesheetParser.parseStylesheet(css, rules[0..8]);
    if (n >= 1 && rules[0].kind == CssRuleKind.Media) {
        const is_dark_desktop = rules[0].media.matches(1280.0, true); // true
        _ = is_dark_desktop;
    }

    let min_buf: [512]u8 = [0; 512];
    const minified = CssStylesheetParser.minify(css, min_buf[0..512]);
    _ = minified;
}
```

---

## 四、 与 LightningCSS / Bun CSS 对照表

| 功能分类 | LightningCSS (Rust) | Bun CSS (`src/css/`) | CP 标准库 (`cap:css`) |
| :--- | :--- | :--- | :--- |
| **CSS Color 4 / OKLCH** | `values::color::CssColor` | `src/css/values/color.cp` | `CssColor.parse` / `fromHsl` / `fromOklch` / `mix` / `contrastRatio` |
| **单位解析与像素换算** | `values::length::LengthValue` | `src/css/values/{length,angle,time}.cp` | `CssUnit` / `CssLength.parse` / `toPx` / `toMs` / `toDeg` |
| **`calc()` 数学求值** | `values::calc::Calc` | `src/css/values/calc.cp` | `CssCalc.evalPx(expr, root_px, vw, vh)` |
| **选择器解析与权重** | `selector::SelectorList` / `Specificity` | `src/css/selectors/{parser,selector}.cp` | `CssSelector.parse` / `CssSpecificity` / `matchesElement` |
| **声明与自定义属性** | `declaration::DeclarationBlock` | `src/css/declaration.cp` | `CssProperty.parse` / `isCustomProperty` / `format` |
| **`@media` 查询求值** | `media_query::MediaList` | `src/css/media_query.cp` | `CssMediaQuery.parse` / `matches(viewport_w, is_dark)` |
| **`@keyframes` 关键帧** | `rules::keyframes::KeyframesRule` | `src/css/rules/keyframes.cp` | `CssKeyframes.parse` / `CssKeyframeStep` |
| **样式表解析与压缩** | `StyleSheet::parse` / `to_css(minify: true)` | `src/css/css_parser.cp` / `printer.cp` | `CssStylesheetParser.parseStylesheet` / `formatRule` / `minify` |
