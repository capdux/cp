[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# estree-walker

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:estree-walker` ([https://github.com/Rich-Harris/estree-walker](https://github.com/Rich-Harris/estree-walker))

Zero-allocation index-pooled ESTree AST arena and non-recursive DFS walker for CP — clean-room MIT parity with npm `estree-walker` (305M+/wk).

纯 CP 零堆分配索引池 ESTree AST 容器与非递归深度优先遍历器，支持 `Enter`/`Leave` 双阶段事件流、`.skip()` 子树剪枝，以及 `removeNode`/`replaceNode` 原地树变换。

## Install / 安装

```bash
cap add estree-walker
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `EstreeNodeKind` | `enum` | `Program`, `Identifier`, `Literal`, `BinaryExpression`, `CallExpression`, `FunctionDeclaration`, `BlockStatement`, `ReturnStatement`, `VariableDeclaration` |
| `EstreeNode` | `struct` | Pooled AST node (`kind`, `name`, `first_child`, `next_sibling`, `parent`, `removed`) |
| `EstreeArena` | `struct` | 64-node arena: `init()`, `addNode()`, `appendChild()`, `removeNode()`, `replaceNode()`, `getNode()` |
| `EstreeWalker` | `struct` | Explicit-stack DFS iterator: `init(root_id)`, `nextEvent(*_, arena): WalkEvent?`, `skip(*_)` |

## Usage / 使用示例

```cp
import { EstreeArena, EstreeWalker } from "estree-walker";

let arena = EstreeArena.init();
const root = arena.addNode(.Program, "root");
const id = arena.addNode(.Identifier, "x");
* = arena.appendChild(root, id);

let walker = EstreeWalker.init(root);
while (ev := walker.nextEvent(arena)) {
    // handle Enter / Leave events
}
```
