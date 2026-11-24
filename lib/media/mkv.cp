// Reference-hardened against RFC 8794 / RFC 9559 & ebml-iterable (MIT)
// Origin: Pure-CP Stdlib (Zero-Alloc Matroska / WebM EBML Container Parser & Strict Validator) | Created: 2026-11-22

import {
    EBML_MAX_DEPTH,
    decodeEbmlId,
    decodeEbmlSize,
    decodeEbmlUint,
    decodeEbmlFloat,
    simdEbmlBytesEqual,
} from "./ebml.cp";

pub const ID_EBML: u32 = 0x1A45DFA3;
pub const ID_DOC_TYPE: u32 = 0x4282;
pub const ID_SEGMENT: u32 = 0x18538067;
pub const ID_INFO: u32 = 0x1549A966;
pub const ID_TIMECODE_SCALE: u32 = 0x2AD7B1;
pub const ID_DURATION: u32 = 0x4489;
pub const ID_MUXING_APP: u32 = 0x4D80;
pub const ID_WRITING_APP: u32 = 0x5741;
pub const ID_TRACKS: u32 = 0x1654AE6B;
pub const ID_TRACK_ENTRY: u32 = 0xAE;
pub const ID_TRACK_NUMBER: u32 = 0xD7;
pub const ID_TRACK_TYPE: u32 = 0x83;
pub const ID_CODEC_ID: u32 = 0x86;
pub const ID_VIDEO: u32 = 0xE0;
pub const ID_PIXEL_WIDTH: u32 = 0xB0;
pub const ID_PIXEL_HEIGHT: u32 = 0xBA;
pub const ID_AUDIO: u32 = 0xE1;
pub const ID_SAMPLING_FREQ: u32 = 0xB5;
pub const ID_CHANNELS: u32 = 0x9F;
pub const ID_CLUSTER: u32 = 0x1F43B675;
pub const ID_CLUSTER_TIMECODE: u32 = 0xE7;
pub const ID_SIMPLE_BLOCK: u32 = 0xA3;
pub const ID_BLOCK_GROUP: u32 = 0xA0;
pub const ID_BLOCK: u32 = 0xA1;

pub const MKV_DEFAULT_TIMECODE_SCALE_NS: u64 = 1_000_000;

pub err MkvError {
    InvalidEbmlHeader,
    UnsupportedDocType,
    MissingSegment,
    MalformedElement,
    InvalidTrackEntry,
    InvalidSimpleBlock,
    InvalidLacing,
    PayloadOutOfBounds,
    MaxDepthExceeded,
}

pub enum MkvLacingKind: u8 {
    none = 0,
    xiph = 1,
    fixed_size = 2,
    ebml = 3,
}

pub struct MkvSimpleBlock {
    track_number: u64,
    timecode_rel: i16,
    keyframe: bool,
    invisible: bool,
    discardable: bool,
    lacing: MkvLacingKind,
    frame_count: u16,
    payload: []const u8,
}

pub enum MkvTrackType: u8 {
    unknown = 0,
    video = 1,
    audio = 2,
    complex = 3,
    logo = 16,
    subtitle = 17,
    buttons = 18,
    control = 32,
}

pub fn mkvTrackTypeFromInt(raw: u64): MkvTrackType {
    if (raw == 1) return MkvTrackType.video;
    if (raw == 2) return MkvTrackType.audio;
    if (raw == 3) return MkvTrackType.complex;
    if (raw == 16) return MkvTrackType.logo;
    if (raw == 17) return MkvTrackType.subtitle;
    if (raw == 18) return MkvTrackType.buttons;
    if (raw == 32) return MkvTrackType.control;
    return MkvTrackType.unknown;
}

fn trimTrailingNul(s: []const u8): []const u8 {
    let end: usize = s.len;
    while (end > 0 && s[end - 1] == 0) {
        end -= 1;
    }
    return s[0..end];
}

