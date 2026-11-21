# `cap:x3d` — Web3D X3D 3.x/4.0 XML 3D 场景零分配解析库

> **导入方式**：`import { isX3d, parseX3dHeader, X3dMetaIter, findX3dMeta, X3dNodeIter, X3dFloatIter, X3dIntIter, summarizeX3d } from "cap:x3d";`  
> **内存模型**：100% 纯 CP 零堆分配，所有标签名、元数据与属性切片均零拷贝指向原始 XML 缓冲区。

---

## 核心数据结构

- **`X3dHeader`**：`profile`, `version`, `scene_offset`
- **`X3dMeta`**：`name`, `content`
- **`X3dNode`**：`tag`, `def_name`, `use_name`, `attrs_slice`, `depth`（提供 `getAttr(name): ([]const u8)?`）
- **`X3dStats`**：`node_count`, `def_count`, `use_count`, `shape_count`, `transform_count`, `route_count`, `max_depth`

---

## API 列表

| 函数 / 类型 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isX3d` / `parseX3dHeader` | `fn parseX3dHeader(xml): X3dHeader?` | 跳过 BOM/XML 声明/DOCTYPE/注释，提取 `<X3D profile version>` 与 `<Scene>` 偏移 |
| `X3dMetaIter` / `findX3dMeta` | `fn findX3dMeta(xml, name): ([]const u8)?` | 零分配遍历/查找 `<head>` 内 `<meta name content/>` |
| `X3dNodeIter` | `X3dNodeIter.init(xml)` / `next(): X3dNode?` | 前序遍历 `<Scene>` 内全部 3D 节点，支持 `DEF`/`USE` 与 `node.getAttr(...)` |
| `X3dFloatIter` / `X3dIntIter` | `init(attr_val)` / `next()` | 零分配解码空白或逗号分隔的 `MFVec3f`/`MFFloat`/`MFInt32`（含 `-1` 面分隔符） |
| `summarizeX3d` | `fn summarizeX3d(xml): X3dStats?` | 单遍统计节点、DEF/USE、Shape/Transform/ROUTE 与最大嵌套深度 |

---

## 使用示例

```typescript
import { parseX3dHeader, X3dNodeIter, X3dIntIter } from "cap:x3d";

fn inspectX3d(xml: []const u8) {
    if (hdr := parseX3dHeader(xml)) {
        * = hdr.version;
        let it = X3dNodeIter.init(xml);
        while (n := it.next()) {
            if (idx_str := n.getAttr("coordIndex")) {
                let ints = X3dIntIter.init(idx_str);
                while (v := ints.next()) {
                    * = v;
                }
            }
        }
    }
}
```
