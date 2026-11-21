# `cap:xcf` — 纯 CP GIMP XCF 分层图像零分配解析库

## 概述

`cap:xcf` 提供纯 CP、零外部依赖、零堆分配的 GIMP XCF（v000–v014）分层图像文件解析器，完整支持：
- **14B 魔数与多版本文件头**：支持 `"file\0"`（v0）与 `"v001\0"`..`"v014\0"`、v4+ `XcfPrecision` 位深精度与 v11+ 64-bit 偏移自动判定；
- **属性表流式解析**：`XcfPropIter` 遍历 `[type, size, payload]` 直至 `PROP_END(0)` 并定位图层偏移列表；
- **图层/通道偏移指针表与图层头**：`XcfPointerIter`（32/64-bit，自动防越界）与 `parseXcfLayerHeader`（零拷贝剥离末尾 `'\0'` 图层名）。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `XCF_MAGIC`, `PROP_END`, `PROP_COMPRESSION`, `PROP_RESOLUTION`, ... | 9B 魔数前缀与常用属性 ID 常量 |
| `XcfBaseType`, `XcfCompression`, `XcfPrecision` | 色彩模式、瓦片压缩算法与 v4+ 精度枚举 |
| `isXcf(data)` / `XcfHeader` / `parseXcfHeader(data)` | 探测与解析 XCF 文件头 |
| `XcfProperty` / `XcfPropIter` / `parseXcfCompression(payload)` | 属性表流式迭代器（含 `layersListOffset()`） |
| `XcfPointerIter` | 以 `0` 结尾的 32/64-bit 图层与通道偏移表迭代器 |
| `XcfLayerInfo` / `parseXcfLayerHeader(data, offset, uses_64bit)` | 图层尺寸、类型与 UTF-8 图层名解析 |

## 示例

```cp
import { parseXcfHeader, XcfPropIter, XcfPointerIter, parseXcfLayerHeader } from "cap:xcf";

fn countLayers(data: []const u8): usize {
    if (hdr := parseXcfHeader(data)) {
        let pit = XcfPropIter.init(data, hdr.props_offset);
        while (prop := pit.next()) {
            * = prop.prop_type;
        }
        if (list_off := pit.layersListOffset()) {
            let lit = XcfPointerIter.init(data, list_off, hdr.uses_64bit_offsets);
            let n: usize = 0;
            while (off := lit.next()) {
                if (layer := parseXcfLayerHeader(data, off, hdr.uses_64bit_offsets)) {
                    * = layer.name.len;
                    n += 1;
                }
            }
            return n;
        }
    }
    return 0;
}
```
