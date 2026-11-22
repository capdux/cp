// Origin: Pure-CP Stdlib (H.264/AVC & H.265/HEVC NAL Bitstream, SPS/PPS Parser, Annex-B<->AVCC & RFC 6184/7798 RTP Packetizer) | Created: 2026-11-23
// 100% pure CP, zero dynamic heap allocation, unbounded caller-slice real-time video bitstream engine:
//   - Zero-copy Annex-B (`00 00 01` / `00 00 00 01`) & AVCC/HVCC (1/2/4-byte length prefix) NAL iterators & lossless converters
//   - ITU-T H.264/H.265 §7.3.1 EBSP <-> RBSP emulation prevention byte (`00 00 03`) stripping & insertion
//   - Exp-Golomb (`ue(v)` / `se(v)`) bitstream reader (`NalBitReader`)
//   - Full H.264/AVC SPS parser (Baseline/Main/High/High10/High422/High444, scaling matrices, exact frame cropping)
//     + ISO/IEC 14496-15 `avcC` (`AVCDecoderConfigurationRecord`) encoder
//   - H.265/HEVC 2-byte NAL header & basic SPS parser (`profile_tier_level`, `conformance_window` cropping) + keyframe detector
//   - RFC 6184 H.264 (Single NAL, STAP-A, FU-A) & RFC 7798 H.265 (Single NAL, AP, FU) RTP packetizers & loss-resilient depacketizers

// ============================================================================
// 1. Constants, Error Set & Enums
// ============================================================================

/// Error set for video NAL parsing, framing conversion, SPS decoding, and RTP packetization.
pub err VideoNalError {
    InvalidNalHeader,
    InvalidStartCode,
    InvalidAvccLength,
    TruncatedBitstream,
    InvalidExpGolomb,
    InvalidSps,
    UnsupportedProfile,
    BufferTooSmall,
    OutputSliceTooSmall,
    InvalidMtu,
    InvalidRtpPayload,
    IncompleteFragment,
}

/// Supported video bitstream standards.
pub enum VideoCodecKind {
    h264,
    h265,
}

/// Common H.264/AVC NAL unit types (ITU-T H.264 Table 7-1 & RFC 6184).
pub const H264_NAL_SLICE_NON_IDR: u8 = 1;
pub const H264_NAL_SLICE_IDR: u8 = 5;
pub const H264_NAL_SEI: u8 = 6;
pub const H264_NAL_SPS: u8 = 7;
pub const H264_NAL_PPS: u8 = 8;
pub const H264_NAL_AUD: u8 = 9;
pub const H264_NAL_STAP_A: u8 = 24;
pub const H264_NAL_FU_A: u8 = 28;

/// Common H.265/HEVC NAL unit types (ITU-T H.265 Table 7-1 & RFC 7798).
pub const H265_NAL_TRAIL_R: u8 = 1;
pub const H265_NAL_IDR_W_RADL: u8 = 19;
pub const H265_NAL_IDR_N_LP: u8 = 20;
pub const H265_NAL_CRA_NUT: u8 = 21;
pub const H265_NAL_VPS: u8 = 32;
pub const H265_NAL_SPS: u8 = 33;
pub const H265_NAL_PPS: u8 = 34;
pub const H265_NAL_AUD: u8 = 35;
pub const H265_NAL_PREFIX_SEI: u8 = 39;
pub const H265_NAL_AP: u8 = 48;
pub const H265_NAL_FU: u8 = 49;

// ============================================================================
// 2. Annex-B / AVCC Framing Iterators, Lossless Converters & EBSP <-> RBSP
// ============================================================================

/// Zero-copy span describing one NAL unit inside an Annex-B or AVCC/HVCC buffer.
pub struct AnnexBNalUnit {
    start_code_offset: usize = 0;
    start_code_len: usize = 0;
    nal_offset: usize = 0;
    nal_len: usize = 0;

    fn nalSlice(data: []const u8): []const u8 {
        if (nal_len == 0 || nal_offset + nal_len > data.len) return "";
        return data[nal_offset .. nal_offset + nal_len];
    }
}

fn findStartCode(data: []const u8, start_at: usize): ([2]usize)? {
    let i: usize = start_at;
    while (i + 3 <= data.len) : (i += 1) {
        if (data[i] == 0 && data[i + 1] == 0) {
            if (data[i + 2] == 1) {
                return [i, 3];
            }
            if (i + 4 <= data.len && data[i + 2] == 0 && data[i + 3] == 1) {
                return [i, 4];
            }
        }
    }
    return null;
}

/// Zero-allocation iterator over Annex-B (`00 00 01` or `00 00 00 01`) NAL units.
pub struct AnnexBIter {
    data: []const u8 = [],
    pos: usize = 0;

    static fn init(data: []const u8): AnnexBIter {
        return AnnexBIter{ data: data, pos: 0 };
    }

    /// Returns the next NAL unit span, or `null` when no more start codes remain.
    fn next(*_): AnnexBNalUnit? {
        const sc_opt = findStartCode(data, pos);
        if (sc_opt == null) {
            pos = data.len;
            return null;
        }
        const sc = sc_opt.?;
        const sc_off: usize = sc[0];
        const sc_len: usize = sc[1];
        const nal_start: usize = sc_off + sc_len;

        let nal_end: usize = data.len;
        if (next_sc := findStartCode(data, nal_start)) {
            nal_end = next_sc[0];
            // Strip any trailing zero padding immediately preceding a 3-byte start code
            while (nal_end > nal_start && data[nal_end - 1] == 0 && next_sc[1] == 3) {
                nal_end -= 1;
            }
            pos = next_sc[0];
        } else {
            pos = data.len;
        }

        return AnnexBNalUnit{
            start_code_offset: sc_off,
            start_code_len: sc_len,
            nal_offset: nal_start,
            nal_len: if (nal_end >= nal_start) (nal_end - nal_start) else 0,
        };
    }
}

