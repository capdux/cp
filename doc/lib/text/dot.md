# `cap:dot` — 纯 CP Graphviz DOT 有向/无向图零分配解析与序列化库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:text`，导入请使用 `import { ... } from "cap:text"`。

## 概述

`cap:dot` 提供纯 CP、零外部依赖、零堆分配的 Graphviz DOT 图描述语言头部解析、边流式迭代（支持有向 `->`、无向 `--`、链式边 `a -> b -> c` 与双引号节点名）、字符串/注释感知括号平衡校验及自动引号转义序列化器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `DotGraphKind` | 图类型枚举：`graph`（无向）、`digraph`（有向） |
| `DotHeader` | 图头部元数据：`is_strict`, `kind`, `name` |
| `DotEdge` | 零拷贝边结构：`from_node`, `to_node`, `is_directed`, `attrs` |
| `parseDotHeader(input)` | 跳过 `//`/`/* */`/`#` 注释，解析 `[strict] (graph|digraph) [name] {` |
| `DotEdgeIter` | 流式边迭代器：`init(input)`、`next(*_): DotEdge?`，支持链式边与双引号节点名 |
| `validateDotBraces(input)` | 字符串/注释感知校验 `{}` 与 `[]` 嵌套平衡与引号闭合 |
| `formatDotEdge(from, to, directed, label, out)` | 格式化输出 DOT 边语句（必要时自动加引号并转义 `"`/`\`），缓冲不足返回 `null` |

## 示例

```cp
import { parseDotHeader, DotEdgeIter, formatDotEdge, validateDotBraces } from "cap:text";

fn example(src: []const u8): bool {
    if (!validateDotBraces(src)) return false;
    if (hdr := parseDotHeader(src)) {
        let it = DotEdgeIter.init(src);
        if (edge := it.next()) {
            let buf: [128]u8 = undefined;
            return formatDotEdge(edge.from_node, edge.to_node, edge.is_directed, "dep", buf[0..]) != null && hdr.kind == hdr.kind;
        }
    }
    return false;
}
```
