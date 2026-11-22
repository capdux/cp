// Source: Absorbed from /data/dev/bun/src/bake/DevServer/SerializedFailure.cp | Synced: 2026-11-22T15:03:00Z
// Pure-CP zero-allocation HMR DevServer Binary Error & Diagnostic Overlay Wire Codec:
// - Bit-packs `HmrFailureOwner` into a 32-bit opaque handle (`low 30b index | high 2b kind`)
//   for `None`, `Route`, `Client`, and `Server` incremental graph owners
// - Encodes/decodes all 12 `HmrFailureErrorKind` variants (5 bundler log severities + 7 JS errors)
// - Encodes/decodes `HmrFailureData` (UTF-8 message + optional 1-based line / 0-based column / length / line_text)
//   and nested diagnostic notes in exact Bun Bake `SerializedFailure` little-endian wire layout
// - Provides O(1) `peekOwner()` without full frame decoding and strict truncation/bounds guards.

/// 单条失败帧支持的最大诊断消息数
pub const HMR_FAILURE_MAX_MSGS: usize = 16;
/// 单条诊断消息支持的最大附注（note）数
pub const HMR_FAILURE_MAX_NOTES: usize = 8;
/// 30-bit 句柄索引上限（`2^30 - 1`）
pub const HMR_FAILURE_MAX_OWNER_INDEX: u32 = 0x3FFF_FFFF;

/// HMR 失败帧编解码错误集
pub err HmrFailureError {
    EmptyMessages,
    InvalidOwnerIndex,
    InvalidErrorKind,
    InvalidColumn,
    TooManyMessages,
    TooManyNotes,
    TruncatedFrame,
    BufferTooSmall,
}

/// 增量图错误归属类别（2-bit 标签，精确对齐 Bun `OwnerKind: u2`）
pub enum HmrFailureOwnerKind: u8 {
    None = 0,
    Route = 1,
    Client = 2,
    Server = 3,

    static fn fromU2(bits: u32): HmrFailureOwnerKind {
        return switch (bits & 0x3) {
            0 => .None,
            1 => .Route,
            2 => .Client,
            else => .Server,
        };
    }

    fn toU2(): u32 {
        return switch (_) {
            .None => 0,
            .Route => 1,
            .Client => 2,
            .Server => 3,
        };
    }
}

/// HMR 增量图错误归属句柄（32-bit 紧凑打包：低 30 位为索引，高 2 位为类别）
pub struct HmrFailureOwner {
    kind: HmrFailureOwnerKind = .None,
    index: u32 = 0,

    static fn none(): HmrFailureOwner {
        return HmrFailureOwner{ kind: .None, index: 0 };
    }

    static fn init(kind: HmrFailureOwnerKind, index: u32): HmrFailureOwner! {
        if (index > HMR_FAILURE_MAX_OWNER_INDEX) {
            return err.InvalidOwnerIndex;
        }
        return HmrFailureOwner{ kind: kind, index: index };
    }

    /// 打包为 32 位不透明句柄（与 Bun `OwnerPacked` 内存布局完全一致）
    fn pack(): u32 {
        return (_.index & HMR_FAILURE_MAX_OWNER_INDEX) | (_.kind.toU2() << 30);
    }

    /// 从 32 位不透明句柄解包
    static fn unpack(raw: u32): HmrFailureOwner {
        return HmrFailureOwner{
            kind: HmrFailureOwnerKind.fromU2(raw >> 30),
            index: raw & HMR_FAILURE_MAX_OWNER_INDEX,
        };
    }

    fn eql(other: HmrFailureOwner): bool {
        return _.pack() == other.pack();
    }
}

/// HMR 失败消息类型（完整对齐 Bun `FailureErrorKind: u8` 12 变体）
pub enum HmrFailureErrorKind: u8 {
    BundlerLogErr = 0,
    BundlerLogWarn = 1,
    BundlerLogNote = 2,
    BundlerLogDebug = 3,
    BundlerLogVerbose = 4,
    JsError = 5,
    JsErrorType = 6,
    JsErrorRange = 7,
    JsErrorExtra = 8,
    JsPrimitiveException = 9,
    JsPrimitive = 10,
    JsAggregate = 11,

    static fn fromByte(b: u8): HmrFailureErrorKind! {
        return switch (b) {
            0 => .BundlerLogErr,
            1 => .BundlerLogWarn,
            2 => .BundlerLogNote,
            3 => .BundlerLogDebug,
            4 => .BundlerLogVerbose,
            5 => .JsError,
            6 => .JsErrorType,
            7 => .JsErrorRange,
            8 => .JsErrorExtra,
            9 => .JsPrimitiveException,
            10 => .JsPrimitive,
            11 => .JsAggregate,
            else => err.InvalidErrorKind,
        };
    }

