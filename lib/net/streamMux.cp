// Origin: Pure-CP Stdlib (Yamux-Style Zero-Heap Binary Stream Multiplexing Codec & Flow-Control Session) | Created: 2026-11-22
// Pure CP zero-heap-allocation binary stream multiplexer for high-throughput RPC and cross-node AI Agent comms:
//   1. 12-byte big-endian wire codec (`MUX_PROTO_VERSION = 0`, `MUX_HEADER_LEN = 12`):
//      `MuxFrameType` (`Data | WindowUpdate | Ping | GoAway`), `MuxFlags` (`SYN`/`ACK`/`FIN`/`RST`),
//      `MuxFrameHeader`, `MuxDecodedFrame`, `MuxCodecError`,
//      `encodeMuxFrameHeader`, `decodeMuxFrameHeader`, `encodeMuxFrame`, `decodeMuxFrame`
//   2. Bidirectional flow-control & lifecycle state machine (`MuxSession`):
//      - Client-odd / Server-even stream ID allocation with concurrent stream limit
//      - Strict `send_window` / `recv_window` credit tracking & `WindowUpdate` replenishment
//      - Half-close (`LocalHalfClosed` / `RemoteHalfClosed`), `RST`, and graceful `GoAway` drain

const std = @import("std");

/// Yamux-style binary multiplexing protocol version.
pub const MUX_PROTO_VERSION: u8 = 0;

/// Fixed big-endian frame header size in bytes.
pub const MUX_HEADER_LEN: usize = 12;

/// Default initial per-stream credit window (256 KiB).
pub const MUX_DEFAULT_INITIAL_WINDOW: u32 = 262144;

/// Standard GoAway status codes.
pub const MUX_GOAWAY_NORMAL: u32 = 0;
pub const MUX_GOAWAY_PROTOCOL_ERROR: u32 = 1;
pub const MUX_GOAWAY_INTERNAL_ERROR: u32 = 2;

/// Multiplexed wire frame type tag.
pub enum MuxFrameType {
    Data,
    WindowUpdate,
    Ping,
    GoAway,
}

/// Bitmask flags carried in the 16-bit frame header flags field.
pub struct MuxFlags {
    const NONE: u16 = 0x0000;
    const SYN: u16 = 0x0001;
    const ACK: u16 = 0x0002;
    const FIN: u16 = 0x0004;
    const RST: u16 = 0x0008;
    const VALID_MASK: u16 = 0x000F;
}

/// Wire codec error set for `stream_mux.cp`.
pub err MuxCodecError {
    TruncatedHeader,
    TruncatedPayload,
    UnsupportedVersion,
    UnknownFrameType,
    InvalidFlags,
    InvalidStreamId,
    BufferTooSmall,
    PayloadNotAllowed,
}

/// Parsed 12-byte big-endian multiplexing frame header.
pub struct MuxFrameHeader {
    version: u8 = MUX_PROTO_VERSION;
    frame_type: MuxFrameType = MuxFrameType.Data;
    flags: u16 = 0;
    stream_id: u32 = 0;
    length: u32 = 0;
}

/// Zero-copy decoded frame view returned by `decodeMuxFrame`.
pub struct MuxDecodedFrame {
    header: MuxFrameHeader;
    payload: []const u8;
    total_bytes: usize = 0;
}

fn frameTypeToU8(ft: MuxFrameType): u8 {
    if (ft == MuxFrameType.Data) return 0;
    if (ft == MuxFrameType.WindowUpdate) return 1;
    if (ft == MuxFrameType.Ping) return 2;
    return 3;
}

fn frameTypeFromU8(raw: u8): MuxFrameType? {
    if (raw == 0) return MuxFrameType.Data;
    if (raw == 1) return MuxFrameType.WindowUpdate;
    if (raw == 2) return MuxFrameType.Ping;
    if (raw == 3) return MuxFrameType.GoAway;
    return null;
}

