// Origin: Pure-CP Stdlib (WebM/EBML Streaming Muxer/Demuxer, VP8/VP9/AV1 OBU Parsers & RFC 7741 VP8 RTP Engine) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, zero external FFI dependencies, caller-slice streaming media core:
//   - RFC 8794 EBML VINT (1..8 bytes + streaming unknown-size sentinel 0x01FFFFFFFFFFFFFF) & Element ID codecs
//   - Zero-heap streaming WebM Muxer (`WebmMuxer`) & zero-copy Demuxer (`WebmDemuxIter`) with multi-track
//     VP8/VP9/AV1 Video + Opus Audio (`CodecPrivate` OpusHead) & `Cluster`/`SimpleBlock` roundtrip
//   - RFC 6386 VP8 uncompressed frame header & keyframe sync (`0x9D 0x01 0x2A`) dimension decoder
//   - VP9 uncompressed header (`frame_marker==0b10`, profiles 0..3, sync `0x49 0x83 0x42`, color config & frame size)
//   - AOMedia AV1 OBU zero-copy iterator (`Av1ObuIter` + LEB128) & `parseAv1SequenceHeaderBasic` resolution decoder
//   - RFC 7741 WebRTC VP8 RTP payload descriptor (`X/N/S/PID/I/L/T/K`), MTU packetizer & loss-resilient depacketizer

// ============================================================================
// 1. Constants, EBML/WebM Element IDs & Error Set
// ============================================================================

/// Decoded sentinel value representing an EBML streaming unknown-size element (`0x01FFFFFFFFFFFFFF`).
pub const WEBM_EBML_UNKNOWN_SIZE: u64 = 0x00FFFFFFFFFFFFFF;

/// Raw 8-byte EBML VINT wire encoding for unknown size (`0x01FFFFFFFFFFFFFF`).
pub const WEBM_EBML_UNKNOWN_SIZE_RAW: u64 = 0x01FFFFFFFFFFFFFF;

/// Default WebM / Matroska TimecodeScale in nanoseconds (`1_000_000` ns = 1 ms).
pub const WEBM_DEFAULT_TIMECODE_SCALE_NS: u64 = 1000000;

// --- EBML & WebM / Matroska Element IDs ---
pub const WEBM_ID_EBML: u32 = 0x1A45DFA3;
pub const WEBM_ID_EBML_VERSION: u32 = 0x4286;
pub const WEBM_ID_EBML_READ_VERSION: u32 = 0x42F7;
pub const WEBM_ID_EBML_MAX_ID_LENGTH: u32 = 0x42F2;
pub const WEBM_ID_EBML_MAX_SIZE_LENGTH: u32 = 0x42F3;
pub const WEBM_ID_DOC_TYPE: u32 = 0x4282;
pub const WEBM_ID_DOC_TYPE_VERSION: u32 = 0x4287;
pub const WEBM_ID_DOC_TYPE_READ_VERSION: u32 = 0x4285;

pub const WEBM_ID_SEGMENT: u32 = 0x18538067;
pub const WEBM_ID_INFO: u32 = 0x1549A966;
pub const WEBM_ID_TIMECODE_SCALE: u32 = 0x2AD7B1;
pub const WEBM_ID_MUXING_APP: u32 = 0x4D80;
pub const WEBM_ID_WRITING_APP: u32 = 0x5741;
pub const WEBM_ID_DURATION: u32 = 0x4489;

pub const WEBM_ID_TRACKS: u32 = 0x1654AE6B;
pub const WEBM_ID_TRACK_ENTRY: u32 = 0xAE;
pub const WEBM_ID_TRACK_NUMBER: u32 = 0xD7;
pub const WEBM_ID_TRACK_UID: u32 = 0x73C5;
pub const WEBM_ID_TRACK_TYPE: u32 = 0x83;
pub const WEBM_ID_CODEC_ID: u32 = 0x86;
pub const WEBM_ID_CODEC_PRIVATE: u32 = 0x63A2;
pub const WEBM_ID_VIDEO: u32 = 0xE0;
pub const WEBM_ID_PIXEL_WIDTH: u32 = 0xB0;
pub const WEBM_ID_PIXEL_HEIGHT: u32 = 0xBA;
pub const WEBM_ID_AUDIO: u32 = 0xE1;
pub const WEBM_ID_SAMPLING_FREQ: u32 = 0xB5;
pub const WEBM_ID_CHANNELS: u32 = 0x9F;
pub const WEBM_ID_BIT_DEPTH: u32 = 0x6264;

pub const WEBM_ID_CLUSTER: u32 = 0x1F43B675;
pub const WEBM_ID_TIMECODE: u32 = 0xE7;
pub const WEBM_ID_SIMPLE_BLOCK: u32 = 0xA3;

// --- Track Types & SimpleBlock Flags ---
pub const WEBM_TRACK_TYPE_VIDEO: u8 = 1;
pub const WEBM_TRACK_TYPE_AUDIO: u8 = 2;

pub const WEBM_BLOCK_FLAG_KEYFRAME: u8 = 0x80;
pub const WEBM_BLOCK_FLAG_INVISIBLE: u8 = 0x08;
pub const WEBM_BLOCK_FLAG_DISCARDABLE: u8 = 0x01;

// --- VP8 / VP9 Sync Codes ---
pub const VP8_KEYFRAME_SYNC_0: u8 = 0x9D;
pub const VP8_KEYFRAME_SYNC_1: u8 = 0x01;
pub const VP8_KEYFRAME_SYNC_2: u8 = 0x2A;

pub const VP9_KEYFRAME_SYNC_0: u8 = 0x49;
pub const VP9_KEYFRAME_SYNC_1: u8 = 0x83;
pub const VP9_KEYFRAME_SYNC_2: u8 = 0x42;

// --- AOMedia AV1 OBU Types (§5.3.2) ---
pub const AV1_OBU_SEQUENCE_HEADER: u8 = 1;
pub const AV1_OBU_TEMPORAL_DELIMITER: u8 = 2;
pub const AV1_OBU_FRAME_HEADER: u8 = 3;
pub const AV1_OBU_TILE_GROUP: u8 = 4;
pub const AV1_OBU_METADATA: u8 = 5;
pub const AV1_OBU_FRAME: u8 = 6;
pub const AV1_OBU_REDUNDANT_FRAME_HEADER: u8 = 7;
pub const AV1_OBU_TILE_LIST: u8 = 8;
pub const AV1_OBU_PADDING: u8 = 15;

/// Error set returned by WebM/EBML mux/demux, VP8/VP9/AV1 parsers, and RFC 7741 VP8 RTP operations.
pub err WebmMuxError {
    BufferTooSmall,
    TruncatedData,
    InvalidVint,
    InvalidElementId,
    InvalidEbmlHeader,
    InvalidWebmContainer,
    InvalidSimpleBlock,
    TimestampOutOfRange,
    InvalidVp8Frame,
    InvalidVp9Frame,
    InvalidAv1Obu,
    InvalidAv1SequenceHeader,
    InvalidVp8RtpPacket,
    MtuTooSmall,
    PacketSpanOverflow,
    InvalidState,
}

// ============================================================================
// 2. Internal Byte/Slice Helpers & MSB-First BitReader
// ============================================================================

fn copyBytes(dst: []u8, dst_off: usize, src: []const u8) {
    let i: usize = 0;
    while (i < src.len) : (i += 1) {
        dst[dst_off + i] = src[i];
    }
}

