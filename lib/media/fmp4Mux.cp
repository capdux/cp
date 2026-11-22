// Origin: Pure-CP Stdlib (ISO/IEC 14496-12 CMAF fMP4 Mux/Demux + RFC 3533/7845 Ogg-Opus Streaming Container) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice streaming media container engine:
//   - ISO/IEC 14496-12 / CMAF Fragmented MP4 (fMP4) Init Segment (`ftyp` + `moov`/`mvhd`/`trak`/`mdia`/`mvex`/`trex`)
//   - Zero-copy `Fmp4BoxIter` supporting 32-bit & 64-bit `largesize` boxes with truncation guards
//   - CMAF Media Fragment (`moof`/`mfhd`/`traf`/`tfhd`/`tfdt`(64-bit)/`trun` + `mdat`) muxing & demuxing
//     with exact `trun.data_offset == moof_size + 8` alignment, keyframe flags, and B-frame signed `cts_offset`
//   - RFC 3533 Ogg page framing (`encodeOggPage` / `parseOggPage`) with 255-byte lacing segmentation and
//     non-reflected `0x04C11DB7` Ogg CRC32 verification
//   - RFC 7845 `OpusHead` (19-byte ID header) & `OpusTags` (vendor/comment metadata) codec

// ============================================================================
// 1. Constants & Error Set
// ============================================================================

/// ISO BMFF 4CC box type constants (big-endian packed ASCII).
pub const FMP4_BOX_FTYP: u32 = 0x66747970; // "ftyp"
pub const FMP4_BOX_MOOV: u32 = 0x6D6F6F76; // "moov"
pub const FMP4_BOX_MVHD: u32 = 0x6D766864; // "mvhd"
pub const FMP4_BOX_TRAK: u32 = 0x7472616B; // "trak"
pub const FMP4_BOX_TKHD: u32 = 0x746B6864; // "tkhd"
pub const FMP4_BOX_MDIA: u32 = 0x6D646961; // "mdia"
pub const FMP4_BOX_MDHD: u32 = 0x6D646864; // "mdhd"
pub const FMP4_BOX_HDLR: u32 = 0x68646C72; // "hdlr"
pub const FMP4_BOX_MINF: u32 = 0x6D696E66; // "minf"
pub const FMP4_BOX_STBL: u32 = 0x7374626C; // "stbl"
pub const FMP4_BOX_MVEX: u32 = 0x6D766578; // "mvex"
pub const FMP4_BOX_TREX: u32 = 0x74726578; // "trex"
pub const FMP4_BOX_MOOF: u32 = 0x6D6F6F66; // "moof"
pub const FMP4_BOX_MFHD: u32 = 0x6D666864; // "mfhd"
pub const FMP4_BOX_TRAF: u32 = 0x74726166; // "traf"
pub const FMP4_BOX_TFHD: u32 = 0x74666864; // "tfhd"
pub const FMP4_BOX_TFDT: u32 = 0x74666474; // "tfdt"
pub const FMP4_BOX_TRUN: u32 = 0x7472756E; // "trun"
pub const FMP4_BOX_MDAT: u32 = 0x6D646174; // "mdat"

/// ISO/IEC 14496-12 §8.8.3.1 sample flag: sync sample (keyframe / IDR, `sample_depends_on=2`, `is_non_sync=0`).
pub const FMP4_SAMPLE_FLAG_SYNC: u32 = 0x02000000;

/// ISO/IEC 14496-12 §8.8.3.1 sample flag: non-sync sample (P/B frame, `sample_depends_on=1`, `is_non_sync=1`).
pub const FMP4_SAMPLE_FLAG_NON_SYNC: u32 = 0x01010000;

/// RFC 3533 Ogg page capture pattern (`"OggS"`).
pub const OGG_CAPTURE_PATTERN: []const u8 = "OggS";

/// RFC 3533 Ogg page header flags.
pub const OGG_PAGE_FLAG_CONT: u8 = 0x01;
pub const OGG_PAGE_FLAG_BOS: u8 = 0x02;
pub const OGG_PAGE_FLAG_EOS: u8 = 0x04;

/// RFC 7845 §5.1 minimum OpusHead identification header size in bytes.
pub const OPUS_HEAD_MIN_LEN: usize = 19;

const FMP4_BOX_STSD: u32 = 0x73747364; // "stsd"
const FMP4_BOX_STTS: u32 = 0x73747473; // "stts"
const FMP4_BOX_STSC: u32 = 0x73747363; // "stsc"
const FMP4_BOX_STSZ: u32 = 0x7374737A; // "stsz"
const FMP4_BOX_STCO: u32 = 0x7374636F; // "stco"
const FMP4_BOX_VMHD: u32 = 0x766D6864; // "vmhd"
const FMP4_BOX_SMHD: u32 = 0x736D6864; // "smhd"
const FMP4_BOX_DINF: u32 = 0x64696E66; // "dinf"
const FMP4_BOX_DREF: u32 = 0x64726566; // "dref"
const FMP4_BOX_URL: u32 = 0x75726C20;  // "url "

const OGG_HEADER_FIXED_LEN: usize = 27;
const OGG_MAX_SEGMENTS: usize = 255;

/// Error set for fMP4 muxing/demuxing and Ogg-Opus container operations.
pub err Fmp4MuxError {
    InvalidBox,
    TruncatedBox,
    BufferTooSmall,
    OutputSliceTooSmall,
    InvalidConfig,
    InvalidOggMagic,
    OggChecksumMismatch,
    InvalidOpusHead,
    TooManySegments,
}

// ============================================================================
// 2. Byte-Order, Ogg CRC32 & Nested Box Writer Helpers
// ============================================================================

fn readBeU16(buf: []const u8, off: usize): u16 {
    return ((buf[off] as u16) << 8) | (buf[off + 1] as u16);
}

fn readBeU32(buf: []const u8, off: usize): u32 {
    return ((buf[off] as u32) << 24)
        | ((buf[off + 1] as u32) << 16)
        | ((buf[off + 2] as u32) << 8)
        | (buf[off + 3] as u32);
}