/// Zero-allocation iterator over AVCC/HVCC length-prefixed NAL units (`length_size` in `{1, 2, 4}`).
pub struct AvccIter {
    data: []const u8 = [],
    pos: usize = 0;
    length_size: usize = 4;

    static fn init(data: []const u8, length_size: usize): AvccIter ! VideoNalError {
        if (length_size != 1 && length_size != 2 && length_size != 4) {
            return VideoNalError.InvalidAvccLength;
        }
        return AvccIter{ data: data, pos: 0, length_size: length_size };
    }

    fn next(*_): (AnnexBNalUnit?) ! VideoNalError {
        if (pos >= data.len) return null;
        if (pos + length_size > data.len) return VideoNalError.TruncatedBitstream;

        let nal_len: usize = 0;
        if (length_size == 1) {
            nal_len = @as(usize, data[pos]);
        } else if (length_size == 2) {
            nal_len = (@as(usize, data[pos]) << 8) | @as(usize, data[pos + 1]);
        } else {
            nal_len = (@as(usize, data[pos]) << 24)
                | (@as(usize, data[pos + 1]) << 16)
                | (@as(usize, data[pos + 2]) << 8)
                | @as(usize, data[pos + 3]);
        }

        const nal_off: usize = pos + length_size;
        if (nal_off + nal_len > data.len) {
            return VideoNalError.TruncatedBitstream;
        }

        const unit = AnnexBNalUnit{
            start_code_offset: pos,
            start_code_len: length_size,
            nal_offset: nal_off,
            nal_len: nal_len,
        };
        pos = nal_off + nal_len;
        return unit;
    }
}

/// Converts an Annex-B bitstream to AVCC/HVCC length-prefixed format (`length_size` in `{1, 2, 4}`).
pub fn annexBToAvcc(annexb: []const u8, length_size: usize, out_buf: []u8): usize ! VideoNalError {
    if (length_size != 1 && length_size != 2 && length_size != 4) {
        return VideoNalError.InvalidAvccLength;
    }
    let it = AnnexBIter.init(annexb);
    let pos: usize = 0;
    while (unit := it.next()) {
        if (unit.nal_len == 0) continue;
        if (length_size == 1 && unit.nal_len > 255) return VideoNalError.InvalidAvccLength;
        if (length_size == 2 && unit.nal_len > 65535) return VideoNalError.InvalidAvccLength;
        if (pos + length_size + unit.nal_len > out_buf.len) return VideoNalError.BufferTooSmall;

        if (length_size == 1) {
            out_buf[pos] = @as(u8, @intCast(unit.nal_len));
        } else if (length_size == 2) {
            out_buf[pos] = @as(u8, @intCast((unit.nal_len >> 8) & 0xFF));
            out_buf[pos + 1] = @as(u8, @intCast(unit.nal_len & 0xFF));
        } else {
            out_buf[pos] = @as(u8, @intCast((unit.nal_len >> 24) & 0xFF));
            out_buf[pos + 1] = @as(u8, @intCast((unit.nal_len >> 16) & 0xFF));
            out_buf[pos + 2] = @as(u8, @intCast((unit.nal_len >> 8) & 0xFF));
            out_buf[pos + 3] = @as(u8, @intCast(unit.nal_len & 0xFF));
        }
        pos += length_size;

        const nal = unit.nalSlice(annexb);
        let i: usize = 0;
        while (i < nal.len) : (i += 1) {
            out_buf[pos + i] = nal[i];
        }
        pos += nal.len;
    }
    return pos;
}

/// Converts an AVCC/HVCC length-prefixed bitstream to standard 4-byte start-code (`00 00 00 01`) Annex-B format.
pub fn avccToAnnexB(avcc: []const u8, length_size: usize, out_buf: []u8): usize ! VideoNalError {
    let it = AvccIter.init(avcc, length_size)?;
    let pos: usize = 0;
    while (true) {
        const opt = it.next()?;
        if (unit := opt) {
            if (pos + 4 + unit.nal_len > out_buf.len) return VideoNalError.BufferTooSmall;
            out_buf[pos + 0] = 0x00;
            out_buf[pos + 1] = 0x00;
            out_buf[pos + 2] = 0x00;
            out_buf[pos + 3] = 0x01;
            pos += 4;
            const nal = unit.nalSlice(avcc);
            let i: usize = 0;
            while (i < nal.len) : (i += 1) {
                out_buf[pos + i] = nal[i];
            }
            pos += nal.len;
        } else {
            break;
        }
    }
    return pos;
}

/// Strips ITU-T H.264/H.265 §7.3.1 emulation prevention bytes (`00 00 03` -> `00 00`),
/// converting EBSP payload bytes into raw RBSP bytes.
pub fn ebspToRbsp(ebsp: []const u8, out_rbsp: []u8): usize ! VideoNalError {
    let out_pos: usize = 0;
    let zeros: usize = 0;
    let i: usize = 0;
    while (i < ebsp.len) : (i += 1) {
        const b = ebsp[i];
        if (zeros >= 2 && b == 0x03) {
            zeros = 0;
            continue;
        }
        if (out_pos >= out_rbsp.len) return VideoNalError.BufferTooSmall;
        out_rbsp[out_pos] = b;
        out_pos += 1;
        if (b == 0x00) {
            zeros += 1;
        } else {
            zeros = 0;
        }
    }
    return out_pos;
}

