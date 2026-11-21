# `cap:riff` — 纯 CP RIFF / RIFX 二进制块容器零分配解析库

## 概述

`cap:riff` 提供纯 CP、零外部依赖、零堆分配的 RIFF（小端）与 RIFX（大端）二进制块容器解析器，通用于 WAV、WebP、AVI 等多媒体格式，自动处理奇数块长度的 1 字节偶数对齐填充、嵌套 `LIST` 子块迭代及 `u64` 防溢出越界保护。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `RIFF_HEADER_SIZE` / `RIFF_CHUNK_HEADER_SIZE` | 常量：`12` 字节文件头、`8` 字节块头 |
| `RiffHeader` | 12 字节容器头：`is_big_endian: bool`（`RIFF`=小端 / `RIFX`=大端）、`file_size: u32`、`form_type: [4]u8`；附 `isForm(s)` 只读方法 |
| `RiffChunk` | 零拷贝数据块：`id: [4]u8`, `size: u32`, `data: []const u8` |
| `fourccEquals(tag, s)` | 比较 4 字节 FourCC 标识与切片是否相等 |
| `parseRiffHeader(data)` | 校验 12 字节头、`"RIFF"`/`"RIFX"` 魔数、`file_size >= 4` 及容器总长不越界 |
| `parseListType(list_payload)` | 读取 `LIST` 块负载前 4 字节子类型（如 `"INFO"`, `"hdrl"`） |
| `RiffChunkIter` | 流式块迭代器：`init(data)`、`initSubchunks(list_payload, is_big_endian)`（自动跳过 4 字节 `listType`）、`next(*_): RiffChunk?`（自动处理奇数长度 1 字节对齐填充与防溢出越界检查） |
| `findRiffChunk(data, chunk_id)` | 按 4 字节 FourCC 查找首个匹配块（如 `"fmt "`、`"data"`、`"VP8 "`） |

## 示例

```cp
import { parseRiffHeader, findRiffChunk } from "cap:riff";

fn extractWavPcm(wav: []const u8): ([]const u8)? {
    const hdr = parseRiffHeader(wav) ?? return null;
    if (!hdr.isForm("WAVE")) return null;
    const data_chunk = findRiffChunk(wav, "data") ?? return null;
    return data_chunk.data;
}
```
