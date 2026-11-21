# `cap:flv` — 纯 CP Adobe FLV 流媒体容器头与音视频 Tag 零分配解析库

## 概述

`cap:flv` 提供纯 CP、零外部依赖、零堆分配的 Adobe FLV 流媒体容器 9 字节文件头解析、11 字节 Tag 流式迭代（含 32-bit 扩展时间戳组合与尾部 `PreviousTagSize` 一致性校验）及 AAC/MP3/AVC/HEVC 音视频首字节快捷解码。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `FLV_HEADER_SIZE` / `FLV_TAG_HEADER_SIZE` / `FLV_TAG_AUDIO` / `FLV_TAG_VIDEO` / `FLV_TAG_SCRIPT` | 常量：`9`, `11`, `8`, `9`, `18` |
| `isFlv(data)` | 校验首 3B `"FLV"`、长度 `>= 9` 且 `data_offset` 合法不越界 |
| `FlvHeader` / `parseFlvHeader(data)` | 解析 `version`, `has_audio`, `has_video`, `data_offset` |
| `FlvTagType` / `FlvTag` / `FlvTagIter` | 跳过 `data_offset` 与 4B `PreviousTagSize0`，逐帧解析 11B 头、32-bit 组合时间戳、零拷贝 `payload` 并校验尾部 `PreviousTagSize == 11 + data_size` |
| `FlvAudioInfo` / `parseFlvAudioHeader(payload)` | 解码 `sound_format` (2=MP3, 10=AAC), `sample_rate_hz`, `is_16bit`, `is_stereo`, `aac_packet_type` |
| `FlvVideoInfo` / `parseFlvVideoHeader(payload)` | 解码 `frame_type`, `codec_id` (7=AVC, 12=HEVC), `avc_packet_type`, 有符号 24-bit `composition_time_ms`；附只读方法 `isKeyframe()` |

## 示例

```cp
import { isFlv, FlvTagIter, FlvTagType, parseFlvVideoHeader } from "cap:flv";

fn countKeyframes(flv: []const u8): usize {
    if (!isFlv(flv)) return 0;
    let it = FlvTagIter.init(flv);
    let keys: usize = 0;
    while (tag := it.next()) {
        if (tag.tag_type == FlvTagType.video) {
            if (v := parseFlvVideoHeader(tag.payload)) {
                if (v.isKeyframe()) keys += 1;
            }
        }
    }
    return keys;
}
```
