# `cap:obj` — 纯 CP Wavefront 3D `.obj` 网格零分配解析库

## 概述

`cap:obj` 提供纯 CP、零外部依赖、零堆分配的 Wavefront 3D `.obj` 网格流式行记录迭代器（自动处理 `\n`/`\r\n` 与 `#` 行尾注释）、双精度三维向量解析器、支持正/负相对索引与 4 种斜杠格式的面片顶点索引解析器以及一趟网格统计摘要。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `ObjRecordKind` | 指令枚举：`vertex`, `texcoord`, `normal`, `face`, `object_name`, `group_name`, `usemtl`, `mtllib`, `other` |
| `ObjVec3` | 双精度 3D 向量：`x: f64`, `y: f64`, `z: f64` |
| `ObjFaceVertex` | 面片顶点引用：`v: i32`, `vt: i32?`, `vn: i32?` |
| `ObjStats` | 统计摘要：`vertices`, `texcoords`, `normals`, `faces`, `objects`, `groups` |
| `ObjRecord` | 零拷贝行记录：`kind: ObjRecordKind`, `rest: []const u8` |
| `ObjLineIter` | 流式行迭代器：`init(input)`、`next(*_): ObjRecord?` |
| `parseObjVec3(args)` | 解析 3 个浮点数 `x y z`（可选第 4 齐次分量 `w` 忽略） |
| `parseObjFaceVertex(token)` | 解析 `v`、`v/vt`、`v//vn`、`v/vt/vn`，支持负相对索引并拦截 `0` 索引 |
| `summarizeObj(input)` | 零分配统计 `.obj` 网格元素数量 |

## 示例

```cp
import { ObjLineIter, ObjRecordKind, parseObjVec3, summarizeObj } from "cap:obj";

fn inspectObj(src: []const u8): bool {
    const stats = summarizeObj(src);
    if (stats.vertices == 0 || stats.faces == 0) return false;
    let it = ObjLineIter.init(src);
    while (rec := it.next()) {
        if (rec.kind == ObjRecordKind.vertex) {
            return parseObjVec3(rec.rest) != null;
        }
    }
    return false;
}
```