fn readBeI32(buf: []const u8, off: usize): i32 {
    const u: u32 = readBeU32(buf, off);
    return @bitCast(u);
}

fn readBeU64(buf: []const u8, off: usize): u64 {
    const hi: u64 = readBeU32(buf, off) as u64;
    const lo: u64 = readBeU32(buf, off + 4) as u64;
    return (hi << 32) | lo;
}

fn writeBeU16(buf: []u8, off: usize, v: u16) {
    buf[off] = ((v >> 8) & 0xFF) as u8;
    buf[off + 1] = (v & 0xFF) as u8;
}

fn writeBeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = ((v >> 24) & 0xFF) as u8;
    buf[off + 1] = ((v >> 16) & 0xFF) as u8;
    buf[off + 2] = ((v >> 8) & 0xFF) as u8;
    buf[off + 3] = (v & 0xFF) as u8;
}

fn writeBeI32(buf: []u8, off: usize, v: i32) {
    const u: u32 = @bitCast(v);
    writeBeU32(buf, off, u);
}

fn writeBeU64(buf: []u8, off: usize, v: u64) {
    writeBeU32(buf, off, ((v >> 32) & 0xFFFFFFFF) as u32);
    writeBeU32(buf, off + 4, (v & 0xFFFFFFFF) as u32);
}

fn readLeU16(buf: []const u8, off: usize): u16 {
    return (buf[off] as u16) | ((buf[off + 1] as u16) << 8);
}

fn readLeI16(buf: []const u8, off: usize): i16 {
    const u: u16 = readLeU16(buf, off);
    return @bitCast(u);
}

fn readLeU32(buf: []const u8, off: usize): u32 {
    return (buf[off] as u32)
        | ((buf[off + 1] as u32) << 8)
        | ((buf[off + 2] as u32) << 16)
        | ((buf[off + 3] as u32) << 24);
}

fn readLeU64(buf: []const u8, off: usize): u64 {
    const lo: u64 = readLeU32(buf, off) as u64;
    const hi: u64 = readLeU32(buf, off + 4) as u64;
    return lo | (hi << 32);
}

fn writeLeU16(buf: []u8, off: usize, v: u16) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
}

fn writeLeI16(buf: []u8, off: usize, v: i16) {
    const u: u16 = @bitCast(v);
    writeLeU16(buf, off, u);
}

fn writeLeU32(buf: []u8, off: usize, v: u32) {
    buf[off] = (v & 0xFF) as u8;
    buf[off + 1] = ((v >> 8) & 0xFF) as u8;
    buf[off + 2] = ((v >> 16) & 0xFF) as u8;
    buf[off + 3] = ((v >> 24) & 0xFF) as u8;
}

fn writeLeU64(buf: []u8, off: usize, v: u64) {
    writeLeU32(buf, off, (v & 0xFFFFFFFF) as u32);
    writeLeU32(buf, off + 4, ((v >> 32) & 0xFFFFFFFF) as u32);
}

fn oggCrc32Step(crc_in: u32, byte: u8): u32 {
    let crc: u32 = crc_in ^ ((byte as u32) << 24);
    let bit: u32 = 0;
    while (bit < 8) : (bit += 1) {
        if ((crc & 0x80000000) != 0) {
            crc = (crc << 1) ^ 0x04C11DB7;
        } else {
            crc = crc << 1;
        }
    }
    return crc;
}

/// Computes the standard RFC 3533 Ogg CRC-32 (non-reflected polynomial `0x04C11DB7`, init `0`, no final XOR).
pub fn computeOggCrc32(data: []const u8): u32 {
    let crc: u32 = 0;
    let i: usize = 0;
    while (i < data.len) : (i += 1) {
        crc = oggCrc32Step(crc, data[i]);
    }
    return crc;
}

fn eqBytes(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

struct BoxWriter {
    buf: []u8 = [],
    pos: usize = 0,

    static fn init(buf: []u8): BoxWriter {
        return BoxWriter{ buf: buf, pos: 0 };
    }

    fn ensure(n: usize): void ! Fmp4MuxError {
        if (pos + n > buf.len) return Fmp4MuxError.BufferTooSmall;
    }

    fn putU8(*_, v: u8): void ! Fmp4MuxError {
        ensure(1)?;
        buf[pos] = v;
        pos += 1;
    }

    fn putBeU16(*_, v: u16): void ! Fmp4MuxError {
        ensure(2)?;
        writeBeU16(buf, pos, v);
        pos += 2;
    }

    fn putBeU32(*_, v: u32): void ! Fmp4MuxError {
        ensure(4)?;
        writeBeU32(buf, pos, v);
        pos += 4;
    }

    fn putBeI32(*_, v: i32): void ! Fmp4MuxError {
        ensure(4)?;
        writeBeI32(buf, pos, v);
        pos += 4;
    }

    fn putBeU64(*_, v: u64): void ! Fmp4MuxError {
        ensure(8)?;
        writeBeU64(buf, pos, v);
        pos += 8;
    }

    fn putZeros(*_, n: usize): void ! Fmp4MuxError {
        ensure(n)?;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            buf[pos + i] = 0;
        }
        pos += n;
    }

    fn putBytes(*_, src: []const u8): void ! Fmp4MuxError {
        ensure(src.len)?;
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            buf[pos + i] = src[i];
        }
        pos += src.len;
    }

    fn beginBox(*_, box_type: u32): usize ! Fmp4MuxError {
        const start = pos;
        putBeU32(0)?; // placeholder length
        putBeU32(box_type)?;
        return start;
    }

    fn beginFullBox(*_, box_type: u32, version: u8, flags: u32): usize ! Fmp4MuxError {
        const start = beginBox(box_type)?;
        const vf: u32 = ((version as u32) << 24) | (flags & 0x00FFFFFF);
        putBeU32(vf)?;
        return start;
    }

    fn endBox(*_, start: usize) {
        const len: usize = pos - start;
        writeBeU32(buf, start, @as(u32, @intCast(len)));
    }
}

