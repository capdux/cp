# `cap:dae` — 纯 CP Khronos COLLADA 1.4/1.5 `.dae` 3D 资产零分配解析库

## 概述

`cap:dae` 提供纯 CP、零外部依赖、零堆分配的 Khronos COLLADA 1.4.1 / 1.5.0（`.dae`）3D 资产 XML 解析器：

- **格式与 `<asset>` 元数据解析**：自动跳过 UTF-8 BOM、XML 声明与注释识别 `<COLLADA>`，提取 `version`、`author`、`authoring_tool`、`created`、`modified`、`unit_name`、`unit_meter`（默认 `1.0`）与 `up_axis`（默认 `"Y_UP"`）；
- **几何体与浮点数组零分配流**：`DaeGeometryIter` 迭代 `<library_geometries>` 中的 `<geometry>`，`DaeFloatArrayIter` 提取 `<float_array id="..." count="...">`，`DaeFloatIter` 零分配逐个解码空白分隔 `f64`（完整支持科学计数法 `e`/`E`）；
- **场景树节点迭代**：`DaeSceneNodeIter` 前序迭代 `<library_visual_scenes>` 中所有 `<node>`（含嵌套子节点），并提取节点自身的 `<instance_geometry url="..."/>` 引用；
- **资产统计汇总**：`summarizeDae` 一键统计几何体数、浮点数组数、总浮点元素数、场景节点数与材质数。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `isCollada(data): bool` | 检测 `<COLLADA` 根元素（跳过 BOM / PI / 注释） |
| `DaeAsset` / `parseDaeAsset(xml): DaeAsset?` | 解析根 `version` 与 `<asset>` 元数据（非 COLLADA 返回 `null`） |
| `DaeGeometry` / `DaeGeometryIter` | 零分配迭代 `<library_geometries>` 中的 `<geometry>` |
| `DaeFloatArray` / `DaeFloatArrayIter` | 零分配迭代 `<float_array id="..." count="...">...</float_array>` |
| `DaeFloatIter` | 零分配流式解码空白分隔 `f64`（支持科学计数法，`had_error` 标记畸形数值） |
| `DaeSceneNode` / `DaeSceneNodeIter` | 前序迭代 `<library_visual_scenes>` 中的 `<node>` 与直接 `<instance_geometry url="..."/>` |
| `DaeStats` / `summarizeDae(xml): DaeStats` | 统计 `geometry_count`, `float_array_count`, `total_floats`, `node_count`, `material_count` |

## 示例

```cp
import { parseDaeAsset, DaeGeometryIter, DaeFloatArrayIter, DaeFloatIter, summarizeDae } from "cap:dae";

fn inspectCollada(xml: []const u8): usize {
    const asset = parseDaeAsset(xml) ?? return 0;
    * = asset.unit_meter;

    let verts: usize = 0;
    let git = DaeGeometryIter.init(xml);
    while (g := git.next()) {
        let fait = DaeFloatArrayIter.init(g.raw_xml);
        while (fa := fait.next()) {
            let fit = DaeFloatIter.init(fa.raw_values);
            while (v := fit.next()) {
                * = v;
                verts += 1;
            }
        }
    }
    * = summarizeDae(xml);
    return verts;
}
```
