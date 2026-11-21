# `cap:exr` — 纯 CP OpenEXR HDR 图像容器零分配头部与 Attribute 解析库

## 概述

`cap:exr` 提供纯 CP、零外部依赖、零堆分配的 OpenEXR（`.exr`）高动态范围图像容器解析器，支持 8 字节魔数与版本/标志位解码（Tiled、Long Names、Non-Image Parts、Multipart）、零分配 `ExrAttrIter` 属性遍历、`box2i` 包围框宽高计算、`ExrCompression` 压缩算法枚举映射与完整头部元数据提取。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `EXR_MAGIC` / `EXR_FLAG_*` | OpenEXR 魔数 `20000630` 及版本标志位常量（`0x0200`..`0x1000`） |
| `ExrCompression` | 压缩算法枚举：`none`..`dwab`, `unknown` |
| `ExrBox2i` | 闭区间包围框：`x_min`, `y_min`, `x_max`, `y_max`；只读方法：`width()`, `height()` |
| `ExrAttr` | 零拷贝属性：`name`, `type_name`, `size`, `value` |
| `ExrHeader` | 头部摘要：`version`, `flags`, `is_tiled`, `has_long_names`, `has_non_image_parts`, `is_multipart`, `compression`, `data_window`, `width`, `height`, `attr_count`, `data_offset` |
| `isExr(data)` | 快速检测 OpenEXR 魔数 |
| `compressionFromByte(b)` | 单字节压缩编码转 `ExrCompression` |
| `parseExrBox2i(val)` | 解析 16 字节 `box2i` 属性负载 |
| `ExrAttrIter` | 流式属性迭代器：`init(data)`、`next(*_): ExrAttr?`（遇 `\0` 结束并记录 `data_offset`） |
| `parseExrHeader(data)` | 完整校验并解析 OpenEXR 头部元数据 |

## 示例

```cp
import { parseExrHeader, ExrCompression } from "cap:exr";

fn inspectExr(buf: []const u8): bool {
    if (hdr := parseExrHeader(buf)) {
        return hdr.width > 0 && hdr.height > 0 && hdr.compression != ExrCompression.unknown;
    }
    return false;
}
```
