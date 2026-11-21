# `cap:css_supports`

纯 CP 零堆分配 CSS `@supports` 条件表达式解析、求值与规范化序列化库，吸收自 `/data/dev/bun/src/css/rules/supports.cp`。

## 导出类型

- `SupportsKind`：`declaration | selector | unknown | not_cond | and_cond | or_cond`
- `SupportsNode`：条件树节点
- `SupportsTree`：固定容量 Arena 条件树
  - `SupportsTree.parse(input)`：解析 `not (...)`、`(...) and (...)`、`(...) or (...)`、`(prop: val)`、`selector(...)`、嵌套括号与未知条件（严格拦截同级混用 `and`/`or`）
  - `toCss(out)`：规范化序列化（自动补全/省略必要括号）
  - `eval(decl_fn, sel_fn)`：按自定义声明与选择器支持谓词求值
