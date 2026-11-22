# `cap:mp4` — 纯 CP ISO BMFF / MP4 容器 Box/Atom 零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

## 概述

`cap:mp4` 提供纯 CP、零外部依赖、零堆分配的 ISO/IEC 14496-12 (ISO Base Media File Format / MP4) 容器解析器，支持标准 8 字节 Box、64-bit `largesize` (`size == 1`)、`size == 0` 流末尾延伸与 16 字节 `uuid` 扩展类型，内置 `ftyp`、`mvhd` (v0/v1)、`mdhd` (v0/v1)、`tkhd` (v0/v1) 解析与递归容器路径查找。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `Mp4Box` | 零拷贝 Box：`box_type`, `size`, `header_size`, `has_uuid`, `uuid`, `payload`；附只读方法 `isType(fourcc)`, `isContainer()`, `children(): BoxCursor`（自动跳过 `meta` 的 4 字节 FullBox 头） |
| `BoxCursor` | 流式 Box 迭代器：`init(data)`, `next(*_): Mp4Box?`, `remaining(): usize` |
| `findBox(data, fourcc)` | 在当前层级查找首个匹配的 4 字节 FourCC Box |
| `findBoxPath(data, path)` | 按斜杠路径递归查找嵌套 Box（如 `"moov/mvhd"`、`"moov/trak/mdia/mdhd"`、`"moov/udta/meta/ilst"`） |
| `FtypBox` / `parseFtyp(payload)` / `isMp4(data)` | 解析 `ftyp` 品牌：`major_brand`, `minor_version`, `compatible_brands`，附 `brandCount()`, `brandAt(i)`, `hasBrand(brand)` |
| `MvhdBox` / `parseMvhd(payload)` | 解析 Movie Header（v0 32-bit / v1 64-bit `timescale` & `duration`），附 `durationMs(): u64` 与 `durationSecs(): f64` |
| `MdhdBox` / `parseMdhd(payload)` | 解析 Media Header（v0/v1），解码 3 字节 ISO-639-2/T 语言码与 `durationMs(): u64` |
| `TkhdBox` / `parseTkhd(payload)` | 解析 Track Header（v0/v1），提取 `track_id`, `duration`, `width`, `height` |
| `fourccEquals(tag, s)` | 4 字节 FourCC 比较工具 |

## 示例

```cp
import { isMp4, findBoxPath, parseMvhd } from "cap:media";

fn getMp4DurationMs(bytes: []const u8): u64 {
    if (!isMp4(bytes)) return 0;
    const mvhd_box = findBoxPath(bytes, "moov/mvhd") ?? return 0;
    const mvhd = parseMvhd(mvhd_box.payload) ?? return 0;
    return mvhd.durationMs();
}
```
