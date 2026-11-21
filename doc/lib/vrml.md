# `cap:vrml` — VRML97 & Classic X3D 3D 场景零分配解析库

> **导入方式**：`import { VrmlFlavor, VrmlTokenKind, parseVrmlHeader, isVrml, VrmlLexer, VrmlNodeIter, findVrmlField, summarizeVrml } from "cap:vrml";`  
> **内存模型**：100% 纯 CP 零堆分配，所有标识符、字符串、节点体与字段值切片均零拷贝指向原始场景缓冲区。

---

## 核心类型

- **`VrmlFlavor: u8`**：`vrml97`, `x3d_classic`, `unknown`
- **`VrmlTokenKind: u8`**：`ident`, `number`, `string`, `lbrace`, `rbrace`, `lbracket`, `rbracket`
- **`VrmlHeader`**：`flavor`, `version`, `encoding`, `body_offset`
- **`VrmlToken`**：`kind`, `text`, `start`, `end`
- **`VrmlNode`**：`def_name`, `node_type`, `is_use`, `body_slice`
- **`VrmlStats`**：`node_count`, `def_count`, `use_count`, `route_count`, `max_depth`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `parseVrmlHeader` / `isVrml` | `fn parseVrmlHeader(data): VrmlHeader?` | 解析 `#VRML V2.0 utf8` 或 `#X3D V3.x/4.0 utf8` 头 |
| `VrmlLexer` | `VrmlLexer.init(src)` / `next(): VrmlToken?` | 零分配词法流（逗号视为空白，跳过 `#` 注释，支持转义字符串、科学计数法与十六进制数） |
| `VrmlNodeIter` | `VrmlNodeIter.init(data)` / `next(): VrmlNode?` | 遍历顶层或嵌套 `DEF` / `USE` / 普通节点声明 |
| `findVrmlField` | `fn findVrmlField(body_slice, field_name): ([]const u8)?` | 在节点 `{ ... }` 顶层定位字段值切片（如 `translation`、`point` 数组） |
| `summarizeVrml` | `fn summarizeVrml(data): VrmlStats?` | 单遍统计节点/DEF/USE/ROUTE 与最大深度，严格校验 `{}`/`[]` 括号平衡与字符串闭合 |

---

## 使用示例

```typescript
import { VrmlNodeIter, findVrmlField, summarizeVrml } from "cap:vrml";

fn inspectScene(vrml_bytes: []const u8) {
    if (stats := summarizeVrml(vrml_bytes)) {
        * = stats.node_count;
    }
    let nodes = VrmlNodeIter.init(vrml_bytes);
    while (n := nodes.next()) {
        if (tr := findVrmlField(n.body_slice, "translation")) {
            * = tr;
        }
    }
}
```