fn writeU16BE(buf: *[]u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeU32BE(buf: *[]u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn readU16BE(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readU32BE(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24) |
        ((buf[off + 1] as u32) << 16) |
        ((buf[off + 2] as u32) << 8) |
        (buf[off + 3] as u32);
}

fn validateHeaderFields(hdr: MuxFrameHeader): bool ! MuxCodecError {
    if (hdr.version != MUX_PROTO_VERSION) {
        return MuxCodecError.UnsupportedVersion;
    }
    if ((hdr.flags & (~MuxFlags.VALID_MASK)) != 0) {
        return MuxCodecError.InvalidFlags;
    }
    if (hdr.frame_type == MuxFrameType.Ping || hdr.frame_type == MuxFrameType.GoAway) {
        if (hdr.stream_id != 0) return MuxCodecError.InvalidStreamId;
    } else {
        if (hdr.stream_id == 0) return MuxCodecError.InvalidStreamId;
    }
    return true;
}

/// Encodes a 12-byte big-endian `MuxFrameHeader` into `out_buf[0..12]`.
pub fn encodeMuxFrameHeader(hdr: MuxFrameHeader, out_buf: *[]u8): usize ! MuxCodecError {
    * = validateHeaderFields(hdr)?;
    if (out_buf.len < MUX_HEADER_LEN) {
        return MuxCodecError.BufferTooSmall;
    }
    out_buf[0] = hdr.version;
    out_buf[1] = frameTypeToU8(hdr.frame_type);
    writeU16BE(out_buf, 2, hdr.flags);
    writeU32BE(out_buf, 4, hdr.stream_id);
    writeU32BE(out_buf, 8, hdr.length);
    return MUX_HEADER_LEN;
}

/// Decodes and validates a 12-byte big-endian `MuxFrameHeader` from `data`.
pub fn decodeMuxFrameHeader(data: []const u8): MuxFrameHeader ! MuxCodecError {
    if (data.len < MUX_HEADER_LEN) {
        return MuxCodecError.TruncatedHeader;
    }
    const ver: u8 = data[0];
    if (ver != MUX_PROTO_VERSION) {
        return MuxCodecError.UnsupportedVersion;
    }
    const ft: MuxFrameType = frameTypeFromU8(data[1]) ?? return MuxCodecError.UnknownFrameType;
    const flags: u16 = readU16BE(data, 2);
    const sid: u32 = readU32BE(data, 4);
    const len: u32 = readU32BE(data, 8);

    const hdr = MuxFrameHeader{
        version: ver,
        frame_type: ft,
        flags: flags,
        stream_id: sid,
        length: len,
    };
    * = validateHeaderFields(hdr)?;
    return hdr;
}

/// Encodes a full multiplexing frame (12B big-endian header + optional Data payload) into `out_buf`.
/// For `Data` frames, `hdr.length` is overwritten with `payload.len`.
/// For control frames (`WindowUpdate`, `Ping`, `GoAway`), `payload` must be empty.
pub fn encodeMuxFrame(hdr: MuxFrameHeader, payload: []const u8, out_buf: *[]u8): usize ! MuxCodecError {
    if (hdr.frame_type == MuxFrameType.Data) {
        if (payload.len > (std.math.maxInt(u32) as usize)) {
            return MuxCodecError.BufferTooSmall;
        }
        if (out_buf.len < MUX_HEADER_LEN || out_buf.len - MUX_HEADER_LEN < payload.len) {
            return MuxCodecError.BufferTooSmall;
        }
        const eff_hdr = MuxFrameHeader{
            version: hdr.version,
            frame_type: hdr.frame_type,
            flags: hdr.flags,
            stream_id: hdr.stream_id,
            length: payload.len as u32,
        };
        * = encodeMuxFrameHeader(eff_hdr, out_buf)?;
        let i: usize = 0;
        while (i < payload.len) : (i += 1) {
            out_buf[MUX_HEADER_LEN + i] = payload[i];
        }
        return MUX_HEADER_LEN + payload.len;
    }

    if (payload.len > 0) {
        return MuxCodecError.PayloadNotAllowed;
    }
    return encodeMuxFrameHeader(hdr, out_buf);
}

/// Zero-copy decodes a complete multiplexing frame from `data`.
/// For `Data` frames, returns a slice into `data[12 .. 12 + hdr.length]`.
/// For control frames, returns an empty payload slice and `total_bytes = 12`.
pub fn decodeMuxFrame(data: []const u8): MuxDecodedFrame ! MuxCodecError {
    const hdr = decodeMuxFrameHeader(data)?;
    if (hdr.frame_type == MuxFrameType.Data) {
        const plen: usize = hdr.length as usize;
        if (data.len - MUX_HEADER_LEN < plen) {
            return MuxCodecError.TruncatedPayload;
        }
        return MuxDecodedFrame{
            header: hdr,
            payload: data[MUX_HEADER_LEN .. MUX_HEADER_LEN + plen],
            total_bytes: MUX_HEADER_LEN + plen,
        };
    }
    return MuxDecodedFrame{
        header: hdr,
        payload: data[0..0],
        total_bytes: MUX_HEADER_LEN,
    };
}

// ============================================================================
// Multiplexing Session & Bidirectional Flow-Control State Machine
// ============================================================================

/// Stream lifecycle state supporting full-duplex half-close semantics.
pub enum MuxStreamState {
    Open,
    LocalHalfClosed,
    RemoteHalfClosed,
    Closed,
}

/// Per-stream state and credit window descriptor stored in caller-provided buffer.
pub struct MuxStreamSlot {
    active: bool = false;
    stream_id: u32 = 0;
    state: MuxStreamState = MuxStreamState.Closed;
    send_window: u32 = 0;
    recv_window: u32 = 0;
    bytes_sent: u64 = 0;
    bytes_received: u64 = 0;
}

/// Telemetry snapshot for `MuxSession`.
pub struct MuxSessionStats {
    active_streams: usize = 0;
    max_concurrent_streams: usize = 0;
    local_goaway: bool = false;
    remote_goaway: bool = false;
    goaway_code: u32 = 0;
    total_streams_opened: u64 = 0;
    total_bytes_sent: u64 = 0;
    total_bytes_received: u64 = 0;
}

/// Zero-heap-allocation Yamux-style multiplexed session with odd/even stream IDs,
/// bidirectional credit flow control, half-close/RST handling, and graceful GoAway drain.
pub struct MuxSession {
    streams: []MuxStreamSlot;
    is_client: bool = true;
    next_stream_id: u32 = 1;
    max_concurrent_streams: usize = 0;
    initial_window: u32 = MUX_DEFAULT_INITIAL_WINDOW;
    active_count: usize = 0;
    local_goaway: bool = false;
    remote_goaway: bool = false;
    goaway_code: u32 = 0;
    total_streams_opened: u64 = 0;
    total_bytes_sent: u64 = 0;
    total_bytes_received: u64 = 0;

    /// Initializes a multiplexed session over caller-provided `streams_buf`.
    /// - `is_client = true` allocates odd stream IDs (`1, 3, 5, ...`); `false` allocates even (`2, 4, 6, ...`).
    /// Returns `null` on invalid capacity or zero `initial_window`.
    static fn init(
        streams_buf: *[]MuxStreamSlot,
        is_client: bool,
        max_concurrent_streams: usize,
        initial_window: u32,
    ): MuxSession? {
        if (streams_buf.len == 0 || max_concurrent_streams == 0 || max_concurrent_streams > streams_buf.len) {
            return null;
        }
        if (initial_window == 0) return null;

        let i: usize = 0;
        while (i < streams_buf.len) : (i += 1) {
            streams_buf[i] = MuxStreamSlot{};
        }

        return MuxSession{
            streams: streams_buf,
            is_client: is_client,
            next_stream_id: if (is_client) 1 else 2,
            max_concurrent_streams: max_concurrent_streams,
            initial_window: initial_window,
            active_count: 0,
            local_goaway: false,
            remote_goaway: false,
            goaway_code: 0,
            total_streams_opened: 0,
            total_bytes_sent: 0,
            total_bytes_received: 0,
        };
    }

    fn findActiveStream(stream_id: u32): usize? {
        let i: usize = 0;
        while (i < _.streams.len) : (i += 1) {
            if (_.streams[i].active && _.streams[i].stream_id == stream_id && _.streams[i].state != MuxStreamState.Closed) {
                return i;
            }
        }
        return null;
    }

    fn findFreeSlot(): usize? {
        let i: usize = 0;
        while (i < _.streams.len) : (i += 1) {
            if (!_.streams[i].active || _.streams[i].state == MuxStreamState.Closed) {
                return i;
            }
        }
        return null;
    }

    /// Opens a new locally initiated stream with automatic odd (client) / even (server) ID assignment.
    /// Returns `null` if GoAway is active, concurrency limit is reached, or stream ID space is exhausted.
    fn openStream(*_): u32? {
        if (_.local_goaway || _.remote_goaway) return null;
        if (_.active_count >= _.max_concurrent_streams) return null;
        if (_.next_stream_id == 0) return null;

        const slot: usize = _.findFreeSlot() ?? return null;
        const sid: u32 = _.next_stream_id;
        if (sid > std.math.maxInt(u32) - 2) {
            _.next_stream_id = 0;
        } else {
            _.next_stream_id = sid + 2;
        }

        _.streams[slot] = MuxStreamSlot{
            active: true,
            stream_id: sid,
            state: MuxStreamState.Open,
            send_window: _.initial_window,
            recv_window: _.initial_window,
            bytes_sent: 0,
            bytes_received: 0,
        };
        _.active_count += 1;
        _.total_streams_opened += 1;
        return sid;
    }

    /// Accepts a peer-initiated stream (`SYN`) after verifying opposite parity, GoAway state, and capacity.
    fn acceptRemoteStream(*_, stream_id: u32): bool {
        if (stream_id == 0 || _.local_goaway) return false;
        const expected_peer_parity: u32 = if (_.is_client) 0 else 1;
        if ((stream_id & 1) != expected_peer_parity) return false;
        if (_.active_count >= _.max_concurrent_streams) return false;
        if (_.findActiveStream(stream_id) != null) return false;

        const slot: usize = _.findFreeSlot() ?? return false;
        _.streams[slot] = MuxStreamSlot{
            active: true,
            stream_id: stream_id,
            state: MuxStreamState.Open,
            send_window: _.initial_window,
            recv_window: _.initial_window,
            bytes_sent: 0,
            bytes_received: 0,
        };
        _.active_count += 1;
        _.total_streams_opened += 1;
        return true;
    }

    /// Returns `true` if `stream_id` can currently transmit `payload_len` bytes within its `send_window`.
    fn canSend(stream_id: u32, payload_len: u32): bool {
        const idx: usize = _.findActiveStream(stream_id) ?? return false;
        const s: MuxStreamSlot = _.streams[idx];
        if (s.state != MuxStreamState.Open && s.state != MuxStreamState.RemoteHalfClosed) {
            return false;
        }
        return s.send_window >= payload_len;
    }

    /// Deducts `payload_len` from `send_window` when transmitting a `Data` frame.
    /// Returns `false` if the stream is closed/local-half-closed or lacks sufficient send credit.
    fn consumeSendWindow(*_, stream_id: u32, payload_len: u32): bool {
        const idx: usize = _.findActiveStream(stream_id) ?? return false;
        const st: MuxStreamState = _.streams[idx].state;
        if (st != MuxStreamState.Open && st != MuxStreamState.RemoteHalfClosed) {
            return false;
        }
        if (_.streams[idx].send_window < payload_len) return false;

        _.streams[idx].send_window -= payload_len;
        _.streams[idx].bytes_sent += payload_len as u64;
        _.total_bytes_sent += payload_len as u64;
        return true;
    }

    /// Processes an incoming `Data` frame on `stream_id`, enforcing `recv_window` flow-control limits
    /// and handling `FIN` (remote half-close) or `RST` (immediate close).
    /// Returns `false` if the stream is unknown, already remote-closed, or peer exceeded `recv_window`.
    fn onRecvData(*_, stream_id: u32, payload_len: u32, flags: u16): bool {
        const idx: usize = _.findActiveStream(stream_id) ?? return false;
        if ((flags & MuxFlags.RST) != 0) {
            _.streams[idx].state = MuxStreamState.Closed;
            if (_.active_count > 0) _.active_count -= 1;
            return true;
        }

        const st: MuxStreamState = _.streams[idx].state;
        if (st != MuxStreamState.Open && st != MuxStreamState.LocalHalfClosed) {
            return false;
        }
        if (_.streams[idx].recv_window < payload_len) {
            return false;
        }

        _.streams[idx].recv_window -= payload_len;
        _.streams[idx].bytes_received += payload_len as u64;
        _.total_bytes_received += payload_len as u64;

        if ((flags & MuxFlags.FIN) != 0) {
            if (st == MuxStreamState.Open) {
                _.streams[idx].state = MuxStreamState.RemoteHalfClosed;
            } else {
                _.streams[idx].state = MuxStreamState.Closed;
                if (_.active_count > 0) _.active_count -= 1;
            }
        }
        return true;
    }

    /// Processes an incoming `WindowUpdate` frame from the peer, replenishing `send_window`
    /// with overflow protection and applying optional `FIN`/`RST` flags.
    fn onWindowUpdate(*_, stream_id: u32, delta: u32, flags: u16): bool {
        const idx: usize = _.findActiveStream(stream_id) ?? return false;
        if ((flags & MuxFlags.RST) != 0) {
            _.streams[idx].state = MuxStreamState.Closed;
            if (_.active_count > 0) _.active_count -= 1;
            return true;
        }

        if (delta > std.math.maxInt(u32) - _.streams[idx].send_window) {
            return false;
        }
        _.streams[idx].send_window += delta;

        if ((flags & MuxFlags.FIN) != 0) {
            const st: MuxStreamState = _.streams[idx].state;
            if (st == MuxStreamState.Open) {
                _.streams[idx].state = MuxStreamState.RemoteHalfClosed;
            } else if (st == MuxStreamState.LocalHalfClosed) {
                _.streams[idx].state = MuxStreamState.Closed;
                if (_.active_count > 0) _.active_count -= 1;
            }
        }
        return true;
    }

    /// Replenishes local `recv_window` by `delta` and returns the corresponding outbound
    /// `WindowUpdate` frame header to transmit to the peer.
    fn grantRecvWindow(*_, stream_id: u32, delta: u32): MuxFrameHeader? {
        if (delta == 0) return null;
        const idx: usize = _.findActiveStream(stream_id) ?? return null;
        const st: MuxStreamState = _.streams[idx].state;
        if (st != MuxStreamState.Open && st != MuxStreamState.LocalHalfClosed) {
            return null;
        }
        if (delta > std.math.maxInt(u32) - _.streams[idx].recv_window) {
            return null;
        }
        _.streams[idx].recv_window += delta;
        return MuxFrameHeader{
            version: MUX_PROTO_VERSION,
            frame_type: MuxFrameType.WindowUpdate,
            flags: MuxFlags.NONE,
            stream_id: stream_id,
            length: delta,
        };
    }

    /// Transitions a stream through local half-close (`FIN`):
    /// `Open -> LocalHalfClosed`, `RemoteHalfClosed -> Closed`.
    fn closeLocal(*_, stream_id: u32): bool {
        const idx: usize = _.findActiveStream(stream_id) ?? return false;
        const st: MuxStreamState = _.streams[idx].state;
        if (st == MuxStreamState.Open) {
            _.streams[idx].state = MuxStreamState.LocalHalfClosed;
            return true;
        }
        if (st == MuxStreamState.RemoteHalfClosed) {
            _.streams[idx].state = MuxStreamState.Closed;
            if (_.active_count > 0) _.active_count -= 1;
            return true;
        }
        return false;
    }

    /// Immediately aborts an active stream (`RST`), transitioning it to `Closed`.
    fn resetStream(*_, stream_id: u32): bool {
        const idx: usize = _.findActiveStream(stream_id) ?? return false;
        _.streams[idx].state = MuxStreamState.Closed;
        if (_.active_count > 0) _.active_count -= 1;
        return true;
    }

    /// Initiates graceful session shutdown: rejects new local/remote streams while allowing
    /// in-flight streams to drain cleanly. Returns the outbound `GoAway` frame header.
    fn sendGoAway(*_, code: u32): MuxFrameHeader {
        _.local_goaway = true;
        _.goaway_code = code;
        return MuxFrameHeader{
            version: MUX_PROTO_VERSION,
            frame_type: MuxFrameType.GoAway,
            flags: MuxFlags.NONE,
            stream_id: 0,
            length: code,
        };
    }

    /// Records a peer `GoAway` frame: prevents opening new local streams while letting
    /// already-open streams complete normally.
    fn onGoAway(*_, code: u32) {
        _.remote_goaway = true;
        _.goaway_code = code;
    }

    /// Looks up a stream slot by `stream_id` (returns `null` if never opened or already reclaimed).
    fn getStream(stream_id: u32): MuxStreamSlot? {
        let closed_match: MuxStreamSlot? = null;
        let i: usize = 0;
        while (i < _.streams.len) : (i += 1) {
            const s: MuxStreamSlot = _.streams[i];
            if (!s.active || s.stream_id != stream_id) continue;
            if (s.state != MuxStreamState.Closed) return s;
            closed_match = s;
        }
        return closed_match;
    }

    /// Returns a point-in-time session telemetry snapshot.
    fn stats(): MuxSessionStats {
        return MuxSessionStats{
            active_streams: _.active_count,
            max_concurrent_streams: _.max_concurrent_streams,
            local_goaway: _.local_goaway,
            remote_goaway: _.remote_goaway,
            goaway_code: _.goaway_code,
            total_streams_opened: _.total_streams_opened,
            total_bytes_sent: _.total_bytes_sent,
            total_bytes_received: _.total_bytes_received,
        };
    }
}