// ============================================================================
// 3. ISO BMFF / CMAF fMP4 Box Iterator & Init Segment Codec
// ============================================================================

/// Media track codec kind for CMAF / fMP4 Init Segment generation.
pub enum Fmp4TrackKind {
    video_avc,
    video_hevc,
    video_av1,
    audio_aac,
    audio_opus,

    fn isVideo(): bool {
        return match (_) {
            Fmp4TrackKind.video_avc => true,
            Fmp4TrackKind.video_hevc => true,
            Fmp4TrackKind.video_av1 => true,
            Fmp4TrackKind.audio_aac => false,
            Fmp4TrackKind.audio_opus => false,
        };
    }

    fn handler4cc(): u32 {
        return if (isVideo()) 0x76696465 else 0x736F756E; // "vide" / "soun"
    }

    fn sampleEntry4cc(): u32 {
        return match (_) {
            Fmp4TrackKind.video_avc => 0x61766331,  // "avc1"
            Fmp4TrackKind.video_hevc => 0x68766331, // "hvc1"
            Fmp4TrackKind.video_av1 => 0x61763031,  // "av01"
            Fmp4TrackKind.audio_aac => 0x6D703461,  // "mp4a"
            Fmp4TrackKind.audio_opus => 0x4F707573, // "Opus"
        };
    }
}

/// Configuration for encoding an ISO/IEC 14496-12 / CMAF fMP4 Initialization Segment.
pub struct Fmp4InitConfig {
    track_id: u32 = 1;
    timescale: u32 = 90000;
    kind: Fmp4TrackKind = Fmp4TrackKind.video_avc;
    width: u16 = 1920;
    height: u16 = 1080;
    sample_rate_hz: u32 = 48000;
    channels: u16 = 2;

    fn isValid(): bool {
        if (track_id == 0 || timescale == 0) return false;
        if (kind.isVideo()) {
            if (width == 0 || height == 0) return false;
        } else {
            if (sample_rate_hz == 0 || channels == 0) return false;
        }
        return true;
    }
}

/// Zero-copy header metadata for a parsed ISO BMFF box.
pub struct Fmp4BoxHeader {
    box_type: u32 = 0;
    header_len: usize = 8;
    payload_offset: usize = 0;
    payload_len: usize = 0;
    total_len: usize = 0;
}

/// Zero-allocation iterator over sequential ISO BMFF boxes (supporting both 32-bit and 64-bit `largesize`).
pub struct Fmp4BoxIter {
    data: []const u8 = [],
    offset: usize = 0;

    static fn init(data: []const u8): Fmp4BoxIter {
        return Fmp4BoxIter{ data: data, offset: 0 };
    }

    /// Reads the next box header and advances the cursor, or returns `null` at EOF.
    fn next(*_): (Fmp4BoxHeader?) ! Fmp4MuxError {
        if (offset >= data.len) return null;
        if (offset + 8 > data.len) return Fmp4MuxError.TruncatedBox;

        const raw_size: u32 = readBeU32(data, offset);
        const btype: u32 = readBeU32(data, offset + 4);
        let hdr_len: usize = 8;
        let total_len: usize = 0;

        if (raw_size == 1) {
            if (offset + 16 > data.len) return Fmp4MuxError.TruncatedBox;
            const large: u64 = readBeU64(data, offset + 8);
            if (large < 16) return Fmp4MuxError.InvalidBox;
            if (large > @as(u64, data.len - offset)) return Fmp4MuxError.TruncatedBox;
            hdr_len = 16;
            total_len = @as(usize, @intCast(large));
        } else if (raw_size == 0) {
            total_len = data.len - offset;
        } else if (raw_size < 8) {
            return Fmp4MuxError.InvalidBox;
        } else {
            total_len = @as(usize, raw_size);
            if (offset + total_len > data.len) return Fmp4MuxError.TruncatedBox;
        }

        const hdr = Fmp4BoxHeader{
            box_type: btype,
            header_len: hdr_len,
            payload_offset: offset + hdr_len,
            payload_len: total_len - hdr_len,
            total_len: total_len,
        };
        offset += total_len;
        return hdr;
    }

    /// Returns the zero-copy payload slice for a box header produced by this iterator.
    fn payloadSlice(hdr: Fmp4BoxHeader): []const u8 {
        if (hdr.payload_offset + hdr.payload_len > data.len) return "";
        return data[hdr.payload_offset .. hdr.payload_offset + hdr.payload_len];
    }
}

fn writeUnityMatrix(w: *BoxWriter): void ! Fmp4MuxError {
    w.putBeU32(0x00010000)?;
    w.putBeU32(0)?;
    w.putBeU32(0)?;
    w.putBeU32(0)?;
    w.putBeU32(0x00010000)?;
    w.putBeU32(0)?;
    w.putBeU32(0)?;
    w.putBeU32(0)?;
    w.putBeU32(0x40000000)?;
}

