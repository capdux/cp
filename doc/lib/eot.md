# `cap:eot` — 零分配 Microsoft Embedded OpenType (`.eot`) Web 字体容器解析库

> **模块路径**：`lib/eot/`（入口 `lib/eot/index.cp`）  
> **特性**：纯 CP 实现、零堆分配（Zero-Alloc）、零 C 依赖，完整遵循 W3C / Microsoft EOT Format Specification（小端序），支持 v1.0 / v2.1 / v2.2 头部校验、UTF-16LE 名称提取与 UTF-8 解码、域名白名单、v2.2 签名/EUDC 字段解析及内嵌字体数据零拷贝切片。

---

## 一、导入方式

```cp
import {
    EOT_MAGIC,
    EOT_FIXED_HEADER_SIZE,
    EOT_VERSION_1,
    EOT_VERSION_2_1,
    EOT_VERSION_2_2,
    EOT_FLAG_COMPRESSED,
    EOT_FLAG_SUBSET,
    EOT_FLAG_XOR_ENCRYPTED,
    EotHeader,
    isEot,
    parseEotHeader,
    decodeUtf16LeName,
} from "cap:eot";
```

---

## 二、核心常量与数据结构

| 符号 | 值 / 字段 | 说明 |
| :--- | :--- | :--- |
| `EOT_MAGIC` | `0x504C` | 偏移 34 处 2B LE 魔数 (`'L' 'P'`) |
| `EOT_FIXED_HEADER_SIZE` | `82` | 固定头长度（含 `Reserved1..4` 与 `Padding1`） |
| `EOT_VERSION_1` / `2_1` / `2_2` | `0x00010000` / `0x00020001` / `0x00020002` | 支持的 EOT 规范版本 |
| `EOT_FLAG_COMPRESSED` / `SUBSET` / `XOR_ENCRYPTED` | `0x1` / `0x2` / `0x10000000` | MTX 压缩、子集化与 XOR 加密标志 |
| `EotHeader` | 固定头字段 + 4 组 `*_utf16` + `root_string_utf16` + v2.2 `signature`/`eudc_*` + `font_data_offset`/`font_data` | 零拷贝 EOT 容器视图；附 `isCompressed()`, `isXorEncrypted()`, `isSubset()` |

---

## 三、核心 API

- `isEot(data: []const u8): bool` — 校验最小长度 82B、偏移 34 魔数 `0x504C`、合法版本及 `eot_size <= data.len`。
- `parseEotHeader(data: []const u8): EotHeader?` — 严格校验 `Reserved1..4` 全零及各 `Padding` 为 `0x0000`，顺序解析 UTF-16LE 名称与版本扩展字段，最终精确定位 `font_data` 零拷贝切片。
- `decodeUtf16LeName(raw: []const u8, out: []u8): usize?` — 零分配将 UTF-16LE 名称（支持 BMP 与 UTF-16 代理对）解码为 UTF-8 写入 `out`。

---

## 四、使用示例

```cp
import { parseEotHeader, decodeUtf16LeName } from "cap:eot";

fn inspectEot(data: []const u8) {
    if (hdr := parseEotHeader(data)) {
        let name_buf: [128]u8 = [0; 128];
        if (n := decodeUtf16LeName(hdr.family_name_utf16, name_buf[0..])) {
            * = name_buf[0..n];
        }
        * = hdr.font_data;
    }
}
```
