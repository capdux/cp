// Source: Absorbed from /data/dev/bun/src/ (Zero-Alloc Matroska / WebM EBML Container Parser)

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

pub const MKV_DEFAULT_TIMECODE_SCALE_NS: u64 = 1_000_000;

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

fn sliceEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
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

    fn durationMs(): f64 {
        return (_.duration_ticks * (_.timecode_scale_ns as f64)) / 1_000_000.0;
    }

    fn isWebm(): bool {
        return sliceEq(_.doc_type, "webm");
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

struct VintId {
    id: u32,
    len: usize,
}

struct VintSize {
    size: u64,
    len: usize,
    unknown: bool,
}

struct EbmlElement {
    id: u32,
    payload: []const u8,
    next_pos: usize,
    unknown_size: bool,
}

fn readVintId(data: []const u8, pos: usize): VintId? {
    if (pos >= data.len) {
        return null;
    }
    const b0 = data[pos];
    if (b0 == 0) {
        return null;
    }

    let width: usize = 0;
    if ((b0 & 0x80) != 0) {
        width = 1;
    } else if ((b0 & 0x40) != 0) {
        width = 2;
    } else if ((b0 & 0x20) != 0) {
        width = 3;
    } else if ((b0 & 0x10) != 0) {
        width = 4;
    } else {
        return null;
    }

    if (width > data.len - pos) {
        return null;
    }

    let id: u32 = 0;
    let i: usize = 0;
    while (i < width) {
        id = (id << 8) | (data[pos + i] as u32);
        i += 1;
    }

    return VintId{
        .id = id,
        .len = width,
    };
}

fn readVintSize(data: []const u8, pos: usize): VintSize? {
    if (pos >= data.len) {
        return null;
    }
    const b0 = data[pos];
    if (b0 == 0) {
        return null;
    }

    let width: usize = 1;
    let mask: u8 = 0x80;
    while (width <= 8 && (b0 & mask) == 0) {
        mask = mask >> 1;
        width += 1;
    }
    if (width > 8 || width > data.len - pos) {
        return null;
    }

    const first_data_mask: u8 = mask - 1;
    let val: u64 = (b0 & first_data_mask) as u64;
    let all_ones: u64 = first_data_mask as u64;

    let i: usize = 1;
    while (i < width) {
        val = (val << 8) | (data[pos + i] as u64);
        all_ones = (all_ones << 8) | (0xFF as u64);
        i += 1;
    }

    return VintSize{
        .size = val,
        .len = width,
        .unknown = (val == all_ones),
    };
}

fn readElementAt(data: []const u8, pos: usize, allow_unknown: bool): EbmlElement? {
    const vid = readVintId(data, pos) ?? return null;
    const vsz = readVintSize(data, pos + vid.len) ?? return null;
    const payload_start = pos + vid.len + vsz.len;
    const rem = data.len - payload_start;

    if (vsz.unknown) {
        if (!allow_unknown) {
            return null;
        }
        return EbmlElement{
            .id = vid.id,
            .payload = data[payload_start..data.len],
            .next_pos = data.len,
            .unknown_size = true,
        };
    }

    if (vsz.size > (rem as u64)) {
        return null;
    }

    const payload_end = payload_start + (vsz.size as usize);
    return EbmlElement{
        .id = vid.id,
        .payload = data[payload_start..payload_end],
        .next_pos = payload_end,
        .unknown_size = false,
    };
}

fn readEbmlUint(payload: []const u8): u64? {
    if (payload.len > 8) {
        return null;
    }
    let val: u64 = 0;
    let i: usize = 0;
    while (i < payload.len) {
        val = (val << 8) | (payload[i] as u64);
        i += 1;
    }
    return val;
}

fn readEbmlFloat(payload: []const u8): f64? {
    if (payload.len == 0) {
        return 0.0;
    }
    if (payload.len == 4) {
        const bits32: u32 = ((payload[0] as u32) << 24) |
            ((payload[1] as u32) << 16) |
            ((payload[2] as u32) << 8) |
            (payload[3] as u32);
        unsafe {
            const f: f32 = @bitCast(bits32);
            return f as f64;
        }
    }
    if (payload.len == 8) {
        const bits64: u64 = ((payload[0] as u64) << 56) |
            ((payload[1] as u64) << 48) |
            ((payload[2] as u64) << 40) |
            ((payload[3] as u64) << 32) |
            ((payload[4] as u64) << 24) |
            ((payload[5] as u64) << 16) |
            ((payload[6] as u64) << 8) |
            (payload[7] as u64);
        unsafe {
            const d: f64 = @bitCast(bits64);
            return d;
        }
    }
    return null;
}

struct DocTypeSlice {
    value: []const u8,
}

fn parseEbmlDocType(data: []const u8): DocTypeSlice? {
    const ebml = readElementAt(data, 0, false) ?? return null;
    if (ebml.id != ID_EBML) {
        return null;
    }
    let pos: usize = 0;
    while (pos < ebml.payload.len) {
        const child = readElementAt(ebml.payload, pos, false) ?? return null;
        pos = child.next_pos;
        if (child.id == ID_DOC_TYPE) {
            return DocTypeSlice{ .value = trimTrailingNul(child.payload) };
        }
    }
    return null;
}

pub fn isMkv(data: []const u8): bool {
    const dt = parseEbmlDocType(data) ?? return false;
    return sliceEq(dt.value, "matroska") || sliceEq(dt.value, "webm");
}

pub fn isWebm(data: []const u8): bool {
    const dt = parseEbmlDocType(data) ?? return false;
    return sliceEq(dt.value, "webm");
}

fn parseVideoElement(payload: []const u8, track: *MkvTrack) {
    let pos: usize = 0;
    while (pos < payload.len) {
        const child = readElementAt(payload, pos, false) ?? break;
        pos = child.next_pos;
        if (child.id == ID_PIXEL_WIDTH) {
            if (w := readEbmlUint(child.payload)) {
                track.pixel_width = w as u32;
            }
        } else if (child.id == ID_PIXEL_HEIGHT) {
            if (h := readEbmlUint(child.payload)) {
                track.pixel_height = h as u32;
            }
        }
    }
}

fn parseAudioElement(payload: []const u8, track: *MkvTrack) {
    let pos: usize = 0;
    while (pos < payload.len) {
        const child = readElementAt(payload, pos, false) ?? break;
        pos = child.next_pos;
        if (child.id == ID_CHANNELS) {
            if (ch := readEbmlUint(child.payload)) {
                track.channels = ch as u32;
            }
        } else if (child.id == ID_SAMPLING_FREQ) {
            if (sr := readEbmlFloat(child.payload)) {
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
        const child = readElementAt(payload, pos, false) ?? break;
        pos = child.next_pos;
        if (child.id == ID_TRACK_NUMBER) {
            if (n := readEbmlUint(child.payload)) {
                track.track_number = n;
            }
        } else if (child.id == ID_TRACK_TYPE) {
            if (tt := readEbmlUint(child.payload)) {
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

    fn init(tracks_payload: []const u8): MkvTrackIter {
        return MkvTrackIter{
            .data = tracks_payload,
            .pos = 0,
            .done = false,
        };
    }

    fn next(*_): MkvTrack? {
        if (_.done) {
            return null;
        }
        while (_.pos < _.data.len) {
            if (elem := readElementAt(_.data, _.pos, false)) {
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

    fn tracks(): MkvTrackIter {
        return MkvTrackIter.init(_.tracks_payload);
    }
}

fn parseInfoElement(payload: []const u8, info: *MkvInfo) {
    let pos: usize = 0;
    while (pos < payload.len) {
        const child = readElementAt(payload, pos, false) ?? break;
        pos = child.next_pos;
        if (child.id == ID_TIMECODE_SCALE) {
            if (ts := readEbmlUint(child.payload)) {
                info.timecode_scale_ns = ts;
            }
        } else if (child.id == ID_DURATION) {
            if (d := readEbmlFloat(child.payload)) {
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
    const ebml = readElementAt(data, 0, false) ?? return null;
    if (ebml.id != ID_EBML) {
        return null;
    }

    let doc_type: []const u8 = data[0..0];
    let epos: usize = 0;
    while (epos < ebml.payload.len) {
        const child = readElementAt(ebml.payload, epos, false) ?? return null;
        epos = child.next_pos;
        if (child.id == ID_DOC_TYPE) {
            doc_type = trimTrailingNul(child.payload);
        }
    }

    if (!sliceEq(doc_type, "matroska") && !sliceEq(doc_type, "webm")) {
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
        const top = readElementAt(data, top_pos, true) ?? return null;
        top_pos = top.next_pos;

        if (top.id == ID_SEGMENT) {
            found_segment = true;
            let spos: usize = 0;
            while (spos < top.payload.len) {
                // Allow unknown size only for Cluster so streaming media stops cleanly
                const elem = readElementAt(top.payload, spos, true) ?? break;
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
