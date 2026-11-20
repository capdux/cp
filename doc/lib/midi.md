# CP 标准库：Standard MIDI File (SMF) 编解码引擎手册 (`cap:midi` / `cap.midi`)

## 一、 架构总览与设计准则

`cap:midi` 是 CP 语言官方内置的纯原生 Standard MIDI File（SMF Format 0 / 1 / 2）编解码与时间换算标准库，完整支持 7-bit 大端序可变长数量（VLQ）、MIDI Running Status 状态复用与清除规则、通道音色/控制/弯音事件、Meta Tempo 与 SysEx 载荷往返。

全模块**零 C 依赖、零 `std.posix`、零 `@import("bun")`**，支持 `import ... from "cap:midi"` 与 `import cap from "cap"; cap.midi.*` 编译期自动惰性挂载。

### 模块文件结构

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/midi/midi.cp` | `// Source: cp/lib/midi/midi.cp (Pure CP Standard MIDI File Format 0/1 Parser, VLQ, Running Status & Builder)` | SMF Format 0/1/2 解析器、编码器、VLQ 编解码与 Running Status 状态机 |
| `lib/midi/index.cp` | `// Source: cp/lib/midi/index.cp (Unified Exports for cap:midi & cap.midi)` | `cap:midi` 与 `cap.midi` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误集与格式枚举

- **`MidiError`**：
  - `InvalidHeader`：`MThd` 魔数缺失、头长度非 `6`、Format 0 音轨数非 `1` 或 `ticks_per_quarter == 0`
  - `InvalidTrackHeader`：`MTrk` 块魔数缺失
  - `UnexpectedEof`：头部、音轨载荷、VLQ 或事件数据字节意外截断
  - `InvalidVlq`：超过 4 字节或数值超出 `0x0FFFFFFF` 的畸形 VLQ
  - `InvalidStatusByte`：无有效 Running Status 的裸数据字节、非法状态字节或数据字节最高位为 `1`
  - `UnsupportedFormat`：不支持的 SMF 格式（非 `0/1/2`）或 SMPTE 时间码分度（最高位为 `1`）
  - `OutOfMemory`：分配器内存不足
- **`MidiFormat` (`u16`)**：
  - `SingleTrack = 0`（Format 0 单轨）
  - `MultiTrackSync = 1`（Format 1 多轨同步）
  - `MultiTrackAsync = 2`（Format 2 多轨异步）

### 2. 事件与容器类型

- **`MidiEventKind`（Tagged Union）**：
  - `note_off: NoteEventData`（`channel: u4, note: u7, velocity: u7`）
  - `note_on: NoteEventData`（`channel: u4, note: u7, velocity: u7`）
  - `poly_pressure: PolyPressureData`（`channel: u4, note: u7, pressure: u7`）
  - `control_change: ControlChangeData`（`channel: u4, controller: u7, value: u7`）
  - `program_change: ProgramChangeData`（`channel: u4, program: u7`）
  - `channel_pressure: ChannelPressureData`（`channel: u4, pressure: u7`）
  - `pitch_bend: PitchBendData`（`channel: u4, value: u14`）
  - `meta_tempo: u32`（`us_per_quarter`，每四分音符微秒数）
  - `meta_end_of_track: void`（`FF 2F 00`）
  - `meta_other: MetaOtherData`（`meta_type: u8, data: []const u8`）
  - `sysex: []const u8`
- **`MidiEvent`**：`delta_ticks: u32`、`kind: MidiEventKind`
- **`MidiTrack`**：`events: []MidiEvent`
- **`MidiFile`**：
  - 字段：`format: MidiFormat`、`ticks_per_quarter: u16`、`tracks: []MidiTrack`
  - `deinit(*_, allocator: std.mem.Allocator)`：释放所有音轨、事件数组及深拷贝的 `meta_other.data` / `sysex` 切片
  - `totalTicks(): u64`：计算所有音轨中的最大累计 tick 时长
  - `ticksToMs(ticks: u64, us_per_quarter: u32): f64`：将 tick 数换算为毫秒

### 3. 编解码函数

- **`parse(allocator: std.mem.Allocator, smf_bytes: []const u8): MidiError!MidiFile`**：
  - 严格校验 `MThd` 与 `MTrk` 块头及字节边界
  - 解码 1..4 字节大端序 7-bit VLQ（拦截 `> 0x0FFFFFFF`）
  - 完整支持 MIDI Running Status 复用前一条通道状态字节，并在遇到 SysEx（`0xF0`/`0xF7`）或 Meta（`0xFF`）事件时按 MIDI 1.0 规范清除 Running Status
- **`encode(allocator: std.mem.Allocator, midi_file: MidiFile): MidiError![]u8`**：
  - 规范输出 `MThd` + 各 `MTrk` 块及 VLQ 时间增量
  - 若音轨末尾未显式包含 `meta_end_of_track`，自动补齐 `00 FF 2F 00`

---

## 三、 使用示例

```typescript
const std = @import("std");
import cap from "cap";
import {
    MidiFile,
    MidiTrack,
    MidiEvent,
    MidiEventKind,
    MidiFormat,
    NoteEventData,
    parse,
    encode,
} from "cap:midi";

pub fn buildAndInspect(alloc: std.mem.Allocator): !void {
    let evs: [2]MidiEvent = [
        MidiEvent {
            delta_ticks: 0,
            kind: MidiEventKind { note_on: NoteEventData { channel: 0, note: 60, velocity: 100 } },
        },
        MidiEvent {
            delta_ticks: 480,
            kind: MidiEventKind { note_off: NoteEventData { channel: 0, note: 60, velocity: 0 } },
        },
    ];
    let trs: [1]MidiTrack = [MidiTrack { events: &evs }];
    const mf = MidiFile {
        format: MidiFormat.SingleTrack,
        ticks_per_quarter: 480,
        tracks: &trs,
    };

    const bytes = encode(alloc, mf)?;
    defer alloc.free(bytes);

    let parsed = parse(alloc, bytes)?;
    defer parsed.deinit(alloc);

    const ms = parsed.ticksToMs(parsed.totalTicks(), 500_000);
    * = ms;
    * = cap.midi.MidiFormat.SingleTrack;
}
```
