# `cap:fbx` — 零分配 Autodesk FBX Binary (7100–7700) 3D 场景树解析库

> **模块路径**：`lib/fbx/`（入口 `lib/fbx/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整支持 Autodesk FBX Binary 小端序 32-bit (`<7500`) 与 64-bit (`>=7500`) 节点头、NULL sentinel 终止、多级嵌套子节点零拷贝遍历及全类型标量/字符串/原始与 zlib 压缩数组属性解码。

---

## 一、导入方式

```cp
import {
    FBX_MAGIC,
    FBX_HEADER_SIZE,
    FBX_V7500_THRESHOLD,
    FbxHeader,
    FbxNode,
    FbxProp,
    FbxNodeIter,
    FbxPropIter,
    isFbxBinary,
    parseFbxHeader,
} from "cap:fbx";
```

---

## 二、核心常量与数据结构

| 符号 | 类型 / 字段 | 说明 |
| :--- | :--- | :--- |
| `FBX_MAGIC` | `[]const u8` | 21 字节魔数 `"Kaydara FBX Binary  \x00"`（后接 `0x1A 0x00` 与 4B LE `version`，总头长 27B） |
| `FBX_V7500_THRESHOLD` | `u32 = 7500` | `>= 7500` 切换为 64-bit 节点头（25B）与 25B NULL sentinel；`< 7500` 为 32-bit（13B） |
| `FbxHeader` | `version: u32`, `is_64bit: bool`, `nodes_offset: usize`; `rootNodes(data)` | 27B 文件头解析结果 |
| `FbxNode` | `name: []const u8`, `num_properties: u64`, `props_slice: []const u8`, `children_slice: []const u8`, `children_base_offset: usize`; `properties()`, `children(is_64bit)` | 零拷贝场景树节点（自动维护绝对偏移基址以支持任意深度嵌套） |
| `FbxProp` | `type_code: u8`, `int_val: i64`, `float_val: f64`, `bool_val: bool`, `raw_bytes: []const u8`, `array_len: u32`, `array_encoding: u32`, `array_compressed_len: u32` | 解码后的强类型属性 |

---

## 三、支持的属性类型码 (`type_code`)

- **标量**：`'Y'` (`i16`), `'C'` (`bool`/`u8`), `'I'` (`i32`), `'F'` (`f32`), `'D'` (`f64`), `'L'` (`i64`)
- **字符串 / 二进制**：`'S'` (UTF-8/raw string), `'R'` (raw binary) — 4B LE 长度 + 载荷切片
- **数组**：`'f'` (`f32[]`), `'d'` (`f64[]`), `'l'` (`i64[]`), `'i'` (`i32[]`), `'b'` (`bool[]`) — 12B 头 (`array_len`, `encoding` 0=raw / 1=zlib, `compressed_len`) + 载荷切片

---

## 四、使用示例

```cp
import { parseFbxHeader } from "cap:fbx";

fn inspectFbx(data: []const u8) {
    if (hdr := parseFbxHeader(data)) {
        let root_it = hdr.rootNodes(data);
        while (node := root_it.next()) {
            let prop_it = node.properties();
            while (prop := prop_it.next()) {
                * = prop;
            }
            let child_it = node.children(hdr.is_64bit);
            while (child := child_it.next()) {
                * = child;
            }
        }
    }
}
```