pub struct MkvInfo {
    doc_type: []const u8,
    timecode_scale_ns: u64,
    duration_ticks: f64,
    muxing_app: []const u8,
    writing_app: []const u8,

    pub fn durationMs(): f64 {
        return (_.duration_ticks * (_.timecode_scale_ns as f64)) / 1_000_000.0;
    }

    pub fn isWebm(): bool {
        return simdEbmlBytesEqual(_.doc_type, "webm");
    }
}

pub struct MkvTrack {
    track_number: u64,
    track_type: MkvTrackType,
    codec_id: []const u8,
    pixel_width: u32,
    pixel_height: u32,
    channels: u32,
    sampling_frequency: f64,
}

struct MkvRawElem {
    id: u32,
    payload: []const u8,
    next_pos: usize,
    unknown_size: bool,
}

fn readMkvElementAt(data: []const u8, pos: usize, allow_unknown: bool): MkvRawElem? {
    if (pos >= data.len) return null;
    const vid = decodeEbmlId(data[pos..]) ?? return null;
    const size_pos = pos + (vid.bytes_read as usize);
    const vsz = decodeEbmlSize(data[size_pos..]) ?? return null;
    const payload_start = size_pos + (vsz.bytes_read as usize);
    const rem = data.len - payload_start;

    if (vsz.is_unknown_size) {
        if (!allow_unknown) return null;
        return MkvRawElem{
            .id = vid.raw_id,
            .payload = data[payload_start..data.len],
            .next_pos = data.len,
            .unknown_size = true,
        };
    }

    if (vsz.value > (rem as u64)) {
        return null;
    }

    const payload_end = payload_start + (vsz.value as usize);
    return MkvRawElem{
        .id = vid.raw_id,
        .payload = data[payload_start..payload_end],
        .next_pos = payload_end,
        .unknown_size = false,
    };
}

