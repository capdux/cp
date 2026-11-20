# CP 标准库：RFC 6570 URI Template 引擎手册 (`cap:uritemplate`)

## 一、 架构总览与设计准则

`cap:uritemplate` 是 CP 语言官方内置的纯 CP **RFC 6570 Level 1~4 URI Template 解析、标准展开（Expand）与变量清单提取引擎**，**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，专为 REST API 客户端、OpenAPI/MCP 动态端点构造与路由参数校验提供安全、零泄漏的原语。既可通过 `import { ... } from "cap:uritemplate"` 导入，也支持 `import cap from "cap"; cap.uritemplate.*` 动态懒挂载调用。

### 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/uritemplate/template.cp` | `// Source: cp/lib/uritemplate/template.cp (...)` | `UriTemplateError`、`TemplateOp`、`AssocPair`、`TemplateVars`、`UriTemplate` |
| `lib/uritemplate/index.cp` | `// Source: cp/lib/uritemplate/index.cp (...)` | `cap:uritemplate` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与操作符枚举

- **`UriTemplateError`**：
  - `UnclosedExpression`：表达式缺少闭合 `}` 或出现嵌套 `{`。
  - `UnexpectedCloseBrace`：模板中出现孤立未配对的 `}`。
  - `InvalidOperator`：表达式使用了 RFC 6570 保留的非法操作符（`=`、`,`、`!`、`@`、`|`）。
  - `InvalidVarName`：空表达式 `{}`、空变量名或包含非法字符/连续点号 `..` 的变量名。
  - `InvalidModifier`：非法的修饰符（如非数字 `:abc`、`:0`、超过 4 位数字或组合 `:3*`）。
  - `OutOfMemory`：底层缓冲区或切片分配失败。
- **`TemplateOp`**：对应 RFC 6570 全部 8 种操作符：
  - `Simple`（无前缀）、`Reserved`（`+`）、`Fragment`（`#`）、`Label`（`.`）、`Path`（`/`）、`PathParam`（`;`）、`Query`（`?`）、`QueryCont`（`&`）。

### 2. `TemplateVars`（变量值绑定容器）

- **`TemplateVars.init(alloc: std.mem.Allocator): TemplateVars`**：初始化变量绑定容器。
- **`deinit(*_)`**：确定性释放全部内部深拷贝的键名与变量值内存。
- **`set(*_, name: []const u8, value: []const u8): void ! UriTemplateError`**：绑定单值字符串（同名覆盖）。
- **`setList(*_, name: []const u8, items: []const []const u8): void ! UriTemplateError`**：绑定字符串列表（用于 `{list}` 与 `{list*}`）。
- **`setAssoc(*_, name: []const u8, pairs: []const AssocPair): void ! UriTemplateError`**：绑定键值关联表（用于 `{keys}` 与 `{keys*}`）。

### 3. `UriTemplate`（解析、变量提取与展开）

- **`UriTemplate.parse(alloc: std.mem.Allocator, pattern: []const u8): UriTemplate ! UriTemplateError`**：解析并严格校验 RFC 6570 Level 1~4 模板。
- **`deinit(*_)`**：释放模板 AST 内存。
- **`variables(alloc: std.mem.Allocator): [][]u8 ! UriTemplateError`**：按首次出现顺序返回模板声明的去重变量名清单（调用方需逐项释放元素并释放外层切片）。
- **`expand(alloc: std.mem.Allocator, vars: *TemplateVars): []u8 ! UriTemplateError`**：严格遵循 RFC 6570 Level 1~4 规范展开模板：
  - 精确百分号编码：`Simple`/`.`/`/`/`;`/`?`/`&` 仅保留 unreserved 字符（`ALPHA / DIGIT / "-" / "." / "_" / "~"`）；`+`/`#` 保留 reserved 字符及已有合法 `%XX` 编码不重复转义；
  - 支持 `{var:N}` UTF-8 字符安全前缀截取（先截断再编码）；
  - 支持 `{list*}` / `{keys*}` 爆炸展开修饰符；
  - 未定义变量或空列表/空关联表自动跳过，不产生多余分隔符。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { UriTemplate, TemplateVars, AssocPair } from "cap:uritemplate";
import cap from "cap";

pub fn demoUriTemplate(): ! {
    const alloc = std.heap.page_allocator;

    let tmpl = UriTemplate.parse(alloc, "https://api.example.com{/version,resource}{?q,tags*}")?;
    defer tmpl.deinit();

    let vars = TemplateVars.init(alloc);
    defer vars.deinit();
    vars.set("version", "v2")?;
    vars.set("resource", "users")?;
    vars.set("q", "hello world")?;
    vars.setList("tags", ["ai", "cp"])?;

    const url = tmpl.expand(alloc, &vars)?;
    defer alloc.free(url);
    // url == "https://api.example.com/v2/users?q=hello%20world&tags=ai&tags=cp"

    // 亦支持 cap.uritemplate 动态懒挂载
    let t2 = cap.uritemplate.UriTemplate.parse(alloc, "{+path}/here")?;
    defer t2.deinit();
}
```
