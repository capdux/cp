# `cap:flac` — 纯 CP FLAC 无损音频流与元数据块零分配解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

## 概述

`cap:flac` 提供纯 CP、零外部依赖、零堆分配的 FLAC（RFC 9639）无损音频 `"fLaC"` 魔数校验、`STREAMINFO` 34 字节大端位字段解析（含 20-bit 采样率、3-bit 声道数、5-bit 位深、36-bit 总采样数与 16 字节 MD5）及元数据块流式迭代器，与 `cap:riff` / `cap:id3` 配套服务于多媒体资产校验。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `FLAC_MAGIC_SIZE` / `FLAC_BLOCK_HEADER_SIZE` / `FLAC_STREAMINFO_SIZE` | 常量：`4`、`4`、`34` |
| `FlacBlockType` | 元数据块类型枚举：`streaminfo`, `padding`, `application`, `seektable`, `vorbis_comment`, `cuesheet`, `picture`, `unknown` |
| `FlacMetadataBlock` | 零拷贝元数据块：`is_last: bool`, `block_type: FlacBlockType`, `raw_type: u8`, `payload: []const u8` |
| `FlacStreamInfo` | 34 字节 `STREAMINFO` 结构：`min_block_size`, `max_block_size`, `min_frame_size`, `max_frame_size`, `sample_rate` (20b), `channels` (1..8), `bits_per_sample` (1..32), `total_samples` (36b), `md5: [16]u8`；附防溢出 `durationMs(): u64` |
| `isFlac(data)` | 4 字节 `"fLaC"` 魔数快速判定 |
| `FlacBlockIter` | 元数据块流式迭代器：`init(data)`, `next(*_): FlacMetadataBlock?`；严格边界校验、拦截 RFC 9639 禁止类型 `127` 并在 `is_last == true` 后停止 |
| `parseStreamInfoPayload(payload)` | 直接解析 34 字节 `STREAMINFO` 负载 |
| `parseFlacStreamInfo(data)` | 解析完整 FLAC 流的首个 `STREAMINFO` 元数据块 |

## 示例

```cp
import { isFlac, parseFlacStreamInfo, FlacBlockIter } from "cap:media";

fn inspectFlac(bytes: []const u8): u64 {
    if (!isFlac(bytes)) return 0;
    const info = parseFlacStreamInfo(bytes) ?? return 0;
    return info.durationMs();
}
```
