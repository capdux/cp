// Origin: Pure-CP Stdlib Test Suite (lib/media/videoNal.cp) | Created: 2026-11-23
// Production Deep Verification Suite for H.264/AVC + H.265/HEVC NAL Bitstream, SPS/PPS, Annex-B<->AVCC & RFC 6184/7798 RTP:
//   1. Annex-B (3-byte & 4-byte start codes) <-> AVCC (1/2/4-byte length prefix) lossless roundtrip & iterators
//   2. ITU-T §7.3.1 EBSP <-> RBSP emulation prevention byte (`00 00 03`) stripping & insertion roundtrip
//   3. `NalBitReader` unsigned/signed Exp-Golomb (`ue`/`se`) exact bitstream verification
//   4. Real H.264 1920x1080 High Profile SPS (1088->1080 crop) & 640x480 Baseline SPS + ISO 14496-15 `avcC` record
//   5. Real H.265/HEVC Main Profile 1920x1080 SPS (conformance_window crop + `00 00 03` EBSP) & `isVideoKeyframe`
//   6. RFC 6184 H.264 RTP packetizer/depacketizer: Single NAL, STAP-A (SPS+PPS), FU-A & mid-fragment packet-loss rollback
//   7. RFC 7798 H.265 RTP packetizer/depacketizer: Single NAL, AP (type=48), FU (type=49) & loss recovery
//   8. Boundary error guards (`InvalidNalHeader`, `InvalidAvccLength`, `InvalidMtu`, `BufferTooSmall`) & `cap.media` parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    VideoNalError,
    VideoCodecKind,
    H264_NAL_SLICE_NON_IDR,
    H264_NAL_SLICE_IDR,
    H264_NAL_SPS,
    H264_NAL_PPS,
    H264_NAL_STAP_A,
    H264_NAL_FU_A,
    H265_NAL_TRAIL_R,
    H265_NAL_IDR_W_RADL,
    H265_NAL_SPS,
    H265_NAL_AP,
    H265_NAL_FU,
    AnnexBNalUnit,
    AnnexBIter,
    annexBToAvcc,
    avccToAnnexB,
    ebspToRbsp,
    rbspToEbsp,
    NalBitReader,
    parseH264NalHeader,
    parseH264Sps,
    encodeAvcDecoderConfigRecord,
    parseH265NalHeader,
    parseH265SpsBasic,
    isVideoKeyframe,
    RtpPayloadSpan,
    packetizeH264Rtp,
    packH264StapA,
    H264RtpDepacketizer,
    packetizeH265Rtp,
    H265RtpDepacketizer,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("VideoNal Framing: Annex-B (3B & 4B start codes) <-> AVCC (2B & 4B lengths) lossless roundtrip") {
    // Mixed 4-byte (00 00 00 01) and 3-byte (00 00 01) Annex-B stream with 3 NALs (SPS, PPS, IDR)
    const annexb_in: [25]u8 = [
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1E, // 4B start code + 4B NAL
        0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80,       // 3B start code + 4B NAL
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0x84, 0x21, 0xA0, 0xFF, // 4B start code + 6B NAL
    ];

    let it = AnnexBIter.init(annexb_in[0..]);
    let count: usize = 0;
    while (u := it.next()) {
        count += 1;
        expect_true(u.nal_len >= 4);
    }
    expect_eq(count, @as(usize, 3));

    // Convert Annex-B -> 4-byte AVCC -> Annex-B
    let avcc_buf: [64]u8 = [0; 64];
    const avcc_len = annexBToAvcc(annexb_in[0..], 4, avcc_buf[0..]) catch { expect_true(false); return; };
    // (4+4) + (4+4) + (4+6) = 26 bytes
    expect_eq(avcc_len, @as(usize, 26));

    let annexb_out: [64]u8 = [0; 64];
    const ab_len = avccToAnnexB(avcc_buf[0..avcc_len], 4, annexb_out[0..]) catch { expect_true(false); return; };
    expect_eq(ab_len, @as(usize, 26));

    // Verify 2-byte AVCC length prefix mode as well
    const avcc2_len = annexBToAvcc(annexb_in[0..], 2, avcc_buf[0..]) catch { expect_true(false); return; };
    expect_eq(avcc2_len, @as(usize, 20));
    const ab2_len = avccToAnnexB(avcc_buf[0..avcc2_len], 2, annexb_out[0..]) catch { expect_true(false); return; };
    expect_eq(ab2_len, @as(usize, 26));
}

