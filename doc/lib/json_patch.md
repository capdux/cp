# cap:json_patch (RFC 6902 / RFC 7396 / RFC 6901)

## 1. 概述

`cap:json_patch`（同时支持 `import cap from "cap"` 下的 `cap.json_patch` 动态挂载）提供纯 CP 原生实现的工业级 JSON 变更协议套件：
- **RFC 6901 JSON Pointer**：支持 `~0` / `~1` 转义、数组索引校验与 `-` 追加语义。
- **RFC 6902 JSON Patch**：完整支持 `add`、`remove`、`replace`、`move`、`copy`、`test` 六大原子操作，支持任意步骤失败时自动回滚（不污染源文档，零内存泄漏）。
- **RFC 7396 JSON Merge Patch**：支持对象递归合并、`null` 键删除与非对象覆盖。
- **结构化 Diff**：自动计算两个 JSON 文档间的最小补丁序列并支持往返一致性验证。

## 2. 核心类型

- `PatchOpKind`: 枚举 `{ Add, Remove, Replace, Move, Copy, Test }`。
- `JsonPatchOp`: 补丁操作结构体 `{ op: PatchOpKind, path: []const u8, from: []const u8? = null, value: std.json.Value? = null }`。
- `JsonPatchError`: 错误集合 `{ InvalidPointer, PointerNotFound, IndexOutOfBounds, InvalidArrayIndex, MissingFrom, MissingValue, TestFailed, CannotMoveToChild, InvalidPatch, InvalidJson, TypeMismatch }`。

## 3. API 速查

| 函数 | 说明 |
| :--- | :--- |
| `resolvePointer(root, pointer)` | 按 RFC 6901 定位 `std.json.Value` 节点 |
| `applyPatch(alloc, doc, ops)` | 对文档应用 RFC 6902 补丁序列，返回深拷贝后的新文档（失败原子回滚） |
| `applyPatchJson(alloc, doc_json, patch_json)` | 直接解析 JSON 文本并应用 RFC 6902 Patch，返回序列化 JSON 字符串 |
| `mergePatch(alloc, target, patch)` | 执行 RFC 7396 Merge Patch 递归合并（`null` 删除键） |
| `diff(alloc, source, target)` | 计算 `source -> target` 的 RFC 6902 补丁序列 |
| `freeOps(alloc, ops)` | 释放 `diff` 生成的补丁切片及其内部资源 |
| `cloneValue(alloc, val)` / `freeValue(alloc, val)` | 递归深拷贝与零泄漏释放 `std.json.Value` |
| `valuesEqual(a, b)` / `stringifyValue(alloc, val)` | JSON 深度相等比较与 RFC 8259 序列化 |

## 4. 示例

```cp
import { applyPatchJson, mergePatch } from "cap:json_patch";
import cap from "cap";

const patched = try applyPatchJson(
    alloc,
    "{\"a\":\"foo\"}",
    "[{\"op\":\"replace\",\"path\":\"/a\",\"value\":\"bar\"}]"
);
defer alloc.free(patched);

// 动态挂载调用
const val = try cap.json_patch.resolvePointer(doc, "/a");
```