/// Inserts ITU-T H.264/H.265 §7.3.1 emulation prevention bytes (`00 00 {00..03}` -> `00 00 03 {00..03}`),
/// converting raw RBSP bytes into start-code-safe EBSP bytes.
pub fn rbspToEbsp(rbsp: []const u8, out_ebsp: []u8): usize ! VideoNalError {
    let out_pos: usize = 0;
    let zeros: usize = 0;
    let i: usize = 0;
    while (i < rbsp.len) : (i += 1) {
        const b = rbsp[i];
        if (zeros >= 2 && b <= 0x03) {
            if (out_pos >= out_ebsp.len) return VideoNalError.BufferTooSmall;
            out_ebsp[out_pos] = 0x03;
            out_pos += 1;
            zeros = 0;
        }
        if (out_pos >= out_ebsp.len) return VideoNalError.BufferTooSmall;
        out_ebsp[out_pos] = b;
        out_pos += 1;
        if (b == 0x00) {
            zeros += 1;
        } else {
            zeros = 0;
        }
    }
    return out_pos;
}

// ============================================================================
// 3. Exp-Golomb Bitstream Reader (`NalBitReader`)
// ============================================================================

/// MSB-first bitstream reader supporting fixed-width bits and ITU-T H.264/H.265 Exp-Golomb (`ue`/`se`).
pub struct NalBitReader {
    data: []const u8 = [],
    bit_pos: usize = 0;

    static fn init(data: []const u8): NalBitReader {
        return NalBitReader{ data: data, bit_pos: 0 };
    }

    fn bitsRemaining(): usize {
        const total: usize = data.len * 8;
        return if (bit_pos <= total) (total - bit_pos) else 0;
    }

    fn readBit(*_): u8 ! VideoNalError {
        if (bit_pos >= data.len * 8) return VideoNalError.TruncatedBitstream;
        const byte_idx: usize = bit_pos >> 3;
        const shift: u3 = @as(u3, @intCast(7 - (bit_pos & 7)));
        bit_pos += 1;
        return (data[byte_idx] >> shift) & 1;
    }

    fn readBits(*_, n: usize): u32 ! VideoNalError {
        if (n > 32) return VideoNalError.InvalidExpGolomb;
        if (n > bitsRemaining()) return VideoNalError.TruncatedBitstream;
        let val: u32 = 0;
        let i: usize = 0;
        while (i < n) : (i += 1) {
            const bit: u8 = readBit()?;
            val = (val << 1) | (bit as u32);
        }
        return val;
    }

    fn skipBits(*_, n: usize): void ! VideoNalError {
        if (n > bitsRemaining()) return VideoNalError.TruncatedBitstream;
        bit_pos += n;
    }

    /// Reads an unsigned Exp-Golomb code `ue(v)` (ITU-T H.264 §9.1).
    fn readUe(*_): u32 ! VideoNalError {
        let lz: usize = 0;
        while (true) {
            const bit = readBit()?;
            if (bit == 1) break;
            lz += 1;
            if (lz > 31) return VideoNalError.InvalidExpGolomb;
        }
        if (lz == 0) return 0;
        const suffix: u32 = readBits(lz)?;
        const base: u32 = (@as(u32, 1) << (@as(u5, @intCast(lz)))) - 1;
        return base + suffix;
    }

    /// Reads a signed Exp-Golomb code `se(v)` (ITU-T H.264 §9.1.1).
    fn readSe(*_): i32 ! VideoNalError {
        const code: u32 = readUe()?;
        if ((code & 1) != 0) {
            return @as(i32, @intCast((code + 1) >> 1));
        }
        return -@as(i32, @intCast(code >> 1));
    }
}

// ============================================================================
// 4. H.264/AVC NAL Header, Full SPS Parser & `avcC` Config Record Encoder
// ============================================================================

/// Parsed 1-byte H.264/AVC NAL unit header.
pub struct H264NalHeader {
    forbidden_zero_bit: u8 = 0;
    nal_ref_idc: u8 = 0;
    nal_unit_type: u8 = 0;

    fn isIdr(): bool {
        return nal_unit_type == H264_NAL_SLICE_IDR;
    }

    fn isSps(): bool {
        return nal_unit_type == H264_NAL_SPS;
    }

    fn isPps(): bool {
        return nal_unit_type == H264_NAL_PPS;
    }
}

/// Parses a 1-byte H.264 NAL unit header, rejecting `forbidden_zero_bit != 0`.
pub fn parseH264NalHeader(nal: []const u8): H264NalHeader ! VideoNalError {
    if (nal.len == 0) return VideoNalError.InvalidNalHeader;
    const b = nal[0];
    const f: u8 = (b >> 7) & 1;
    if (f != 0) return VideoNalError.InvalidNalHeader;
    return H264NalHeader{
        forbidden_zero_bit: f,
        nal_ref_idc: (b >> 5) & 0x03,
        nal_unit_type: b & 0x1F,
    };
}

/// Parsed H.264/AVC Sequence Parameter Set (SPS) metadata with exact cropped pixel dimensions.
pub struct H264SpsInfo {
    profile_idc: u8 = 0;
    constraint_set_flags: u8 = 0;
    level_idc: u8 = 0;
    sps_id: u32 = 0;
    chroma_format_idc: u32 = 1;
    bit_depth_luma: u8 = 8;
    bit_depth_chroma: u8 = 8;
    log2_max_frame_num: u8 = 4;
    pic_order_cnt_type: u32 = 0;
    max_num_ref_frames: u32 = 0;
    frame_mbs_only: bool = true;
    coded_width: u32 = 0;
    coded_height: u32 = 0;
    crop_left: u32 = 0;
    crop_right: u32 = 0;
    crop_top: u32 = 0;
    crop_bottom: u32 = 0;
    width: u32 = 0;
    height: u32 = 0;
}

fn isH264HighProfile(p: u8): bool {
    return p == 100 || p == 110 || p == 122 || p == 244 || p == 44
        || p == 83 || p == 86 || p == 118 || p == 128 || p == 138
        || p == 139 || p == 134 || p == 135;
}

