# `cap:srt` — 零分配 SubRip (`.srt`) 字幕解析、时间戳编解码与格式化标准库

> [!NOTE]
> 本子模块已归并至统一主包 `cap:media`，导入请使用 `import { ... } from "cap:media"`。

`cap:srt` 提供零堆分配的 SubRip 字幕流式解析、标准 `HH:MM:SS,mmm` 时间戳编解码、时间轴平移以及规范化 SRT 块序列化能力，与 `cap:vtt` 配套覆盖主流字幕格式。

## 核心 API

### 数据结构
- `SrtCue`：`seq: u32`, `start_ms: u64`, `end_ms: u64`, `text: []const u8`
  - `durationMs(): u64` — 饱和减法返回持续毫秒数

### 时间戳与格式化函数
- `parseSrtTimestampMs(s: []const u8): u64?` — 解析 `HH:MM:SS,mmm`（兼容 `.` 毫秒分隔符，严格校验分/秒 `< 60` 与 3 位毫秒）
- `formatSrtTimestampMs(ms: u64, out: []u8): ([]const u8)?` — 输出标准 `HH:MM:SS,mmm`，缓冲不足返回 `null`
- `formatSrtCue(cue: SrtCue, out: []u8): ([]const u8)?` — 输出规范化 SRT 块，缓冲不足返回 `null`
- `shiftSrtCue(cue: SrtCue, offset_ms: i64): SrtCue` — 整体平移时间轴，负偏移饱和钳位至 `0`

### 流式迭代器
- `SrtCueIter`：
  - `SrtCueIter.init(input: []const u8): SrtCueIter` — 支持 UTF-8 BOM 与 `\r\n`/`\n`
  - `next(*_): SrtCue?` — 零分配逐条解析序号、时间轴与多行正文，自动跳过空块与畸形块

## 使用示例

```cp
import { SrtCueIter, shiftSrtCue, formatSrtCue } from "cap:media";

fn demo(src: []const u8) {
    let it = SrtCueIter.init(src);
    while (cue := it.next()) {
        const shifted = shiftSrtCue(cue, 500);
        let buf: [512]u8 = undefined;
        if (formatted := formatSrtCue(shifted, &buf)) {
            * = formatted;
        }
    }
}
```