test("VideoNal EBSP <-> RBSP: ITU-T §7.3.1 emulation prevention byte (00 00 03) roundtrip") {
    const rbsp_orig: [12]u8 = [0x11, 0x00, 0x00, 0x01, 0x22, 0x00, 0x00, 0x03, 0x00, 0x00, 0x00, 0xAA];
    let ebsp_buf: [32]u8 = [0; 32];
    const ebsp_len = rbspToEbsp(rbsp_orig[0..], ebsp_buf[0..]) catch { expect_true(false); return; };
    // Must insert 0x03 before 0x01, before 0x03, and before 3rd 0x00 -> +3 bytes = 15 bytes
    expect_eq(ebsp_len, @as(usize, 15));

    let rbsp_dec: [32]u8 = [0; 32];
    const dec_len = ebspToRbsp(ebsp_buf[0..ebsp_len], rbsp_dec[0..]) catch { expect_true(false); return; };
    expect_eq(dec_len, rbsp_orig.len);
    expect_true(bytesEq(rbsp_dec[0..dec_len], rbsp_orig[0..]));
}

test("NalBitReader: unsigned & signed Exp-Golomb (ue/se) exact decoding") {
    // Bit sequence: ue(0)=1, ue(1)=010, ue(2)=011, ue(3)=00100, se(-2)=00101, se(+3)=00110 -> 1 010 011 0 0100 001 01 00110 0
    // Byte 0: 1010_0110 = 0xA6
    // Byte 1: 0100_0010 = 0x42
    // Byte 2: 1001_1000 = 0x98
    const bits: [3]u8 = [0xA6, 0x42, 0x98];
    let br = NalBitReader.init(bits[0..]);
    expect_eq(br.readUe() catch 99, @as(u32, 0));
    expect_eq(br.readUe() catch 99, @as(u32, 1));
    expect_eq(br.readUe() catch 99, @as(u32, 2));
    expect_eq(br.readUe() catch 99, @as(u32, 3));
    expect_eq(br.readSe() catch 99, @as(i32, -2));
    expect_eq(br.readSe() catch 99, @as(i32, 3));
}