fn skipH264ScalingList(br: *NalBitReader, size_of_list: usize): void ! VideoNalError {
    let last_scale: i32 = 8;
    let next_scale: i32 = 8;
    let j: usize = 0;
    while (j < size_of_list) : (j += 1) {
        if (next_scale != 0) {
            const delta_scale: i32 = br.readSe()?;
            next_scale = (last_scale + delta_scale + 256) % 256;
        }
        last_scale = if (next_scale == 0) last_scale else next_scale;
    }
}

/// Parses an H.264/AVC SPS NAL unit (without start code) using caller-provided `rbsp_scratch`,
/// supporting Baseline/Main/High/High10/High422/High444 profiles, scaling matrices, and exact frame cropping.
pub fn parseH264Sps(sps_nal: []const u8, rbsp_scratch: []u8): H264SpsInfo ! VideoNalError {
    const hdr = parseH264NalHeader(sps_nal)?;
    if (hdr.nal_unit_type != H264_NAL_SPS || sps_nal.len < 4) {
        return VideoNalError.InvalidSps;
    }

    const rbsp_len = ebspToRbsp(sps_nal[1..], rbsp_scratch)?;
    let br = NalBitReader.init(rbsp_scratch[0..rbsp_len]);

    let info = H264SpsInfo{};
    info.profile_idc = @as(u8, @intCast(br.readBits(8)?));
    info.constraint_set_flags = @as(u8, @intCast(br.readBits(8)?));
    info.level_idc = @as(u8, @intCast(br.readBits(8)?));
    info.sps_id = br.readUe()?;

    let separate_colour_plane: bool = false;
    if (isH264HighProfile(info.profile_idc)) {
        info.chroma_format_idc = br.readUe()?;
        if (info.chroma_format_idc == 3) {
            separate_colour_plane = (br.readBit()? == 1);
        }
        const bd_luma = br.readUe()?;
        const bd_chroma = br.readUe()?;
        info.bit_depth_luma = @as(u8, @intCast((bd_luma + 8) & 0xFF));
        info.bit_depth_chroma = @as(u8, @intCast((bd_chroma + 8) & 0xFF));
        * = br.readBit()?; // qpprime_y_zero_transform_bypass_flag
        const scaling_present = br.readBit()?;
        if (scaling_present == 1) {
            const n_lists: usize = if (info.chroma_format_idc != 3) 8 else 12;
            let li: usize = 0;
            while (li < n_lists) : (li += 1) {
                if (br.readBit()? == 1) {
                    skipH264ScalingList(*br, if (li < 6) 16 else 64)?;
                }
            }
        }
    }

    const log2_mfn = br.readUe()?;
    info.log2_max_frame_num = @as(u8, @intCast((log2_mfn + 4) & 0xFF));
    info.pic_order_cnt_type = br.readUe()?;
    if (info.pic_order_cnt_type == 0) {
        * = br.readUe()?; // log2_max_pic_order_cnt_lsb_minus4
    } else if (info.pic_order_cnt_type == 1) {
        * = br.readBit()?; // delta_pic_order_always_zero_flag
        * = br.readSe()?;  // offset_for_non_ref_pic
        * = br.readSe()?;  // offset_for_top_to_bottom_field
        const n_ref = br.readUe()?;
        let ri: u32 = 0;
        while (ri < n_ref) : (ri += 1) {
            * = br.readSe()?;
        }
    }

    info.max_num_ref_frames = br.readUe()?;
    * = br.readBit()?; // gaps_in_frame_num_value_allowed_flag

    const pic_w_mbs_m1 = br.readUe()?;
    const pic_h_map_m1 = br.readUe()?;
    const frame_mbs_only_bit = br.readBit()?;
    info.frame_mbs_only = (frame_mbs_only_bit == 1);
    if (!info.frame_mbs_only) {
        * = br.readBit()?; // mb_adaptive_frame_field_flag
    }
    * = br.readBit()?; // direct_8x8_inference_flag

    info.coded_width = (pic_w_mbs_m1 + 1) * 16;
    info.coded_height = (pic_h_map_m1 + 1) * 16 * (if (info.frame_mbs_only) @as(u32, 1) else @as(u32, 2));

    const cropping_flag = br.readBit()?;
    if (cropping_flag == 1) {
        info.crop_left = br.readUe()?;
        info.crop_right = br.readUe()?;
        info.crop_top = br.readUe()?;
        info.crop_bottom = br.readUe()?;
    }

    const chroma_array_type: u32 = if (separate_colour_plane) 0 else info.chroma_format_idc;
    const sub_w: u32 = if (chroma_array_type == 1 || chroma_array_type == 2) 2 else 1;
    const sub_h: u32 = if (chroma_array_type == 1) 2 else 1;
    const crop_unit_x: u32 = if (chroma_array_type == 0) 1 else sub_w;
    const crop_unit_y: u32 = (if (chroma_array_type == 0) @as(u32, 1) else sub_h) * (if (info.frame_mbs_only) @as(u32, 1) else @as(u32, 2));

    const crop_w: u32 = (info.crop_left + info.crop_right) * crop_unit_x;
    const crop_h: u32 = (info.crop_top + info.crop_bottom) * crop_unit_y;
    if (crop_w >= info.coded_width || crop_h >= info.coded_height) {
        return VideoNalError.InvalidSps;
    }
    info.width = info.coded_width - crop_w;
    info.height = info.coded_height - crop_h;
    return info;
}

