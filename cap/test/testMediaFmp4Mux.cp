// Origin: Pure-CP Stdlib Test Suite (lib/media/fmp4Mux.cp) | Created: 2026-11-23
// Production Deep Verification Suite for ISO/IEC 14496-12 CMAF fMP4 & RFC 3533/7845 Ogg-Opus Container Engine:
//   1. fMP4 Init Segment encoding (video AVC & audio Opus) + `Fmp4BoxIter` nested box tree verification
//   2. Multi-frame `moof`+`mdat` mux/demux with keyframes, B-frame signed `cts_offset`, exact `trun.data_offset == moof_size + 8`
//   3. 64-bit `tfdt` long live-stream timestamps & 32-sample external-slice fragment roundtrip
//   4. `Fmp4BoxIter` 64-bit `largesize` (`size==1`) & extend-to-EOF (`size==0`) box support
//   5. RFC 3533 Ogg page framing (0B / 300B / 1500B multi-segment lacing) + single-byte tamper `OggChecksumMismatch`
//   6. RFC 7845 `OpusHead` (incl. negative Q8 gain) & `OpusTags` roundtrip wrapped in Ogg BOS pages
//   7. Malformed box/truncation/buffer/magic/OpusHead guards & `cap.media` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    FMP4_BOX_FTYP,
    FMP4_BOX_MOOV,
    FMP4_BOX_MVHD,
    FMP4_BOX_TRAK,
    FMP4_BOX_TKHD,
    FMP4_BOX_MDIA,
    FMP4_BOX_MVEX,
    FMP4_BOX_TREX,
    FMP4_BOX_MOOF,
    FMP4_BOX_MDAT,
    FMP4_SAMPLE_FLAG_SYNC,
    FMP4_SAMPLE_FLAG_NON_SYNC,
    OGG_CAPTURE_PATTERN,
    OGG_PAGE_FLAG_BOS,
    OGG_PAGE_FLAG_EOS,
    OPUS_HEAD_MIN_LEN,
    Fmp4MuxError,
    computeOggCrc32,
    Fmp4TrackKind,
    Fmp4InitConfig,
    Fmp4BoxHeader,
    Fmp4BoxIter,
    encodeFmp4InitSegment,
    Fmp4SampleMeta,
    Fmp4ParsedSample,
    encodeFmp4Fragment,
    parseFmp4Fragment,
    encodeOggPage,
    parseOggPage,
    OpusHeadInfo,
    encodeOpusHead,
    parseOpusHeadInfo,
    encodeOpusTags,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("fMP4 Init Segment: video AVC & audio Opus nested box structure verification") {
    // 1. Video AVC 1920x1080
    let buf: [1024]u8 = [0; 1024];
    const vcfg = Fmp4InitConfig{
        track_id: 1,
        timescale: 90000,
        kind: Fmp4TrackKind.video_avc,
        width: 1920,
        height: 1080,
    };
    const vlen = encodeFmp4InitSegment(vcfg, buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(vlen > 200);

    let top = Fmp4BoxIter.init(buf[0..vlen]);
    const o_ftyp = top.next() catch { expect_true(false); return; };
    expect_true(o_ftyp != null);
    const b_ftyp = o_ftyp ?? Fmp4BoxHeader{};
    expect_eq(b_ftyp.box_type, FMP4_BOX_FTYP);
    const ftyp_pay = top.payloadSlice(b_ftyp);
    expect_true(bytesEq(ftyp_pay[0..4], "isom"));
    expect_true(bytesEq(ftyp_pay[16..20], "cmfc"));

    const o_moov = top.next() catch { expect_true(false); return; };
    expect_true(o_moov != null);
    const b_moov = o_moov ?? Fmp4BoxHeader{};
    expect_eq(b_moov.box_type, FMP4_BOX_MOOV);
    expect_true((top.next() catch { expect_true(false); return; }) == null);

    // Inspect children of moov: mvhd, trak (tkhd + mdia), mvex (trex)
    let moov_it = Fmp4BoxIter.init(top.payloadSlice(b_moov));
    const o_mvhd = moov_it.next() catch { expect_true(false); return; };
    expect_true(o_mvhd != null);
    const b_mvhd = o_mvhd ?? Fmp4BoxHeader{};
    expect_eq(b_mvhd.box_type, FMP4_BOX_MVHD);

    const o_trak = moov_it.next() catch { expect_true(false); return; };
    expect_true(o_trak != null);
    const b_trak = o_trak ?? Fmp4BoxHeader{};
    expect_eq(b_trak.box_type, FMP4_BOX_TRAK);
    let trak_it = Fmp4BoxIter.init(moov_it.payloadSlice(b_trak));
    const o_tkhd = trak_it.next() catch { expect_true(false); return; };
    const o_mdia = trak_it.next() catch { expect_true(false); return; };
    expect_true(o_tkhd != null && o_mdia != null);
    const b_tkhd = o_tkhd ?? Fmp4BoxHeader{};
    const b_mdia = o_mdia ?? Fmp4BoxHeader{};
    expect_eq(b_tkhd.box_type, FMP4_BOX_TKHD);
    expect_eq(b_mdia.box_type, FMP4_BOX_MDIA);

    const o_mvex = moov_it.next() catch { expect_true(false); return; };
    expect_true(o_mvex != null);
    const b_mvex = o_mvex ?? Fmp4BoxHeader{};
    expect_eq(b_mvex.box_type, FMP4_BOX_MVEX);
    let mvex_it = Fmp4BoxIter.init(moov_it.payloadSlice(b_mvex));
    const o_trex = mvex_it.next() catch { expect_true(false); return; };
    expect_true(o_trex != null);
    const b_trex = o_trex ?? Fmp4BoxHeader{};
    expect_eq(b_trex.box_type, FMP4_BOX_TREX);

    // 2. Audio Opus 48kHz stereo
    const acfg = Fmp4InitConfig{
        track_id: 2,
        timescale: 48000,
        kind: Fmp4TrackKind.audio_opus,
        sample_rate_hz: 48000,
        channels: 2,
    };
    const alen = encodeFmp4InitSegment(acfg, buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(alen > 200);
}

test("fMP4 Media Fragment: moof+mdat mux/demux, B-frame cts_offset & strict trun.data_offset alignment") {
    // 4 frames: I (cts=+6000), P (cts=+3000), B (cts=-3000), P (cts=0)
    const samples: [4]Fmp4SampleMeta = [
        Fmp4SampleMeta{ duration: 3000, size: 5, flags: FMP4_SAMPLE_FLAG_SYNC, cts_offset: 6000 },
        Fmp4SampleMeta{ duration: 3000, size: 3, flags: FMP4_SAMPLE_FLAG_NON_SYNC, cts_offset: 3000 },
        Fmp4SampleMeta{ duration: 3000, size: 4, flags: FMP4_SAMPLE_FLAG_NON_SYNC, cts_offset: -3000 },
        Fmp4SampleMeta{ duration: 3000, size: 2, flags: FMP4_SAMPLE_FLAG_NON_SYNC, cts_offset: 0 },
    ];
    const payload: [14]u8 = [
        0xAA, 0xA1, 0xA2, 0xA3, 0xA4, // frame 0 (5B)
        0xBB, 0xB1, 0xB2,             // frame 1 (3B)
        0xCC, 0xC1, 0xC2, 0xC3,       // frame 2 (4B)
        0xDD, 0xD1,                   // frame 3 (2B)
    ];

    let frag_buf: [512]u8 = [0; 512];
    const frag_len = encodeFmp4Fragment(7, 1, 90000, samples[0..], payload[0..], frag_buf[0..]) catch {
        expect_true(false);
        return;
    };

    let parsed: [8]Fmp4ParsedSample = [Fmp4ParsedSample{}; 8];
    const hdr = parseFmp4Fragment(frag_buf[0..frag_len], parsed[0..]) catch {
        expect_true(false);
        return;
    };

    expect_eq(hdr.sequence_number, @as(u32, 7));
    expect_eq(hdr.track_id, @as(u32, 1));
    expect_eq(hdr.base_decode_time, @as(u64, 90000));
    expect_eq(hdr.sample_count, @as(usize, 4));
    expect_eq(hdr.mdat_payload_len, @as(usize, 14));
    // Strict RFC/CMAF alignment: trun.data_offset == moof_size + 8
    expect_eq(@as(usize, @intCast(hdr.trun_data_offset)), hdr.moof_size + 8);
    expect_eq(parsed[0].data_offset, hdr.mdat_payload_offset);

    // Verify frame 0 (keyframe, cts=+6000)
    expect_true(parsed[0].is_keyframe);
    expect_eq(parsed[0].dts, @as(u64, 90000));
    expect_eq(parsed[0].pts, @as(u64, 96000));
    expect_eq(parsed[0].data_len, @as(usize, 5));
    expect_true(bytesEq(frag_buf[parsed[0].data_offset .. parsed[0].data_offset + 5], payload[0..5]));

    // Verify frame 2 (B-frame, negative cts=-3000)
    expect_true(!parsed[2].is_keyframe);
    expect_eq(parsed[2].dts, @as(u64, 96000));
    expect_eq(parsed[2].pts, @as(u64, 93000));
    expect_eq(parsed[2].data_len, @as(usize, 4));
    expect_true(bytesEq(frag_buf[parsed[2].data_offset .. parsed[2].data_offset + 4], payload[8..12]));

    // Verify frame 3
    expect_eq(parsed[3].dts, @as(u64, 99000));
    expect_eq(parsed[3].pts, @as(u64, 99000));
    expect_true(bytesEq(frag_buf[parsed[3].data_offset .. parsed[3].data_offset + 2], payload[12..14]));
}

test("fMP4 64-bit tfdt long live-stream timestamp & 32-frame external-slice roundtrip") {
    const long_dts: u64 = 0x0000_0005_1234_5678; // > 2^32 (long 24/7 live broadcast)
    let samples: [32]Fmp4SampleMeta = [Fmp4SampleMeta{}; 32];
    let payload: [512]u8 = [0; 512];
    let i: usize = 0;
    while (i < 32) : (i += 1) {
        samples[i] = Fmp4SampleMeta{
            duration: 1800,
            size: 16,
            flags: if ((i % 16) == 0) FMP4_SAMPLE_FLAG_SYNC else FMP4_SAMPLE_FLAG_NON_SYNC,
            cts_offset: 0,
        };
        let b: usize = 0;
        while (b < 16) : (b += 1) {
            payload[i * 16 + b] = @as(u8, @intCast((i * 17 + b) & 0xFF));
        }
    }

    let frag_buf: [2048]u8 = [0; 2048];
    const frag_len = encodeFmp4Fragment(4096, 1, long_dts, samples[0..], payload[0..], frag_buf[0..]) catch {
        expect_true(false);
        return;
    };

    let parsed: [32]Fmp4ParsedSample = [Fmp4ParsedSample{}; 32];
    const hdr = parseFmp4Fragment(frag_buf[0..frag_len], parsed[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(hdr.sequence_number, @as(u32, 4096));
    expect_eq(hdr.base_decode_time, long_dts);
    expect_eq(hdr.sample_count, @as(usize, 32));
    expect_true(parsed[0].is_keyframe);
    expect_true(!parsed[1].is_keyframe);
    expect_true(parsed[16].is_keyframe);
    expect_eq(parsed[31].dts, long_dts + @as(u64, 31 * 1800));
    expect_true(bytesEq(frag_buf[parsed[31].data_offset .. parsed[31].data_offset + 16], payload[31 * 16 .. 32 * 16]));
}

test("Fmp4BoxIter: 64-bit largesize (size==1) & extend-to-EOF (size==0) box decoding") {
    // Box 1: largesize (size=1, type="mdat", largesize=20, 4 bytes payload)
    // Box 2: extend-to-EOF (size=0, type="moof", 6 bytes payload)
    const raw: [34]u8 = [
        0x00, 0x00, 0x00, 0x01, // size == 1 (largesize)
        0x6D, 0x64, 0x61, 0x74, // "mdat"
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x14, // largesize == 20
        0xDE, 0xAD, 0xBE, 0xEF, // 4B payload
        0x00, 0x00, 0x00, 0x00, // size == 0 (extend to EOF -> 14B total, 6B payload)
        0x6D, 0x6F, 0x6F, 0x66, // "moof"
        1, 2, 3, 4, 5, 6,
    ];

    let it = Fmp4BoxIter.init(raw[0..]);
    const o_b1 = it.next() catch { expect_true(false); return; };
    expect_true(o_b1 != null);
    const b1 = o_b1 ?? Fmp4BoxHeader{};
    expect_eq(b1.box_type, FMP4_BOX_MDAT);
    expect_eq(b1.header_len, @as(usize, 16));
    expect_eq(b1.payload_len, @as(usize, 4));
    expect_eq(it.payloadSlice(b1)[0], @as(u8, 0xDE));

    const o_b2 = it.next() catch { expect_true(false); return; };
    expect_true(o_b2 != null);
    const b2 = o_b2 ?? Fmp4BoxHeader{};
    expect_eq(b2.box_type, FMP4_BOX_MOOF);
    expect_eq(b2.header_len, @as(usize, 8));
    expect_eq(b2.payload_len, @as(usize, 6));
    expect_eq(it.payloadSlice(b2)[5], @as(u8, 6));

    expect_true((it.next() catch { expect_true(false); return; }) == null);
}

test("RFC 3533 Ogg Page: 0B / 300B / 1500B lacing segmentation & single-byte tamper OggChecksumMismatch") {
    let page_buf: [2048]u8 = [0; 2048];

    // 1. 0-byte payload (1 lacing segment = 0)
    const empty_p: [0]u8 = [];
    const n0 = encodeOggPage(OGG_PAGE_FLAG_BOS, 0, 0x11223344, 0, empty_p[0..], page_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(n0, @as(usize, 28));
    const h0 = parseOggPage(page_buf[0..n0]) catch { expect_true(false); return; };
    expect_eq(h0.segment_count, @as(u8, 1));
    expect_eq(h0.payload_len, @as(usize, 0));
    expect_eq(h0.serial_no, @as(u32, 0x11223344));

    // 2. 300-byte payload (2 lacing segments: 255 + 45)
    let p300: [300]u8 = [0; 300];
    let i: usize = 0;
    while (i < 300) : (i += 1) {
        p300[i] = @as(u8, @intCast((i * 13 + 7) & 0xFF));
    }
    const n300 = encodeOggPage(0, 960, 0x11223344, 1, p300[0..], page_buf[0..]) catch {
        expect_true(false);
        return;
    };
    const h300 = parseOggPage(page_buf[0..n300]) catch { expect_true(false); return; };
    expect_eq(h300.segment_count, @as(u8, 2));
    expect_eq(h300.payload_len, @as(usize, 300));
    expect_eq(h300.granule_pos, @as(u64, 960));
    expect_true(bytesEq(page_buf[h300.payload_offset .. h300.payload_offset + 300], p300[0..]));

    // 3. 1500-byte payload (6 lacing segments: 5*255 + 225)
    let p1500: [1500]u8 = [0; 1500];
    i = 0;
    while (i < 1500) : (i += 1) {
        p1500[i] = @as(u8, @intCast((i * 31 + 3) & 0xFF));
    }
    const n1500 = encodeOggPage(OGG_PAGE_FLAG_EOS, 48000, 0xAABBCCDD, 2, p1500[0..], page_buf[0..]) catch {
        expect_true(false);
        return;
    };
    const h1500 = parseOggPage(page_buf[0..n1500]) catch { expect_true(false); return; };
    expect_eq(h1500.segment_count, @as(u8, 6));
    expect_eq(h1500.payload_len, @as(usize, 1500));
    expect_eq(h1500.flags, OGG_PAGE_FLAG_EOS);
    expect_true(bytesEq(page_buf[h1500.payload_offset .. h1500.payload_offset + 1500], p1500[0..]));

    // Single-byte tamper in payload -> OggChecksumMismatch
    page_buf[h1500.payload_offset + 500] ^= 0x01;
    let got_crc_err: bool = false;
    * = parseOggPage(page_buf[0..n1500]) catch |e| {
        if (e == Fmp4MuxError.OggChecksumMismatch) got_crc_err = true;
    };
    expect_true(got_crc_err);
}

test("RFC 7845 OpusHead & OpusTags roundtrip wrapped in Ogg BOS/metadata pages") {
    const head_in = OpusHeadInfo{
        version: 1,
        channels: 2,
        pre_skip: 312,
        input_sample_rate: 48000,
        output_gain_q8: -256, // -1.0 dB in Q7.8
        mapping_family: 0,
    };
    let head_bytes: [32]u8 = [0; 32];
    const hlen = encodeOpusHead(head_in, head_bytes[0..]) catch {
        expect_true(false);
        return;
    };
    expect_eq(hlen, OPUS_HEAD_MIN_LEN);

    // Wrap OpusHead in Ogg BOS page, parse Ogg page, then parse OpusHeadInfo
    let page_buf: [128]u8 = [0; 128];
    const plen = encodeOggPage(OGG_PAGE_FLAG_BOS, 0, 0xCAFE0001, 0, head_bytes[0..hlen], page_buf[0..]) catch {
        expect_true(false);
        return;
    };
    const phdr = parseOggPage(page_buf[0..plen]) catch { expect_true(false); return; };
    const head_out = parseOpusHeadInfo(page_buf[phdr.payload_offset .. phdr.payload_offset + phdr.payload_len]) catch {
        expect_true(false);
        return;
    };
    expect_eq(head_out.version, @as(u8, 1));
    expect_eq(head_out.channels, @as(u8, 2));
    expect_eq(head_out.pre_skip, @as(u16, 312));
    expect_eq(head_out.input_sample_rate, @as(u32, 48000));
    expect_eq(head_out.output_gain_q8, @as(i16, -256));
    expect_eq(head_out.mapping_family, @as(u8, 0));

    // Encode OpusTags with vendor + 2 comments
    const comments: [2][]const u8 = ["TITLE=Live Stream", "ENCODER=cap:media"];
    let tags_buf: [128]u8 = [0; 128];
    const tlen = encodeOpusTags("CP-fMP4-Mux", comments[0..], tags_buf[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(tags_buf[0..8], "OpusTags"));
    expect_eq(tlen, @as(usize, 8 + 4 + 11 + 4 + (4 + 17) + (4 + 17)));
}

test("fMP4 & Ogg-Opus boundary error guards + cap.media facade parity") {
    // 1. Truncated box
    const trunc: [5]u8 = [0, 0, 0, 20, 0x6D];
    let it = cap.media.Fmp4BoxIter.init(trunc[0..]);
    let got_trunc: bool = false;
    * = it.next() catch |e| {
        if (e == Fmp4MuxError.TruncatedBox) got_trunc = true;
    };
    expect_true(got_trunc);

    // 2. InvalidBox (size == 4 < 8)
    const bad_box: [8]u8 = [0, 0, 0, 4, 0x6D, 0x6F, 0x6F, 0x66];
    let it2 = Fmp4BoxIter.init(bad_box[0..]);
    let got_inv: bool = false;
    * = it2.next() catch |e| {
        if (e == Fmp4MuxError.InvalidBox) got_inv = true;
    };
    expect_true(got_inv);

    // 3. BufferTooSmall on encodeFmp4InitSegment
    let small: [32]u8 = [0; 32];
    let got_small: bool = false;
    * = cap.media.encodeFmp4InitSegment(Fmp4InitConfig{}, small[0..]) catch |e| {
        if (e == Fmp4MuxError.BufferTooSmall) got_small = true;
    };
    expect_true(got_small);

    // 4. OutputSliceTooSmall on parseFmp4Fragment
    const samples: [2]Fmp4SampleMeta = [
        Fmp4SampleMeta{ duration: 1000, size: 2, flags: FMP4_SAMPLE_FLAG_SYNC, cts_offset: 0 },
        Fmp4SampleMeta{ duration: 1000, size: 2, flags: FMP4_SAMPLE_FLAG_NON_SYNC, cts_offset: 0 },
    ];
    const pay: [4]u8 = [1, 2, 3, 4];
    let fbuf: [256]u8 = [0; 256];
    const flen = encodeFmp4Fragment(1, 1, 0, samples[0..], pay[0..], fbuf[0..]) catch { expect_true(false); return; };
    let one_s: [1]Fmp4ParsedSample = [Fmp4ParsedSample{}; 1];
    let got_out_small: bool = false;
    * = parseFmp4Fragment(fbuf[0..flen], one_s[0..]) catch |e| {
        if (e == Fmp4MuxError.OutputSliceTooSmall) got_out_small = true;
    };
    expect_true(got_out_small);

    // 5. InvalidOggMagic & InvalidOpusHead
    const bad_ogg: [32]u8 = [0; 32];
    let got_ogg: bool = false;
    * = cap.media.parseOggPage(bad_ogg[0..]) catch |e| {
        if (e == Fmp4MuxError.InvalidOggMagic) got_ogg = true;
    };
    expect_true(got_ogg);

    let got_opus: bool = false;
    * = cap.media.parseOpusHeadInfo(bad_ogg[0..]) catch |e| {
        if (e == Fmp4MuxError.InvalidOpusHead) got_opus = true;
    };
    expect_true(got_opus);

    // 6. cap.media facade parity
    expect_eq(cap.media.FMP4_BOX_FTYP, FMP4_BOX_FTYP);
    expect_true(bytesEq(cap.media.OGG_CAPTURE_PATTERN, OGG_CAPTURE_PATTERN));
    expect_eq(cap.media.computeOggCrc32("OggS"), computeOggCrc32("OggS"));
}
