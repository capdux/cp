# `cap:type_traits` — 编译期类型特征自省与指针元数据工具

源自 Bun `meta/traits.cp` 与 `ptr/meta.cp`，提供零运行期开销的编译期类型谓词（字符串/切片/单元素指针/数值/容器/extern/packed/可索引）及 `PointerMeta.inspect` 指针规范解析器。

## 导出符号

- 类型谓词：`isByteString`、`isSlice`、`isSingleItemPtr`、`isConstPtr`、`isOptional`、`isInteger`、`isFloat`、`isNumber`、`isStruct`、`isEnum`、`isUnion`、`isContainer`、`isExternContainer`、`isPackedContainer`、`isIndexable`
- 指针元信息：`PointerKind`（`single` / `slice`）、`PointerParseOptions`、`PointerMeta`（`inspect`）
