<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:flatted` ([https://github.com/WebReflection/flatted](https://github.com/WebReflection/flatted)) | 许可：`ISC`

# `@cap/flatted`

零堆分配纯 CP 循环引用安全扁平化 JSON 编码协议，完整对标 npm **#208** `flatted`（真实周下载 **2.10 亿/周**，ESLint / WebReflection 核心依赖）。

## 核心 API

- `FlatNodeKind`：`Null`、`Bool`、`Int`、`Str`、`Array`、`Object`
- `FlatArena`：定容 64 槽节点图，支持循环边 `.setField(*_, obj_id, key, child_id)`、`.pushElem(*_, arr_id, child_id)` 及节点查询
- `stringifyFlatted(arena: FlatArena, root_id: usize, out: []u8): usize`：BFS 编号展开为标准 `flatted` 扁平 JSON 数组，循环引用与字符串自动按槽位索引字符串 `"1"` 去重
- `parseFlatted(json: string, out_arena: *FlatArena): i64`：解析 `flatted` JSON 数组并还原对象图与循环引用，失败返回 `-1`
