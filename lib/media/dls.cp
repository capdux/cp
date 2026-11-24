// Origin: Pure-CP Stdlib (Zero-Alloc MMA Downloadable Sounds .dls RIFF Parser) | Created: 2026-11-22
// Pure CP zero-heap-allocation MMA DLS Level 1/2 RIFF parser:
// colh/vers/INFO/ptbl header, LIST/lins instrument iterator (with drum bank flag),
// and LIST/wvpl wave pool iterator (fmt / wsmp / data PCM slices).

const std = @import("std");

/// 4-byte RIFF container magic `"RIFF"`.
pub const DLS_RIFF_MAGIC: []const u8 = "RIFF";

/// 4-byte RIFF form type `"DLS "`.
pub const DLS_FORM_TYPE: []const u8 = "DLS ";

/// Bit 31 in `insh.ulBank` indicating a percussion / drum kit bank.
pub const DLS_BANK_DRUM_FLAG: u32 = 0x80000000;

/// DLS `vers` chunk 64-bit version split into MS and LS 32-bit words.
pub struct DlsVersion {
    ms_version: u32 = 0,
    ls_version: u32 = 0,

    fn major(): u16 {
        return ((_.ms_version >> 16) & 0xFFFF) as u16;
    }

    fn minor(): u16 {
        return (_.ms_version & 0xFFFF) as u16;
    }
}

/// Zero-copy metadata extracted from a RIFF `LIST/INFO` chunk.
pub struct DlsInfo {
    inam: []const u8 = "",
    icop: []const u8 = "",
    ieng: []const u8 = "",
    icmt: []const u8 = "",
    isft: []const u8 = "",
    icrd: []const u8 = "",
}

/// Zero-copy top-level parsed DLS header and container section slices.
pub struct DlsHeader {
    instrument_count: u32 = 0,
    version: DlsVersion = DlsVersion {},
    has_version: bool = false,
    info: DlsInfo = DlsInfo {},
    lins_slice: []const u8 = "",
    wvpl_slice: []const u8 = "",
    ptbl_slice: []const u8 = "",
    cue_count: u32 = 0,

    /// Returns a zero-allocation iterator over `LIST/ins ` instruments inside `lins_slice`.
    fn instruments(): DlsInstIter {
        return DlsInstIter.init(_.lins_slice);
    }

    /// Returns a zero-allocation iterator over `LIST/wave` entries inside `wvpl_slice`.
    fn waves(): DlsWaveIter {
        return DlsWaveIter.init(_.wvpl_slice);
    }
}

/// Decoded instrument header (`insh`) and optional `LIST/INFO` name from `LIST/ins `.
pub struct DlsInstrument {
    name: []const u8 = "",
    regions: u32 = 0,
    bank: u32 = 0,
    instrument_program: u32 = 0,

    /// Returns `true` if the percussion / drum bank flag (`0x80000000`) is set.
    fn isDrum(): bool {
        return (_.bank & DLS_BANK_DRUM_FLAG) != 0;
    }
}

/// Decoded wave pool entry from `LIST/wave` (`fmt `, optional `wsmp`, `data` PCM slice).
pub struct DlsWave {
    name: []const u8 = "",
    format_tag: u16 = 0,
    channels: u16 = 0,
    sample_rate: u32 = 0,
    bits_per_sample: u16 = 0,
    unity_note: u16 = 0,
    fine_tune: i16 = 0,
    attenuation: i32 = 0,
    has_wsmp: bool = false,
    pcm_data: []const u8 = "",
}

struct RiffChunk {
    id: []const u8,
    payload: []const u8,
    next_pos: usize,
}

// ---------------------------------------------------------------------------
// Internal Little-Endian & RIFF Chunk Helpers
// ---------------------------------------------------------------------------

fn readU16Le(data: []const u8, off: usize): u16 {
    return (data[off] as u16) | ((data[off + 1] as u16) << 8);
}

fn readI16Le(data: []const u8, off: usize): i16 {
    const bits = readU16Le(data, off);
    return @bitCast(bits);
}

fn readU32Le(data: []const u8, off: usize): u32 {
    return (data[off] as u32) |
        ((data[off + 1] as u32) << 8) |
        ((data[off + 2] as u32) << 16) |
        ((data[off + 3] as u32) << 24);
}

fn readI32Le(data: []const u8, off: usize): i32 {
    const bits = readU32Le(data, off);
    return @bitCast(bits);
}

fn trimCString(slice: []const u8): []const u8 {
    let end: usize = 0;
    while (end < slice.len && slice[end] != 0) {
        end += 1;
    }
    return slice[0..end];
}

