# @cap/kind-of

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:kind-of` ([https://github.com/jonschlinkert/kind-of](https://github.com/jonschlinkert/kind-of))

零堆分配的 CP 原生值类型识别库，Clean-Room MIT 实现对标 npm 顶流包 `kind-of`（1.2 亿+/周下载，同时覆盖 `isobject` / `is-plain-object` / `is-primitive` 语义）。

## 特性

- `kindOf(raw)` / `kindOfName(raw)`：识别 `null`、`undefined`、`boolean`、`number`（含科学计数法/`NaN`/`Infinity`）、`bigint`、`string`、`date`（ISO-8601）、`array`、`object`、`regexp`、`function`
- `isPrimitive(raw)` 与 `isPlainObject(raw)` 便捷谓词
- 零堆分配、单遍扫描

## 致谢

Clean-Room MIT 实现，API 语义对标开源生态 [`jonschlinkert/kind-of`](https://github.com/jonschlinkert/kind-of)。
