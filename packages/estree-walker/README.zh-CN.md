[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# estree-walker

Zero-allocation index-pooled ESTree AST arena and non-recursive DFS walker with skip, remove, and replace for CP

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:estree-walker` ([https://github.com/Rich-Harris/estree-walker](https://github.com/Rich-Harris/estree-walker))

| 元信息 | 值 |
| --- | --- |
| 包名 | `estree-walker` |
| 版本 | `1.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add estree-walker
```

```cp
import {
    EstreeNodeKind,
    WalkPhase,
    WalkEvent,
    EstreeNode,
    EstwFrame,
    EstreeArena,
    EstreeWalker,
    estwValidId,
} from "estree-walker";
```

## 核心 API

### 类型、枚举与错误集

| 种类 | 名称 | 定义 |
| --- | --- | --- |
| `enum` | `EstreeNodeKind` | `Program, Identifier, Literal, BinaryExpression, CallExpression, FunctionDeclaration, BlockStatement, ReturnStatement ...` |
| `enum` | `WalkPhase` | `Enter, Leave` |

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `WalkEvent` | `class` | 字段: `phase: WalkPhase, node_id: i32, parent_id: i32, depth: usize` |
| `EstreeNode` | `class` | 字段: `kind: EstreeNodeKind, name: string, first_child: i32, next_sibling: i32, parent: i32, removed: bool` |
| `EstwFrame` | `class` | 字段: `node_id: i32, next_child: i32, entered: bool, skipped: bool` |
| `EstreeArena` | `class` | 字段: `count: usize, nodes: [64]EstreeNode` · 方法: `init, addNode, getNode, appendChild, removeNode, replaceNode` |
| `EstreeWalker` | `class` | 字段: `sp: usize, frames: [64]EstwFrame` · 方法: `init, skip, nextEvent` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `estwValidId` | `(id: i32, count: usize): bool` | 调用 `estwValidId` 并返回 `bool`。 |

## 最小调用示例

```cp
import { estwValidId } from "estree-walker";

pub fn main() {
    let result = estwValidId(0, 0);
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
