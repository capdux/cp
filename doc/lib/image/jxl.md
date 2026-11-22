# `cap:jxl` — 纯 CP 零分配 JPEG XL 裸码流与 ISOBMFF 容器解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:image`，导入请使用 `import { ... } from "cap:image"`。

## 概述

`cap:jxl` 提供纯 CP、零外部依赖、零堆分配的 JPEG XL（ISO/IEC 18181-1）裸码流签名与 LSB-first `SizeHeader` 解码器、ISOBMFF Box 迭代器以及自动兼容裸码流/容器的统一元数据解析入口 `parseJxlInfo`。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `JxlContainerKind` | 封装形态枚举：`codestream`, `container`, `unknown` |
| `isJxlCodestream(data)` / `isJxlContainer(data)` / `isJxl(data)` / `detectJxlKind(data)` | 裸码流（`0xFF 0x0A`）与 ISOBMFF 容器（12 字节 `JXL ` 签名）识别 |
| `JxlSizeHeader` | 裸码流 `SizeHeader`：`width: u32`, `height: u32`, `is_small: bool`, `ratio: u8`, `bits_read: u32` |
| `parseJxlSizeHeader(codestream)` | 按 ISO/IEC 18181-1 LSB-first 位流解码 `small=1`（5-bit * 8）与 `small=0`（9/13/18/30-bit）及全部 0..7 宽高比 |
| `JxlBox` & `JxlBoxIter` | 零拷贝 ISOBMFF Box 迭代器（支持 32-bit、`size=1` 64-bit 扩展与 `size=0` 至 EOF） |
| `JxlInfo` & `parseJxlInfo(data)` | 统一解析入口，返回 `kind`, `width`, `height`, `has_exif`, `has_xmp`, `is_partial`（兼容 `jxlc`、`jxlp` 首分片、`Exif`、`xml ` 与 `brob`） |

## 示例

```cp
import { isJxl, parseJxlInfo } from "cap:image";

fn inspectJxl(bytes: []const u8): bool {
    if (!isJxl(bytes)) return false;
    const info = parseJxlInfo(bytes) ?? return false;
    return info.width > 0 && info.height > 0;
}
```