/// Reads the next RIFF chunk at `pos` within `buf` (respecting 2-byte even pad alignment).
/// Returns `null` if truncated or if chunk size exceeds `buf.len`.
fn readRiffChunk(buf: []const u8, pos: usize): RiffChunk? {
    if (pos + 8 > buf.len) {
        return null;
    }
    const id = buf[pos .. pos + 4];
    const size = readU32Le(buf, pos + 4) as usize;
    const payload_start = pos + 8;
    if (size > buf.len - payload_start) {
        return null;
    }
    const payload_end = payload_start + size;
    const pad: usize = size & 1;
    const next_pos = if (pad <= buf.len - payload_end) (payload_end + pad) else payload_end;
    return RiffChunk {
        id: id,
        payload: buf[payload_start..payload_end],
        next_pos: next_pos,
    };
}

/// Parses a `LIST/INFO` chunk body (after the 4-byte `"INFO"` list type) into `DlsInfo`.
fn parseInfoBody(body: []const u8): DlsInfo {
    let info = DlsInfo {};
    let pos: usize = 0;
    while (pos < body.len) {
        const ch_opt = readRiffChunk(body, pos);
        if (ch_opt == null) {
            break;
        }
        const ch = ch_opt.?;
        pos = ch.next_pos;
        const val = trimCString(ch.payload);
        if (std.mem.eql(u8, ch.id, "INAM")) {
            info.inam = val;
        } else if (std.mem.eql(u8, ch.id, "ICOP")) {
            info.icop = val;
        } else if (std.mem.eql(u8, ch.id, "IENG")) {
            info.ieng = val;
        } else if (std.mem.eql(u8, ch.id, "ICMT")) {
            info.icmt = val;
        } else if (std.mem.eql(u8, ch.id, "ISFT")) {
            info.isft = val;
        } else if (std.mem.eql(u8, ch.id, "ICRD")) {
            info.icrd = val;
        }
    }
    return info;
}

// ---------------------------------------------------------------------------
// 1. Detection & Top-Level DLS Parser
// ---------------------------------------------------------------------------

/// Returns `true` if `data` has a valid 12-byte RIFF `DLS ` header with `8 + size <= data.len`.
pub fn isDls(data: []const u8): bool {
    if (data.len < 12) {
        return false;
    }
    if (!std.mem.eql(u8, data[0..4], DLS_RIFF_MAGIC)) {
        return false;
    }
    if (!std.mem.eql(u8, data[8..12], DLS_FORM_TYPE)) {
        return false;
    }
    const riff_size = readU32Le(data, 4) as usize;
    return riff_size >= 4 && riff_size <= data.len - 8;
}

/// Parses a RIFF `DLS ` container header and top-level chunks (`colh`, `vers`, `LIST/INFO`,
/// `LIST/lins`, `ptbl`, `LIST/wvpl`) with zero heap allocation.
pub fn parseDls(data: []const u8): DlsHeader? {
    if (!isDls(data)) {
        return null;
    }
    const riff_size = readU32Le(data, 4) as usize;
    const body = data[12 .. riff_size + 8];

    let hdr = DlsHeader {};
    let pos: usize = 0;

    while (pos < body.len) {
        const ch = readRiffChunk(body, pos) ?? return null;
        pos = ch.next_pos;

        if (std.mem.eql(u8, ch.id, "colh")) {
            if (ch.payload.len < 4) {
                return null;
            }
            hdr.instrument_count = readU32Le(ch.payload, 0);
        } else if (std.mem.eql(u8, ch.id, "vers")) {
            if (ch.payload.len < 8) {
                return null;
            }
            hdr.version = DlsVersion {
                ms_version: readU32Le(ch.payload, 0),
                ls_version: readU32Le(ch.payload, 4),
            };
            hdr.has_version = true;
        } else if (std.mem.eql(u8, ch.id, "ptbl")) {
            if (ch.payload.len < 8) {
                return null;
            }
            hdr.ptbl_slice = ch.payload;
            hdr.cue_count = readU32Le(ch.payload, 4);
        } else if (std.mem.eql(u8, ch.id, "LIST")) {
            if (ch.payload.len < 4) {
                return null;
            }
            const list_type = ch.payload[0..4];
            const list_body = ch.payload[4..];
            if (std.mem.eql(u8, list_type, "INFO")) {
                hdr.info = parseInfoBody(list_body);
            } else if (std.mem.eql(u8, list_type, "lins")) {
                hdr.lins_slice = list_body;
            } else if (std.mem.eql(u8, list_type, "wvpl")) {
                hdr.wvpl_slice = list_body;
            }
        }
    }

    return hdr;
}

