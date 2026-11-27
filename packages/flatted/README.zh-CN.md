[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# flatted

适用于 CP 语言的零堆分配、支持循环引用的扁平 JSON 序列化与解析库，对齐 npm [`flatted`](https://github.com/WebReflection/flatted)（ISC 协议，CP v1.0.0）。

通过固定容量 64 节点 / 256 边的图 Arena（`FlatArena`）表达任意对象与数组图结构（包括自引用与相互循环引用），以标准 `flatted` 索引数组 JSON 格式完成无堆分配编解码。

## 安装与导入

```bash
cap add flatted
```

```cp
import {
    FlatNodeKind,
    FlatArena,
    stringifyFlatted,
    parseFlatted,
} from "flatted";
```

## 核心 API 参考

### 常量与数据结构

| 符号 | 定义 | 说明 |
| :--- | :--- | :--- |
| `FLAT_MAX_NODES` | `usize = 64` | `FlatArena` 最大节点数。 |
| `FLAT_MAX_EDGES` | `usize = 256` | `FlatArena` 最大边数（对象属性与数组元素总和）。 |
| `FLAT_NONE` | `i32 = -1` | 空链表指针或查找失败哨兵值。 |
| `FlatNodeKind` | `Null \| Bool \| Int \| Str \| Array \| Object` | 节点值类型枚举。 |
| `FlatNode` | `kind`, `bool_val`, `int_val`, `str_val`, `first_edge`, `last_edge`, `edge_count` | Arena 图节点结构。 |
| `FlatEdge` | `key: string`, `child_id: usize`, `next: i32` | 容器节点到子节点 ID 的链式边结构。 |
| `FltSpan` | `ok: bool`, `start: usize`, `end: usize`, `next: usize` | 字符串区间扫描辅助结果。 |

### `FlatArena` 方法

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `FlatArena.init()` | `(): FlatArena` | 初始化空的 64 节点 / 256 边 Arena。 |
| `reset()` | `()` | 清空全部节点和边，便于重复利用。 |
| `nodeCount()` | `(): usize` | 返回当前已分配节点数。 |
| `addNull()` / `addBool(v)` / `addInt(v)` / `addStr(s)` | `(...): usize` | 添加基础类型节点并返回节点 ID。 |
| `addArray()` / `addObject()` | `(): usize` | 添加数组或对象容器节点并返回节点 ID。 |
| `setField(obj_id, key, child_id)` | `(usize, string, usize): bool` | 设置或更新对象属性边（天然支持循环引用）。 |
| `pushElem(arr_id, child_id)` | `(usize, usize): bool` | 向数组节点追加元素边（支持循环引用）。 |
| `nodeKind(id)` | `(usize): FlatNodeKind` | 获取节点类型。 |
| `nodeBool(id)` / `nodeInt(id)` / `nodeStr(id)` | `(usize): bool \| i64 \| string` | 读取节点对应基础值。 |
| `elemCount(arr_id)` | `(usize): usize` | 返回容器节点的元素/属性边数量。 |
| `getField(obj_id, key)` | `(usize, string): i64` | 按键查找对象子节点 ID，未找到返回 `-1`。 |
| `getElem(arr_id, idx)` | `(usize, usize): i64` | 按索引查找数组子节点 ID，越界返回 `-1`。 |

### 编解码函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `stringifyFlatted` | `(arena: FlatArena, root_id: usize, out: []u8): usize` | 将从 `root_id` 可达的图序列化为 `flatted` JSON 写入 `out`，返回写入字节数（失败或溢出返回 `0`）。 |
| `parseFlatted` | `(json: string, out_arena: &mut FlatArena): i64` | 将 `flatted` JSON 解析到 `out_arena`，返回根节点 ID（`>= 0`）或 `-1`。 |

## 代码示例

### 1. 序列化包含循环引用的对象图

```cp
import { FlatArena, stringifyFlatted } from "flatted";

fn circularSerialize() {
    let mut arena = FlatArena.init();
    let root = arena.addObject();
    let name = arena.addStr("agent-1");
    arena.setField(root, "name", name);
    arena.setField(root, "self", root); // 自引用循环！

    let out: [512]u8 = undefined;
    let n = stringifyFlatted(arena, root, out[0..512]);
    if (n > 0) {
        let json = out[0..n];
        // [{"name":"1","self":"0"},"agent-1"]
    }
}
```

### 2. 解析 `flatted` JSON 并遍历循环引用

```cp
import { FlatArena, parseFlatted } from "flatted";

fn circularParse() {
    let input = "[{\"name\":\"1\",\"self\":\"0\"},\"agent-1\"]";
    let mut arena = FlatArena.init();
    let root_id = parseFlatted(input, &mut arena);

    if (root_id >= 0) {
        let r = root_id as usize;
        let self_id = arena.getField(r, "self");
        // self_id == root_id（循环引用已无损还原）
        let name_id = arena.getField(r, "name");
        if (name_id >= 0) {
            let name = arena.nodeStr(name_id as usize); // "agent-1"
        }
    }
}
```

## 与源包差异与零拷贝安全亮点

- **显式索引图 Arena**：CP 无 GC 对象图，使用整数节点 ID（`0..63`）在 `FlatArena` 中表示引用关系，彻底避免指针环内存泄漏。
- **字符串零拷贝切片**：`parseFlatted` 解析出的字符串直接引用原始输入 `json` 的字节切片，读取 `arena.nodeStr(...)` 期间需保证 `json` 保持存活。
- **固定栈内存可复用**：64 节点 / 256 边容量完全确定，可配合 `arena.reset()` 在高频请求中零分配复用。
