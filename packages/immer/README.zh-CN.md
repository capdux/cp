[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# immer

纯 CP 实现的零堆不可变状态更新库，提供 `produce`、`produceWithPatches`、`applyPatches`、RFC 6902 结构化补丁与逆向补丁生成，以及 RFC 6901 JSON Pointer 编解码。

- **源包对齐**：参考 npm [`immer`](https://github.com/immerjs/immer) `10.1.1`（`MIT`）。
- **包版本**：`10.1.1`

## 安装与导入

```bash
cap add immer
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "immer": "^10.1.1"
  }
}
```

```typescript
import {
    ImmerState, Draft, ProduceResult, StatePatch, PatchOp,
    produce, produceWithPatches, applyPatches, freeze, isFrozen,
    invertPatch, inversePatches, formatJsonPointer, parseJsonPointer,
    unescapePointerToken, formatPatchJson, patchOpName,
    IMMER_MAX_ENTRIES, IMMER_MAX_PATCHES
} from "immer";
```

## 核心 API 参考

### 常量、枚举与数据结构

| 符号 | 定义 | 说明 |
| :--- | :--- | :--- |
| `IMMER_MAX_ENTRIES` / `IMMER_MAX_PATCHES` | `64` / `64` | 状态条目与补丁记录的固定栈上容量上限 |
| `PatchOp` | `enum { Add, Replace, Remove }` | RFC 6902 补丁操作类型 |
| `StatePatch` | `class { op: PatchOp, path: string, value: string, prev_value: string }` | 结构化补丁，保留 `prev_value` 以支持逆向回滚 |
| `StateEntry` | `class { key: string, value: string }` | `ImmerState` 中的键值条目 |
| `ProduceResult` | `class { next_state: ImmerState, patches: [64]StatePatch, patch_count: usize, inverse_patches: [64]StatePatch, inverse_count: usize }` | `produceWithPatches` 的完整返回结果 |

### `class ImmerState` 与 `class Draft`

| 类型 | 方法 | 说明 |
| :--- | :--- | :--- |
| `ImmerState` | `new()`, `count(): usize`, `get(key: string): string?`, `has(key: string): bool`, `setUnfrozen(*_, key, value): bool`, `removeUnfrozen(*_, key): bool` | 支持冻结保护的键值状态容器（最多 64 项） |
| `Draft` | `fromState(base: ImmerState): Draft`, `get(key): string?`, `has(key): bool`, `set(*_, key, value): bool`, `remove(*_, key): bool`, `finish(): ImmerState` | 可变草稿副本，每次 `set`/`remove` 自动记录 `StatePatch` |

### 核心函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `produce` | `produce(base: ImmerState, mutator: (&mut Draft) => void): ImmerState` | 基于 `base` 产生新的冻结 `ImmerState`，不修改原状态 |
| `produceWithPatches` | `produceWithPatches(base: ImmerState, mutator: (&mut Draft) => void): ProduceResult` | 产生新状态，并同时返回正向补丁与逆向补丁 |
| `applyPatches` | `applyPatches(base: ImmerState, patches: &[]StatePatch): ImmerState` | 将补丁切片应用到 `base` 并返回新状态 |
| `freeze` / `isFrozen` | `freeze(state: &mut ImmerState)` / `isFrozen(state: ImmerState): bool` | 冻结状态或查询冻结标记 |
| `invertPatch` | `invertPatch(p: StatePatch): StatePatch` | 对单条补丁求逆（`Add <-> Remove`，`Replace` 交换新旧值） |
| `inversePatches` | `inversePatches(patches: &[]StatePatch, out: []StatePatch): usize` | 将补丁切片逆序求逆并写入 `out` |
| `patchOpName` | `patchOpName(op: PatchOp): string` | 返回 `"add"`、`"replace"` 或 `"remove"` |
| `formatJsonPointer` | `formatJsonPointer(segments: &[]string, out: []u8): usize` | 按 RFC 6901 编码路径段（`~` -> `~0`，`/` -> `~1`） |
| `parseJsonPointer` | `parseJsonPointer(ptr: string, out: []string): usize` | 按 RFC 6901 解析 JSON Pointer 为原始段切片 |
| `unescapePointerToken` | `unescapePointerToken(token: string, out: []u8): usize` | 反转义单个 RFC 6901 路径段（`~1` -> `/`，`~0` -> `~`） |
| `formatPatchJson` | `formatPatchJson(p: StatePatch, out: []u8): usize` | 将 `StatePatch` 序列化为 RFC 6902 JSON 字符串写入 `out` |

## 使用示例

### 1. 使用 `produceWithPatches` 实现不可变更新与撤销

```typescript
import { ImmerState, Draft, produceWithPatches, applyPatches } from "immer";

fn demoUndo(): bool {
    let mut base = ImmerState.new();
    base.setUnfrozen("theme", "light");
    base.setUnfrozen("lang", "en");

    let res = produceWithPatches(base, (draft: &mut Draft) => {
        draft.set("theme", "dark");
        draft.set("font", "mono");
    });

    // 应用逆向补丁撤销变更
    let restored = applyPatches(res.next_state, res.inverse_patches[0..res.inverse_count]);
    let theme = restored.get("theme") orelse "";
    return theme == "light" && !restored.has("font");
}
```

### 2. RFC 6901 JSON Pointer 编码与补丁序列化

```typescript
import { formatJsonPointer, formatPatchJson, StatePatch, PatchOp } from "immer";

fn encodePointerAndPatch(): usize {
    let segs = ["users", "a/b", "profile~1"];
    let mut ptr_buf: [64]u8 = undefined;
    let ptr_len = formatJsonPointer(&segs, &mut ptr_buf);
    // "/users/a~1b/profile~01"

    let p = StatePatch { op: PatchOp.Replace, path: "/theme", value: "dark", prev_value: "light" };
    let mut json_buf: [128]u8 = undefined;
    let json_len = formatPatchJson(p, &mut json_buf);
    return ptr_len + json_len;
}
```

## 与源包差异与 CP 安全亮点

- **零堆栈上结构**：`ImmerState` 与 `Draft` 固定内联存储最多 64 项键值与 64 条补丁，无需 JS `Proxy` 与堆分配。
- **借用安全草稿修改**：`mutator: (&mut Draft) => void` 在编译期保证草稿独占可变借用，原 `base` 状态不受影响。
- **内置 RFC 6901/6902 零拷贝编解码**：全部格式化函数直接写入调用方 `[]u8` 缓冲区并返回写入字节数。
