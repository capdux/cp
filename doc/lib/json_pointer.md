# `cap:json_pointer` — RFC 6901 JSON Pointer 解析与路径段迭代库

> 来源：吸收自 `/data/dev/bun/src/json_parser.zig`（RFC 6901 JSON Pointer）

`cap:json_pointer` 提供纯 CP、零外部依赖、零堆分配的 RFC 6901 JSON Pointer 语法校验、数组索引解析、`~0`/`~1` 转义与反转义、流式路径段迭代及父子指针构造工具。

## 核心函数与类型

### 校验与索引解析

```cp
pub fn isValidPointer(ptr: []const u8): bool
pub fn parseArrayIndex(token: []const u8): usize?
```

- `isValidPointer`：空串 `""` 合法代表根文档；非空必须以 `/` 开头；任意 `~` 后必须紧跟 `0` 或 `1`。
- `parseArrayIndex`：按 RFC 6901 §4 仅接受 `"0"` 或 `[1-9][0-9]*`，拒绝前导零（如 `"01"`）、负数与整数溢出。

### 转义与反转义

```cp
pub fn escapeToken(raw: []const u8, out: []u8): usize?
pub fn unescapeToken(raw: []const u8, out: []u8): usize?
```

- `escapeToken`：`~` → `~0`，`/` → `~1`。
- `unescapeToken`：`~1` → `/`，`~0` → `~`；遇到非法 `~` 序列或输出缓冲不足返回 `null`。

### `PointerIter`

```cp
pub struct PointerIter {
    static fn init(ptr: []const u8): PointerIter?
    fn nextRaw(*_): ([]const u8)?
    fn nextUnescaped(*_, out: []u8): ([]const u8)?
    fn remaining(): []const u8
}
```

### 指针拼接与父指针

```cp
pub fn appendToken(base_ptr: []const u8, token: []const u8, out: []u8): usize?
pub fn parentPointer(ptr: []const u8): ([]const u8)?
```

- `appendToken`：在合法 `base_ptr` 后追加 `/` 与自动转义后的 `token`。
- `parentPointer`：根 `""` 或非法指针返回 `null`，`"/a"` 返回 `""`，`"/a/b"` 返回 `"/a"`。

## 使用示例

```cp
import { PointerIter, appendToken, parentPointer, parseArrayIndex } from "cap:json_pointer";

let it = PointerIter.init("/items/0/a~1b/m~0n").?;
let seg_buf: [64]u8 = [0; 64];
// 依次产出: "items", "0" (index 0), "a/b", "m~n"
```