/// Encodes an ISO/IEC 14496-12 / CMAF fMP4 Initialization Segment (`ftyp` + `moov` containing
/// `mvhd`, `trak`(`tkhd` + `mdia`(`mdhd`, `hdlr`, `minf`/`stbl`)), and `mvex`(`trex`)) into `out_buf`.
pub fn encodeFmp4InitSegment(cfg: Fmp4InitConfig, out_buf: []u8): usize ! Fmp4MuxError {
    if (!cfg.isValid()) {
        return Fmp4MuxError.InvalidConfig;
    }

    let w = BoxWriter.init(out_buf);

    // 1. `ftyp` box: major=isom, minor=512, compatible=[isom, iso6, cmfc, mp41]
    const ftyp = w.beginBox(FMP4_BOX_FTYP)?;
    w.putBytes("isom")?;
    w.putBeU32(0x00000200)?;
    w.putBytes("isom")?;
    w.putBytes("iso6")?;
    w.putBytes("cmfc")?;
    w.putBytes("mp41")?;
    w.endBox(ftyp);

    // 2. `moov` box
    const moov = w.beginBox(FMP4_BOX_MOOV)?;

    // 2a. `mvhd` (version 0)
    const mvhd = w.beginFullBox(FMP4_BOX_MVHD, 0, 0)?;
    w.putBeU32(0)?;              // creation_time
    w.putBeU32(0)?;              // modification_time
    w.putBeU32(cfg.timescale)?;  // timescale
    w.putBeU32(0)?;              // duration (fragmented = 0)
    w.putBeU32(0x00010000)?;     // rate = 1.0
    w.putBeU16(0x0100)?;         // volume = 1.0
    w.putZeros(10)?;             // reserved
    writeUnityMatrix(*w)?;       // 36-byte unity matrix
    w.putZeros(24)?;             // pre_defined
    w.putBeU32(cfg.track_id + 1)?; // next_track_ID
    w.endBox(mvhd);

    // 2b. `trak`
    const trak = w.beginBox(FMP4_BOX_TRAK)?;

    // `tkhd` (version 0, flags = 0x03 enabled + in_movie)
    const tkhd = w.beginFullBox(FMP4_BOX_TKHD, 0, 0x000003)?;
    w.putBeU32(0)?;              // creation_time
    w.putBeU32(0)?;              // modification_time
    w.putBeU32(cfg.track_id)?;   // track_ID
    w.putBeU32(0)?;              // reserved
    w.putBeU32(0)?;              // duration
    w.putZeros(8)?;              // reserved
    w.putBeU16(0)?;              // layer
    w.putBeU16(0)?;              // alternate_group
    w.putBeU16(if (cfg.kind.isVideo()) 0 else 0x0100)?; // volume
    w.putBeU16(0)?;              // reserved
    writeUnityMatrix(*w)?;       // unity matrix
    if (cfg.kind.isVideo()) {
        w.putBeU32((cfg.width as u32) << 16)?;
        w.putBeU32((cfg.height as u32) << 16)?;
    } else {
        w.putBeU32(0)?;
        w.putBeU32(0)?;
    }
    w.endBox(tkhd);

    // `mdia`
    const mdia = w.beginBox(FMP4_BOX_MDIA)?;

    // `mdhd` (version 0)
    const mdhd = w.beginFullBox(FMP4_BOX_MDHD, 0, 0)?;
    w.putBeU32(0)?;              // creation_time
    w.putBeU32(0)?;              // modification_time
    w.putBeU32(cfg.timescale)?;  // timescale
    w.putBeU32(0)?;              // duration
    w.putBeU16(0x55C4)?;         // language = "und"
    w.putBeU16(0)?;              // pre_defined
    w.endBox(mdhd);

    // `hdlr` (version 0)
    const hdlr = w.beginFullBox(FMP4_BOX_HDLR, 0, 0)?;
    w.putBeU32(0)?;                  // pre_defined
    w.putBeU32(cfg.kind.handler4cc())?;
    w.putZeros(12)?;                 // reserved
    w.putBytes("CP-fMP4")?;
    w.putU8(0)?;                     // null terminator
    w.endBox(hdlr);

    // `minf`
    const minf = w.beginBox(FMP4_BOX_MINF)?;
    if (cfg.kind.isVideo()) {
        const vmhd = w.beginFullBox(FMP4_BOX_VMHD, 0, 1)?;
        w.putZeros(8)?;
        w.endBox(vmhd);
    } else {
        const smhd = w.beginFullBox(FMP4_BOX_SMHD, 0, 0)?;
        w.putZeros(4)?;
        w.endBox(smhd);
    }

    // `dinf` -> `dref` -> self-contained `url `
    const dinf = w.beginBox(FMP4_BOX_DINF)?;
    const dref = w.beginFullBox(FMP4_BOX_DREF, 0, 0)?;
    w.putBeU32(1)?;
    const url_box = w.beginFullBox(FMP4_BOX_URL, 0, 1)?;
    w.endBox(url_box);
    w.endBox(dref);
    w.endBox(dinf);

    // `stbl` (empty sample tables required for fragmented MP4)
    const stbl = w.beginBox(FMP4_BOX_STBL)?;

    // `stsd` with 1 minimal sample entry
    const stsd = w.beginFullBox(FMP4_BOX_STSD, 0, 0)?;
    w.putBeU32(1)?; // entry_count
    const entry = w.beginBox(cfg.kind.sampleEntry4cc())?;
    w.putZeros(6)?; // reserved
    w.putBeU16(1)?; // data_reference_index
    if (cfg.kind.isVideo()) {
        w.putZeros(16)?;          // pre_defined + reserved
        w.putBeU16(cfg.width)?;   // width
        w.putBeU16(cfg.height)?;  // height
        w.putBeU32(0x00480000)?;  // horizresolution = 72 dpi
        w.putBeU32(0x00480000)?;  // vertresolution = 72 dpi
        w.putBeU32(0)?;           // reserved
        w.putBeU16(1)?;           // frame_count
        w.putZeros(32)?;          // compressorname
        w.putBeU16(0x0018)?;      // depth = 24
        w.putBeU16(0xFFFF)?;      // pre_defined = -1
    } else {
        w.putZeros(8)?;                         // reserved
        w.putBeU16(cfg.channels)?;              // channelcount
        w.putBeU16(16)?;                        // samplesize = 16 bits
        w.putBeU16(0)?;                         // pre_defined
        w.putBeU16(0)?;                         // reserved
        w.putBeU32((cfg.sample_rate_hz & 0xFFFF) << 16)?; // samplerate 16.16 fixed
    }
    w.endBox(entry);
    w.endBox(stsd);

    const stts = w.beginFullBox(FMP4_BOX_STTS, 0, 0)?;
    w.putBeU32(0)?;
    w.endBox(stts);

    const stsc = w.beginFullBox(FMP4_BOX_STSC, 0, 0)?;
    w.putBeU32(0)?;
    w.endBox(stsc);

    const stsz = w.beginFullBox(FMP4_BOX_STSZ, 0, 0)?;
    w.putBeU32(0)?;
    w.putBeU32(0)?;
    w.endBox(stsz);

    const stco = w.beginFullBox(FMP4_BOX_STCO, 0, 0)?;
    w.putBeU32(0)?;
    w.endBox(stco);

    w.endBox(stbl);
    w.endBox(minf);
    w.endBox(mdia);
    w.endBox(trak);

    // 2c. `mvex` -> `trex`
    const mvex = w.beginBox(FMP4_BOX_MVEX)?;
    const trex = w.beginFullBox(FMP4_BOX_TREX, 0, 0)?;
    w.putBeU32(cfg.track_id)?; // track_ID
    w.putBeU32(1)?;            // default_sample_description_index
    w.putBeU32(0)?;            // default_sample_duration
    w.putBeU32(0)?;            // default_sample_size
    w.putBeU32(0)?;            // default_sample_flags
    w.endBox(trex);
    w.endBox(mvex);

    w.endBox(moov);
    return w.pos;
}

