[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# unist-util-visit

Zero-heap unist AST tree pre-order/post-order visitor and node matcher (covers unist-util-visit-parents and unist-util-is)

| 元信息 | 值 |
| --- | --- |
| 包名 | `unist-util-visit` |
| 版本 | `5.0.0` |
| 许可证 | `MIT` |
| 类型声明 | [`src/index.d.cp`](./src/index.d.cp) |

## 安装与导入

```bash
cap add unist-util-visit
```

```cp
import { UnistNode, UnistTree, uuvStrEq, newUnistTree } from "unist-util-visit";
```

## 核心 API

### 数据结构

| 类型名 | 种类 | 字段与方法 |
| --- | --- | --- |
| `UnistNode` | `class` | 字段: `kind: string, value: string, parent: u32, first_child: u32, next_sibling: u32` |
| `UnistTree` | `class` | 字段: `nodes: [128]UnistNode, len: usize` · 方法: `addNode, isNode, visitPreOrder, getAncestors` |

### 函数列表

| 函数名 | 签名 | 说明 |
| --- | --- | --- |
| `uuvStrEq` | `(a: string, b: string): bool` | 调用 `uuvStrEq` 并返回 `bool`。 |
| `newUnistTree` | `(): UnistTree` | 创建或初始化实例，返回 `UnistTree`。 |

## 最小调用示例

```cp
import { uuvStrEq } from "unist-util-visit";

pub fn main() {
    let result = uuvStrEq("example", "example");
    _ = result;
}
```

## CP 零堆与借用安全亮点

- **纯 CP 零堆分配**：基于栈结构体与切片视图完成计算，无隐式堆内存分配与垃圾回收抖动。
- **零拷贝字符串切片**：输入与返回的字符串直接借用调用方切片内存，避免无谓拷贝。
- **编译期借用与类型安全**：由 `cpc` 编译期静态检查可变借用（`&mut`）与不可变借用边界。
