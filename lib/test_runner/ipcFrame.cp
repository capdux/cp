// Source: Absorbed from /data/dev/bun/src/cli/test/parallel/Frame.cp | Synced: 2026-11-22T03:52:30Z
// Pure-CP zero-allocation parallel test-runner IPC binary framing codec:
// - Wire format: `[u32 LE payload_len][u8 kind][payload]`
// - Embedded string format: `[u32 LE byte_len][bytes]`
// - 64 MiB single-frame upper bound prevents `5 + len` u32 overflow & unbounded memory spikes
// - Safe truncating string encoder (`writeStrTruncating`) + zero-copy `IpcPayloadReader`
//   and strict checked decoders (`decodeIpcFrame`, `readU32Checked`, `readStrChecked`).

/// IPC 帧头固定长度（4 字节小端 payload_len + 1 字节 kind）
pub const IPC_FRAME_HEADER_SIZE: usize = 5;
/// 单帧最大载荷上限（64 MiB，对齐 Bun `Frame.max_payload`）
pub const IPC_FRAME_MAX_PAYLOAD: u32 = 64 * 1024 * 1024;
/// 超长字符串截断后缀标记
pub const IPC_TRUNC_MARKER: []const u8 = "\n... [output truncated]\n";

/// 并行测试 IPC 帧编解码错误集
pub err IpcFrameError {
    BufferTooSmall,
    TruncatedFrame,
    PayloadTooLarge,
    InvalidKind,
    FrameNotStarted,
}

/// 并行测试 Worker <-> Coordinator 消息类型（对齐 Bun `Frame.Kind`）
pub enum IpcFrameKind: u8 {
    // worker -> coordinator
    ready = 0,
    file_start = 1,
    test_done = 2,
    file_done = 3,
    repeat_bufs = 4,
    junit_file = 5,
    coverage_file = 6,
    // coordinator -> worker
    run = 7,
    shutdown = 8,

    /// 从原始字节安全转换为 `IpcFrameKind`（未知枚举值返回 `null`）
    static fn fromU8(v: u8): IpcFrameKind? {
        return switch (v) {
            0 => IpcFrameKind.ready,
            1 => IpcFrameKind.file_start,
            2 => IpcFrameKind.test_done,
            3 => IpcFrameKind.file_done,
            4 => IpcFrameKind.repeat_bufs,
            5 => IpcFrameKind.junit_file,
            6 => IpcFrameKind.coverage_file,
            7 => IpcFrameKind.run,
            8 => IpcFrameKind.shutdown,
            else => null,
        };
    }
}

/// 零拷贝解码后的 IPC 帧视图
pub struct IpcDecodedFrame {
    kind: IpcFrameKind = .ready,
    payload: []const u8 = "",
    total_bytes: usize = 0,
}

fn readLeU32(s: []const u8): u32 {
    return @as(u32, s[0]) |
        (@as(u32, s[1]) << 8) |
        (@as(u32, s[2]) << 16) |
        (@as(u32, s[3]) << 24);
}

fn writeLeU32(out: []u8, at: usize, v: u32) {
    out[at] = @intCast(v & 0xff);
    out[at + 1] = @intCast((v >> 8) & 0xff);
    out[at + 2] = @intCast((v >> 16) & 0xff);
    out[at + 3] = @intCast((v >> 24) & 0xff);
}

/// 零拷贝载荷读取器（对齐 Bun `Frame.Reader`）
pub struct IpcPayloadReader {
    p: []const u8 = "",

    /// 初始化载荷读取器
    static fn init(payload: []const u8): IpcPayloadReader {
        return IpcPayloadReader{ p: payload };
    }

    /// 返回剩余未读字节数
    fn remaining(): usize {
        return _.p.len;
    }

    /// 读取小端 `u32`（对齐 Bun `u32_()`：截断时安全返回 0）
    fn readU32(*_): u32 {
        if (_.p.len < 4) {
            return 0;
        }
        const v = readLeU32(_.p[0..4]);
        _.p = _.p[4..];
        return v;
    }

