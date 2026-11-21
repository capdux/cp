# `cap:css_geometry` — CSS 几何与布局基础类型

> 溯源：从 `/data/dev/bun/src/css/values/ratio.cp`、`values/rect.cp` 与 `properties/overflow.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:css_geometry` 提供零分配、强类型的 CSS 几何与布局基础数据结构：

- **`Ratio`**：CSS `<ratio>` 宽高比（分子/分母），支持最简分数化简、退化比例与零分母安全防护、倒数与等价比较。
- **`Rect(T)`**：泛型四边盒模型矩形 `{ top, right, bottom, left }`，适配 `margin` / `padding` / `border-width` 等属性，支持 1/2/3 值简写构造、均一性判断与水平/垂直边和计算。
- **`OverflowKeyword` / `Overflow` / `TextOverflow`**：CSS Overflow Module Level 3 枚举与双轴结构体，支持关键词解析、内容裁剪判定及单/双值简写解析。

---

## 快速导入

```cp
import {
    Ratio,
    Rect,
    OverflowKeyword,
    Overflow,
    TextOverflow,
} from "cap:css_geometry";
```

---

## API 参考

### 1. `Ratio`

| 成员 / 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| 字段 | `numerator: f64`, `denominator: f64 = 1.0` | 宽高比分子与分母 |
| `new` | `static fn new(numerator: f64, denominator: f64): Ratio` | 构造指定分子分母的比例 |
| `fromSingle` | `static fn fromSingle(numerator: f64): Ratio` | 构造分母为 `1.0` 的单值比例 |
| `isDegenerate` | `fn isDegenerate(): bool` | 是否为退化比例（任一分量 `<= 0`、NaN 或 Inf） |
| `asF64` | `fn asF64(): f64?` | 计算浮点比值；零分母或非有限值返回 `null` |
| `simplify` | `fn simplify(): Ratio` | 通过 GCD 化简为最简比例；退化比例原样返回 |
| `invert` | `fn invert(): Ratio` | 返回倒数比例 |
| `eql` | `fn eql(other: Ratio): bool` | 字段级精确相等比较 |
| `equivalent` | `fn equivalent(other: Ratio): bool` | 数学比值等价比较（退化比例返回 `false`） |

### 2. `Rect(comptime T: type)`

| 成员 / 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| 字段 | `top: T`, `right: T`, `bottom: T`, `left: T` | 顺时针四边值 |
| `all` | `static fn all(v: T): Rect(T)` | 四边统一赋值为 `v` |
| `symmetric` | `static fn symmetric(vertical: T, horizontal: T): Rect(T)` | `top = bottom = vertical`, `left = right = horizontal` |
| `fromThree` | `static fn fromThree(top_v: T, horizontal: T, bottom_v: T): Rect(T)` | CSS 3 值简写构造 |
| `isUniform` | `fn isUniform(): bool` | 判断四边是否全部相等 |
| `horizontalSum` | `fn horizontalSum(): T` | 返回 `left + right` |
| `verticalSum` | `fn verticalSum(): T` | 返回 `top + bottom` |
| `eql` | `fn eql(other: Rect(T)): bool` | 四边逐项相等比较 |

### 3. `OverflowKeyword`、`Overflow` & `TextOverflow`

| 类型 / 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `OverflowKeyword` | `visible`, `hidden`, `clip`, `scroll`, `auto` | CSS 溢出关键词枚举 |
| `OverflowKeyword.fromStr` | `static fn fromStr(s: string): OverflowKeyword?` | 严格解析 5 个标准小写关键词，未知输入返回 `null` |
| `OverflowKeyword.asStr` | `fn asStr(): string` | 返回对应 CSS 关键词字符串 |
| `OverflowKeyword.clipsContent` | `fn clipsContent(): bool` | 是否裁剪溢出内容（仅 `visible` 为 `false`） |
| `OverflowKeyword.allowsScroll` | `fn allowsScroll(): bool` | 是否允许滚动（`hidden` / `scroll` / `auto`） |
| `Overflow` | `{ x: OverflowKeyword = .visible, y: OverflowKeyword = .visible }` | 双轴溢出配置 |
| `Overflow.all` / `new` | `static fn all(kw)` / `static fn new(x_kw, y_kw)` | 单轴统一或双轴分别构造 |
| `Overflow.parseShorthand` | `static fn parseShorthand(s: string): Overflow?` | 解析单值（`"hidden"`）或双值（`"hidden scroll"`）简写 |
| `Overflow.isUniform` / `clipsAny` / `eql` | `fn ...(): bool` | 均一性、裁剪判定与相等比较 |
| `TextOverflow` | `clip`, `ellipsis` + `fromStr` / `asStr` | CSS `text-overflow` 关键词枚举 |

---

## 使用示例

```cp
import { Ratio, Rect, Overflow, OverflowKeyword } from "cap:css_geometry";

fn computeContentBox() {
    const aspect = Ratio.new(1920.0, 1080.0).simplify(); // 16 / 9
    const pad = Rect(f64).symmetric(12.0, 24.0);
    const content_w = 1920.0 - pad.horizontalSum(); // 1872.0
    const content_h = 1080.0 - pad.verticalSum();   // 1056.0
    * = aspect;
    * = content_w;
    * = content_h;

    if (ov := Overflow.parseShorthand("hidden auto")) {
        const clipped = ov.clipsAny(); // true
        * = clipped;
    }
}
```