/// Parses and strictly validates an RFC 9559 Matroska `SimpleBlock` / `Block` binary payload,
/// including TrackNumber VINT, relative `i16` timecode, flags, and all 4 lacing modes
/// (`none`, `xiph`, `fixed_size`, `ebml`).
pub fn parseMkvSimpleBlock(raw: []const u8): MkvError!MkvSimpleBlock {
    if (raw.len < 4) return err.InvalidSimpleBlock;
    const trk = decodeEbmlSize(raw) ?? return err.InvalidSimpleBlock;
    if (trk.is_unknown_size || trk.value == 0) return err.InvalidSimpleBlock;
    const hdr_end = (trk.bytes_read as usize) + 3;
    if (hdr_end > raw.len) return err.InvalidSimpleBlock;

    const tc_u16: u16 = ((raw[trk.bytes_read as usize] as u16) << 8)
        | (raw[(trk.bytes_read as usize) + 1] as u16);
    const tc_rel: i16 = @bitCast(tc_u16);
    const flags = raw[(trk.bytes_read as usize) + 2];

    const keyframe = (flags & 0x80) != 0;
    const invisible = (flags & 0x08) != 0;
    const discardable = (flags & 0x01) != 0;
    const lace_bits: u8 = (flags >> 1) & 0x03;

    const lacing = if (lace_bits == 0)
        MkvLacingKind.none
    else if (lace_bits == 1)
        MkvLacingKind.xiph
    else if (lace_bits == 2)
        MkvLacingKind.fixed_size
    else
        MkvLacingKind.ebml;

    let pos: usize = hdr_end;
    let frame_count: u16 = 1;

    if (lacing != MkvLacingKind.none) {
        if (pos >= raw.len) return err.InvalidLacing;
        const n_minus_1 = raw[pos];
        pos += 1;
        frame_count = (n_minus_1 as u16) + 1;
        const num_frames: usize = frame_count as usize;

        if (lacing == MkvLacingKind.fixed_size) {
            const data_rem = raw.len - pos;
            if (data_rem == 0 || (data_rem % num_frames) != 0) {
                return err.InvalidLacing;
            }
        } else if (lacing == MkvLacingKind.xiph) {
            let sum_frames: u64 = 0;
            let fi: usize = 0;
            while (fi + 1 < num_frames) : (fi += 1) {
                let flen: u64 = 0;
                while (true) {
                    if (pos >= raw.len) return err.InvalidLacing;
                    const b = raw[pos];
                    pos += 1;
                    flen += b as u64;
                    if (flen > (raw.len as u64)) return err.InvalidLacing;
                    if (b != 255) break;
                }
                sum_frames += flen;
                if (sum_frames > (raw.len as u64)) return err.InvalidLacing;
            }
            if (sum_frames > ((raw.len - pos) as u64)) return err.InvalidLacing;
        } else {
            // EBML lacing: first frame unsigned VINT, subsequent num_frames - 2 signed VINT deltas
            const first_v = decodeEbmlSize(raw[pos..]) ?? return err.InvalidLacing;
            if (first_v.is_unknown_size) return err.InvalidLacing;
            pos += first_v.bytes_read as usize;
            let cur_len: i64 = first_v.value as i64;
            let sum_frames: u64 = first_v.value;
            if (sum_frames > (raw.len as u64)) return err.InvalidLacing;

            let fi: usize = 1;
            while (fi + 1 < num_frames) : (fi += 1) {
                const dv = decodeEbmlSize(raw[pos..]) ?? return err.InvalidLacing;
                if (dv.is_unknown_size) return err.InvalidLacing;
                pos += dv.bytes_read as usize;
                const shift: u6 = ((dv.bytes_read * 7) - 1) as u6;
                const bias: i64 = (((1 as u64) << shift) - (1 as u64)) as i64;
                const delta: i64 = (dv.value as i64) - bias;
                cur_len += delta;
                if (cur_len < 0) return err.InvalidLacing;
                sum_frames += cur_len as u64;
                if (sum_frames > (raw.len as u64)) return err.InvalidLacing;
            }
            if (sum_frames > ((raw.len - pos) as u64)) return err.InvalidLacing;
        }
    }

    return MkvSimpleBlock{
        .track_number = trk.value,
        .timecode_rel = tc_rel,
        .keyframe = keyframe,
        .invisible = invisible,
        .discardable = discardable,
        .lacing = lacing,
        .frame_count = frame_count,
        .payload = raw[pos..raw.len],
    };
}

struct DocTypeSlice {
    value: []const u8,
}

fn parseEbmlDocType(data: []const u8): DocTypeSlice? {
    const ebml = readMkvElementAt(data, 0, false) ?? return null;
    if (ebml.id != ID_EBML) {
        return null;
    }
    let pos: usize = 0;
    while (pos < ebml.payload.len) {
        const child = readMkvElementAt(ebml.payload, pos, false) ?? return null;
        pos = child.next_pos;
        if (child.id == ID_DOC_TYPE) {
            return DocTypeSlice{ .value = trimTrailingNul(child.payload) };
        }
    }
    return null;
}

pub fn isMkv(data: []const u8): bool {
    const dt = parseEbmlDocType(data) ?? return false;
    return simdEbmlBytesEqual(dt.value, "matroska") || simdEbmlBytesEqual(dt.value, "webm");
}

pub fn isWebm(data: []const u8): bool {
    const dt = parseEbmlDocType(data) ?? return false;
    return simdEbmlBytesEqual(dt.value, "webm");
}

fn parseVideoElement(payload: []const u8, track: *MkvTrack) {
    let pos: usize = 0;
    while (pos < payload.len) {
        const child = readMkvElementAt(payload, pos, false) ?? break;
        pos = child.next_pos;
        if (child.id == ID_PIXEL_WIDTH) {
            if (w := decodeEbmlUint(child.payload)) {
                track.pixel_width = w as u32;
            }
        } else if (child.id == ID_PIXEL_HEIGHT) {
            if (h := decodeEbmlUint(child.payload)) {
                track.pixel_height = h as u32;
            }
        }
    }
}