// ============================================================================
// 4. CMAF fMP4 Media Fragment (`moof` + `mdat`) Muxer & Demuxer
// ============================================================================

/// Per-sample timing, size, sync flags, and B-frame composition time offset (`cts_offset`).
pub struct Fmp4SampleMeta {
    duration: u32 = 0;
    size: u32 = 0;
    flags: u32 = FMP4_SAMPLE_FLAG_NON_SYNC;
    cts_offset: i32 = 0;
}

/// Parsed fragment-level metadata from `moof` (`mfhd`, `tfhd`, `tfdt`, `trun`) and `mdat`.
pub struct Fmp4FragmentHeader {
    sequence_number: u32 = 0;
    track_id: u32 = 0;
    base_decode_time: u64 = 0;
    sample_count: usize = 0;
    mdat_payload_offset: usize = 0;
    mdat_payload_len: usize = 0;
    moof_size: usize = 0;
    trun_data_offset: i32 = 0;
}

/// Parsed per-sample timing and byte-range descriptor pointing into the original fragment buffer.
pub struct Fmp4ParsedSample {
    pts: u64 = 0;
    dts: u64 = 0;
    duration: u32 = 0;
    data_offset: usize = 0;
    data_len: usize = 0;
    is_keyframe: bool = false;
}

/// Encodes a complete CMAF / ISO BMFF Media Fragment (`moof` + `mdat`) into `out_buf`:
/// - `moof` contains `mfhd`, `traf`(`tfhd` with `default-base-is-moof`, 64-bit `tfdt` v1,
///   and `trun` v1 with exact `data_offset == moof_size + 8` and signed `cts_offset`).
/// - `mdat` carries `concatenated_payload` matching `sum(samples[i].size)`.
pub fn encodeFmp4Fragment(
    seq_num: u32,
    track_id: u32,
    base_decode_time: u64,
    samples: []const Fmp4SampleMeta,
    concatenated_payload: []const u8,
    out_buf: []u8,
): usize ! Fmp4MuxError {
    if (track_id == 0) {
        return Fmp4MuxError.InvalidConfig;
    }

    // Verify sample sizes sum exactly to concatenated_payload.len
    let sum_sizes: usize = 0;
    let si: usize = 0;
    while (si < samples.len) : (si += 1) {
        sum_sizes += @as(usize, samples[si].size);
    }
    if (sum_sizes != concatenated_payload.len) {
        return Fmp4MuxError.InvalidConfig;
    }

    let w = BoxWriter.init(out_buf);

    // 1. `moof`
    const moof = w.beginBox(FMP4_BOX_MOOF)?;

    // 1a. `mfhd` (version 0)
    const mfhd = w.beginFullBox(FMP4_BOX_MFHD, 0, 0)?;
    w.putBeU32(seq_num)?;
    w.endBox(mfhd);

    // 1b. `traf`
    const traf = w.beginBox(FMP4_BOX_TRAF)?;

    // `tfhd` (version 0, flags = 0x020000 default-base-is-moof)
    const tfhd = w.beginFullBox(FMP4_BOX_TFHD, 0, 0x020000)?;
    w.putBeU32(track_id)?;
    w.endBox(tfhd);

    // `tfdt` (version 1 -> 64-bit baseMediaDecodeTime for long live streams)
    const tfdt = w.beginFullBox(FMP4_BOX_TFDT, 1, 0)?;
    w.putBeU64(base_decode_time)?;
    w.endBox(tfdt);

    // `trun` (version 1 -> signed cts_offset; flags = 0x000F01: data_offset + dur + size + flags + cts)
    const trun_flags: u32 = 0x000001 | 0x000100 | 0x000200 | 0x000400 | 0x000800;
    const trun = w.beginFullBox(FMP4_BOX_TRUN, 1, trun_flags)?;
    w.putBeU32(@as(u32, @intCast(samples.len)))?;
    const data_offset_pos: usize = w.pos;
    w.putBeI32(0)?; // backfilled once moof closes

    let i: usize = 0;
    while (i < samples.len) : (i += 1) {
        const s = samples[i];
        w.putBeU32(s.duration)?;
        w.putBeU32(s.size)?;
        w.putBeU32(s.flags)?;
        w.putBeI32(s.cts_offset)?;
    }
    w.endBox(trun);
    w.endBox(traf);
    w.endBox(moof);

    // Backfill `trun.data_offset == moof_size + 8` (byte 0 of `mdat` payload)
    const moof_size: usize = w.pos - moof;
    const trun_data_offset: i32 = @as(i32, @intCast(moof_size + 8));
    writeBeI32(w.buf, data_offset_pos, trun_data_offset);

    // 2. `mdat`
    const mdat = w.beginBox(FMP4_BOX_MDAT)?;
    w.putBytes(concatenated_payload)?;
    w.endBox(mdat);

    return w.pos;
}

