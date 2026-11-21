# `cap:vtt` — 纯 CP W3C WebVTT 字幕与时间轴零分配解析/格式化库

## 概述

`cap:vtt` 提供纯 CP、零外部依赖、零堆分配的 W3C WebVTT 字幕文件头校验、时间戳编解码、流式 Cue 迭代器（支持 `NOTE`/`STYLE`/`REGION` 块过滤与多行正文）以及规范化 Cue 格式化器。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `VttCue` | 字幕条目：`id`, `start_ms`, `end_ms`, `settings`, `payload` 及 `durationMs(): u64` |
| `isWebVttHeader(input)` | 校验 WebVTT 文件头（支持可选 UTF-8 BOM，首行以 `"WEBVTT"` 开头） |
| `parseVttTimestampMs(s)` | 解析 `MM:SS.mmm` 与 `HH:MM:SS.mmm` 为毫秒，严格校验分/秒 `< 60` 与 3 位毫秒 |
| `formatVttTimestampMs(ms, out)` | 零分配输出 `HH:MM:SS.mmm`，缓冲不足返回 `null` |
| `VttCueIter` | 流式 Cue 迭代器：`init(input)`、`next(*_): VttCue?`，跳过 `WEBVTT`/`NOTE`/`STYLE`/`REGION` 块 |
| `formatVttCue(cue, out)` | 规范化序列化完整 WebVTT Cue 块，缓冲不足返回 `null` |

## 示例

```cp
import { isWebVttHeader, VttCueIter, formatVttCue } from "cap:vtt";

fn example(vtt: []const u8): bool {
    if (!isWebVttHeader(vtt)) return false;
    let it = VttCueIter.init(vtt);
    if (cue := it.next()) {
        let buf: [256]u8 = undefined;
        return cue.durationMs() > 0 && formatVttCue(cue, buf[0..]) != null;
    }
    return false;
}
```