fn strEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn readBeU16(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn writeBeU16(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn readBeUintN(buf: []const u8): u64 {
    let acc: u64 = 0;
    let i: usize = 0;
    while (i < buf.len && i < 8) : (i += 1) {
        acc = (acc << 8) | (buf[i] as u64);
    }
    return acc;
}

fn readBeF64(buf: []const u8): f64 {
    if (buf.len == 8) {
        const bits: u64 = readBeUintN(buf);
        return @as(f64, @bitCast(bits));
    }
    if (buf.len == 4) {
        const bits32: u32 = readBeUintN(buf) as u32;
        const f: f32 = @as(f32, @bitCast(bits32));
        return f as f64;
    }
    return 0.0;
}

fn uintByteLen(v: u64): usize {
    if (v <= 0xFF) return 1;
    if (v <= 0xFFFF) return 2;
    if (v <= 0xFFFFFF) return 3;
    if (v <= 0xFFFFFFFF) return 4;
    if (v <= 0xFFFFFFFFFF) return 5;
    if (v <= 0xFFFFFFFFFFFF) return 6;
    if (v <= 0xFFFFFFFFFFFFFF) return 7;
    return 8;
}

/// MSB-first bit reader for VP9 uncompressed header and AV1 Sequence Header parsing.
pub struct WebmBitReader {
    data: []const u8 = "";
    bit_pos: usize = 0;

    static fn init(buf: []const u8): WebmBitReader {
        return WebmBitReader{
            data: buf,
            bit_pos: 0,
        };
    }

    fn bitsRemaining(): usize {
        const total: usize = data.len * 8;
        if (bit_pos >= total) return 0;
        return total - bit_pos;
    }

    fn readBit(*_): u8 ! WebmMuxError {
        if (bit_pos >= data.len * 8) {
            return WebmMuxError.TruncatedData;
        }
        const byte_idx: usize = bit_pos >> 3;
        const shift: u3 = @as(u3, @intCast(7 - (bit_pos & 7)));
        bit_pos += 1;
        return (data[byte_idx] >> shift) & 1;
    }

    fn readBits(*_, count: usize): u32 ! WebmMuxError {
        if (count == 0) return 0;
        if (count > 32 || count > bitsRemaining()) {
            return WebmMuxError.TruncatedData;
        }
        let acc: u32 = 0;
        let i: usize = 0;
        while (i < count) : (i += 1) {
            const b = readBit()?;
            acc = (acc << 1) | (b as u32);
        }
        return acc;
    }

    fn skipBits(*_, count: usize): bool ! WebmMuxError {
        if (count > bitsRemaining()) {
            return WebmMuxError.TruncatedData;
        }
        bit_pos += count;
        return true;
    }
}

// ============================================================================
// 3. RFC 8794 EBML VINT (1..8 Bytes + Unknown Size) & Element ID Codecs
// ============================================================================

/// Result of decoding an EBML variable-length integer (VINT data size / track number).
pub struct WebmVintDecoded {
    value: u64 = 0;
    length: usize = 0;
    is_unknown: bool = false;
}

/// Result of decoding an EBML Element ID (1..4 bytes, keeping the leading VINT marker bit).
pub struct WebmElementIdDecoded {
    id: u32 = 0;
    length: usize = 0;
}

/// Returns the minimum byte width (`1..8`) needed to encode EBML VINT data size `value`.
pub fn ebmlVintWidth(value: u64): usize ! WebmMuxError {
    if (value == WEBM_EBML_UNKNOWN_SIZE) {
        return 8;
    }
    // For width n (1..8), max known value is (2^(7*n)) - 2 (all-1s is reserved for unknown size).
    if (value <= 0x7E) return 1;
    if (value <= 0x3FFE) return 2;
    if (value <= 0x1FFFFE) return 3;
    if (value <= 0x0FFFFFFE) return 4;
    if (value <= 0x07FFFFFFFFE) return 5;
    if (value <= 0x03FFFFFFFFFFE) return 6;
    if (value <= 0x01FFFFFFFFFFFFE) return 7;
    if (value <= 0x00FFFFFFFFFFFFFE) return 8;
    return WebmMuxError.InvalidVint;
}

/// Encodes an EBML VINT in an exact byte width (`1..8`) into `out`.
/// When `value == WEBM_EBML_UNKNOWN_SIZE` and `width == 8`, writes `0x01FFFFFFFFFFFFFF`.
pub fn encodeEbmlVintFixed(value: u64, width: usize, out: []u8): usize ! WebmMuxError {
    if (width < 1 || width > 8) {
        return WebmMuxError.InvalidVint;
    }
    if (out.len < width) {
        return WebmMuxError.BufferTooSmall;
    }
    if (value == WEBM_EBML_UNKNOWN_SIZE) {
        if (width != 8) {
            return WebmMuxError.InvalidVint;
        }
        out[0] = 0x01;
        let k: usize = 1;
        while (k < 8) : (k += 1) {
            out[k] = 0xFF;
        }
        return 8;
    }
    const max_for_width: u64 = ((@as(u64, 1) << (@as(u6, @intCast(width * 7)))) - 2);
    if (value > max_for_width) {
        return WebmMuxError.InvalidVint;
    }
    const marker: u64 = @as(u64, 1) << (@as(u6, @intCast(width * 7)));
    const encoded: u64 = marker | value;
    let i: usize = 0;
    while (i < width) : (i += 1) {
        const shift: u6 = @as(u6, @intCast((width - 1 - i) * 8));
        out[i] = ((encoded >> shift) & 0xFF) as u8;
    }
    return width;
}

/// Encodes an EBML VINT using the shortest legal width (`1..8` bytes, or 8 bytes for unknown size).
pub fn encodeEbmlVint(value: u64, out: []u8): usize ! WebmMuxError {
    const w = ebmlVintWidth(value)?;
    return encodeEbmlVintFixed(value, w, out);
}

/// Decodes an EBML VINT (`1..8` bytes), flagging all-1s data bits as `is_unknown = true`.
pub fn decodeEbmlVint(data: []const u8): WebmVintDecoded ! WebmMuxError {
    if (data.len == 0) {
        return WebmMuxError.TruncatedData;
    }
    const first: u8 = data[0];
    if (first == 0) {
        return WebmMuxError.InvalidVint;
    }
    let width: usize = 1;
    let mask: u8 = 0x80;
    while ((first & mask) == 0) {
        width += 1;
        mask >>= 1;
    }
    if (width > 8) {
        return WebmMuxError.InvalidVint;
    }
    if (data.len < width) {
        return WebmMuxError.TruncatedData;
    }
    let val: u64 = (first & (~mask)) as u64;
    let i: usize = 1;
    while (i < width) : (i += 1) {
        val = (val << 8) | (data[i] as u64);
    }
    const all_ones: u64 = (@as(u64, 1) << (@as(u6, @intCast(width * 7)))) - 1;
    const is_unk: bool = (val == all_ones);
    return WebmVintDecoded{
        value: if (is_unk) WEBM_EBML_UNKNOWN_SIZE else val,
        length: width,
        is_unknown: is_unk,
    };
}

/// Encodes an EBML Element ID (`1..4` bytes) into `out`.
pub fn encodeEbmlElementId(id: u32, out: []u8): usize ! WebmMuxError {
    if (id == 0) {
        return WebmMuxError.InvalidElementId;
    }
    let width: usize = 1;
    if (id > 0xFFFFFF) {
        width = 4;
    } else if (id > 0xFFFF) {
        width = 3;
    } else if (id > 0xFF) {
        width = 2;
    }
    if (out.len < width) {
        return WebmMuxError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < width) : (i += 1) {
        const shift: u5 = @as(u5, @intCast((width - 1 - i) * 8));
        out[i] = ((id >> shift) & 0xFF) as u8;
    }
    return width;
}

/// Decodes an EBML Element ID (`1..4` bytes) from `data`.
pub fn decodeEbmlElementId(data: []const u8): WebmElementIdDecoded ! WebmMuxError {
    if (data.len == 0) {
        return WebmMuxError.TruncatedData;
    }
    const first: u8 = data[0];
    if (first == 0) {
        return WebmMuxError.InvalidElementId;
    }
    let width: usize = 1;
    let mask: u8 = 0x80;
    while ((first & mask) == 0) {
        width += 1;
        mask >>= 1;
        if (width > 4) {
            return WebmMuxError.InvalidElementId;
        }
    }
    if (data.len < width) {
        return WebmMuxError.TruncatedData;
    }
    let id: u32 = first as u32;
    let i: usize = 1;
    while (i < width) : (i += 1) {
        id = (id << 8) | (data[i] as u32);
    }
    return WebmElementIdDecoded{
        id: id,
        length: width,
    };
}

fn writeElemUint(out: []u8, pos: usize, id: u32, value: u64): usize ! WebmMuxError {
    let cur: usize = pos;
    const id_len = encodeEbmlElementId(id, out[cur..out.len])?;
    cur += id_len;
    const val_len = uintByteLen(value);
    const sz_len = encodeEbmlVint(val_len as u64, out[cur..out.len])?;
    cur += sz_len;
    if (cur + val_len > out.len) {
        return WebmMuxError.BufferTooSmall;
    }
    let i: usize = 0;
    while (i < val_len) : (i += 1) {
        const shift: u6 = @as(u6, @intCast((val_len - 1 - i) * 8));
        out[cur + i] = ((value >> shift) & 0xFF) as u8;
    }
    return cur + val_len;
}

fn writeElemBytes(out: []u8, pos: usize, id: u32, payload: []const u8): usize ! WebmMuxError {
    let cur: usize = pos;
    const id_len = encodeEbmlElementId(id, out[cur..out.len])?;
    cur += id_len;
    const sz_len = encodeEbmlVint(payload.len as u64, out[cur..out.len])?;
    cur += sz_len;
    if (cur + payload.len > out.len) {
        return WebmMuxError.BufferTooSmall;
    }
    copyBytes(out, cur, payload);
    return cur + payload.len;
}

fn writeElemF64(out: []u8, pos: usize, id: u32, value: f64): usize ! WebmMuxError {
    const bits: u64 = @as(u64, @bitCast(value));
    let raw: [8]u8 = [0; 8];
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        const shift: u6 = @as(u6, @intCast((7 - i) * 8));
        raw[i] = ((bits >> shift) & 0xFF) as u8;
    }
    return writeElemBytes(out, pos, id, raw[0..8]);
}

// ============================================================================
// 4. WebM Streaming Muxer & Zero-Copy Demuxer
// ============================================================================

/// Supported royalty-free WebM video codecs.
pub enum WebmVideoCodec {
    vp8,
    vp9,
    av1,

    /// Returns the canonical Matroska/WebM CodecID string (`"V_VP8"`, `"V_VP9"`, `"V_AV1"`).
    fn codecId(): []const u8 {
        return match (_) {
            WebmVideoCodec.vp8 => "V_VP8",
            WebmVideoCodec.vp9 => "V_VP9",
            WebmVideoCodec.av1 => "V_AV1",
        };
    }
}

/// Track descriptor used by `WebmMuxer` when writing the `Tracks` element.
pub struct WebmTrackConfig {
    track_number: u64 = 1;
    track_uid: u64 = 1;
    track_type: u8 = WEBM_TRACK_TYPE_VIDEO;
    codec_id: []const u8 = "V_VP9";
    pixel_width: u32 = 0;
    pixel_height: u32 = 0;
    sampling_frequency: f64 = 48000.0;
    channels: u8 = 2;
    bit_depth: u8 = 0;
    codec_private: []const u8 = "";

    /// Convenience constructor for a VP8 / VP9 / AV1 video track.
    static fn video(track_num: u64, codec: WebmVideoCodec, width: u32, height: u32): WebmTrackConfig {
        return WebmTrackConfig{
            track_number: track_num,
            track_uid: track_num,
            track_type: WEBM_TRACK_TYPE_VIDEO,
            codec_id: codec.codecId(),
            pixel_width: width,
            pixel_height: height,
            sampling_frequency: 0.0,
            channels: 0,
            bit_depth: 0,
            codec_private: "",
        };
    }

    /// Convenience constructor for an Opus (`A_OPUS`) audio track with optional `OpusHead` CodecPrivate.
    static fn opusAudio(
        track_num: u64,
        sample_rate_hz: f64,
        channel_count: u8,
        opus_head: []const u8,
    ): WebmTrackConfig {
        return WebmTrackConfig{
            track_number: track_num,
            track_uid: track_num,
            track_type: WEBM_TRACK_TYPE_AUDIO,
            codec_id: "A_OPUS",
            pixel_width: 0,
            pixel_height: 0,
            sampling_frequency: sample_rate_hz,
            channels: channel_count,
            bit_depth: 16,
            codec_private: opus_head,
        };
    }
}

/// Zero-heap streaming WebM muxer writing `EBML`, `Segment`, `Info`, `Tracks`, `Cluster` & `SimpleBlock`
/// directly into a caller-provided output slice.
pub struct WebmMuxer {
    buf: []u8 = [];
    pos: usize = 0;
    timecode_scale_ns: u64 = WEBM_DEFAULT_TIMECODE_SCALE_NS;
    streaming_unknown_size: bool = false;
    header_written: bool = false;
    segment_size_off: usize = 0;
    segment_data_start: usize = 0;
    cluster_open: bool = false;
    cluster_size_off: usize = 0;
    cluster_data_start: usize = 0;
    cluster_timecode_ms: u64 = 0;
    blocks_written: u64 = 0;
    clusters_written: u64 = 0;

    /// Initializes a WebM muxer over caller-owned slice `out`.
    /// Set `streaming_unknown` to `true` for live MediaSource/HTTP chunked streaming (`0x01FFFFFFFFFFFFFF` sizes).
    static fn init(out: []u8, scale_ns: u64, streaming_unknown: bool): WebmMuxer ! WebmMuxError {
        if (out.len < 64) {
            return WebmMuxError.BufferTooSmall;
        }
        return WebmMuxer{
            buf: out,
            pos: 0,
            timecode_scale_ns: if (scale_ns == 0) WEBM_DEFAULT_TIMECODE_SCALE_NS else scale_ns,
            streaming_unknown_size: streaming_unknown,
            header_written: false,
            segment_size_off: 0,
            segment_data_start: 0,
            cluster_open: false,
            cluster_size_off: 0,
            cluster_data_start: 0,
            cluster_timecode_ms: 0,
            blocks_written: 0,
            clusters_written: 0,
        };
    }

    /// Returns the muxed WebM byte slice produced so far.
    fn bytes(): []const u8 {
        return buf[0..pos];
    }

    fn writeMasterPlaceholder(*_, id: u32, size_width: usize): usize ! WebmMuxError {
        const id_len = encodeEbmlElementId(id, buf[pos..buf.len])?;
        pos += id_len;
        const size_off: usize = pos;
        const init_val: u64 = if (size_width == 8 && streaming_unknown_size) WEBM_EBML_UNKNOWN_SIZE else 0;
        const sz_len = encodeEbmlVintFixed(init_val, size_width, buf[pos..buf.len])?;
        pos += sz_len;
        return size_off;
    }

    fn patchMasterSize(*_, size_off: usize, data_start: usize, size_width: usize): bool ! WebmMuxError {
        const body_len: u64 = (pos - data_start) as u64;
        * = encodeEbmlVintFixed(body_len, size_width, buf[size_off..(size_off + size_width)])?;
        return true;
    }

    /// Writes `EBML` header (`DocType = "webm"`), `Segment`, `Info`, and all `Tracks` entries.
    fn writeHeader(
        *_,
        tracks: []const WebmTrackConfig,
        muxing_app: []const u8,
        writing_app: []const u8,
    ): usize ! WebmMuxError {
        if (header_written) {
            return WebmMuxError.InvalidState;
        }

        // 1. EBML Header (4-byte fixed size, patched after children)
        const ebml_sz_off = writeMasterPlaceholder(WEBM_ID_EBML, 4)?;
        const ebml_start: usize = pos;
        pos = writeElemUint(buf, pos, WEBM_ID_EBML_VERSION, 1)?;
        pos = writeElemUint(buf, pos, WEBM_ID_EBML_READ_VERSION, 1)?;
        pos = writeElemUint(buf, pos, WEBM_ID_EBML_MAX_ID_LENGTH, 4)?;
        pos = writeElemUint(buf, pos, WEBM_ID_EBML_MAX_SIZE_LENGTH, 8)?;
        pos = writeElemBytes(buf, pos, WEBM_ID_DOC_TYPE, "webm")?;
        pos = writeElemUint(buf, pos, WEBM_ID_DOC_TYPE_VERSION, 4)?;
        pos = writeElemUint(buf, pos, WEBM_ID_DOC_TYPE_READ_VERSION, 2)?;
        * = patchMasterSize(ebml_sz_off, ebml_start, 4)?;

        // 2. Segment (8-byte VINT: unknown size in streaming mode, or patched on finish())
        segment_size_off = writeMasterPlaceholder(WEBM_ID_SEGMENT, 8)?;
        segment_data_start = pos;

        // 3. Info
        const info_sz_off = writeMasterPlaceholder(WEBM_ID_INFO, 4)?;
        const info_start: usize = pos;
        pos = writeElemUint(buf, pos, WEBM_ID_TIMECODE_SCALE, timecode_scale_ns)?;
        if (muxing_app.len > 0) {
            pos = writeElemBytes(buf, pos, WEBM_ID_MUXING_APP, muxing_app)?;
        }
        if (writing_app.len > 0) {
            pos = writeElemBytes(buf, pos, WEBM_ID_WRITING_APP, writing_app)?;
        }
        * = patchMasterSize(info_sz_off, info_start, 4)?;

        // 4. Tracks
        const tracks_sz_off = writeMasterPlaceholder(WEBM_ID_TRACKS, 4)?;
        const tracks_start: usize = pos;
        let i: usize = 0;
        while (i < tracks.len) : (i += 1) {
            const tr = tracks[i];
            const te_sz_off = writeMasterPlaceholder(WEBM_ID_TRACK_ENTRY, 4)?;
            const te_start: usize = pos;
            pos = writeElemUint(buf, pos, WEBM_ID_TRACK_NUMBER, tr.track_number)?;
            pos = writeElemUint(buf, pos, WEBM_ID_TRACK_UID, if (tr.track_uid == 0) tr.track_number else tr.track_uid)?;
            pos = writeElemUint(buf, pos, WEBM_ID_TRACK_TYPE, tr.track_type as u64)?;
            pos = writeElemBytes(buf, pos, WEBM_ID_CODEC_ID, tr.codec_id)?;
            if (tr.codec_private.len > 0) {
                pos = writeElemBytes(buf, pos, WEBM_ID_CODEC_PRIVATE, tr.codec_private)?;
            }
            if (tr.track_type == WEBM_TRACK_TYPE_VIDEO) {
                const vid_sz_off = writeMasterPlaceholder(WEBM_ID_VIDEO, 2)?;
                const vid_start: usize = pos;
                pos = writeElemUint(buf, pos, WEBM_ID_PIXEL_WIDTH, tr.pixel_width as u64)?;
                pos = writeElemUint(buf, pos, WEBM_ID_PIXEL_HEIGHT, tr.pixel_height as u64)?;
                * = patchMasterSize(vid_sz_off, vid_start, 2)?;
            } else if (tr.track_type == WEBM_TRACK_TYPE_AUDIO) {
                const aud_sz_off = writeMasterPlaceholder(WEBM_ID_AUDIO, 2)?;
                const aud_start: usize = pos;
                pos = writeElemF64(buf, pos, WEBM_ID_SAMPLING_FREQ, tr.sampling_frequency)?;
                pos = writeElemUint(buf, pos, WEBM_ID_CHANNELS, tr.channels as u64)?;
                if (tr.bit_depth > 0) {
                    pos = writeElemUint(buf, pos, WEBM_ID_BIT_DEPTH, tr.bit_depth as u64)?;
                }
                * = patchMasterSize(aud_sz_off, aud_start, 2)?;
            }
            * = patchMasterSize(te_sz_off, te_start, 4)?;
        }
        * = patchMasterSize(tracks_sz_off, tracks_start, 4)?;

        header_written = true;
        return pos;
    }

    fn closeActiveCluster(*_): bool ! WebmMuxError {
        if (!cluster_open) {
            return false;
        }
        if (!streaming_unknown_size) {
            * = patchMasterSize(cluster_size_off, cluster_data_start, 8)?;
        }
        cluster_open = false;
        return true;
    }

    /// Starts a new `Cluster` at `timecode_ms` (closing any previous open cluster first).
    fn startCluster(*_, timecode_ms: u64): usize ! WebmMuxError {
        if (!header_written) {
            return WebmMuxError.InvalidState;
        }
        * = closeActiveCluster()?;
        cluster_size_off = writeMasterPlaceholder(WEBM_ID_CLUSTER, 8)?;
        cluster_data_start = pos;
        cluster_timecode_ms = timecode_ms;
        pos = writeElemUint(buf, pos, WEBM_ID_TIMECODE, timecode_ms)?;
        cluster_open = true;
        clusters_written += 1;
        return pos;
    }

    /// Writes a `SimpleBlock` (`0xA3`) frame for `track_number` at `timestamp_ms`.
    /// Automatically opens a new `Cluster` when none is open or when the relative timecode exceeds `i16` range.
    fn writeSimpleBlock(
        *_,
        track_number: u64,
        timestamp_ms: u64,
        is_keyframe: bool,
        invisible: bool,
        discardable: bool,
        payload: []const u8,
    ): usize ! WebmMuxError {
        if (!header_written) {
            return WebmMuxError.InvalidState;
        }
        if (!cluster_open || timestamp_ms < cluster_timecode_ms || (timestamp_ms - cluster_timecode_ms) > 32767) {
            * = startCluster(timestamp_ms)?;
        }
        const rel_u64: u64 = timestamp_ms - cluster_timecode_ms;
        const rel_i16: i16 = rel_u64 as i16;

        let track_vint: [8]u8 = [0; 8];
        const tv_len = encodeEbmlVint(track_number, track_vint[0..8])?;
        const block_body_len: usize = payload.len + tv_len + 3;

        const id_len = encodeEbmlElementId(WEBM_ID_SIMPLE_BLOCK, buf[pos..buf.len])?;
        pos += id_len;
        const sz_len = encodeEbmlVint(block_body_len as u64, buf[pos..buf.len])?;
        pos += sz_len;
        if (pos + block_body_len > buf.len) {
            return WebmMuxError.BufferTooSmall;
        }

        copyBytes(buf, pos, track_vint[0..tv_len]);
        pos += tv_len;
        const rel_u16: u16 = @as(u16, @bitCast(rel_i16));
        writeBeU16(buf, pos, rel_u16);
        pos += 2;

        let flags: u8 = 0;
        if (is_keyframe) flags |= WEBM_BLOCK_FLAG_KEYFRAME;
        if (invisible) flags |= WEBM_BLOCK_FLAG_INVISIBLE;
        if (discardable) flags |= WEBM_BLOCK_FLAG_DISCARDABLE;
        buf[pos] = flags;
        pos += 1;

        copyBytes(buf, pos, payload);
        pos += payload.len;
        blocks_written += 1;
        return pos;
    }

    /// Finalizes the active `Cluster` and patches `Segment` size (in non-streaming mode),
    /// returning the total byte length written.
    fn finish(*_): usize ! WebmMuxError {
        if (!header_written) {
            return WebmMuxError.InvalidState;
        }
        * = closeActiveCluster()?;
        if (!streaming_unknown_size) {
            * = patchMasterSize(segment_size_off, segment_data_start, 8)?;
        }
        return pos;
    }
}

/// Decoded track metadata extracted from a WebM `Tracks` element.
pub struct WebmDemuxTrackInfo {
    track_number: u64 = 0;
    track_uid: u64 = 0;
    track_type: u8 = 0;
    codec_id: []const u8 = "";
    pixel_width: u32 = 0;
    pixel_height: u32 = 0;
    sampling_frequency: f64 = 0.0;
    channels: u8 = 0;
    bit_depth: u8 = 0;
    codec_private: []const u8 = "";
}

/// Zero-copy media frame yielded by `WebmDemuxIter.nextFrame()`.
pub struct WebmDemuxFrame {
    track_number: u64 = 0;
    timestamp_ms: u64 = 0;
    relative_timecode: i16 = 0;
    flags: u8 = 0;
    is_keyframe: bool = false;
    is_invisible: bool = false;
    is_discardable: bool = false;
    payload: []const u8 = "";
}

fn parseTrackEntry(entry_body: []const u8): WebmDemuxTrackInfo ! WebmMuxError {
    let info = WebmDemuxTrackInfo{};
    let off: usize = 0;
    while (off < entry_body.len) {
        const eid = decodeEbmlElementId(entry_body[off..entry_body.len])?;
        off += eid.length;
        const esz = decodeEbmlVint(entry_body[off..entry_body.len])?;
        off += esz.length;
        if (esz.is_unknown) {
            return WebmMuxError.InvalidWebmContainer;
        }
        const elen: usize = esz.value as usize;
        if (off + elen > entry_body.len) {
            return WebmMuxError.TruncatedData;
        }
        const val = entry_body[off..(off + elen)];
        off += elen;

        if (eid.id == WEBM_ID_TRACK_NUMBER) {
            info.track_number = readBeUintN(val);
        } else if (eid.id == WEBM_ID_TRACK_UID) {
            info.track_uid = readBeUintN(val);
        } else if (eid.id == WEBM_ID_TRACK_TYPE) {
            info.track_type = readBeUintN(val) as u8;
        } else if (eid.id == WEBM_ID_CODEC_ID) {
            info.codec_id = val;
        } else if (eid.id == WEBM_ID_CODEC_PRIVATE) {
            info.codec_private = val;
        } else if (eid.id == WEBM_ID_VIDEO) {
            let voff: usize = 0;
            while (voff < val.len) {
                const vid = decodeEbmlElementId(val[voff..val.len])?;
                voff += vid.length;
                const vsz = decodeEbmlVint(val[voff..val.len])?;
                voff += vsz.length;
                const vlen: usize = vsz.value as usize;
                if (voff + vlen > val.len) return WebmMuxError.TruncatedData;
                const vval = val[voff..(voff + vlen)];
                voff += vlen;
                if (vid.id == WEBM_ID_PIXEL_WIDTH) {
                    info.pixel_width = readBeUintN(vval) as u32;
                } else if (vid.id == WEBM_ID_PIXEL_HEIGHT) {
                    info.pixel_height = readBeUintN(vval) as u32;
                }
            }
        } else if (eid.id == WEBM_ID_AUDIO) {
            let aoff: usize = 0;
            while (aoff < val.len) {
                const aid = decodeEbmlElementId(val[aoff..val.len])?;
                aoff += aid.length;
                const asz = decodeEbmlVint(val[aoff..val.len])?;
                aoff += asz.length;
                const alen: usize = asz.value as usize;
                if (aoff + alen > val.len) return WebmMuxError.TruncatedData;
                const aval = val[aoff..(aoff + alen)];
                aoff += alen;
                if (aid.id == WEBM_ID_SAMPLING_FREQ) {
                    info.sampling_frequency = readBeF64(aval);
                } else if (aid.id == WEBM_ID_CHANNELS) {
                    info.channels = readBeUintN(aval) as u8;
                } else if (aid.id == WEBM_ID_BIT_DEPTH) {
                    info.bit_depth = readBeUintN(aval) as u8;
                }
            }
        }
    }
    return info;
}

/// Zero-copy streaming WebM demuxer handling both known-size and unknown-size (`0x01FFFFFFFFFFFFFF`)
/// `Segment` and `Cluster` containers.
pub struct WebmDemuxIter {
    data: []const u8 = "";
    seg_end: usize = 0;
    cursor: usize = 0;
    in_cluster: bool = false;
    cluster_end: usize = 0;
    cluster_unknown: bool = false;
    cluster_timecode_ms: u64 = 0;
    doc_type: []const u8 = "";
    timecode_scale_ns: u64 = WEBM_DEFAULT_TIMECODE_SCALE_NS;
    muxing_app: []const u8 = "";
    writing_app: []const u8 = "";
    track_count: usize = 0;

    /// Parses `EBML` header, enters `Segment`, extracts `Info` and `Tracks` into caller's `out_tracks`,
    /// and positions the iterator at the first `Cluster`.
    static fn init(buf: []const u8, out_tracks: []WebmDemuxTrackInfo): WebmDemuxIter ! WebmMuxError {
        let off: usize = 0;
        // 1. Parse EBML Header
        const ebml_id = decodeEbmlElementId(buf[off..buf.len])?;
        if (ebml_id.id != WEBM_ID_EBML) {
            return WebmMuxError.InvalidEbmlHeader;
        }
        off += ebml_id.length;
        const ebml_sz = decodeEbmlVint(buf[off..buf.len])?;
        off += ebml_sz.length;
        if (ebml_sz.is_unknown) {
            return WebmMuxError.InvalidEbmlHeader;
        }
        const ebml_end: usize = off + (ebml_sz.value as usize);
        if (ebml_end > buf.len) {
            return WebmMuxError.TruncatedData;
        }

        let doc_type_val: []const u8 = "";
        let eoff: usize = off;
        while (eoff < ebml_end) {
            const cid = decodeEbmlElementId(buf[eoff..ebml_end])?;
            eoff += cid.length;
            const csz = decodeEbmlVint(buf[eoff..ebml_end])?;
            eoff += csz.length;
            const clen: usize = csz.value as usize;
            if (eoff + clen > ebml_end) return WebmMuxError.TruncatedData;
            if (cid.id == WEBM_ID_DOC_TYPE) {
                doc_type_val = buf[eoff..(eoff + clen)];
            }
            eoff += clen;
        }
        if (!strEq(doc_type_val, "webm") && !strEq(doc_type_val, "matroska")) {
            return WebmMuxError.InvalidEbmlHeader;
        }
        off = ebml_end;

        // 2. Enter Segment
        const seg_id = decodeEbmlElementId(buf[off..buf.len])?;
        if (seg_id.id != WEBM_ID_SEGMENT) {
            return WebmMuxError.InvalidWebmContainer;
        }
        off += seg_id.length;
        const seg_sz = decodeEbmlVint(buf[off..buf.len])?;
        off += seg_sz.length;
        const seg_end_pos: usize = if (seg_sz.is_unknown) buf.len else (off + (seg_sz.value as usize));
        if (seg_end_pos > buf.len) {
            return WebmMuxError.TruncatedData;
        }

        let demux = WebmDemuxIter{
            data: buf,
            seg_end: seg_end_pos,
            cursor: off,
            in_cluster: false,
            cluster_end: 0,
            cluster_unknown: false,
            cluster_timecode_ms: 0,
            doc_type: doc_type_val,
            timecode_scale_ns: WEBM_DEFAULT_TIMECODE_SCALE_NS,
            muxing_app: "",
            writing_app: "",
            track_count: 0,
        };

        // 3. Scan top-level Segment children until first Cluster
        while (demux.cursor < demux.seg_end) {
            const elem_start: usize = demux.cursor;
            const tid = decodeEbmlElementId(buf[demux.cursor..demux.seg_end])?;
            if (tid.id == WEBM_ID_CLUSTER) {
                demux.cursor = elem_start;
                break;
            }
            demux.cursor += tid.length;
            const tsz = decodeEbmlVint(buf[demux.cursor..demux.seg_end])?;
            demux.cursor += tsz.length;
            if (tsz.is_unknown) {
                return WebmMuxError.InvalidWebmContainer;
            }
            const tlen: usize = tsz.value as usize;
            if (demux.cursor + tlen > demux.seg_end) {
                return WebmMuxError.TruncatedData;
            }
            const body = buf[demux.cursor..(demux.cursor + tlen)];
            demux.cursor += tlen;

            if (tid.id == WEBM_ID_INFO) {
                let ioff: usize = 0;
                while (ioff < body.len) {
                    const iid = decodeEbmlElementId(body[ioff..body.len])?;
                    ioff += iid.length;
                    const isz = decodeEbmlVint(body[ioff..body.len])?;
                    ioff += isz.length;
                    const ilen: usize = isz.value as usize;
                    if (ioff + ilen > body.len) return WebmMuxError.TruncatedData;
                    const ival = body[ioff..(ioff + ilen)];
                    ioff += ilen;
                    if (iid.id == WEBM_ID_TIMECODE_SCALE) {
                        demux.timecode_scale_ns = readBeUintN(ival);
                    } else if (iid.id == WEBM_ID_MUXING_APP) {
                        demux.muxing_app = ival;
                    } else if (iid.id == WEBM_ID_WRITING_APP) {
                        demux.writing_app = ival;
                    }
                }
            } else if (tid.id == WEBM_ID_TRACKS) {
                let toff: usize = 0;
                while (toff < body.len) {
                    const teid = decodeEbmlElementId(body[toff..body.len])?;
                    toff += teid.length;
                    const tesz = decodeEbmlVint(body[toff..body.len])?;
                    toff += tesz.length;
                    const telen: usize = tesz.value as usize;
                    if (toff + telen > body.len) return WebmMuxError.TruncatedData;
                    const te_body = body[toff..(toff + telen)];
                    toff += telen;
                    if (teid.id == WEBM_ID_TRACK_ENTRY && demux.track_count < out_tracks.len) {
                        out_tracks[demux.track_count] = parseTrackEntry(te_body)?;
                        demux.track_count += 1;
                    }
                }
            }
        }
        return demux;
    }

    /// Yields the next `SimpleBlock` frame across all `Cluster` elements, or `null` at EOF.
    fn nextFrame(*_): WebmDemuxFrame? ! WebmMuxError {
        while (cursor < seg_end) {
            if (!in_cluster) {
                const cid = decodeEbmlElementId(data[cursor..seg_end])?;
                cursor += cid.length;
                const csz = decodeEbmlVint(data[cursor..seg_end])?;
                cursor += csz.length;
                if (cid.id == WEBM_ID_CLUSTER) {
                    in_cluster = true;
                    cluster_unknown = csz.is_unknown;
                    cluster_end = if (csz.is_unknown) seg_end else (cursor + (csz.value as usize));
                    if (cluster_end > seg_end) {
                        return WebmMuxError.TruncatedData;
                    }
                    cluster_timecode_ms = 0;
                    continue;
                }
                if (csz.is_unknown) {
                    return WebmMuxError.InvalidWebmContainer;
                }
                const skip_len: usize = csz.value as usize;
                if (cursor + skip_len > seg_end) {
                    return WebmMuxError.TruncatedData;
                }
                cursor += skip_len;
                continue;
            }

            if (cursor >= cluster_end) {
                in_cluster = false;
                continue;
            }

            const elem_start: usize = cursor;
            const bid = decodeEbmlElementId(data[cursor..cluster_end])?;
            // In streaming unknown-size clusters, a new Cluster or top-level Segment element terminates the current cluster.
            if (cluster_unknown && (bid.id == WEBM_ID_CLUSTER || bid.id == WEBM_ID_INFO || bid.id == WEBM_ID_TRACKS)) {
                cursor = elem_start;
                in_cluster = false;
                continue;
            }
            cursor += bid.length;
            const bsz = decodeEbmlVint(data[cursor..cluster_end])?;
            cursor += bsz.length;
            if (bsz.is_unknown) {
                return WebmMuxError.InvalidWebmContainer;
            }
            const blen: usize = bsz.value as usize;
            if (cursor + blen > cluster_end) {
                return WebmMuxError.TruncatedData;
            }
            const body = data[cursor..(cursor + blen)];
            cursor += blen;

            if (bid.id == WEBM_ID_TIMECODE) {
                cluster_timecode_ms = readBeUintN(body);
            } else if (bid.id == WEBM_ID_SIMPLE_BLOCK) {
                const tr_vint = decodeEbmlVint(body)?;
                if (body.len < tr_vint.length + 3) {
                    return WebmMuxError.InvalidSimpleBlock;
                }
                const rel_u16: u16 = readBeU16(body, tr_vint.length);
                const rel_i16: i16 = @as(i16, @bitCast(rel_u16));
                const flags: u8 = body[tr_vint.length + 2];
                const ts_i64: i64 = (cluster_timecode_ms as i64) + (rel_i16 as i64);
                const ts_ms: u64 = if (ts_i64 < 0) 0 else (ts_i64 as u64);
                return WebmDemuxFrame{
                    track_number: tr_vint.value,
                    timestamp_ms: ts_ms,
                    relative_timecode: rel_i16,
                    flags: flags,
                    is_keyframe: (flags & WEBM_BLOCK_FLAG_KEYFRAME) != 0,
                    is_invisible: (flags & WEBM_BLOCK_FLAG_INVISIBLE) != 0,
                    is_discardable: (flags & WEBM_BLOCK_FLAG_DISCARDABLE) != 0,
                    payload: body[(tr_vint.length + 3)..body.len],
                };
            }
        }
        return null;
    }
}

// ============================================================================
// 5. VP8 (RFC 6386), VP9 & AV1 OBU (AOMedia §5) Bitstream Parsers
// ============================================================================

/// Parsed RFC 6386 §9.1 VP8 uncompressed frame header (plus keyframe dimensions when `is_keyframe`).
pub struct Vp8FrameHeader {
    is_keyframe: bool = false;
    version: u8 = 0;
    show_frame: bool = false;
    first_part_size: u32 = 0;
    width: u16 = 0;
    height: u16 = 0;
    horizontal_scale: u8 = 0;
    vertical_scale: u8 = 0;
}

/// Parses an RFC 6386 §9.1 VP8 frame header. For keyframes, validates sync code `0x9D 0x01 0x2A`
/// and extracts `width`, `height`, `horizontal_scale`, and `vertical_scale`.
pub fn parseVp8FrameHeader(frame: []const u8): Vp8FrameHeader ! WebmMuxError {
    if (frame.len < 3) {
        return WebmMuxError.InvalidVp8Frame;
    }
    const tag: u32 = (frame[0] as u32) | ((frame[1] as u32) << 8) | ((frame[2] as u32) << 16);
    const is_key: bool = (tag & 1) == 0;
    const ver: u8 = ((tag >> 1) & 0x07) as u8;
    const show: bool = ((tag >> 4) & 1) != 0;
    const first_part: u32 = tag >> 5;

    let hdr = Vp8FrameHeader{
        is_keyframe: is_key,
        version: ver,
        show_frame: show,
        first_part_size: first_part,
        width: 0,
        height: 0,
        horizontal_scale: 0,
        vertical_scale: 0,
    };

    if (is_key) {
        if (frame.len < 10) {
            return WebmMuxError.InvalidVp8Frame;
        }
        if (frame[3] != VP8_KEYFRAME_SYNC_0 || frame[4] != VP8_KEYFRAME_SYNC_1 || frame[5] != VP8_KEYFRAME_SYNC_2) {
            return WebmMuxError.InvalidVp8Frame;
        }
        const w_raw: u16 = (frame[6] as u16) | ((frame[7] as u16) << 8);
        const h_raw: u16 = (frame[8] as u16) | ((frame[9] as u16) << 8);
        hdr.width = w_raw & 0x3FFF;
        hdr.horizontal_scale = ((w_raw >> 14) & 0x03) as u8;
        hdr.height = h_raw & 0x3FFF;
        hdr.vertical_scale = ((h_raw >> 14) & 0x03) as u8;
    }
    return hdr;
}

/// Parsed VP9 uncompressed header (Bitstream Specification §6.2).
pub struct Vp9FrameHeader {
    profile: u8 = 0;
    show_existing_frame: bool = false;
    frame_to_show_map_idx: u8 = 0;
    is_keyframe: bool = false;
    show_frame: bool = false;
    error_resilient_mode: bool = false;
    bit_depth: u8 = 8;
    color_space: u8 = 0;
    color_range: bool = false;
    subsampling_x: u8 = 1;
    subsampling_y: u8 = 1;
    width: u16 = 0;
    height: u16 = 0;
}

/// Parses a VP9 uncompressed header (`frame_marker == 0b10`, profiles 0..3).
/// For keyframes, verifies the `0x49 0x83 0x42` sync code and decodes color config and dimensions.
pub fn parseVp9FrameHeader(frame: []const u8): Vp9FrameHeader ! WebmMuxError {
    if (frame.len < 2) {
        return WebmMuxError.InvalidVp9Frame;
    }
    let br = WebmBitReader.init(frame);
    const marker = br.readBits(2)?;
    if (marker != 2) {
        return WebmMuxError.InvalidVp9Frame;
    }
    const profile_low = br.readBit()?;
    const profile_high = br.readBit()?;
    const profile: u8 = (profile_high << 1) | profile_low;
    if (profile == 3) {
        const reserved = br.readBit()?;
        if (reserved != 0) {
            return WebmMuxError.InvalidVp9Frame;
        }
    }

    let hdr = Vp9FrameHeader{
        profile: profile,
        show_existing_frame: false,
        frame_to_show_map_idx: 0,
        is_keyframe: false,
        show_frame: false,
        error_resilient_mode: false,
        bit_depth: 8,
        color_space: 0,
        color_range: false,
        subsampling_x: 1,
        subsampling_y: 1,
        width: 0,
        height: 0,
    };

    const show_existing = br.readBit()?;
    if (show_existing != 0) {
        hdr.show_existing_frame = true;
        hdr.frame_to_show_map_idx = (br.readBits(3)?) as u8;
        return hdr;
    }

    const frame_type = br.readBit()?;
    hdr.is_keyframe = (frame_type == 0);
    hdr.show_frame = (br.readBit()?) != 0;
    hdr.error_resilient_mode = (br.readBit()?) != 0;

    if (hdr.is_keyframe) {
        const s0 = (br.readBits(8)?) as u8;
        const s1 = (br.readBits(8)?) as u8;
        const s2 = (br.readBits(8)?) as u8;
        if (s0 != VP9_KEYFRAME_SYNC_0 || s1 != VP9_KEYFRAME_SYNC_1 || s2 != VP9_KEYFRAME_SYNC_2) {
            return WebmMuxError.InvalidVp9Frame;
        }

        // color_config() per VP9 Spec §6.2.2
        if (profile >= 2) {
            const ten_or_twelve = br.readBit()?;
            hdr.bit_depth = if (ten_or_twelve != 0) 12 else 10;
        } else {
            hdr.bit_depth = 8;
        }
        hdr.color_space = (br.readBits(3)?) as u8;
        if (hdr.color_space != 7) {
            hdr.color_range = (br.readBit()?) != 0;
            if (profile == 1 || profile == 3) {
                hdr.subsampling_x = br.readBit()?;
                hdr.subsampling_y = br.readBit()?;
                * = br.readBit()?; // reserved_zero
            } else {
                hdr.subsampling_x = 1;
                hdr.subsampling_y = 1;
            }
        } else {
            hdr.color_range = true;
            hdr.subsampling_x = 0;
            hdr.subsampling_y = 0;
            if (profile == 1 || profile == 3) {
                * = br.readBit()?; // reserved_zero
            }
        }

        // frame_size() per VP9 Spec §6.2.3
        const w_minus_1 = br.readBits(16)?;
        const h_minus_1 = br.readBits(16)?;
        hdr.width = (w_minus_1 + 1) as u16;
        hdr.height = (h_minus_1 + 1) as u16;
    }
    return hdr;
}

/// Decoded AV1 unsigned LEB128 integer (§4.10.5).
pub struct Av1Leb128Decoded {
    value: u64 = 0;
    length: usize = 0;
}

/// Decodes an AV1 LEB128 unsigned integer (1..8 bytes, value <= `0xFFFFFFFF`).
pub fn decodeAv1Leb128(data: []const u8): Av1Leb128Decoded ! WebmMuxError {
    if (data.len == 0) {
        return WebmMuxError.TruncatedData;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        if (i >= data.len) {
            return WebmMuxError.TruncatedData;
        }
        const b: u8 = data[i];
        const shift: u6 = @as(u6, @intCast(i * 7));
        val |= ((b & 0x7F) as u64) << shift;
        if ((b & 0x80) == 0) {
            if (val > 0xFFFFFFFF) {
                return WebmMuxError.InvalidAv1Obu;
            }
            return Av1Leb128Decoded{
                value: val,
                length: i + 1,
            };
        }
    }
    return WebmMuxError.InvalidAv1Obu;
}

/// Encodes a 32-bit unsigned integer as AV1 LEB128 into `out`.
pub fn encodeAv1Leb128(value: u32, out: []u8): usize ! WebmMuxError {
    let v: u32 = value;
    let i: usize = 0;
    while (true) {
        if (i >= out.len) {
            return WebmMuxError.BufferTooSmall;
        }
        let b: u8 = (v & 0x7F) as u8;
        v >>= 7;
        if (v != 0) {
            b |= 0x80;
            out[i] = b;
            i += 1;
        } else {
            out[i] = b;
            return i + 1;
        }
    }
}

/// Zero-copy view of a single AOMedia AV1 Open Bitstream Unit (OBU).
pub struct Av1ObuView {
    obu_type: u8 = 0;
    has_extension: bool = false;
    has_size_field: bool = false;
    temporal_id: u8 = 0;
    spatial_id: u8 = 0;
    payload: []const u8 = "";
    raw: []const u8 = "";
}

/// Zero-copy iterator over AV1 OBUs (`obu_header` + optional extension + LEB128 size + payload).
pub struct Av1ObuIter {
    data: []const u8 = "";
    offset: usize = 0;

    static fn init(bitstream: []const u8): Av1ObuIter {
        return Av1ObuIter{
            data: bitstream,
            offset: 0,
        };
    }

    fn next(*_): Av1ObuView? ! WebmMuxError {
        if (offset >= data.len) {
            return null;
        }
        const start: usize = offset;
        const hdr: u8 = data[offset];
        offset += 1;

        // Bit 7 is obu_forbidden_bit (must be 0)
        if ((hdr & 0x80) != 0) {
            return WebmMuxError.InvalidAv1Obu;
        }
        const otype: u8 = (hdr >> 3) & 0x0F;
        const has_ext: bool = ((hdr >> 2) & 1) != 0;
        const has_sz: bool = ((hdr >> 1) & 1) != 0;

        let tid: u8 = 0;
        let sid: u8 = 0;
        if (has_ext) {
            if (offset >= data.len) {
                return WebmMuxError.TruncatedData;
            }
            const ext: u8 = data[offset];
            offset += 1;
            tid = (ext >> 5) & 0x07;
            sid = (ext >> 3) & 0x03;
        }

        let payload_len: usize = 0;
        if (has_sz) {
            const leb = decodeAv1Leb128(data[offset..data.len])?;
            offset += leb.length;
            payload_len = leb.value as usize;
            if (offset + payload_len > data.len) {
                return WebmMuxError.TruncatedData;
            }
        } else {
            payload_len = data.len - offset;
        }

        const payload_slice = data[offset..(offset + payload_len)];
        offset += payload_len;
        return Av1ObuView{
            obu_type: otype,
            has_extension: has_ext,
            has_size_field: has_sz,
            temporal_id: tid,
            spatial_id: sid,
            payload: payload_slice,
            raw: data[start..offset],
        };
    }
}

/// Decoded AOMedia AV1 Sequence Header OBU basic parameters & max frame resolution (§5.5.1).
pub struct Av1SequenceHeaderInfo {
    seq_profile: u8 = 0;
    still_picture: bool = false;
    reduced_still_picture_header: bool = false;
    seq_level_idx_0: u8 = 0;
    frame_width_bits_minus_1: u8 = 0;
    frame_height_bits_minus_1: u8 = 0;
    max_frame_width: u32 = 0;
    max_frame_height: u32 = 0;
}

/// Parses an AV1 `OBU_SEQUENCE_HEADER` payload to extract profile, still-picture flags,
/// level index 0, and `max_frame_width` x `max_frame_height`.
pub fn parseAv1SequenceHeaderBasic(payload: []const u8): Av1SequenceHeaderInfo ! WebmMuxError {
    if (payload.len < 4) {
        return WebmMuxError.InvalidAv1SequenceHeader;
    }
    let br = WebmBitReader.init(payload);
    const seq_profile: u8 = (br.readBits(3)?) as u8;
    const still_pic: bool = (br.readBit()?) != 0;
    const reduced_hdr: bool = (br.readBit()?) != 0;
    let level0: u8 = 0;

    if (reduced_hdr) {
        level0 = (br.readBits(5)?) as u8;
    } else {
        const timing_present = br.readBit()?;
        let dec_model_present: u8 = 0;
        let buffer_delay_len_minus_1: usize = 0;
        if (timing_present != 0) {
            // num_units_in_display_tick (32) + time_scale (32)
            * = br.skipBits(64)?;
            const eq_pic_int = br.readBit()?;
            if (eq_pic_int != 0) {
                // Skip uvlc(): count leading zeros, then read that many bits
                let lz: usize = 0;
                while (lz < 32) {
                    const b = br.readBit()?;
                    if (b != 0) break;
                    lz += 1;
                }
                if (lz < 32) {
                    * = br.skipBits(lz)?;
                }
            }
            dec_model_present = br.readBit()?;
            if (dec_model_present != 0) {
                buffer_delay_len_minus_1 = (br.readBits(5)?) as usize;
                // num_units_in_decoding_tick(32) + buffer_removal_time_length_minus_1(5) + frame_presentation_time_length_minus_1(5)
                * = br.skipBits(42)?;
            }
        }
        const initial_delay_present = br.readBit()?;
        const op_cnt_minus_1: usize = (br.readBits(5)?) as usize;
        let op: usize = 0;
        while (op <= op_cnt_minus_1) : (op += 1) {
            * = br.skipBits(12)?; // operating_point_idc
            const lvl: u8 = (br.readBits(5)?) as u8;
            if (op == 0) {
                level0 = lvl;
            }
            if (lvl > 7) {
                * = br.skipBits(1)?; // seq_tier
            }
            if (dec_model_present != 0) {
                const dm_op = br.readBit()?;
                if (dm_op != 0) {
                    const n_bits: usize = buffer_delay_len_minus_1 + 1;
                    * = br.skipBits(n_bits * 2 + 1)?;
                }
            }
            if (initial_delay_present != 0) {
                const id_op = br.readBit()?;
                if (id_op != 0) {
                    * = br.skipBits(4)?;
                }
            }
        }
    }

    const fw_bits_m1: u8 = (br.readBits(4)?) as u8;
    const fh_bits_m1: u8 = (br.readBits(4)?) as u8;
    const max_w_m1: u32 = br.readBits((fw_bits_m1 as usize) + 1)?;
    const max_h_m1: u32 = br.readBits((fh_bits_m1 as usize) + 1)?;

    return Av1SequenceHeaderInfo{
        seq_profile: seq_profile,
        still_picture: still_pic,
        reduced_still_picture_header: reduced_hdr,
        seq_level_idx_0: level0,
        frame_width_bits_minus_1: fw_bits_m1,
        frame_height_bits_minus_1: fh_bits_m1,
        max_frame_width: max_w_m1 + 1,
        max_frame_height: max_h_m1 + 1,
    };
}

// ============================================================================
// 6. WebRTC VP8 RTP Payload Descriptor, Packetizer & Depacketizer (RFC 7741)
// ============================================================================

/// RFC 7741 §4.2 VP8 RTP payload descriptor supporting mandatory + `X/I/L/T/K` extension fields.
pub struct Vp8RtpDescriptor {
    x: bool = false;
    n: bool = false;
    s: bool = false;
    pid: u8 = 0;
    has_pic_id: bool = false;
    is_15bit_pic_id: bool = false;
    pic_id: u16 = 0;
    has_tl0picidx: bool = false;
    tl0picidx: u8 = 0;
    has_tid: bool = false;
    has_keyidx: bool = false;
    tid: u8 = 0;
    y: bool = false;
    keyidx: u8 = 0;

    /// Returns the encoded wire length in bytes (`1..6`).
    fn encodedLen(): usize {
        const use_x: bool = x || has_pic_id || has_tl0picidx || has_tid || has_keyidx;
        if (!use_x) {
            return 1;
        }
        let len: usize = 2;
        if (has_pic_id) {
            len += if (is_15bit_pic_id || pic_id > 127) 2 else 1;
        }
        if (has_tl0picidx) {
            len += 1;
        }
        if (has_tid || has_keyidx) {
            len += 1;
        }
        return len;
    }
}

/// Decoded RFC 7741 VP8 RTP payload descriptor plus consumed header byte length.
pub struct Vp8RtpDescriptorDecoded {
    desc: Vp8RtpDescriptor = Vp8RtpDescriptor{};
    header_len: usize = 0;
}

/// Encodes an RFC 7741 §4.2 VP8 RTP payload descriptor into `out`.
pub fn encodeVp8RtpDescriptor(desc: Vp8RtpDescriptor, out: []u8): usize ! WebmMuxError {
    const need: usize = desc.encodedLen();
    if (out.len < need) {
        return WebmMuxError.BufferTooSmall;
    }
    const use_x: bool = desc.x || desc.has_pic_id || desc.has_tl0picidx || desc.has_tid || desc.has_keyidx;
    let b0: u8 = desc.pid & 0x07;
    if (use_x) b0 |= 0x80;
    if (desc.n) b0 |= 0x20;
    if (desc.s) b0 |= 0x10;
    out[0] = b0;
    if (!use_x) {
        return 1;
    }

    let b1: u8 = 0;
    if (desc.has_pic_id) b1 |= 0x80;
    if (desc.has_tl0picidx) b1 |= 0x40;
    if (desc.has_tid) b1 |= 0x20;
    if (desc.has_keyidx) b1 |= 0x10;
    out[1] = b1;
    let pos: usize = 2;

    if (desc.has_pic_id) {
        if (desc.is_15bit_pic_id || desc.pic_id > 127) {
            out[pos] = (((desc.pic_id >> 8) & 0x7F) as u8) | 0x80;
            out[pos + 1] = (desc.pic_id & 0xFF) as u8;
            pos += 2;
        } else {
            out[pos] = (desc.pic_id & 0x7F) as u8;
            pos += 1;
        }
    }
    if (desc.has_tl0picidx) {
        out[pos] = desc.tl0picidx;
        pos += 1;
    }
    if (desc.has_tid || desc.has_keyidx) {
        let tk: u8 = ((desc.tid & 0x03) << 6) | (desc.keyidx & 0x1F);
        if (desc.y) tk |= 0x20;
        out[pos] = tk;
        pos += 1;
    }
    return pos;
}

/// Decodes an RFC 7741 §4.2 VP8 RTP payload descriptor from `pkt`.
pub fn decodeVp8RtpDescriptor(pkt: []const u8): Vp8RtpDescriptorDecoded ! WebmMuxError {
    if (pkt.len < 1) {
        return WebmMuxError.InvalidVp8RtpPacket;
    }
    const b0: u8 = pkt[0];
    let d = Vp8RtpDescriptor{
        x: (b0 & 0x80) != 0,
        n: (b0 & 0x20) != 0,
        s: (b0 & 0x10) != 0,
        pid: b0 & 0x07,
        has_pic_id: false,
        is_15bit_pic_id: false,
        pic_id: 0,
        has_tl0picidx: false,
        tl0picidx: 0,
        has_tid: false,
        has_keyidx: false,
        tid: 0,
        y: false,
        keyidx: 0,
    };
    let pos: usize = 1;
    if (d.x) {
        if (pos >= pkt.len) return WebmMuxError.InvalidVp8RtpPacket;
        const b1: u8 = pkt[pos];
        pos += 1;
        d.has_pic_id = (b1 & 0x80) != 0;
        d.has_tl0picidx = (b1 & 0x40) != 0;
        d.has_tid = (b1 & 0x20) != 0;
        d.has_keyidx = (b1 & 0x10) != 0;

        if (d.has_pic_id) {
            if (pos >= pkt.len) return WebmMuxError.InvalidVp8RtpPacket;
            const p0: u8 = pkt[pos];
            pos += 1;
            if ((p0 & 0x80) != 0) {
                if (pos >= pkt.len) return WebmMuxError.InvalidVp8RtpPacket;
                const p1: u8 = pkt[pos];
                pos += 1;
                d.is_15bit_pic_id = true;
                d.pic_id = (((p0 & 0x7F) as u16) << 8) | (p1 as u16);
            } else {
                d.is_15bit_pic_id = false;
                d.pic_id = (p0 & 0x7F) as u16;
            }
        }
        if (d.has_tl0picidx) {
            if (pos >= pkt.len) return WebmMuxError.InvalidVp8RtpPacket;
            d.tl0picidx = pkt[pos];
            pos += 1;
        }
        if (d.has_tid || d.has_keyidx) {
            if (pos >= pkt.len) return WebmMuxError.InvalidVp8RtpPacket;
            const tk: u8 = pkt[pos];
            pos += 1;
            d.tid = (tk >> 6) & 0x03;
            d.y = ((tk >> 5) & 1) != 0;
            d.keyidx = tk & 0x1F;
        }
    }
    return Vp8RtpDescriptorDecoded{
        desc: d,
        header_len: pos,
    };
}

/// Offset/length descriptor for a packetized VP8 RTP payload inside a caller-provided slab.
pub struct Vp8RtpPacketSpan {
    offset: usize = 0;
    len: usize = 0;

    /// Returns the zero-copy byte slice for this packet inside `slab`.
    fn slice(slab: []const u8): []const u8 {
        return slab[offset..(offset + len)];
    }
}

/// Splits a VP8 encoded frame into MTU-bounded RFC 7741 RTP payloads (`S=1` on first packet, `S=0` on continuations).
/// Zero dynamic heap allocation: writes packet bytes into `out_buf` and spans into `out_spans`, returning packet count.
pub fn packetizeVp8Rtp(
    frame: []const u8,
    mtu: usize,
    base_desc: Vp8RtpDescriptor,
    out_buf: []u8,
    out_spans: []Vp8RtpPacketSpan,
): usize ! WebmMuxError {
    if (frame.len == 0) {
        return WebmMuxError.InvalidVp8Frame;
    }
    const hdr_len: usize = base_desc.encodedLen();
    if (mtu <= hdr_len) {
        return WebmMuxError.MtuTooSmall;
    }
    const max_chunk: usize = mtu - hdr_len;
    let src_off: usize = 0;
    let dst_off: usize = 0;
    let pkt_count: usize = 0;

    while (src_off < frame.len) {
        if (pkt_count >= out_spans.len) {
            return WebmMuxError.PacketSpanOverflow;
        }
        const rem: usize = frame.len - src_off;
        const take: usize = if (rem > max_chunk) max_chunk else rem;
        const pkt_len: usize = hdr_len + take;
        if (dst_off + pkt_len > out_buf.len) {
            return WebmMuxError.BufferTooSmall;
        }

        const d = Vp8RtpDescriptor{
            x: base_desc.x,
            n: base_desc.n,
            s: (pkt_count == 0),
            pid: 0,
            has_pic_id: base_desc.has_pic_id,
            is_15bit_pic_id: base_desc.is_15bit_pic_id,
            pic_id: base_desc.pic_id,
            has_tl0picidx: base_desc.has_tl0picidx,
            tl0picidx: base_desc.tl0picidx,
            has_tid: base_desc.has_tid,
            has_keyidx: base_desc.has_keyidx,
            tid: base_desc.tid,
            y: base_desc.y,
            keyidx: base_desc.keyidx,
        };
        const written_hdr = encodeVp8RtpDescriptor(d, out_buf[dst_off..(dst_off + hdr_len)])?;
        copyBytes(out_buf, dst_off + written_hdr, frame[src_off..(src_off + take)]);

        out_spans[pkt_count] = Vp8RtpPacketSpan{
            offset: dst_off,
            len: written_hdr + take,
        };
        dst_off += written_hdr + take;
        src_off += take;
        pkt_count += 1;
    }
    return pkt_count;
}

/// Outcome returned when feeding an RTP packet into `Vp8RtpDepacketizer`.
pub enum Vp8DepacketizeOutcome {
    in_progress,
    frame_ready,
    dropped_incomplete,
}

/// Zero-heap, loss-resilient RFC 7741 VP8 RTP depacketizer backed by a caller-provided external slice.
/// Detects sequence gaps and truncated/orphaned frames, automatically dropping incomplete frames and tracking stats.
pub struct Vp8RtpDepacketizer {
    buf: []u8 = [];
    written: usize = 0;
    in_frame: bool = false;
    has_seq: bool = false;
    last_seq: u16 = 0;
    last_pic_id: u16 = 0;
    frames_completed: u64 = 0;
    frames_dropped: u64 = 0;
    packets_received: u64 = 0;

    /// Initializes the depacketizer over caller-provided external `storage` slice.
    static fn init(storage: []u8): Vp8RtpDepacketizer ! WebmMuxError {
        if (storage.len == 0) {
            return WebmMuxError.BufferTooSmall;
        }
        return Vp8RtpDepacketizer{
            buf: storage,
            written: 0,
            in_frame: false,
            has_seq: false,
            last_seq: 0,
            last_pic_id: 0,
            frames_completed: 0,
            frames_dropped: 0,
            packets_received: 0,
        };
    }

    /// Resets in-progress assembly state without clearing cumulative counters.
    fn reset(*_) {
        written = 0;
        in_frame = false;
    }

    /// Returns the assembled VP8 frame slice (valid immediately after `.frame_ready`).
    fn frame(): []const u8 {
        return buf[0..written];
    }

    /// Feeds one RFC 7741 VP8 RTP payload with its 16-bit `rtp_seq` and RTP `marker` bit.
    fn pushRtpPacket(*_, rtp_seq: u16, marker: bool, rtp_payload: []const u8): Vp8DepacketizeOutcome ! WebmMuxError {
        const parsed = decodeVp8RtpDescriptor(rtp_payload)?;
        if (parsed.header_len >= rtp_payload.len) {
            return WebmMuxError.InvalidVp8RtpPacket;
        }
        const vp8_data = rtp_payload[parsed.header_len..rtp_payload.len];
        packets_received += 1;

        const is_start: bool = parsed.desc.s && parsed.desc.pid == 0;
        if (is_start) {
            if (in_frame) {
                // Previous frame never saw marker=true -> drop incomplete frame
                frames_dropped += 1;
            }
            if (vp8_data.len > buf.len) {
                in_frame = false;
                written = 0;
                return WebmMuxError.BufferTooSmall;
            }
            copyBytes(buf, 0, vp8_data);
            written = vp8_data.len;
            in_frame = true;
            has_seq = true;
            last_seq = rtp_seq;
            if (parsed.desc.has_pic_id) {
                last_pic_id = parsed.desc.pic_id;
            }
        } else {
            // Continuation packet: must have active frame and contiguous 16-bit RTP sequence
            const expected_seq: u16 = last_seq +% 1;
            if (!in_frame || !has_seq || rtp_seq != expected_seq) {
                if (in_frame) {
                    frames_dropped += 1;
                }
                in_frame = false;
                written = 0;
                has_seq = true;
                last_seq = rtp_seq;
                return Vp8DepacketizeOutcome.dropped_incomplete;
            }
            if (written + vp8_data.len > buf.len) {
                in_frame = false;
                written = 0;
                frames_dropped += 1;
                return WebmMuxError.BufferTooSmall;
            }
            copyBytes(buf, written, vp8_data);
            written += vp8_data.len;
            last_seq = rtp_seq;
        }

        if (marker) {
            in_frame = false;
            frames_completed += 1;
            return Vp8DepacketizeOutcome.frame_ready;
        }
        return Vp8DepacketizeOutcome.in_progress;
    }
}
