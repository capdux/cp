# `cap:mkv` — 纯 CP Matroska / WebM EBML 视频容器零分配解析库

## 概述

`cap:mkv` 提供纯 CP、零外部依赖、零堆分配的 Matroska / WebM（RFC 8794 EBML）容器解析能力，支持 VINT 变长 ID/长度解码、32-bit/64-bit IEEE-754 浮点解码、未知尺寸流式 `Segment`（`0x01FFFFFFFFFFFFFF`）兼容、`Info` 元数据提取以及 `Tracks`/`TrackEntry` 多轨道零分配流式迭代。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `ID_EBML`, `ID_DOC_TYPE`, `ID_SEGMENT`, `ID_INFO`, `ID_TRACKS`, `ID_TRACK_ENTRY`, `ID_VIDEO`, `ID_AUDIO`, `ID_CLUSTER`, ... | EBML / Matroska 标准 Element ID 常量 |
| `MkvTrackType` | 轨道类型枚举：`unknown=0`, `video=1`, `audio=2`, `complex=3`, `logo=16`, `subtitle=17`, `buttons=18`, `control=32` |
| `isMkv(data)` / `isWebm(data)` | 校验 EBML 魔数 `0x1A45DFA3` 及 `DocType`（`"matroska"` / `"webm"`） |
| `MkvInfo` | 容器信息：`doc_type`, `timecode_scale_ns`（默认 `1_000_000`）, `duration_ticks`, `muxing_app`, `writing_app`；附 `durationMs(): f64`、`isWebm(): bool` |
| `MkvTrack` | 轨道信息：`track_number`, `track_type`, `codec_id`, `pixel_width`, `pixel_height`, `channels`, `sampling_frequency` |
| `MkvTrackIter` | 零分配 `TrackEntry` 迭代器：`init(tracks_payload)`, `next(*_): MkvTrack?` |
| `MkvHeader` / `parseMkvHeader(data)` | 单遍解析 `EBML` + `Segment` 内的 `Info` 与 `Tracks` 切片，提供 `tracks(): MkvTrackIter` |

## 示例

```cp
import { isMkv, parseMkvHeader, MkvTrackType } from "cap:mkv";

fn inspectMkv(bytes: []const u8): u32 {
    if (!isMkv(bytes)) return 0;
    const hdr = parseMkvHeader(bytes) ?? return 0;
    let it = hdr.tracks();
    while (track := it.next()) {
        if (track.track_type == MkvTrackType.video) {
            return track.pixel_width;
        }
    }
    return 0;
}
```