/// Encodes an ISO/IEC 14496-15 `AVCDecoderConfigurationRecord` (`avcC` box payload) from raw SPS & PPS NALs.
pub fn encodeAvcDecoderConfigRecord(
    sps_nal: []const u8,
    pps_nal: []const u8,
    out_buf: []u8,
): usize ! VideoNalError {
    if (sps_nal.len < 4 || pps_nal.len < 1 || sps_nal.len > 65535 || pps_nal.len > 65535) {
        return VideoNalError.InvalidSps;
    }
    const req: usize = sps_nal.len + pps_nal.len + 11;
    if (out_buf.len < req) return VideoNalError.BufferTooSmall;

    out_buf[0] = 0x01;       // configurationVersion
    out_buf[1] = sps_nal[1]; // AVCProfileIndication
    out_buf[2] = sps_nal[2]; // profile_compatibility
    out_buf[3] = sps_nal[3]; // AVCLevelIndication
    out_buf[4] = 0xFF;       // 6 reserved 1-bits + lengthSizeMinusOne = 3 (4-byte NAL lengths)
    out_buf[5] = 0xE1;       // 3 reserved 1-bits + numOfSequenceParameterSets = 1
    out_buf[6] = @as(u8, @intCast((sps_nal.len >> 8) & 0xFF));
    out_buf[7] = @as(u8, @intCast(sps_nal.len & 0xFF));
    let pos: usize = 8;
    let i: usize = 0;
    while (i < sps_nal.len) : (i += 1) {
        out_buf[pos + i] = sps_nal[i];
    }
    pos += sps_nal.len;

    out_buf[pos] = 0x01; // numOfPictureParameterSets = 1
    out_buf[pos + 1] = @as(u8, @intCast((pps_nal.len >> 8) & 0xFF));
    out_buf[pos + 2] = @as(u8, @intCast(pps_nal.len & 0xFF));
    pos += 3;
    i = 0;
    while (i < pps_nal.len) : (i += 1) {
        out_buf[pos + i] = pps_nal[i];
    }
    return pos + pps_nal.len;
}

// ============================================================================
// 5. H.265/HEVC NAL Header, Basic SPS Parser & Unified Keyframe Detector
// ============================================================================

/// Parsed 2-byte H.265/HEVC NAL unit header (ITU-T H.265 §7.3.1.2).
pub struct H265NalHeader {
    forbidden_zero_bit: u8 = 0;
    nal_unit_type: u8 = 0;
    nuh_layer_id: u8 = 0;
    nuh_temporal_id_plus1: u8 = 1;

    /// Returns `true` for IRAP random-access keyframes (BLA/IDR/CRA: types 16..=21).
    fn isIrapKeyframe(): bool {
        return nal_unit_type >= 16 && nal_unit_type <= 21;
    }
}

/// Parses a 2-byte H.265/HEVC NAL unit header.
pub fn parseH265NalHeader(nal: []const u8): H265NalHeader ! VideoNalError {
    if (nal.len < 2) return VideoNalError.InvalidNalHeader;
    const b0 = nal[0];
    const b1 = nal[1];
    const f: u8 = (b0 >> 7) & 1;
    const tid: u8 = b1 & 0x07;
    if (f != 0 || tid == 0) return VideoNalError.InvalidNalHeader;
    return H265NalHeader{
        forbidden_zero_bit: f,
        nal_unit_type: (b0 >> 1) & 0x3F,
        nuh_layer_id: ((b0 & 1) << 5) | ((b1 >> 3) & 0x1F),
        nuh_temporal_id_plus1: tid,
    };
}

/// Parsed H.265/HEVC SPS profile, level, and cropped luma dimensions.
pub struct H265SpsInfo {
    vps_id: u8 = 0;
    max_sub_layers_minus1: u8 = 0;
    general_profile_idc: u8 = 0;
    general_level_idc: u8 = 0;
    sps_id: u32 = 0;
    chroma_format_idc: u32 = 1;
    coded_width: u32 = 0;
    coded_height: u32 = 0;
    width: u32 = 0;
    height: u32 = 0;
}

/// Parses an H.265/HEVC SPS NAL unit (without start code) using `rbsp_scratch`, decoding
/// `profile_tier_level`, `chroma_format_idc`, coded dimensions, and `conformance_window` cropping.
pub fn parseH265SpsBasic(sps_nal: []const u8, rbsp_scratch: []u8): H265SpsInfo ! VideoNalError {
    const hdr = parseH265NalHeader(sps_nal)?;
    if (hdr.nal_unit_type != H265_NAL_SPS || sps_nal.len < 16) {
        return VideoNalError.InvalidSps;
    }

    const rbsp_len = ebspToRbsp(sps_nal[2..], rbsp_scratch)?;
    let br = NalBitReader.init(rbsp_scratch[0..rbsp_len]);

    let info = H265SpsInfo{};
    info.vps_id = @as(u8, @intCast(br.readBits(4)?));
    info.max_sub_layers_minus1 = @as(u8, @intCast(br.readBits(3)?));
    * = br.readBit()?; // sps_temporal_id_nesting_flag

    // profile_tier_level(1, max_sub_layers_minus1)
    * = br.readBits(2)?; // general_profile_space
    * = br.readBit()?;   // general_tier_flag
    info.general_profile_idc = @as(u8, @intCast(br.readBits(5)?));
    br.skipBits(32)?;    // general_profile_compatibility_flags
    br.skipBits(48)?;    // general_constraint_indicator_flags
    info.general_level_idc = @as(u8, @intCast(br.readBits(8)?));

    if (info.max_sub_layers_minus1 > 0) {
        let sub_prof: [8]u8 = [0; 8];
        let sub_lvl: [8]u8 = [0; 8];
        let s: usize = 0;
        while (s < @as(usize, info.max_sub_layers_minus1)) : (s += 1) {
            sub_prof[s] = br.readBit()?;
            sub_lvl[s] = br.readBit()?;
        }
        let pad: usize = @as(usize, info.max_sub_layers_minus1);
        while (pad < 8) : (pad += 1) {
            br.skipBits(2)?;
        }
        s = 0;
        while (s < @as(usize, info.max_sub_layers_minus1)) : (s += 1) {
            if (sub_prof[s] == 1) br.skipBits(88)?;
            if (sub_lvl[s] == 1) br.skipBits(8)?;
        }
    }

    info.sps_id = br.readUe()?;
    info.chroma_format_idc = br.readUe()?;
    if (info.chroma_format_idc == 3) {
        * = br.readBit()?; // separate_colour_plane_flag
    }
    info.coded_width = br.readUe()?;
    info.coded_height = br.readUe()?;
    info.width = info.coded_width;
    info.height = info.coded_height;

    const conf_win = br.readBit()?;
    if (conf_win == 1) {
        const left = br.readUe()?;
        const right = br.readUe()?;
        const top = br.readUe()?;
        const bottom = br.readUe()?;
        const sub_w: u32 = if (info.chroma_format_idc == 1 || info.chroma_format_idc == 2) 2 else 1;
        const sub_h: u32 = if (info.chroma_format_idc == 1) 2 else 1;
        const cw = (left + right) * sub_w;
        const ch = (top + bottom) * sub_h;
        if (cw >= info.coded_width || ch >= info.coded_height) return VideoNalError.InvalidSps;
        info.width = info.coded_width - cw;
        info.height = info.coded_height - ch;
    }

    return info;
}

