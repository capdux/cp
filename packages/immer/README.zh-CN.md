[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# immer

面向 CP 的零堆内存不可变状态生成器（`produce` / `produceWithPatches` / `applyPatches`），内置冻结守卫（`freeze` / `isFrozen`）、RFC 6902 状态补丁与自动逆向补丁生成，以及 RFC 6901 JSON Pointer 编解码器。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [npmjs.com/package/immer](https://www.npmjs.com/package/immer) |
| **上游原包名** | `immer` v10.1.1 |
| **上游仓库** | https://github.com/immerjs/immer |
| **许可证** | MIT |

从 TypeScript/npm 迁移到 CP 保持原包名不变：

```ts
// TypeScript (npm)
import { produce, produceWithPatches, applyPatches, freeze } from "immer";
```

```cp
// CP —— 原包名直接导入，零堆不可变状态演进与 RFC 6902 补丁
import {
    ImmerState, Draft, produce, produceWithPatches,
    applyPatches, freeze, isFrozen,
} from "immer";
```

## 安装

```bash
cap add immer
```

## 实战示例

### 1. 不可变更新（`produce`）与冻结保护

```cp
import { ImmerState, Draft, produce, freeze, isFrozen } from "immer";

fn demoProduce() {
    mut base = ImmerState.new();
    let _ = base.setUnfrozen("theme", "light");
    let _ = base.setUnfrozen("lang", "en");
    freeze(&mut base); // isFrozen(base) == true，之后 setUnfrozen/removeUnfrozen 返回 false

    // 基于 base 生成全新不可变状态，不修改原状态
    let next = produce(base, |draft: &mut Draft| {
        let _ = draft.set("theme", "dark");
        let _ = draft.set("sidebar", "open");
        let _ = draft.remove("lang");
    });

    // base 仍保持 theme == "light", lang == "en"
    // next 拥有 theme == "dark", sidebar == "open", has("lang") == false
    let cur = next.get("theme");
}
```

### 2. 时间旅行撤销/重做（`produceWithPatches` & `applyPatches`）

```cp
import {
    ImmerState, Draft, ProduceResult,
    produceWithPatches, applyPatches,
    formatPatchJson, inversePatches, invertPatch,
} from "immer";

fn demoPatches() {
    mut base = ImmerState.new();
    let _ = base.setUnfrozen("count", "1");

    let res: ProduceResult = produceWithPatches(base, |draft: &mut Draft| {
        let _ = draft.set("count", "2");
        let _ = draft.set("status", "active");
    });

    // 正向重放补丁，复现 res.next_state
    let replayed = applyPatches(base, &res.patches[0..res.patch_count]);

    // 逆向重放补丁，无损撤销回 base
    let undone = applyPatches(res.next_state, &res.inverse_patches[0..res.inverse_count]);

    // 将单条补丁序列化为 RFC 6902 JSON 写入栈缓冲区
    mut json_buf: [256]u8 = [0; 256];
    if (res.patch_count > 0) {
        let written = formatPatchJson(res.patches[0], &mut json_buf);
    }
}
```

### 3. RFC 6901 JSON Pointer 编解码

```cp
import { formatJsonPointer, parseJsonPointer, unescapePointerToken } from "immer";

fn demoJsonPointer() {
    let segs: [3]string = ["users", "a/b", "c~d"];
    mut ptr_buf: [128]u8 = [0; 128];
    let n = formatJsonPointer(&segs, &mut ptr_buf); // "/users/a~1b/c~0d"

    mut parsed: [8]string = [""; 8];
    let count = parseJsonPointer("/users/a~1b/c~0d", &mut parsed);

    mut unescaped: [32]u8 = [0; 32];
    let ulen = unescapePointerToken("a~1b", &mut unescaped); // "a/b"
}
```

## 性能与安全亮点

- **零堆内存分配**：`ImmerState`（`[64]StateEntry`）、`Draft`（`[64]StatePatch`）与 `ProduceResult` 全量栈上定长存储，零 GC、零堆碎片。
- **严格不可变守卫**：`produce` 与 `produceWithPatches` 自动冻结输出状态，防止后续误修改。
- **自动逆向补丁**：同步记录正向补丁与精确逆向补丁（`Add` <-> `Remove`、`Replace` 保留 `prev_value`），天然支持 Undo/Redo。
- **完整 RFC 6901 转义**：正确处理 `~0`（`~`）与 `~1`（`/`）指针转义。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