    /// 严格模式读取小端 `u32`（不足 4 字节返回 `TruncatedFrame`）
    fn readU32Checked(*_): u32! {
        if (_.p.len < 4) {
            return err.TruncatedFrame;
        }
        const v = readLeU32(_.p[0..4]);
        _.p = _.p[4..];
        return v;
    }

    /// 读取长度前缀字符串（对齐 Bun `str()`：截断时安全返回 `""`）
    fn readStr(*_): []const u8 {
        const n: usize = @as(usize, _.readU32());
        if (_.p.len < n) {
            return "";
        }
        const s = _.p[0..n];
        _.p = _.p[n..];
        return s;
    }

    /// 严格模式读取长度前缀字符串（长度头或内容不足时原子保持游标并返回 `TruncatedFrame`）
    fn readStrChecked(*_): ([]const u8)! {
        if (_.p.len < 4) {
            return err.TruncatedFrame;
        }
        const n: usize = @as(usize, readLeU32(_.p[0..4]));
        if (_.p.len - 4 < n) {
            return err.TruncatedFrame;
        }
        const s = _.p[4 .. 4 + n];
        _.p = _.p[4 + n ..];
        return s;
    }
}

/// 零堆分配并行测试 IPC 帧编码器（对齐 Bun `Frame`）
pub struct IpcFrameWriter {
    buf: []u8,
    pos: usize = 0,
    started: bool = false,
    max_payload: u32 = IPC_FRAME_MAX_PAYLOAD,

    /// 使用默认 64 MiB 载荷上限初始化帧编码器
    static fn init(buf: []u8): IpcFrameWriter {
        return IpcFrameWriter{
            buf: buf,
            pos: 0,
            started: false,
            max_payload: IPC_FRAME_MAX_PAYLOAD,
        };
    }

    /// 使用自定义单帧载荷上限初始化（便于窄缓冲或回归测试）
    static fn initWithLimit(buf: []u8, max_payload: u32): IpcFrameWriter {
        const clamped = if (max_payload > IPC_FRAME_MAX_PAYLOAD) IPC_FRAME_MAX_PAYLOAD else max_payload;
        return IpcFrameWriter{
            buf: buf,
            pos: 0,
            started: false,
            max_payload: clamped,
        };
    }

    /// 开始构造新帧：清零 4 字节长度占位并写入 1 字节 `kind`（对齐 Bun `begin()`）
    fn begin(*_, kind: IpcFrameKind): usize! {
        if (_.buf.len < IPC_FRAME_HEADER_SIZE) {
            return err.BufferTooSmall;
        }
        writeLeU32(_.buf, 0, 0);
        _.buf[4] = @intFromEnum(kind);
        _.pos = IPC_FRAME_HEADER_SIZE;
        _.started = true;
        return IPC_FRAME_HEADER_SIZE;
    }

    /// 返回当前已写入的载荷净字节数（不含 5 字节帧头）
    fn payloadLen(): usize {
        if (!_.started || _.pos < IPC_FRAME_HEADER_SIZE) {
            return 0;
        }
        return _.pos - IPC_FRAME_HEADER_SIZE;
    }

    /// 写入 4 字节小端 `u32`（对齐 Bun `u32_()`）
    fn writeU32(*_, v: u32): usize! {
        if (!_.started) {
            return err.FrameNotStarted;
        }
        if (_.payloadLen() + 4 > @as(usize, _.max_payload)) {
            return err.PayloadTooLarge;
        }
        if (_.buf.len - _.pos < 4) {
            return err.BufferTooSmall;
        }
        writeLeU32(_.buf, _.pos, v);
        _.pos += 4;
        return 4;
    }

    /// 写入完整长度前缀字符串 `[u32 LE len][bytes]`（超限返回错误）
    fn writeStr(*_, s: []const u8): usize! {
        if (!_.started) {
            return err.FrameNotStarted;
        }
        const cur_payload = _.payloadLen();
        if (s.len > @as(usize, _.max_payload) || cur_payload + 4 + s.len > @as(usize, _.max_payload)) {
            return err.PayloadTooLarge;
        }
        if (_.buf.len - _.pos < 4 + s.len) {
            return err.BufferTooSmall;
        }
        writeLeU32(_.buf, _.pos, @intCast(s.len));
        _.pos += 4;
        let i: usize = 0;
        while (i < s.len) {
            _.buf[_.pos + i] = s[i];
            i += 1;
        }
        _.pos += s.len;
        return 4 + s.len;
    }