/// Returns `true` if the Annex-B access unit contains an H.264 IDR or H.265 IRAP (IDR/CRA/BLA) keyframe.
pub fn isVideoKeyframe(annexb_au: []const u8, codec: VideoCodecKind): bool {
    let it = AnnexBIter.init(annexb_au);
    while (unit := it.next()) {
        const nal = unit.nalSlice(annexb_au);
        if (codec == VideoCodecKind.h264) {
            const h = parseH264NalHeader(nal) catch continue;
            if (h.isIdr()) return true;
        } else {
            const h = parseH265NalHeader(nal) catch continue;
            if (h.isIrapKeyframe()) return true;
        }
    }
    return false;
}

// ============================================================================
// 6. WebRTC Video RTP Packetization & Depacketization (RFC 6184 & RFC 7798)
// ============================================================================

/// Byte span of one generated RTP payload inside caller's `out_buf`.
pub struct RtpPayloadSpan {
    offset: usize = 0;
    len: usize = 0;

    fn slice(buf: []const u8): []const u8 {
        if (len == 0 || offset + len > buf.len) return "";
        return buf[offset .. offset + len];
    }
}

/// RFC 6184 H.264 RTP packetizer: emits a Single NAL packet if `nal.len <= max_payload_bytes`,
/// or splits into FU-A (type=28) packets with exact Start (`0x80`) / End (`0x40`) bits.
pub fn packetizeH264Rtp(
    nal: []const u8,
    max_payload_bytes: usize,
    out_buf: []u8,
    out_spans: []RtpPayloadSpan,
): usize ! VideoNalError {
    if (nal.len == 0) return VideoNalError.InvalidNalHeader;
    if (max_payload_bytes < 4) return VideoNalError.InvalidMtu;

    if (nal.len <= max_payload_bytes) {
        if (out_spans.len < 1) return VideoNalError.OutputSliceTooSmall;
        if (out_buf.len < nal.len) return VideoNalError.BufferTooSmall;
        let i: usize = 0;
        while (i < nal.len) : (i += 1) {
            out_buf[i] = nal[i];
        }
        out_spans[0] = RtpPayloadSpan{ offset: 0, len: nal.len };
        return 1;
    }

    const orig_hdr: u8 = nal[0];
    const fu_indicator: u8 = (orig_hdr & 0xE0) | H264_NAL_FU_A;
    const orig_type: u8 = orig_hdr & 0x1F;
    const max_frag: usize = max_payload_bytes - 2;
    const body: []const u8 = nal[1..];

    let body_off: usize = 0;
    let out_pos: usize = 0;
    let pkt_cnt: usize = 0;

    while (body_off < body.len) {
        if (pkt_cnt >= out_spans.len) return VideoNalError.OutputSliceTooSmall;
        const rem: usize = body.len - body_off;
        const chunk: usize = if (rem < max_frag) rem else max_frag;
        const is_start: bool = (body_off == 0);
        const is_end: bool = (body_off + chunk == body.len);

        if (out_pos + 2 + chunk > out_buf.len) return VideoNalError.BufferTooSmall;
        let fu_hdr: u8 = orig_type;
        if (is_start) fu_hdr |= 0x80;
        if (is_end) fu_hdr |= 0x40;

        const pkt_start = out_pos;
        out_buf[out_pos] = fu_indicator;
        out_buf[out_pos + 1] = fu_hdr;
        out_pos += 2;

        let i: usize = 0;
        while (i < chunk) : (i += 1) {
            out_buf[out_pos + i] = body[body_off + i];
        }
        out_pos += chunk;
        body_off += chunk;

        out_spans[pkt_cnt] = RtpPayloadSpan{ offset: pkt_start, len: 2 + chunk };
        pkt_cnt += 1;
    }

    return pkt_cnt;
}

/// Packs multiple small H.264 NAL units (e.g. SPS + PPS) into a single RFC 6184 STAP-A (`type = 24`) RTP payload.
pub fn packH264StapA(nals: []const []const u8, out_buf: []u8): usize ! VideoNalError {
    if (nals.len == 0) return VideoNalError.InvalidRtpPayload;
    let max_nri: u8 = 0;
    let req: usize = 1;
    let i: usize = 0;
    while (i < nals.len) : (i += 1) {
        if (nals[i].len == 0 || nals[i].len > 65535) return VideoNalError.InvalidRtpPayload;
        const nri: u8 = nals[i][0] & 0x60;
        if (nri > max_nri) max_nri = nri;
        req += nals[i].len + 2;
    }
    if (out_buf.len < req) return VideoNalError.BufferTooSmall;

    out_buf[0] = max_nri | H264_NAL_STAP_A;
    let pos: usize = 1;
    i = 0;
    while (i < nals.len) : (i += 1) {
        const n = nals[i];
        out_buf[pos] = @as(u8, @intCast((n.len >> 8) & 0xFF));
        out_buf[pos + 1] = @as(u8, @intCast(n.len & 0xFF));
        pos += 2;
        let k: usize = 0;
        while (k < n.len) : (k += 1) {
            out_buf[pos + k] = n[k];
        }
        pos += n.len;
    }
    return pos;
}

