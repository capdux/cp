# `cap:rope_text` — 平衡二叉树大文本编辑器缓冲库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:ds`，导入请使用 `import { ... } from "cap:ds"`。

## 概述

`cap:rope_text` 提供基于平衡二叉树（Rope）的工业级大文本数据结构，适用于高并发编辑、大文件分块加载与局部切片提取。叶子节点持有紧凑字节块（`<= 64` 字节），内部节点缓存左子树字节权重 `left_weight` 与左子树换行计数 `left_newlines`，配合 AVL 旋转再平衡，保证随机插入、删除、`charAt` 与 `lineAt` 均具备 $O(\log N)$ 复杂度。

同时支持静态导入 `import { Rope, RopeError } from "cap:ds";` 与动态挂载 `import cap from "cap";`（通过 `cap.rope_text` 访问）。

## 核心类型

- `RopeError`:
  - `OutOfBounds`: 索引、位置或行号越界。
  - `InvalidRange`: 区间非法（`start > end`）。
- `Node` / `NodePtr`:
  - 内部二叉树节点，区分紧凑叶子块与内部权重索引节点。
- `Rope`:
  - `alloc: std.mem.Allocator`
  - `root: NodePtr?`

## API 速查

| 方法 | 签名 | 说明 |
| --- | --- | --- |
| `Rope.init` | `init(alloc) -> Rope` | 初始化空 Rope 实例 |
| `Rope.fromSlice` | `fromSlice(alloc, text: []const u8) -> !Rope` | 从字节切片分块构建平衡 Rope |
| `deinit` | `deinit(&self)` | 递归释放全部节点内存 |
| `len` | `len(self) -> usize` | 返回总字节长度 |
| `lineCount` | `lineCount(self) -> usize` | 返回总行数（`total_newlines + 1`） |
| `charAt` | `charAt(self, idx: usize) -> RopeError!u8` | $O(\log N)$ 读取指定字节 |
| `insert` | `insert(&self, pos: usize, text: []const u8) -> !void` | 在 `pos` 处插入文本并自动分割/再平衡 |
| `delete` | `delete(&self, start: usize, end: usize) -> RopeError!void` | 删除 `[start, end)` 区间并自动合并/再平衡 |
| `sliceRange` | `sliceRange(self, alloc, start: usize, end: usize) -> ![]u8` | 提取 `[start, end)` 区间的新切片（调用方负责释放） |
| `lineAt` | `lineAt(self, alloc, line_idx: usize) -> ![]u8` | $O(\log N)$ 定位并返回第 `line_idx` 行内容（不含 `\n`） |
| `toString` | `toString(self, alloc) -> ![]u8` | 拼装还原全量文本切片（调用方负责释放） |

## 示例

```cp
import std;
import { Rope } from "cap:ds";

fn main() -> !void {
    const alloc = std.heap.page_allocator;
    let r = try Rope.fromSlice(alloc, "hello\nworld");
    defer r.deinit();

    try r.insert(5, " cp");
    const line0 = try r.lineAt(alloc, 0);
    defer alloc.free(line0);
    // line0 == "hello cp"
}
```