fn parseAudioElement(payload: []const u8, track: *MkvTrack) {
    let pos: usize = 0;
    while (pos < payload.len) {
        const child = readMkvElementAt(payload, pos, false) ?? break;
        pos = child.next_pos;
        if (child.id == ID_CHANNELS) {
            if (ch := decodeEbmlUint(child.payload)) {
                track.channels = ch as u32;
            }
        } else if (child.id == ID_SAMPLING_FREQ) {
            if (sr := decodeEbmlFloat(child.payload)) {
                track.sampling_frequency = sr;
            }
        }
    }
}

fn parseTrackEntry(payload: []const u8): MkvTrack {
    let track = MkvTrack{
        .track_number = 0,
        .track_type = MkvTrackType.unknown,
        .codec_id = payload[0..0],
        .pixel_width = 0,
        .pixel_height = 0,
        .channels = 0,
        .sampling_frequency = 0.0,
    };

    let pos: usize = 0;
    while (pos < payload.len) {
        const child = readMkvElementAt(payload, pos, false) ?? break;
        pos = child.next_pos;
        if (child.id == ID_TRACK_NUMBER) {
            if (n := decodeEbmlUint(child.payload)) {
                track.track_number = n;
            }
        } else if (child.id == ID_TRACK_TYPE) {
            if (tt := decodeEbmlUint(child.payload)) {
                track.track_type = mkvTrackTypeFromInt(tt);
            }
        } else if (child.id == ID_CODEC_ID) {
            track.codec_id = trimTrailingNul(child.payload);
        } else if (child.id == ID_VIDEO) {
            parseVideoElement(child.payload, *track);
        } else if (child.id == ID_AUDIO) {
            parseAudioElement(child.payload, *track);
        }
    }

    return track;
}

pub struct MkvTrackIter {
    data: []const u8,
    pos: usize,
    done: bool,

    pub static fn init(tracks_payload: []const u8): MkvTrackIter {
        return MkvTrackIter{
            .data = tracks_payload,
            .pos = 0,
            .done = false,
        };
    }

    pub fn next(*_): MkvTrack? {
        if (_.done) {
            return null;
        }
        while (_.pos < _.data.len) {
            if (elem := readMkvElementAt(_.data, _.pos, false)) {
                _.pos = elem.next_pos;
                if (elem.id == ID_TRACK_ENTRY) {
                    return parseTrackEntry(elem.payload);
                }
            } else {
                _.done = true;
                return null;
            }
        }
        _.done = true;
        return null;
    }
}

pub struct MkvHeader {
    info: MkvInfo,
    tracks_payload: []const u8,
    has_info: bool,
    has_tracks: bool,

    pub fn tracks(): MkvTrackIter {
        return MkvTrackIter.init(_.tracks_payload);
    }
}

fn parseInfoElement(payload: []const u8, info: *MkvInfo) {
    let pos: usize = 0;
    while (pos < payload.len) {
        const child = readMkvElementAt(payload, pos, false) ?? break;
        pos = child.next_pos;
        if (child.id == ID_TIMECODE_SCALE) {
            if (ts := decodeEbmlUint(child.payload)) {
                info.timecode_scale_ns = ts;
            }
        } else if (child.id == ID_DURATION) {
            if (d := decodeEbmlFloat(child.payload)) {
                info.duration_ticks = d;
            }
        } else if (child.id == ID_MUXING_APP) {
            info.muxing_app = trimTrailingNul(child.payload);
        } else if (child.id == ID_WRITING_APP) {
            info.writing_app = trimTrailingNul(child.payload);
        }
    }
}