/// Demuxes a CMAF / ISO BMFF Media Fragment (`moof` + `mdat`) from `raw`, populating `out_samples`
/// with per-sample `dts`, `pts`, `duration`, `data_offset`, `data_len`, and `is_keyframe`.
pub fn parseFmp4Fragment(
    raw: []const u8,
    out_samples: []Fmp4ParsedSample,
): Fmp4FragmentHeader ! Fmp4MuxError {
    let top_iter = Fmp4BoxIter.init(raw);
    let has_moof: bool = false;
    let has_mdat: bool = false;
    let moof_hdr = Fmp4BoxHeader{};
    let moof_start: usize = 0;
    let mdat_hdr = Fmp4BoxHeader{};

    while (true) {
        const cur_off = top_iter.offset;
        const opt_box = top_iter.next()?;
        if (b := opt_box) {
            if (b.box_type == FMP4_BOX_MOOF && !has_moof) {
                has_moof = true;
                moof_hdr = b;
                moof_start = cur_off;
            } else if (b.box_type == FMP4_BOX_MDAT && !has_mdat) {
                has_mdat = true;
                mdat_hdr = b;
            }
        } else {
            break;
        }
    }

    if (!has_moof || !has_mdat) {
        return Fmp4MuxError.InvalidBox;
    }

    let frag = Fmp4FragmentHeader{
        mdat_payload_offset: mdat_hdr.payload_offset,
        mdat_payload_len: mdat_hdr.payload_len,
        moof_size: moof_hdr.total_len,
    };

    let has_mfhd: bool = false;
    let has_traf: bool = false;
    let traf_slice: []const u8 = raw;

    const moof_payload = raw[moof_hdr.payload_offset .. moof_hdr.payload_offset + moof_hdr.payload_len];
    let moof_iter = Fmp4BoxIter.init(moof_payload);
    while (true) {
        const opt_b = moof_iter.next()?;
        if (b := opt_b) {
            const p = moof_iter.payloadSlice(b);
            if (b.box_type == FMP4_BOX_MFHD) {
                if (p.len < 8) return Fmp4MuxError.TruncatedBox;
                frag.sequence_number = readBeU32(p, 4);
                has_mfhd = true;
            } else if (b.box_type == FMP4_BOX_TRAF && !has_traf) {
                traf_slice = p;
                has_traf = true;
            }
        } else {
            break;
        }
    }

    if (!has_mfhd || !has_traf) {
        return Fmp4MuxError.InvalidBox;
    }

    let has_tfhd: bool = false;
    let has_trun: bool = false;
    let def_duration: u32 = 0;
    let def_size: u32 = 0;
    let def_flags: u32 = FMP4_SAMPLE_FLAG_NON_SYNC;
    let trun_slice: []const u8 = raw;

    let traf_iter = Fmp4BoxIter.init(traf_slice);
    while (true) {
        const opt_b = traf_iter.next()?;
        if (b := opt_b) {
            const p = traf_iter.payloadSlice(b);
            if (b.box_type == FMP4_BOX_TFHD) {
                if (p.len < 8) return Fmp4MuxError.TruncatedBox;
                const tf_flags: u32 = readBeU32(p, 0) & 0x00FFFFFF;
                frag.track_id = readBeU32(p, 4);
                let off: usize = 8;
                if ((tf_flags & 0x000001) != 0) {
                    if (off + 8 > p.len) return Fmp4MuxError.TruncatedBox;
                    off += 8; // base_data_offset
                }
                if ((tf_flags & 0x000002) != 0) {
                    if (off + 4 > p.len) return Fmp4MuxError.TruncatedBox;
                    off += 4; // sample_description_index
                }
                if ((tf_flags & 0x000008) != 0) {
                    if (off + 4 > p.len) return Fmp4MuxError.TruncatedBox;
                    def_duration = readBeU32(p, off);
                    off += 4;
                }
                if ((tf_flags & 0x000010) != 0) {
                    if (off + 4 > p.len) return Fmp4MuxError.TruncatedBox;
                    def_size = readBeU32(p, off);
                    off += 4;
                }
                if ((tf_flags & 0x000020) != 0) {
                    if (off + 4 > p.len) return Fmp4MuxError.TruncatedBox;
                    def_flags = readBeU32(p, off);
                }
                has_tfhd = true;
            } else if (b.box_type == FMP4_BOX_TFDT) {
                if (p.len < 8) return Fmp4MuxError.TruncatedBox;
                const ver: u8 = p[0];
                if (ver == 1) {
                    if (p.len < 12) return Fmp4MuxError.TruncatedBox;
                    frag.base_decode_time = readBeU64(p, 4);
                } else {
                    frag.base_decode_time = readBeU32(p, 4) as u64;
                }
            } else if (b.box_type == FMP4_BOX_TRUN && !has_trun) {
                trun_slice = p;
                has_trun = true;
            }
        } else {
            break;
        }
    }

    if (!has_tfhd || !has_trun) {
        return Fmp4MuxError.InvalidBox;
    }

    if (trun_slice.len < 8) return Fmp4MuxError.TruncatedBox;
    const trun_ver: u8 = trun_slice[0];
    const trun_flags: u32 = readBeU32(trun_slice, 0) & 0x00FFFFFF;
    const sample_count: usize = @as(usize, readBeU32(trun_slice, 4));
    if (sample_count > out_samples.len) {
        return Fmp4MuxError.OutputSliceTooSmall;
    }
    frag.sample_count = sample_count;

    let pos: usize = 8;
    let data_cursor: usize = mdat_hdr.payload_offset;
    if ((trun_flags & 0x000001) != 0) {
        if (pos + 4 > trun_slice.len) return Fmp4MuxError.TruncatedBox;
        const doff: i32 = readBeI32(trun_slice, pos);
        pos += 4;
        frag.trun_data_offset = doff;
        const abs_off_i64: i64 = (moof_start as i64) + (doff as i64);
        if (abs_off_i64 < 0 || abs_off_i64 > (raw.len as i64)) {
            return Fmp4MuxError.TruncatedBox;
        }
        data_cursor = @as(usize, @intCast(abs_off_i64));
    }

    let has_first_flags: bool = false;
    let first_flags: u32 = def_flags;
    if ((trun_flags & 0x000004) != 0) {
        if (pos + 4 > trun_slice.len) return Fmp4MuxError.TruncatedBox;
        first_flags = readBeU32(trun_slice, pos);
        pos += 4;
        has_first_flags = true;
    }

    const has_dur: bool = (trun_flags & 0x000100) != 0;
    const has_sz: bool = (trun_flags & 0x000200) != 0;
    const has_fl: bool = (trun_flags & 0x000400) != 0;
    const has_cts: bool = (trun_flags & 0x000800) != 0;

    let cur_dts: u64 = frag.base_decode_time;
    let i: usize = 0;
    while (i < sample_count) : (i += 1) {
        let dur: u32 = def_duration;
        let sz: u32 = def_size;
        let fl: u32 = if (i == 0 && has_first_flags) first_flags else def_flags;
        let cts: i32 = 0;

        if (has_dur) {
            if (pos + 4 > trun_slice.len) return Fmp4MuxError.TruncatedBox;
            dur = readBeU32(trun_slice, pos);
            pos += 4;
        }
        if (has_sz) {
            if (pos + 4 > trun_slice.len) return Fmp4MuxError.TruncatedBox;
            sz = readBeU32(trun_slice, pos);
            pos += 4;
        }
        if (has_fl) {
            if (pos + 4 > trun_slice.len) return Fmp4MuxError.TruncatedBox;
            fl = readBeU32(trun_slice, pos);
            pos += 4;
        }
        if (has_cts) {
            if (pos + 4 > trun_slice.len) return Fmp4MuxError.TruncatedBox;
            if (trun_ver == 0) {
                cts = @as(i32, @intCast(readBeU32(trun_slice, pos) & 0x7FFFFFFF));
            } else {
                cts = readBeI32(trun_slice, pos);
            }
            pos += 4;
        }

        const sz_usize: usize = @as(usize, sz);
        if (data_cursor + sz_usize > raw.len) {
            return Fmp4MuxError.TruncatedBox;
        }

        const pts_i64: i64 = (cur_dts as i64) + (cts as i64);
        const pts_u64: u64 = if (pts_i64 < 0) 0 else @as(u64, @intCast(pts_i64));
        const is_kf: bool = (fl & 0x00010000) == 0;

        out_samples[i] = Fmp4ParsedSample{
            pts: pts_u64,
            dts: cur_dts,
            duration: dur,
            data_offset: data_cursor,
            data_len: sz_usize,
            is_keyframe: is_kf,
        };

        cur_dts += dur as u64;
        data_cursor += sz_usize;
    }

    return frag;
}

