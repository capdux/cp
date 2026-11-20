# CP 标准库：RFC 6901 JSON Pointer & RFC 6902 JSON Patch 原子引擎手册 (`cap:jsonpatch`)

## 一、 架构总览与设计准则

`cap:jsonpatch` 是 CP 语言官方内置的纯 CP、零 C/POSIX 依赖的 **RFC 6901 JSON Pointer & RFC 6902 JSON Patch 原子事务引擎**（基于 `std.json` 构建）。所有修改操作均在隔离的深拷贝 Arena 树上执行，任何一步失败立即回滚释放，原输入文档完全不受影响。既支持 `import { ... } from "cap:jsonpatch"` 静态导入，也支持 `import cap from "cap"; cap.jsonpatch.*` 动态懒挂载调用。

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/jsonpatch/jsonpatch.cp` | `// Source: cp/lib/jsonpatch/jsonpatch.cp (RFC 6901 JSON Pointer & RFC 6902 JSON Patch Atomic Engine)` | `JsonPatchError`、`PatchOpKind`、RFC 6901 Pointer 解析/转义、RFC 6902 原子事务应用与 `createDiffPatch` |
| `lib/jsonpatch/index.cp` | `// Source: cp/lib/jsonpatch/index.cp (Unified Exports for cap:jsonpatch)` | `cap:jsonpatch` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集合与操作枚举

- **`JsonPatchError`**：
  - `InvalidJson`：输入文档或补丁 JSON 语法非法。
  - `InvalidPointer`：JSON Pointer 非空且不以 `/` 开头、包含非法转义（非 `~0`/`~1`）或数组索引存在前导零（如 `"01"`）。
  - `InvalidPatch`：补丁文档非 JSON 数组、操作对象缺少必填字段或 `op` 未知。
  - `PathNotFound`：目标对象键或中间路径不存在。
  - `IndexOutOfBounds`：数组索引越界或在读取/替换/删除中误用 `"-"`。
  - `TestFailed`：`test` 操作深度结构比对失败，触发原子回滚。
  - `CycleMove`：`move` 操作尝试将父路径移动到自身子路径下。
  - `OutOfMemory`：底层内存分配失败。
- **`PatchOpKind`**：`Add` / `Remove` / `Replace` / `Move` / `Copy` / `Test`（支持 `.fromStr()` 与 `.toStr()`）。

### 2. RFC 6901 JSON Pointer

- **`escapeSegment(allocator: std.mem.Allocator, segment: []const u8): ([]u8) ! JsonPatchError`**：
  将单段路径中的 `~` 转义为 `~0`，`/` 转义为 `~1`。
- **`unescapeSegment(allocator: std.mem.Allocator, segment: []const u8): ([]u8) ! JsonPatchError`**：
  将 `~1` 还原为 `/`，`~0` 还原为 `~`，遇到孤立 `~` 或非法转义返回 `JsonPatchError.InvalidPointer`。
- **`resolvePointer(doc_val: std.json.Value, pointer: []const u8): std.json.Value ! JsonPatchError`**：
  零堆分配解析 JSON Pointer。支持根指针 `""`、转义键、严格数组索引（拒绝前导零 `"01"`）。

### 3. RFC 6902 原子补丁应用与 Diff 生成

- **`applyPatch(allocator: std.mem.Allocator, doc_json: []const u8, patch_json: []const u8): ([]u8) ! JsonPatchError`**：
  在独立深拷贝 Arena 树上依次执行 `add`（支持数组末尾 `"-"` 追加）、`remove`、`replace`、`move`（拦截 `CycleMove`）、`copy`、`test`；任一步失败即自动回滚释放，成功返回序列化后的 JSON 字符串。
- **`createDiffPatch(allocator: std.mem.Allocator, from_json: []const u8, to_json: []const u8): ([]u8) ! JsonPatchError`**：
  递归对比两份 JSON 文档，生成最小 RFC 6902 Patch JSON 数组；将该补丁应用回 `from_json` 必得与 `to_json` 等价的文档。
- **`jsonValuesEqual(a: std.json.Value, b: std.json.Value): bool`**：
  按 RFC 6902 语义递归比较两个 `std.json.Value` 是否深度结构相等。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { applyPatch, createDiffPatch, resolvePointer } from "cap:jsonpatch";
import cap from "cap";

pub fn demoJsonPatch(): ! {
    const alloc = std.heap.page_allocator;
    const doc = "{\"a/b\":1,\"c~d\":[10,20]}";
    const patch = "[{\"op\":\"add\",\"path\":\"/c~0d/-\",\"value\":30},{\"op\":\"replace\",\"path\":\"/a~1b\",\"value\":99}]";

    const updated = applyPatch(alloc, doc, patch)?;
    defer alloc.free(updated);

    const diff = cap.jsonpatch.createDiffPatch(alloc, doc, updated)?;
    defer alloc.free(diff);

    const parsed = std.json.parseFromSlice(std.json.Value, alloc, updated, .{})?;
    defer parsed.deinit();
    const val = resolvePointer(parsed.value, "/a~1b")?;
    * = val;
}
```
