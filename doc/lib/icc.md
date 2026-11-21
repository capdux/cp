# `cap:icc` — 纯 CP ICC Color Profile 色彩配置文件零分配解析库

## 概述

`cap:icc` 提供纯 CP、零外部依赖、零堆分配的 ICC Color Profile（ISO 15076-1 / ICC.1）128 字节大端文件头解析、色彩空间/设备类/PCS 判定及标签表流式迭代器，适用于图像解码与 Web 渲染色彩管理。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `ICC_HEADER_SIZE` / `ICC_TAG_TABLE_OFFSET` / `ICC_TAG_ENTRY_SIZE` | 常量：`128`、`128`、`12` |
| `IccHeader` | 128 字节大端配置头：`profile_size`, `cmm_type`, `version_major`, `version_minor`, `version_bugfix`, `device_class`, `color_space`, `pcs`, `rendering_intent`, `signature`；附只读方法 `isRgb()`, `isCmyk()`, `isDisplay()` |
| `IccTagEntry` | 12 字节标签表项：`sig: [4]u8`, `offset: u32`, `size: u32`；附 `slice(data): ([]const u8)?` 防溢出零拷贝切片方法 |
| `iccTagEquals(tag, s)` | 比较 4 字节 ICC 标识与切片是否相等 |
| `isIccProfile(data)` | 校验长度 `>= 128` 且偏移 36 处签名为 `"acsp"` |
| `parseIccHeader(data)` | 校验 `profile_size >= 128 && profile_size <= data.len` 与 `"acsp"` 签名并解析头部 |
| `IccTagIter` | 标签表流式迭代器：`init(data)`, `next(*_): IccTagEntry?`；解析偏移 128 处 `tag_count` 及后续 12 字节条目，严格防越界与超大 `tag_count` 溢出 |
| `findIccTag(data, sig)` | 按 4 字节签名查找首个匹配标签（如 `"desc"`, `"cprt"`, `"rXYZ"`, `"wtpt"`） |

## 示例

```cp
import { isIccProfile, parseIccHeader, findIccTag } from "cap:icc";

fn readCopyright(icc: []const u8): ([]const u8)? {
    if (!isIccProfile(icc)) return null;
    const hdr = parseIccHeader(icc) ?? return null;
    if (!hdr.isRgb()) return null;
    const tag = findIccTag(icc, "cprt") ?? return null;
    return tag.slice(icc);
}
```