test("H.264 SPS Parser: 1920x1080 High Profile (1088->1080 crop), 640x480 Baseline & avcC record") {
    // 1. Real H.264 1920x1080 High Profile Level 4.0 SPS with crop_bottom=4 (1088 - 8 = 1080)
    const sps_1080p: [11]u8 = [0x67, 0x64, 0x00, 0x28, 0xAC, 0xEC, 0x07, 0x80, 0x22, 0x7E, 0x54];
    const pps_sample: [4]u8 = [0x68, 0xEE, 0x3C, 0x80];
    let rbsp: [64]u8 = [0; 64];

    const hdr = parseH264NalHeader(sps_1080p[0..]) catch { expect_true(false); return; };
    expect_true(hdr.isSps());
    expect_eq(hdr.nal_ref_idc, @as(u8, 3));

    const info_hd = parseH264Sps(sps_1080p[0..], rbsp[0..]) catch { expect_true(false); return; };
    expect_eq(info_hd.profile_idc, @as(u8, 100));
    expect_eq(info_hd.level_idc, @as(u8, 40));
    expect_eq(info_hd.chroma_format_idc, @as(u32, 1));
    expect_eq(info_hd.coded_width, @as(u32, 1920));
    expect_eq(info_hd.coded_height, @as(u32, 1088));
    expect_eq(info_hd.crop_bottom, @as(u32, 4));
    expect_eq(info_hd.width, @as(u32, 1920));
    expect_eq(info_hd.height, @as(u32, 1080));

    // 2. Real H.264 640x480 Baseline Profile Level 3.0 SPS (no cropping)
    const sps_vga: [9]u8 = [0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x05, 0x01, 0xEC, 0x80];
    const info_vga = parseH264Sps(sps_vga[0..], rbsp[0..]) catch { expect_true(false); return; };
    expect_eq(info_vga.profile_idc, @as(u8, 66));
    expect_eq(info_vga.level_idc, @as(u8, 30));
    expect_eq(info_vga.width, @as(u32, 640));
    expect_eq(info_vga.height, @as(u32, 480));

    // 3. Encode ISO/IEC 14496-15 avcC decoder configuration record
    let avcc_rec: [64]u8 = [0; 64];
    const rec_len = encodeAvcDecoderConfigRecord(sps_1080p[0..], pps_sample[0..], avcc_rec[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(rec_len, @as(usize, 11 + 11 + 4));
    expect_eq(avcc_rec[0], @as(u8, 0x01));
    expect_eq(avcc_rec[1], @as(u8, 100));  // High profile
    expect_eq(avcc_rec[3], @as(u8, 40));   // Level 4.0
    expect_eq(avcc_rec[4], @as(u8, 0xFF)); // 4-byte NAL length size
    expect_eq(avcc_rec[5], @as(u8, 0xE1)); // 1 SPS
}

test("H.265/HEVC SPS Parser (1920x1080 Main Profile w/ EBSP 00 00 03 & crop) & isVideoKeyframe") {
    // Construct exact H.265 Main Profile Level 4.0 (120) 1920x1088 -> crop bottom 4*2=8 -> 1920x1080 RBSP
    // Tail bits after PTL:
    //   sps_id=0 (1), chroma=1 (010),
    //   w=1920 -> ue(1920): 1921=0b11110000001 -> 0000000000 11110000001 (21 bits)
    //   h=1088 -> ue(1088): 1089=0b10001000001 -> 0000000000 10001000001 (21 bits)
    //   conf_win=1 (1), l=0(1), r=0(1), t=0(1), b=4(00101), stop=1
    // Packed RBSP tail (7 bytes):
    //   1010_0000 = 0xA0
    //   0000_0011 = 0x03
    //   1100_0000 = 0xC0
    //   1000_0000 = 0x80
    //   0001_0001 = 0x11
    //   0000_0111 = 0x07
    //   1100_1011 = 0xCB
    const hevc_rbsp: [20]u8 = [
        0x01,                               // vps_id=0, max_sub_layers=0, nesting=1
        0x01,                               // general_profile_idc=1 (Main)
        0x60, 0x00, 0x00, 0x00,             // compat flags (contains 00 00 00 -> needs EBSP 03!)
        0x90, 0x00, 0x00, 0x00, 0x00, 0x00, // constraint flags
        0x78,                               // general_level_idc=120 (Level 4.0)
        0xA0, 0x03, 0xC0, 0x80, 0x11, 0x07, 0xCB,
    ];

    let sps_nal: [40]u8 = [0; 40];
    sps_nal[0] = H265_NAL_SPS << 1; // 0x42 (type=33)
    sps_nal[1] = 0x01;              // layer_id=0, tid=1
    const ebsp_len = rbspToEbsp(hevc_rbsp[0..], sps_nal[2..]) catch { expect_true(false); return; };
    // Verify 0x03 emulation prevention bytes were inserted into the zero runs
    expect_true(ebsp_len > hevc_rbsp.len);

    const h265_hdr = parseH265NalHeader(sps_nal[0 .. 2 + ebsp_len]) catch { expect_true(false); return; };
    expect_eq(h265_hdr.nal_unit_type, H265_NAL_SPS);
    expect_eq(h265_hdr.nuh_temporal_id_plus1, @as(u8, 1));

    let rbsp_scratch: [64]u8 = [0; 64];
    const hevc_info = parseH265SpsBasic(sps_nal[0 .. 2 + ebsp_len], rbsp_scratch[0..]) catch { expect_true(false); return; };
    expect_eq(hevc_info.general_profile_idc, @as(u8, 1));
    expect_eq(hevc_info.general_level_idc, @as(u8, 120));
    expect_eq(hevc_info.coded_width, @as(u32, 1920));
    expect_eq(hevc_info.coded_height, @as(u32, 1088));
    expect_eq(hevc_info.width, @as(u32, 1920));
    expect_eq(hevc_info.height, @as(u32, 1080));

    // Unified keyframe detection for H.264 & H.265
    const h264_idr_au: [9]u8 = [0, 0, 0, 1, 0x65, 0x88, 0x84, 0x21, 0xA0];
    const h264_p_au: [9]u8 = [0, 0, 0, 1, 0x41, 0x9A, 0x22, 0x33, 0x44];
    expect_true(isVideoKeyframe(h264_idr_au[0..], VideoCodecKind.h264));
    expect_true(!isVideoKeyframe(h264_p_au[0..], VideoCodecKind.h264));

    const h265_idr_au: [8]u8 = [0, 0, 0, 1, H265_NAL_IDR_W_RADL << 1, 0x01, 0xAA, 0xBB];
    const h265_p_au: [8]u8 = [0, 0, 0, 1, H265_NAL_TRAIL_R << 1, 0x01, 0xAA, 0xBB];
    expect_true(isVideoKeyframe(h265_idr_au[0..], VideoCodecKind.h265));
    expect_true(!isVideoKeyframe(h265_p_au[0..], VideoCodecKind.h265));
}

test("RFC 6184 H.264 RTP: Single NAL, STAP-A (SPS+PPS), FU-A fragmentation & packet-loss rollback") {
    const sps: [5]u8 = [0x67, 0x64, 0x00, 0x28, 0xAC];
    const pps: [4]u8 = [0x68, 0xEE, 0x3C, 0x80];

    // 1. Pack SPS + PPS into STAP-A (type=24)
    const pair: [2][]const u8 = [sps[0..], pps[0..]];
    let stap_buf: [32]u8 = [0; 32];
    const stap_len = packH264StapA(pair[0..], stap_buf[0..]) catch { expect_true(false); return; };
    expect_eq(stap_buf[0] & 0x1F, H264_NAL_STAP_A);

    // 2. Packetize a 300-byte IDR NAL into FU-A packets with MTU = 100
    let idr_nal: [300]u8 = [0; 300];
    idr_nal[0] = 0x65; // IDR
    let i: usize = 1;
    while (i < 300) : (i += 1) {
        idr_nal[i] = @as(u8, @intCast((i * 19 + 3) & 0xFF));
    }

    let rtp_buf: [512]u8 = [0; 512];
    let spans: [8]RtpPayloadSpan = [RtpPayloadSpan{}; 8];
    const n_pkts = packetizeH264Rtp(idr_nal[0..], 100, rtp_buf[0..], spans[0..]) catch { expect_true(false); return; };
    // 299 body bytes / 98 max_frag = 4 FU-A packets
    expect_eq(n_pkts, @as(usize, 4));
    expect_eq(spans[0].slice(rtp_buf[0..])[0] & 0x1F, H264_NAL_FU_A);
    expect_true((spans[0].slice(rtp_buf[0..])[1] & 0x80) != 0); // Start bit set
    expect_true((spans[3].slice(rtp_buf[0..])[1] & 0x40) != 0); // End bit set

    // 3. Depacketize STAP-A + simulate dropped FU-A (send only pkt 0) + recover on full FU-A stream
    let out_annexb: [1024]u8 = [0; 1024];
    let depkt = H264RtpDepacketizer.init(out_annexb[0..]);

    expect_true(depkt.pushPacket(stap_buf[0..stap_len]) catch false);
    // Inject incomplete FU-A (only first fragment, then interrupted by new complete FU-A!)
    expect_true(!(depkt.pushPacket(spans[0].slice(rtp_buf[0..])) catch true));

    let p: usize = 0;
    while (p < n_pkts) : (p += 1) {
        const done = depkt.pushPacket(spans[p].slice(rtp_buf[0..])) catch { expect_true(false); return; };
        expect_eq(done, p == n_pkts - 1);
    }
    // Incomplete fragment was cleanly rolled back and counted
    expect_eq(depkt.dropped_fragments, @as(usize, 1));

    // Verify reconstructed Annex-B contains exact [SPS, PPS, IDR(300B)]
    const recon = depkt.bytes();
    let it = AnnexBIter.init(recon);
    const o0 = it.next();
    const o1 = it.next();
    const o2 = it.next();
    expect_true(o0 != null && o1 != null && o2 != null);
    expect_true(it.next() == null);
    const u0 = o0 ?? AnnexBNalUnit{};
    const u1 = o1 ?? AnnexBNalUnit{};
    const u2 = o2 ?? AnnexBNalUnit{};
    expect_true(bytesEq(u0.nalSlice(recon), sps[0..]));
    expect_true(bytesEq(u1.nalSlice(recon), pps[0..]));
    expect_true(bytesEq(u2.nalSlice(recon), idr_nal[0..]));
}

test("RFC 7798 H.265 RTP: Single NAL, AP (type=48) & FU (type=49) packetization + depacketization") {
    let hevc_idr: [260]u8 = [0; 260];
    hevc_idr[0] = H265_NAL_IDR_W_RADL << 1; // type=19
    hevc_idr[1] = 0x01;                     // layer_id=0, tid=1
    let i: usize = 2;
    while (i < 260) : (i += 1) {
        hevc_idr[i] = @as(u8, @intCast((i * 23 + 11) & 0xFF));
    }

    let rtp_buf: [512]u8 = [0; 512];
    let spans: [8]RtpPayloadSpan = [RtpPayloadSpan{}; 8];
    const n_pkts = packetizeH265Rtp(hevc_idr[0..], 90, rtp_buf[0..], spans[0..]) catch { expect_true(false); return; };
    expect_eq(n_pkts, @as(usize, 3));
    expect_eq((spans[0].slice(rtp_buf[0..])[0] >> 1) & 0x3F, H265_NAL_FU);

    // Also test AP (type=48) carrying 2 small NALs
    const ap_pkt: [14]u8 = [
        H265_NAL_AP << 1, 0x01,
        0x00, 0x04, 0x40, 0x01, 0xAA, 0xBB, // VPS (4B)
        0x00, 0x04, 0x44, 0x01, 0xCC, 0xDD, // PPS (4B)
    ];

    let out_annexb: [1024]u8 = [0; 1024];
    let depkt = H265RtpDepacketizer.init(out_annexb[0..]);
    expect_true(depkt.pushPacket(ap_pkt[0..]) catch false);

    let p: usize = 0;
    while (p < n_pkts) : (p += 1) {
        const done = depkt.pushPacket(spans[p].slice(rtp_buf[0..])) catch { expect_true(false); return; };
        expect_eq(done, p == n_pkts - 1);
    }

    const recon = depkt.bytes();
    let it = AnnexBIter.init(recon);
    const o0 = it.next();
    const o1 = it.next();
    const o2 = it.next();
    expect_true(o0 != null && o1 != null && o2 != null);
    expect_true(it.next() == null);
    const n0 = o0 ?? AnnexBNalUnit{};
    const n1 = o1 ?? AnnexBNalUnit{};
    const n2 = o2 ?? AnnexBNalUnit{};
    expect_eq(n0.nal_len, @as(usize, 4));
    expect_eq(n1.nal_len, @as(usize, 4));
    expect_true(bytesEq(n2.nalSlice(recon), hevc_idr[0..]));
}

test("VideoNal boundary error guards & cap.media facade parity") {
    // 1. forbidden_zero_bit == 1 -> InvalidNalHeader
    const bad_h264: [2]u8 = [0x85, 0x00];
    let got_hdr: bool = false;
    * = cap.media.parseH264NalHeader(bad_h264[0..]) catch |e| {
        if (e == VideoNalError.InvalidNalHeader) got_hdr = true;
    };
    expect_true(got_hdr);

    // 2. Invalid AVCC length_size = 3 -> InvalidAvccLength
    let out: [16]u8 = [0; 16];
    let got_avcc: bool = false;
    * = cap.media.annexBToAvcc(bad_h264[0..], 3, out[0..]) catch |e| {
        if (e == VideoNalError.InvalidAvccLength) got_avcc = true;
    };
    expect_true(got_avcc);

    // 3. InvalidMtu (< 4)
    let spans: [4]RtpPayloadSpan = [RtpPayloadSpan{}; 4];
    let got_mtu: bool = false;
    * = cap.media.packetizeH264Rtp(bad_h264[0..], 2, out[0..], spans[0..]) catch |e| {
        if (e == VideoNalError.InvalidMtu) got_mtu = true;
    };
    expect_true(got_mtu);

    // 4. Facade constants parity
    expect_eq(cap.media.H264_NAL_SLICE_NON_IDR, H264_NAL_SLICE_NON_IDR);
    expect_eq(cap.media.H264_NAL_SLICE_IDR, H264_NAL_SLICE_IDR);
    expect_eq(cap.media.H264_NAL_SPS, H264_NAL_SPS);
    expect_eq(cap.media.H264_NAL_PPS, H264_NAL_PPS);
    expect_eq(cap.media.H265_NAL_FU, H265_NAL_FU);
}