/// Caller-slice-backed RFC 6184 H.264 RTP depacketizer supporting Single NAL, STAP-A, and FU-A
/// with automatic incomplete-fragment rollback on packet loss, emitting 4-byte Annex-B frames.
pub struct H264RtpDepacketizer {
    out_buf: []u8 = [],
    pos: usize = 0;
    in_fua: bool = false;
    fua_start_pos: usize = 0;
    fua_nal_type: u8 = 0;
    dropped_fragments: usize = 0;

    static fn init(out_buf: []u8): H264RtpDepacketizer {
        return H264RtpDepacketizer{ out_buf: out_buf };
    }

    fn reset(*_) {
        pos = 0;
        in_fua = false;
        fua_start_pos = 0;
        fua_nal_type = 0;
    }

    fn bytes(): []const u8 {
        return out_buf[0..pos];
    }

    fn abortPendingFua(*_) {
        if (in_fua) {
            pos = fua_start_pos;
            in_fua = false;
            dropped_fragments += 1;
        }
    }

    fn emitStartCode(*_): void ! VideoNalError {
        if (pos + 4 > out_buf.len) return VideoNalError.BufferTooSmall;
        out_buf[pos + 0] = 0x00;
        out_buf[pos + 1] = 0x00;
        out_buf[pos + 2] = 0x00;
        out_buf[pos + 3] = 0x01;
        pos += 4;
    }

    fn emitBytes(*_, src: []const u8): void ! VideoNalError {
        if (pos + src.len > out_buf.len) return VideoNalError.BufferTooSmall;
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            out_buf[pos + i] = src[i];
        }
        pos += src.len;
    }

    /// Feeds one RFC 6184 RTP payload. Returns `true` when one or more complete NAL units were emitted.
    fn pushPacket(*_, rtp_payload: []const u8): bool ! VideoNalError {
        if (rtp_payload.len == 0) return VideoNalError.InvalidRtpPayload;
        const nal_type: u8 = rtp_payload[0] & 0x1F;

        // 1. Single NAL Unit (types 1..23)
        if (nal_type >= 1 && nal_type <= 23) {
            abortPendingFua();
            emitStartCode()?;
            emitBytes(rtp_payload)?;
            return true;
        }

        // 2. STAP-A (type 24)
        if (nal_type == H264_NAL_STAP_A) {
            abortPendingFua();
            let off: usize = 1;
            let emitted: bool = false;
            while (off + 2 <= rtp_payload.len) {
                const sz: usize = (@as(usize, rtp_payload[off]) << 8) | @as(usize, rtp_payload[off + 1]);
                off += 2;
                if (sz == 0 || off + sz > rtp_payload.len) return VideoNalError.InvalidRtpPayload;
                emitStartCode()?;
                emitBytes(rtp_payload[off .. off + sz])?;
                off += sz;
                emitted = true;
            }
            return emitted;
        }

        // 3. FU-A (type 28)
        if (nal_type == H264_NAL_FU_A) {
            if (rtp_payload.len < 3) return VideoNalError.InvalidRtpPayload;
            const fu_ind: u8 = rtp_payload[0];
            const fu_hdr: u8 = rtp_payload[1];
            const is_start: bool = (fu_hdr & 0x80) != 0;
            const is_end: bool = (fu_hdr & 0x40) != 0;
            const orig_type: u8 = fu_hdr & 0x1F;
            const frag: []const u8 = rtp_payload[2..];

            if (is_start) {
                abortPendingFua();
                fua_start_pos = pos;
                in_fua = true;
                fua_nal_type = orig_type;
                emitStartCode()?;
                const recon_hdr: [1]u8 = [(fu_ind & 0xE0) | orig_type];
                emitBytes(recon_hdr[0..])?;
                emitBytes(frag)?;
                if (is_end) {
                    in_fua = false;
                    return true;
                }
                return false;
            }

            if (!in_fua || orig_type != fua_nal_type) {
                // Orphaned continuation fragment after packet loss: discard safely
                dropped_fragments += 1;
                return false;
            }

            emitBytes(frag)?;
            if (is_end) {
                in_fua = false;
                return true;
            }
            return false;
        }

        return VideoNalError.InvalidRtpPayload;
    }
}

