# `cap:id3` — 纯 CP MP3 ID3v1 / ID3v2.3 / ID3v2.4 零分配音频元数据解析库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

## 概述

`cap:id3` 提供纯 CP、零外部依赖、零堆分配的 MP3 音频标签解析器，支持 ID3v2 28-bit Syncsafe 整数解码、ID3v2.3/v2.4 头部解析与帧流式迭代（自动区分 v2.3 普通大端帧长与 v2.4 Syncsafe 帧长，自动跳过扩展头并在零填充区安全停止）、`T***` 文本帧零拷贝提取以及文件尾 128 字节 ID3v1/ID3v1.1 标签解析。

## 核心 API

| 符号 | 说明 |
| :--- | :--- |
| `Id3v2Header` | 10 字节 `"ID3"` 头部：`version_major`, `version_minor`, `flags`, `tag_size`；方法：`totalBytes()` |
| `Id3v2Frame` | 零拷贝帧：`id: [4]u8`, `size`, `flags`, `payload`；方法：`idEquals(other)`, `textContent()` |
| `Id3v1Tag` | 文件尾 128 字节 `"TAG"`：`title`, `artist`, `album`, `year`, `genre`, `track: u8?` |
| `decodeSyncsafeU32(bytes)` | 校验每字节最高位为 0 并解码 28-bit syncsafe 整数 |
| `parseId3v2Header(data)` | 解析 10 字节 ID3v2 头部 |
| `Id3v2FrameIter` | 帧迭代器：`init(data)`、`next(*_): Id3v2Frame?` |
| `parseId3v1Tail(data)` | 解析文件末尾 128 字节 ID3v1/ID3v1.1 标签 |

## 示例

```cp
import { Id3v2FrameIter, parseId3v1Tail } from "cap:media";

fn readTitle(mp3_bytes: []const u8): ([]const u8)? {
    let it = Id3v2FrameIter.init(mp3_bytes);
    while (frame := it.next()) {
        if (frame.idEquals("TIT2")) return frame.textContent();
    }
    if (v1 := parseId3v1Tail(mp3_bytes)) {
        return v1.title;
    }
    return null;
}
```