// ---------------------------------------------------------------------------
// 2. Instrument List Iterator (`LIST/lins` -> `LIST/ins `)
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over `LIST/ins ` instruments inside `DlsHeader.lins_slice`.
pub struct DlsInstIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    static fn init(lins_slice: []const u8): DlsInstIter {
        return DlsInstIter {
            src: lins_slice,
            pos: 0,
            done: lins_slice.len == 0,
        };
    }

    fn next(*_): DlsInstrument? {
        if (_.done) {
            return null;
        }
        while (_.pos < _.src.len) {
            const ch_opt = readRiffChunk(_.src, _.pos);
            if (ch_opt == null) {
                _.done = true;
                return null;
            }
            const ch = ch_opt.?;
            _.pos = ch.next_pos;

            if (!std.mem.eql(u8, ch.id, "LIST") || ch.payload.len < 4) {
                continue;
            }
            if (!std.mem.eql(u8, ch.payload[0..4], "ins ")) {
                continue;
            }

            const ins_body = ch.payload[4..];
            let inst = DlsInstrument {};
            let has_insh = false;
            let ipos: usize = 0;

            while (ipos < ins_body.len) {
                const sub_opt = readRiffChunk(ins_body, ipos);
                if (sub_opt == null) {
                    break;
                }
                const sub = sub_opt.?;
                ipos = sub.next_pos;

                if (std.mem.eql(u8, sub.id, "insh") && sub.payload.len >= 12) {
                    inst.regions = readU32Le(sub.payload, 0);
                    inst.bank = readU32Le(sub.payload, 4);
                    inst.instrument_program = readU32Le(sub.payload, 8);
                    has_insh = true;
                } else if (std.mem.eql(u8, sub.id, "LIST") && sub.payload.len >= 4) {
                    if (std.mem.eql(u8, sub.payload[0..4], "INFO")) {
                        const info = parseInfoBody(sub.payload[4..]);
                        inst.name = info.inam;
                    }
                }
            }

            if (has_insh) {
                return inst;
            }
        }
        _.done = true;
        return null;
    }
}

// ---------------------------------------------------------------------------
// 3. Wave Pool Iterator (`LIST/wvpl` -> `LIST/wave`)
// ---------------------------------------------------------------------------

/// Zero-allocation iterator over `LIST/wave` entries inside `DlsHeader.wvpl_slice`.
pub struct DlsWaveIter {
    src: []const u8 = "",
    pos: usize = 0,
    done: bool = true,

    static fn init(wvpl_slice: []const u8): DlsWaveIter {
        return DlsWaveIter {
            src: wvpl_slice,
            pos: 0,
            done: wvpl_slice.len == 0,
        };
    }

    fn next(*_): DlsWave? {
        if (_.done) {
            return null;
        }
        while (_.pos < _.src.len) {
            const ch_opt = readRiffChunk(_.src, _.pos);
            if (ch_opt == null) {
                _.done = true;
                return null;
            }
            const ch = ch_opt.?;
            _.pos = ch.next_pos;

            if (!std.mem.eql(u8, ch.id, "LIST") || ch.payload.len < 4) {
                continue;
            }
            if (!std.mem.eql(u8, ch.payload[0..4], "wave")) {
                continue;
            }

            const wave_body = ch.payload[4..];
            let wave = DlsWave {};
            let has_fmt = false;
            let wpos: usize = 0;

            while (wpos < wave_body.len) {
                const sub_opt = readRiffChunk(wave_body, wpos);
                if (sub_opt == null) {
                    break;
                }
                const sub = sub_opt.?;
                wpos = sub.next_pos;

                if (std.mem.eql(u8, sub.id, "fmt ") && sub.payload.len >= 16) {
                    wave.format_tag = readU16Le(sub.payload, 0);
                    wave.channels = readU16Le(sub.payload, 2);
                    wave.sample_rate = readU32Le(sub.payload, 4);
                    wave.bits_per_sample = readU16Le(sub.payload, 14);
                    has_fmt = true;
                } else if (std.mem.eql(u8, sub.id, "wsmp") && sub.payload.len >= 20) {
                    wave.unity_note = readU16Le(sub.payload, 4);
                    wave.fine_tune = readI16Le(sub.payload, 6);
                    wave.attenuation = readI32Le(sub.payload, 8);
                    wave.has_wsmp = true;
                } else if (std.mem.eql(u8, sub.id, "data")) {
                    wave.pcm_data = sub.payload;
                } else if (std.mem.eql(u8, sub.id, "LIST") && sub.payload.len >= 4) {
                    if (std.mem.eql(u8, sub.payload[0..4], "INFO")) {
                        const info = parseInfoBody(sub.payload[4..]);
                        wave.name = info.inam;
                    }
                }
            }

            if (has_fmt) {
                return wave;
            }
        }
        _.done = true;
        return null;
    }
}