    fn toByte(): u8 {
        return switch (_) {
            .BundlerLogErr => 0,
            .BundlerLogWarn => 1,
            .BundlerLogNote => 2,
            .BundlerLogDebug => 3,
            .BundlerLogVerbose => 4,
            .JsError => 5,
            .JsErrorType => 6,
            .JsErrorRange => 7,
            .JsErrorExtra => 8,
            .JsPrimitiveException => 9,
            .JsPrimitive => 10,
            .JsAggregate => 11,
        };
    }

    fn isBundlerLog(): bool {
        return _.toByte() <= 4;
    }
}

/// 源码定位信息（行号 > 0 时有效，列号为 0-based 非负整数）
pub struct HmrSourceLoc {
    line: i32 = 0,
    column: u32 = 0,
    length: u32 = 0,
    line_text: []const u8 = "",
}

/// 单条诊断文本与可选源码位置（对齐 Bun `writeLogData`）
pub struct HmrFailureData {
    text: []const u8 = "",
    location: HmrSourceLoc? = null,
}

/// 单条 HMR 失败消息（含主诊断数据及最多 `HMR_FAILURE_MAX_NOTES` 条附注）
pub struct HmrFailureMsg {
    kind: HmrFailureErrorKind = .BundlerLogErr,
    data: HmrFailureData = HmrFailureData{},
    notes: [HMR_FAILURE_MAX_NOTES]HmrFailureData = [HmrFailureData{}; HMR_FAILURE_MAX_NOTES],
    note_count: usize = 0,

    fn addNote(*_, note: HmrFailureData): usize! {
        if (_.note_count >= HMR_FAILURE_MAX_NOTES) {
            return err.TooManyNotes;
        }
        _.notes[_.note_count] = note;
        _.note_count += 1;
        return _.note_count;
    }

    fn noteSlice(): []const HmrFailureData {
        return _.notes[0.._.note_count];
    }
}

/// 完整的 HMR 失败帧视图（零堆分配，字符串切片直指输入缓冲）
pub struct HmrFailureFrame {
    owner: HmrFailureOwner = HmrFailureOwner{},
    display_name: []const u8 = "",
    msgs: [HMR_FAILURE_MAX_MSGS]HmrFailureMsg = [HmrFailureMsg{}; HMR_FAILURE_MAX_MSGS],
    msg_count: usize = 0,

    static fn init(owner: HmrFailureOwner, display_name: []const u8): HmrFailureFrame {
        return HmrFailureFrame{
            owner: owner,
            display_name: display_name,
        };
    }

    fn addMsg(*_, msg: HmrFailureMsg): usize! {
        if (_.msg_count >= HMR_FAILURE_MAX_MSGS) {
            return err.TooManyMessages;
        }
        _.msgs[_.msg_count] = msg;
        _.msg_count += 1;
        return _.msg_count;
    }

    fn msgSlice(): []const HmrFailureMsg {
        return _.msgs[0.._.msg_count];
    }
}

/// 零堆分配 HMR 失败帧二进制编解码器
pub struct HmrFailureCodec {
    /// O(1) 读取帧头前 4 字节中的归属句柄（无需解码整帧）
    static fn peekOwner(wire: []const u8): HmrFailureOwner! {
        if (wire.len < 4) {
            return err.TruncatedFrame;
        }
        return HmrFailureOwner.unpack(readU32Le(wire, 0));
    }

    /// 将 `HmrFailureFrame` 编码为 Bun Bake 线协议字节流
    static fn encode(frame: HmrFailureFrame, out: []u8): usize! {
        if (frame.msg_count == 0) {
            return err.EmptyMessages;
        }
        let pos: usize = 0;
        pos = writeU32LeChecked(out, pos, frame.owner.pack())?;
        pos = writeString32(out, pos, frame.display_name)?;
        pos = writeU32LeChecked(out, pos, @as(u32, @intCast(frame.msg_count)))?;

        let i: usize = 0;
        while (i < frame.msg_count) {
            const m = frame.msgs[i];
            pos = writeByteChecked(out, pos, m.kind.toByte())?;
            pos = writeLogData(out, pos, m.data)?;
            pos = writeU32LeChecked(out, pos, @as(u32, @intCast(m.note_count)))?;
            let j: usize = 0;
            while (j < m.note_count) {
                pos = writeLogData(out, pos, m.notes[j])?;
                j += 1;
            }
            i += 1;
        }
        return pos;
    }