    /// 安全截断模式写入字符串（对齐 Bun `Frame.str()`：空间不足时保留前缀并追加 `IPC_TRUNC_MARKER`，避免破坏 IPC 通道）
    fn writeStrTruncating(*_, s: []const u8): usize! {
        if (!_.started) {
            return err.FrameNotStarted;
        }
        const cur_payload = _.payloadLen();
        const max_p: usize = @as(usize, _.max_payload);
        const cap_by_limit: usize = if (max_p > cur_payload + 4) max_p - cur_payload - 4 else 0;
        const cap_by_buf: usize = if (_.buf.len > _.pos + 4) _.buf.len - _.pos - 4 else 0;
        const room: usize = if (cap_by_limit < cap_by_buf) cap_by_limit else cap_by_buf;

        if (_.buf.len - _.pos < 4 || max_p < cur_payload + 4) {
            return err.BufferTooSmall;
        }

        if (s.len <= room) {
            return _.writeStr(s);
        }

        if (room >= IPC_TRUNC_MARKER.len) {
            const keep = room - IPC_TRUNC_MARKER.len;
            const total_len = keep + IPC_TRUNC_MARKER.len;
            writeLeU32(_.buf, _.pos, @intCast(total_len));
            _.pos += 4;
            let i: usize = 0;
            while (i < keep) {
                _.buf[_.pos + i] = s[i];
                i += 1;
            }
            _.pos += keep;
            let j: usize = 0;
            while (j < IPC_TRUNC_MARKER.len) {
                _.buf[_.pos + j] = IPC_TRUNC_MARKER[j];
                j += 1;
            }
            _.pos += IPC_TRUNC_MARKER.len;
            return 4 + total_len;
        }

        // Room is smaller than marker itself: write truncated prefix up to `room` bytes
        writeLeU32(_.buf, _.pos, @intCast(room));
        _.pos += 4;
        let k: usize = 0;
        while (k < room) {
            _.buf[_.pos + k] = s[k];
            k += 1;
        }
        _.pos += room;
        return 4 + room;
    }

    /// 完成编码并回填 4 字节小端 `payload_len`，返回完整帧只读切片（对齐 Bun `finish()`）
    fn finish(*_): ([]const u8)! {
        if (!_.started || _.pos < IPC_FRAME_HEADER_SIZE) {
            return err.FrameNotStarted;
        }
        const plen = _.pos - IPC_FRAME_HEADER_SIZE;
        if (plen > @as(usize, _.max_payload)) {
            return err.PayloadTooLarge;
        }
        writeLeU32(_.buf, 0, @intCast(plen));
        return _.buf[0.._.pos];
    }
}

/// 零拷贝解码单帧并行测试 IPC 报文（校验 5 字节头、64 MiB 载荷上限、合法 Kind 与截断防护）
pub fn decodeIpcFrame(wire: []const u8): IpcDecodedFrame! {
    if (wire.len < IPC_FRAME_HEADER_SIZE) {
        return err.TruncatedFrame;
    }
    const payload_len_u32 = readLeU32(wire[0..4]);
    if (payload_len_u32 > IPC_FRAME_MAX_PAYLOAD) {
        return err.PayloadTooLarge;
    }
    const kind = IpcFrameKind.fromU8(wire[4]) ?? return err.InvalidKind;
    const payload_len: usize = @as(usize, payload_len_u32);
    if (wire.len - IPC_FRAME_HEADER_SIZE < payload_len) {
        return err.TruncatedFrame;
    }
    return IpcDecodedFrame{
        kind: kind,
        payload: wire[IPC_FRAME_HEADER_SIZE .. IPC_FRAME_HEADER_SIZE + payload_len],
        total_bytes: IPC_FRAME_HEADER_SIZE + payload_len,
    };
}
