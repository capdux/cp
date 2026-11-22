# `cap:ico` — 纯 CP 零分配 Windows ICO / CUR 图标容器解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:image`，导入请使用 `import { ... } from "cap:image"`。

## 概述

`cap:ico` 提供纯 CP、零外部依赖、零堆分配的 Windows ICO（`.ico`）与 CUR（`.cur`）图标容器二进制目录解析、PNG/BMP 图像负载零拷贝切片提取以及面向浏览器 Favicon 的最佳尺寸选择器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `IcoKind` | 容器类型枚举：`icon`（type=1）、`cursor`（type=2）、`unknown` |
| `IcoHeader` | 6 字节小端 `ICONDIR` 头：`kind`, `count: u16` |
| `IcoEntry` | 16 字节 `ICONDIRENTRY` 目录项：`width`/`height`（`0` 解码为 `256`）、`color_count`、`planes_or_hotspot_x`、`bpp_or_hotspot_y`、`bytes_in_res`、`image_offset`、`is_png`；方法 `payload(data): ([]const u8)?` |
| `isIcoOrCur(data)` | 快速校验是否为合法 ICO/CUR 文件头 |
| `parseIcoHeader(data)` | 解析 6 字节小端 `ICONDIR` 头（要求 `reserved == 0` 且 `type` 为 1 或 2） |
| `IcoEntryIter` | 目录项流式迭代器：`init(data)`, `next(*_): IcoEntry?`（严格校验 16 字节目录项与负载边界不越界） |
| `findBestIcoEntry(data, target_size)` | 按最接近 `target_size` 且同尺寸优先 PNG / 高色深选出最佳图标条目 |

## 示例

```cp
import { isIcoOrCur, findBestIcoEntry } from "cap:image";

fn selectFaviconPayload(ico_bytes: []const u8): ([]const u8)? {
    if (!isIcoOrCur(ico_bytes)) {
        return null;
    }
    if (best := findBestIcoEntry(ico_bytes, 32)) {
        return best.payload(ico_bytes);
    }
    return null;
}
```