pub fn parseMkvHeader(data: []const u8): MkvHeader? {
    const ebml = readMkvElementAt(data, 0, false) ?? return null;
    if (ebml.id != ID_EBML) {
        return null;
    }

    let doc_type: []const u8 = data[0..0];
    let epos: usize = 0;
    while (epos < ebml.payload.len) {
        const child = readMkvElementAt(ebml.payload, epos, false) ?? return null;
        epos = child.next_pos;
        if (child.id == ID_DOC_TYPE) {
            doc_type = trimTrailingNul(child.payload);
        }
    }

    if (!simdEbmlBytesEqual(doc_type, "matroska") && !simdEbmlBytesEqual(doc_type, "webm")) {
        return null;
    }

    let info = MkvInfo{
        .doc_type = doc_type,
        .timecode_scale_ns = MKV_DEFAULT_TIMECODE_SCALE_NS,
        .duration_ticks = 0.0,
        .muxing_app = data[0..0],
        .writing_app = data[0..0],
    };
    let tracks_payload: []const u8 = data[0..0];
    let has_info = false;
    let has_tracks = false;
    let found_segment = false;

    let top_pos: usize = ebml.next_pos;
    while (top_pos < data.len) {
        const top = readMkvElementAt(data, top_pos, true) ?? return null;
        top_pos = top.next_pos;

        if (top.id == ID_SEGMENT) {
            found_segment = true;
            let spos: usize = 0;
            while (spos < top.payload.len) {
                const elem = readMkvElementAt(top.payload, spos, true) ?? break;
                if (elem.id == ID_CLUSTER) {
                    break;
                }
                if (elem.unknown_size) {
                    break;
                }
                spos = elem.next_pos;

                if (elem.id == ID_INFO) {
                    has_info = true;
                    parseInfoElement(elem.payload, *info);
                } else if (elem.id == ID_TRACKS) {
                    has_tracks = true;
                    tracks_payload = elem.payload;
                }
            }
            break;
        }
    }

    if (!found_segment) {
        return null;
    }

    return MkvHeader{
        .info = info,
        .tracks_payload = tracks_payload,
        .has_info = has_info,
        .has_tracks = has_tracks,
    };
}

fn validateTrackEntryStrict(payload: []const u8): MkvError!void {
    let pos: usize = 0;
    let has_num = false;
    let has_type = false;
    let has_codec = false;
    while (pos < payload.len) {
        const child = readMkvElementAt(payload, pos, false) ?? return err.InvalidTrackEntry;
        pos = child.next_pos;
        if (child.id == ID_TRACK_NUMBER) {
            const tn = decodeEbmlUint(child.payload) ?? return err.InvalidTrackEntry;
            if (tn == 0) return err.InvalidTrackEntry;
            has_num = true;
        } else if (child.id == ID_TRACK_TYPE) {
            * = decodeEbmlUint(child.payload) ?? return err.InvalidTrackEntry;
            has_type = true;
        } else if (child.id == ID_CODEC_ID) {
            if (trimTrailingNul(child.payload).len == 0) return err.InvalidTrackEntry;
            has_codec = true;
        } else if (child.id == ID_VIDEO) {
            let vp: usize = 0;
            while (vp < child.payload.len) {
                const vc = readMkvElementAt(child.payload, vp, false) ?? return err.InvalidTrackEntry;
                vp = vc.next_pos;
                if (vc.id == ID_PIXEL_WIDTH || vc.id == ID_PIXEL_HEIGHT) {
                    * = decodeEbmlUint(vc.payload) ?? return err.InvalidTrackEntry;
                }
            }
        } else if (child.id == ID_AUDIO) {
            let ap: usize = 0;
            while (ap < child.payload.len) {
                const ac = readMkvElementAt(child.payload, ap, false) ?? return err.InvalidTrackEntry;
                ap = ac.next_pos;
                if (ac.id == ID_CHANNELS) {
                    * = decodeEbmlUint(ac.payload) ?? return err.InvalidTrackEntry;
                } else if (ac.id == ID_SAMPLING_FREQ) {
                    * = decodeEbmlFloat(ac.payload) ?? return err.InvalidTrackEntry;
                }
            }
        }
    }
    if (!has_num || !has_type || !has_codec) return err.InvalidTrackEntry;
}

