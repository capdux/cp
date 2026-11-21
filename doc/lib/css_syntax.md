# `cap:css_syntax` — CSS Properties and Values API `<syntax>` 解析与序列化引擎

> 溯源：从 `/data/dev/bun/src/css/values/syntax.cp` 吸收并规范化为纯 CP 标准库模块。

`cap:css_syntax` 实现 W3C CSS Properties and Values API Level 1 `<syntax>` 语法字符串解析与规范序列化：

- 支持通用语法 `*`（零堆分配）、全部 15 种标准数据类型（`<length>` ~ `<string>`）、自定义字面量标识符、重复乘数 `+`（空格分隔）与 `#`（逗号分隔），以及 `|` 多分支组合；
- 严格执行 `<transform-list>` 禁止附加乘数规则，拦截空分支、未闭合尖括号、未知类型名及尾随 `|` 等非法语法；
- `SyntaxString` 实现 `~init()` 确定性 RAII 析构，零内存泄漏。

---

## 快速导入

```cp
import {
    SyntaxString,
    SyntaxComponent,
    SyntaxType,
    Multiplier,
    Error,
} from "cap:css_syntax";
```

---

## API 参考

### 1. 枚举与错误集

| 符号 | 成员 | 说明 |
| :--- | :--- | :--- |
| `Error` | `EmptySyntax`, `UnclosedTypeName`, `UnknownTypeName`, `MultiplierNotAllowed`, `InvalidComponent`, `UnexpectedToken`, `BufferTooSmall` | 完整解析与序列化错误集 |
| `Multiplier` | `none`, `space` (`"+"`), `comma` (`"#"`) | 组件重复乘数，提供 `asStr()` |
| `SyntaxType` | `length`, `number`, `percentage`, `length_percentage`, `color`, `image`, `url`, `integer`, `angle`, `time`, `resolution`, `transform_function`, `transform_list`, `custom_ident`, `string_type`, `literal` | 提供 `fromTypeName()`、`asTypeStr()`、`allowsMultiplier()` |

### 2. `SyntaxComponent` & `SyntaxString`

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `SyntaxComponent.eql` / `toCss` | `fn eql(other): bool` / `fn toCss(buf: []u8): string!` | 单组件相等比较与序列化 |
| `SyntaxString.parse` | `static fn parse(allocator, input: string): SyntaxString!` | 解析 `<syntax>` 字符串 |
| `SyntaxString.isUniversal` | `fn isUniversal(): bool` | 是否为通用 `*` 语法 |
| `SyntaxString.len` / `items` | `fn len(): usize` / `fn items(): []const SyntaxComponent` | 分支组件数与切片视图 |
| `SyntaxString.toCss` | `fn toCss(buf: []u8): string!` | 规范序列化（如 `"<length>+ | <color> | auto"`） |
| `SyntaxString.toCssQuoted` | `fn toCssQuoted(buf: []u8): string!` | 带外层双引号序列化 |

---

## 使用示例

```cp
import { SyntaxString } from "cap:css_syntax";

fn checkPropertySyntax(alloc: std.mem.Allocator)! {
    const syn = SyntaxString.parse(alloc, " <length>+ | <color># | auto ")?;
    let buf: [64]u8 = [0; 64];
    const canonical = syn.toCss(buf[0..64])?; // "<length>+ | <color># | auto"
    * = canonical;
} // 离开作用域自动触发 syn.~init() 释放内存
```