/// RFC 7798 H.265/HEVC RTP packetizer: Single NAL if `nal.len <= max_payload_bytes`,
/// or Fragmentation Unit FU (`type = 49`) with 2-byte PayloadHdr + 1-byte FU header.
pub fn packetizeH265Rtp(
    nal: []const u8,
    max_payload_bytes: usize,
    out_buf: []u8,
    out_spans: []RtpPayloadSpan,
): usize ! VideoNalError {
    if (nal.len < 2) return VideoNalError.InvalidNalHeader;
    if (max_payload_bytes < 5) return VideoNalError.InvalidMtu;

    if (nal.len <= max_payload_bytes) {
        if (out_spans.len < 1) return VideoNalError.OutputSliceTooSmall;
        if (out_buf.len < nal.len) return VideoNalError.BufferTooSmall;
        let i: usize = 0;
        while (i < nal.len) : (i += 1) {
            out_buf[i] = nal[i];
        }
        out_spans[0] = RtpPayloadSpan{ offset: 0, len: nal.len };
        return 1;
    }

    const orig_type: u8 = (nal[0] >> 1) & 0x3F;
    const payload_hdr0: u8 = (nal[0] & 0x81) | (H265_NAL_FU << 1);
    const payload_hdr1: u8 = nal[1];
    const max_frag: usize = max_payload_bytes - 3;
    const body: []const u8 = nal[2..];

    let body_off: usize = 0;
    let out_pos: usize = 0;
    let pkt_cnt: usize = 0;

    while (body_off < body.len) {
        if (pkt_cnt >= out_spans.len) return VideoNalError.OutputSliceTooSmall;
        const rem: usize = body.len - body_off;
        const chunk: usize = if (rem < max_frag) rem else max_frag;
        const is_start: bool = (body_off == 0);
        const is_end: bool = (body_off + chunk == body.len);

        if (out_pos + 3 + chunk > out_buf.len) return VideoNalError.BufferTooSmall;
        let fu_hdr: u8 = orig_type;
        if (is_start) fu_hdr |= 0x80;
        if (is_end) fu_hdr |= 0x40;

        const pkt_start = out_pos;
        out_buf[out_pos] = payload_hdr0;
        out_buf[out_pos + 1] = payload_hdr1;
        out_buf[out_pos + 2] = fu_hdr;
        out_pos += 3;

        let i: usize = 0;
        while (i < chunk) : (i += 1) {
            out_buf[out_pos + i] = body[body_off + i];
        }
        out_pos += chunk;
        body_off += chunk;

        out_spans[pkt_cnt] = RtpPayloadSpan{ offset: pkt_start, len: 3 + chunk };
        pkt_cnt += 1;
    }

    return pkt_cnt;
}

/// Caller-slice-backed RFC 7798 H.265/HEVC RTP depacketizer supporting Single NAL, AP (`type = 48`),
/// and FU (`type = 49`) with incomplete-fragment rollback on packet loss, emitting 4-byte Annex-B frames.
pub struct H265RtpDepacketizer {
    out_buf: []u8 = [],
    pos: usize = 0;
    in_fu: bool = false;
    fu_start_pos: usize = 0;
    fu_nal_type: u8 = 0;
    dropped_fragments: usize = 0;

    static fn init(out_buf: []u8): H265RtpDepacketizer {
        return H265RtpDepacketizer{ out_buf: out_buf };
    }

    fn reset(*_) {
        pos = 0;
        in_fu = false;
        fu_start_pos = 0;
        fu_nal_type = 0;
    }

    fn bytes(): []const u8 {
        return out_buf[0..pos];
    }

    fn abortPendingFu(*_) {
        if (in_fu) {
            pos = fu_start_pos;
            in_fu = false;
            dropped_fragments += 1;
        }
    }

    fn emitStartCode(*_): void ! VideoNalError {
        if (pos + 4 > out_buf.len) return VideoNalError.BufferTooSmall;
        out_buf[pos + 0] = 0x00;
        out_buf[pos + 1] = 0x00;
        out_buf[pos + 2] = 0x00;
        out_buf[pos + 3] = 0x01;
        pos += 4;
    }

    fn emitBytes(*_, src: []const u8): void ! VideoNalError {
        if (pos + src.len > out_buf.len) return VideoNalError.BufferTooSmall;
        let i: usize = 0;
        while (i < src.len) : (i += 1) {
            out_buf[pos + i] = src[i];
        }
        pos += src.len;
    }

    fn pushPacket(*_, rtp_payload: []const u8): bool ! VideoNalError {
        if (rtp_payload.len < 2) return VideoNalError.InvalidRtpPayload;
        const nal_type: u8 = (rtp_payload[0] >> 1) & 0x3F;

        // 1. Single NAL Unit (types 0..47)
        if (nal_type < 48) {
            abortPendingFu();
            emitStartCode()?;
            emitBytes(rtp_payload)?;
            return true;
        }

        // 2. Aggregation Packet AP (type 48)
        if (nal_type == H265_NAL_AP) {
            abortPendingFu();
            let off: usize = 2;
            let emitted: bool = false;
            while (off + 2 <= rtp_payload.len) {
                const sz: usize = (@as(usize, rtp_payload[off]) << 8) | @as(usize, rtp_payload[off + 1]);
                off += 2;
                if (sz == 0 || off + sz > rtp_payload.len) return VideoNalError.InvalidRtpPayload;
                emitStartCode()?;
                emitBytes(rtp_payload[off .. off + sz])?;
                off += sz;
                emitted = true;
            }
            return emitted;
        }

        // 3. Fragmentation Unit FU (type 49)
        if (nal_type == H265_NAL_FU) {
            if (rtp_payload.len < 4) return VideoNalError.InvalidRtpPayload;
            const fu_hdr: u8 = rtp_payload[2];
            const is_start: bool = (fu_hdr & 0x80) != 0;
            const is_end: bool = (fu_hdr & 0x40) != 0;
            const orig_type: u8 = fu_hdr & 0x3F;
            const frag: []const u8 = rtp_payload[3..];

            if (is_start) {
                abortPendingFu();
                fu_start_pos = pos;
                in_fu = true;
                fu_nal_type = orig_type;
                emitStartCode()?;
                const recon_hdr: [2]u8 = [
                    (rtp_payload[0] & 0x81) | (orig_type << 1),
                    rtp_payload[1],
                ];
                emitBytes(recon_hdr[0..])?;
                emitBytes(frag)?;
                if (is_end) {
                    in_fu = false;
                    return true;
                }
                return false;
            }

            if (!in_fu || orig_type != fu_nal_type) {
                dropped_fragments += 1;
                return false;
            }

            emitBytes(frag)?;
            if (is_end) {
                in_fu = false;
                return true;
            }
            return false;
        }

        return VideoNalError.InvalidRtpPayload;
    }
}
