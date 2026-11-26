<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:type-check` ([https://github.com/gkz/type-check](https://github.com/gkz/type-check)) | 许可：`MIT`

# `@cap/type-check`

零堆分配纯 CP 运行时类型表达式解析与校验器，对标 npm **#241** `type-check`（真实周下载 **1.95 亿/周**，ESLint 核心依赖），支持基础类型、联合类型 `A | B`、`Maybe T`、强类型数组 `[T]`、通配符 `*` 与嵌套括号表达式。

## 核心 API

- `typeCheck(type_expr: string, val: DynValue): bool`
- `isValidTypeExpr(type_expr: string): bool`
- `DynValue.of(kind)` / `DynValue.arrayOf(elem_kind, len)`