// ============================================================================
// 5. RFC 3533 Ogg Page Framing & RFC 7845 OpusHead / OpusTags Codec
// ============================================================================

/// Parsed RFC 3533 Ogg page header and payload byte span.
pub struct OggPageHeader {
    version: u8 = 0;
    flags: u8 = 0;
    granule_pos: u64 = 0;
    serial_no: u32 = 0;
    page_seq: u32 = 0;
    checksum: u32 = 0;
    segment_count: u8 = 0;
    payload_offset: usize = 0;
    payload_len: usize = 0;
    total_len: usize = 0;
}

/// Encodes a single RFC 3533 Ogg page with automatic 255-byte lacing table segmentation
/// and computed non-reflected `0x04C11DB7` Ogg CRC-32 into `out_buf`.
pub fn encodeOggPage(
    flags: u8,
    granule_pos: u64,
    serial_no: u32,
    page_seq: u32,
    payload: []const u8,
    out_buf: []u8,
): usize ! Fmp4MuxError {
    const full_segs: usize = payload.len / 255;
    const rem_bytes: usize = payload.len % 255;
    const seg_count: usize = full_segs + 1;
    if (seg_count > OGG_MAX_SEGMENTS) {
        return Fmp4MuxError.TooManySegments;
    }

    const total_len: usize = OGG_HEADER_FIXED_LEN + seg_count + payload.len;
    if (out_buf.len < total_len) {
        return Fmp4MuxError.BufferTooSmall;
    }

    // 27-byte fixed header
    out_buf[0] = 'O';
    out_buf[1] = 'g';
    out_buf[2] = 'g';
    out_buf[3] = 'S';
    out_buf[4] = 0; // stream_structure_version
    out_buf[5] = flags;
    writeLeU64(out_buf, 6, granule_pos);
    writeLeU32(out_buf, 14, serial_no);
    writeLeU32(out_buf, 18, page_seq);
    writeLeU32(out_buf, 22, 0); // CRC placeholder
    out_buf[26] = @as(u8, @intCast(seg_count));

    // Lacing table
    let s: usize = 0;
    while (s < full_segs) : (s += 1) {
        out_buf[OGG_HEADER_FIXED_LEN + s] = 255;
    }
    out_buf[OGG_HEADER_FIXED_LEN + full_segs] = @as(u8, @intCast(rem_bytes));

    // Payload
    const pay_off: usize = OGG_HEADER_FIXED_LEN + seg_count;
    let i: usize = 0;
    while (i < payload.len) : (i += 1) {
        out_buf[pay_off + i] = payload[i];
    }

    // Compute & backfill Ogg CRC-32
    const crc: u32 = computeOggCrc32(out_buf[0..total_len]);
    writeLeU32(out_buf, 22, crc);
    return total_len;
}