    /// 零拷贝解码 Bun Bake `SerializedFailure` 字节流
    static fn decode(wire: []const u8): HmrFailureFrame! {
        let pos: usize = 0;
        const owner_raw = readU32LeChecked(wire, *pos)?;
        const owner = HmrFailureOwner.unpack(owner_raw);
        const display_name = readString32(wire, *pos)?;
        const msg_count_u32 = readU32LeChecked(wire, *pos)?;
        if (msg_count_u32 == 0) {
            return err.EmptyMessages;
        }
        if (@as(usize, msg_count_u32) > HMR_FAILURE_MAX_MSGS) {
            return err.TooManyMessages;
        }

        let frame = HmrFailureFrame.init(owner, display_name);
        let i: u32 = 0;
        while (i < msg_count_u32) {
            const kb = readByteChecked(wire, *pos)?;
            const kind = HmrFailureErrorKind.fromByte(kb)?;
            const data = readLogData(wire, *pos)?;
            const note_count_u32 = readU32LeChecked(wire, *pos)?;
            if (@as(usize, note_count_u32) > HMR_FAILURE_MAX_NOTES) {
                return err.TooManyNotes;
            }
            let msg = HmrFailureMsg{ kind: kind, data: data };
            let j: u32 = 0;
            while (j < note_count_u32) {
                const nd = readLogData(wire, *pos)?;
                * = msg.addNote(nd)?;
                j += 1;
            }
            * = frame.addMsg(msg)?;
            i += 1;
        }
        return frame;
    }
}

// ----------------------------------------------------------------------------
// 内部零分配小端读写辅助
// ----------------------------------------------------------------------------

fn writeByteChecked(out: []u8, pos: usize, b: u8): usize! {
    if (pos >= out.len) return err.BufferTooSmall;
    out[pos] = b;
    return pos + 1;
}

fn writeU32LeChecked(out: []u8, pos: usize, v: u32): usize! {
    if (pos + 4 > out.len) return err.BufferTooSmall;
    out[pos] = @as(u8, @truncate(v));
    out[pos + 1] = @as(u8, @truncate(v >> 8));
    out[pos + 2] = @as(u8, @truncate(v >> 16));
    out[pos + 3] = @as(u8, @truncate(v >> 24));
    return pos + 4;
}

fn writeString32(out: []u8, pos: usize, s: []const u8): usize! {
    let p = writeU32LeChecked(out, pos, @as(u32, @intCast(s.len)))?;
    if (s.len > out.len - p) return err.BufferTooSmall;
    let i: usize = 0;
    while (i < s.len) {
        out[p + i] = s[i];
        i += 1;
    }
    return p + s.len;
}

fn writeLogData(out: []u8, pos: usize, data: HmrFailureData): usize! {
    let p = writeString32(out, pos, data.text)?;
    if (loc := data.location) {
        if (loc.line <= 0) {
            return writeU32LeChecked(out, p, 0);
        }
        p = writeU32LeChecked(out, p, @as(u32, @bitCast(loc.line)))?;
        p = writeU32LeChecked(out, p, loc.column)?;
        p = writeU32LeChecked(out, p, loc.length)?;
        p = writeString32(out, p, loc.line_text)?;
        return p;
    }
    return writeU32LeChecked(out, p, 0);
}

fn readU32Le(buf: []const u8, off: usize): u32 {
    return @as(u32, buf[off]) |
        (@as(u32, buf[off + 1]) << 8) |
        (@as(u32, buf[off + 2]) << 16) |
        (@as(u32, buf[off + 3]) << 24);
}

fn readByteChecked(buf: []const u8, pos: *usize): u8! {
    if (pos.* >= buf.len) return err.TruncatedFrame;
    const b = buf[pos.*];
    pos.* += 1;
    return b;
}

fn readU32LeChecked(buf: []const u8, pos: *usize): u32! {
    if (pos.* + 4 > buf.len) return err.TruncatedFrame;
    const v = readU32Le(buf, pos.*);
    pos.* += 4;
    return v;
}

fn readString32(buf: []const u8, pos: *usize): ([]const u8)! {
    const len_u32 = readU32LeChecked(buf, pos)?;
    const len = @as(usize, len_u32);
    if (len > buf.len - pos.*) return err.TruncatedFrame;
    const s = buf[pos.* .. pos.* + len];
    pos.* += len;
    return s;
}

fn readLogData(buf: []const u8, pos: *usize): HmrFailureData! {
    const text = readString32(buf, pos)?;
    const raw_line = readU32LeChecked(buf, pos)?;
    if (raw_line == 0) {
        return HmrFailureData{ text: text, location: null };
    }
    const line_i32 = @as(i32, @bitCast(raw_line));
    const col = readU32LeChecked(buf, pos)?;
    const len = readU32LeChecked(buf, pos)?;
    const line_text = readString32(buf, pos)?;
    return HmrFailureData{
        text: text,
        location: HmrSourceLoc{
            line: line_i32,
            column: col,
            length: len,
            line_text: line_text,
        },
    };
}