fn validateClusterStrict(payload: []const u8): MkvError!void {
    let pos: usize = 0;
    while (pos < payload.len) {
        const child = readMkvElementAt(payload, pos, false) ?? return err.MalformedElement;
        pos = child.next_pos;
        if (child.id == ID_CLUSTER_TIMECODE) {
            * = decodeEbmlUint(child.payload) ?? return err.MalformedElement;
        } else if (child.id == ID_SIMPLE_BLOCK) {
            * = parseMkvSimpleBlock(child.payload)?;
        } else if (child.id == ID_BLOCK_GROUP) {
            let bp: usize = 0;
            while (bp < child.payload.len) {
                const bc = readMkvElementAt(child.payload, bp, false) ?? return err.MalformedElement;
                bp = bc.next_pos;
                if (bc.id == ID_BLOCK) {
                    * = parseMkvSimpleBlock(bc.payload)?;
                }
            }
        }
    }
}

/// Zero-allocation strict Matroska / WebM (RFC 9559) container validator.
/// Verifies EBML Header, DocType (`matroska`/`webm`), Segment, Info, Tracks, Cluster,
/// and SimpleBlock/Block lacing & payload bounds.
pub fn validateMkvStrict(data: []const u8): MkvError!void {
    const ebml = readMkvElementAt(data, 0, false) ?? return err.InvalidEbmlHeader;
    if (ebml.id != ID_EBML) return err.InvalidEbmlHeader;

    let doc_type: []const u8 = data[0..0];
    let epos: usize = 0;
    while (epos < ebml.payload.len) {
        const child = readMkvElementAt(ebml.payload, epos, false) ?? return err.InvalidEbmlHeader;
        epos = child.next_pos;
        if (child.id == ID_DOC_TYPE) {
            doc_type = trimTrailingNul(child.payload);
        }
    }
    if (!simdEbmlBytesEqual(doc_type, "matroska") && !simdEbmlBytesEqual(doc_type, "webm")) {
        return err.UnsupportedDocType;
    }

    let found_segment = false;
    let top_pos: usize = ebml.next_pos;
    while (top_pos < data.len) {
        const top = readMkvElementAt(data, top_pos, true) ?? return err.MalformedElement;
        top_pos = top.next_pos;
        if (top.id == ID_SEGMENT) {
            found_segment = true;
            let spos: usize = 0;
            let depth_guard: usize = 0;
            while (spos < top.payload.len) {
                if (depth_guard > EBML_MAX_DEPTH * 64) return err.MaxDepthExceeded;
                depth_guard += 1;
                const elem = readMkvElementAt(top.payload, spos, true) ?? return err.MalformedElement;
                spos = elem.next_pos;
                if (elem.id == ID_INFO) {
                    let ip: usize = 0;
                    while (ip < elem.payload.len) {
                        const ic = readMkvElementAt(elem.payload, ip, false) ?? return err.MalformedElement;
                        ip = ic.next_pos;
                        if (ic.id == ID_TIMECODE_SCALE) {
                            const ts = decodeEbmlUint(ic.payload) ?? return err.MalformedElement;
                            if (ts == 0) return err.MalformedElement;
                        } else if (ic.id == ID_DURATION) {
                            * = decodeEbmlFloat(ic.payload) ?? return err.MalformedElement;
                        }
                    }
                } else if (elem.id == ID_TRACKS) {
                    let tp: usize = 0;
                    while (tp < elem.payload.len) {
                        const tc = readMkvElementAt(elem.payload, tp, false) ?? return err.InvalidTrackEntry;
                        tp = tc.next_pos;
                        if (tc.id == ID_TRACK_ENTRY) {
                            validateTrackEntryStrict(tc.payload)?;
                        }
                    }
                } else if (elem.id == ID_CLUSTER) {
                    validateClusterStrict(elem.payload)?;
                }
            }
        }
    }
    if (!found_segment) return err.MissingSegment;
}