/// Parses and validates an RFC 3533 Ogg page from `raw`, verifying `"OggS"` magic,
/// version `0`, lacing segment sum, and Ogg CRC-32 integrity without mutating `raw`.
pub fn parseOggPage(raw: []const u8): OggPageHeader ! Fmp4MuxError {
    if (raw.len < OGG_HEADER_FIXED_LEN) {
        return Fmp4MuxError.TruncatedBox;
    }
    if (!eqBytes(raw[0..4], OGG_CAPTURE_PATTERN) || raw[4] != 0) {
        return Fmp4MuxError.InvalidOggMagic;
    }

    const seg_count: usize = @as(usize, raw[26]);
    if (OGG_HEADER_FIXED_LEN + seg_count > raw.len) {
        return Fmp4MuxError.TruncatedBox;
    }

    let pay_len: usize = 0;
    let s: usize = 0;
    while (s < seg_count) : (s += 1) {
        pay_len += @as(usize, raw[OGG_HEADER_FIXED_LEN + s]);
    }

    const pay_off: usize = OGG_HEADER_FIXED_LEN + seg_count;
    const total_len: usize = pay_off + pay_len;
    if (total_len > raw.len) {
        return Fmp4MuxError.TruncatedBox;
    }

    // Compute Ogg CRC-32 treating bytes [22..26] as zero (non-mutating)
    const wire_crc: u32 = readLeU32(raw, 22);
    let calc_crc: u32 = 0;
    let i: usize = 0;
    while (i < 22) : (i += 1) {
        calc_crc = oggCrc32Step(calc_crc, raw[i]);
    }
    calc_crc = oggCrc32Step(calc_crc, 0);
    calc_crc = oggCrc32Step(calc_crc, 0);
    calc_crc = oggCrc32Step(calc_crc, 0);
    calc_crc = oggCrc32Step(calc_crc, 0);
    i = 26;
    while (i < total_len) : (i += 1) {
        calc_crc = oggCrc32Step(calc_crc, raw[i]);
    }

    if (wire_crc != calc_crc) {
        return Fmp4MuxError.OggChecksumMismatch;
    }

    return OggPageHeader{
        version: raw[4],
        flags: raw[5],
        granule_pos: readLeU64(raw, 6),
        serial_no: readLeU32(raw, 14),
        page_seq: readLeU32(raw, 18),
        checksum: wire_crc,
        segment_count: raw[26],
        payload_offset: pay_off,
        payload_len: pay_len,
        total_len: total_len,
    };
}

/// RFC 7845 §5.1 OpusHead identification header metadata.
pub struct OpusHeadInfo {
    version: u8 = 1;
    channels: u8 = 2;
    pre_skip: u16 = 312;
    input_sample_rate: u32 = 48000;
    output_gain_q8: i16 = 0;
    mapping_family: u8 = 0;
}

/// Encodes a standard 19-byte RFC 7845 §5.1 `OpusHead` packet into `out_buf`.
pub fn encodeOpusHead(info: OpusHeadInfo, out_buf: []u8): usize ! Fmp4MuxError {
    if (info.version != 1 || info.channels == 0) {
        return Fmp4MuxError.InvalidOpusHead;
    }
    if (out_buf.len < OPUS_HEAD_MIN_LEN) {
        return Fmp4MuxError.BufferTooSmall;
    }

    const magic = "OpusHead";
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        out_buf[i] = magic[i];
    }
    out_buf[8] = info.version;
    out_buf[9] = info.channels;
    writeLeU16(out_buf, 10, info.pre_skip);
    writeLeU32(out_buf, 12, info.input_sample_rate);
    writeLeI16(out_buf, 16, info.output_gain_q8);
    out_buf[18] = info.mapping_family;
    return OPUS_HEAD_MIN_LEN;
}

/// Parses and validates an RFC 7845 §5.1 `OpusHead` packet from `raw`.
pub fn parseOpusHeadInfo(raw: []const u8): OpusHeadInfo ! Fmp4MuxError {
    if (raw.len < OPUS_HEAD_MIN_LEN) {
        return Fmp4MuxError.InvalidOpusHead;
    }
    if (!eqBytes(raw[0..8], "OpusHead")) {
        return Fmp4MuxError.InvalidOpusHead;
    }
    const ver: u8 = raw[8];
    const ch: u8 = raw[9];
    if (ver != 1 || ch == 0) {
        return Fmp4MuxError.InvalidOpusHead;
    }
    return OpusHeadInfo{
        version: ver,
        channels: ch,
        pre_skip: readLeU16(raw, 10),
        input_sample_rate: readLeU32(raw, 12),
        output_gain_q8: readLeI16(raw, 16),
        mapping_family: raw[18],
    };
}

/// Encodes an RFC 7845 §5.2 `OpusTags` vendor and Vorbis-comment metadata packet into `out_buf`.
pub fn encodeOpusTags(
    vendor: []const u8,
    comments: []const []const u8,
    out_buf: []u8,
): usize ! Fmp4MuxError {
    let req: usize = vendor.len + 16;
    let ci: usize = 0;
    while (ci < comments.len) : (ci += 1) {
        req += comments[ci].len + 4;
    }
    if (out_buf.len < req) {
        return Fmp4MuxError.BufferTooSmall;
    }

    const magic = "OpusTags";
    let pos: usize = 0;
    let i: usize = 0;
    while (i < 8) : (i += 1) {
        out_buf[pos + i] = magic[i];
    }
    pos += 8;

    writeLeU32(out_buf, pos, @as(u32, @intCast(vendor.len)));
    pos += 4;
    i = 0;
    while (i < vendor.len) : (i += 1) {
        out_buf[pos + i] = vendor[i];
    }
    pos += vendor.len;

    writeLeU32(out_buf, pos, @as(u32, @intCast(comments.len)));
    pos += 4;

    ci = 0;
    while (ci < comments.len) : (ci += 1) {
        const c = comments[ci];
        writeLeU32(out_buf, pos, @as(u32, @intCast(c.len)));
        pos += 4;
        let k: usize = 0;
        while (k < c.len) : (k += 1) {
            out_buf[pos + k] = c[k];
        }
        pos += c.len;
    }

    return pos;
}
