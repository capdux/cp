// Production SIMD Verification & 6,000-Iteration Full-Protocol Deterministic Fuzz Suite (Wave 27 Round 5)
//   - Test 1 (1,200 iters): H.264/H.265 SIMD Annex-B/EBSP + SPS + RTP Depacketizers
//   - Test 2 (1,200 iters): WebM/EBML + VP8/VP9/AV1 OBU + VP8 RTP Depacketizer
//   - Test 3 (1,200 iters): SCTP/PR-SCTP/DCEP wire chunks + CRC32c + SACK/ForwardTSN
//   - Test 4 (1,200 iters): Audio full-stack (WAV, ADTS/AAC ASC, Opus frames, MP3/Xing)
//   - Test 5 (1,200 iters): JPEG SIMD 8x8 DCT + WebP + Image probe malformed fuzz

import { test, expect_eq, expect_true } from "cap:test";
import {
    AnnexBIter,
    AvccIter,
    ebspToRbsp,
    parseH264Sps,
    parseH265SpsBasic,
    H264RtpDepacketizer,
    H265RtpDepacketizer,
    decodeEbmlVint,
    WebmDemuxIter,
    parseVp8FrameHeader,
    parseVp9FrameHeader,
    Av1ObuIter,
    parseAv1SequenceHeaderBasic,
    Vp8RtpDepacketizer,
    WavCuePoint,
    parseWavContainer,
    AdtsFrameIter,
    decodeAudioSpecificConfig,
    OpusFrameSlice,
    parseOpusPacketFrames,
    Mp3StreamIter,
    parseMp3XingHeader,
} from "../../lib/media/index.cp";
import {
    SctpCommonHeader,
    verifySctpPacketCrc32c,
    SctpWireChunkIter,
    GapAckBlock,
    ForwardTsnStreamEntry,
    decodeSctpDataChunk,
    decodeSctpInitChunk,
    decodeSctpSackChunk,
    decodeSctpForwardTsnChunk,
    decodeDcepOpen,
} from "../../lib/net/index.cp";
import {
    jpegEncodeInto,
    probe,
} from "../../lib/image/index.cp";

fn xorshift64(state: *u64): u64 {
    let x: u64 = state.*;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    state.* = x;
    return x;
}

// ============================================================================
// Test 1: H.264/H.265 SIMD Annex-B & EBSP + 1,200-Iter Fuzz
// ============================================================================
test("Fuzz 1/5 (1,200 iters): H.264/H.265 AnnexB, AVCC, EBSP, SPS & RTP Depacketizers") {
    // Quick SIMD correctness check: long non-zero payload (>40 bytes) + 00 00 03 escape + Annex-B start codes
    let stream: [80]u8 = [0xAA; 80];
    stream[0] = 0x00;
    stream[1] = 0x00;
    stream[2] = 0x00;
    stream[3] = 0x01;
    stream[4] = 0x65;
    stream[42] = 0x00;
    stream[43] = 0x00;
    stream[44] = 0x03;
    stream[45] = 0x01;
    stream[60] = 0x00;
    stream[61] = 0x00;
    stream[62] = 0x01;
    stream[63] = 0x41;

    let check_it = AnnexBIter.init(stream[0..]);
    let nal_cnt: usize = 0;
    while (u := check_it.next()) {
        * = u;
        nal_cnt += 1;
    }
    expect_eq(nal_cnt, 2);

    let rbsp_out: [256]u8 = [0; 256];
    const rlen: usize = ebspToRbsp(stream[4..60], rbsp_out[0..]) catch { expect_true(false); return; };
    expect_eq(rlen, 55); // stripped single 0x03 byte

    let rng: u64 = 0x1020304050607081;
    let buf: [256]u8 = [0; 256];
    let depkt_buf: [512]u8 = [0; 512];
    let h264_dep = H264RtpDepacketizer.init(depkt_buf[0..]);
    let h265_dep = H265RtpDepacketizer.init(depkt_buf[0..]);

    let iter: usize = 0;
    while (iter < 1200) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) {
            const rv: u64 = xorshift64(*rng);
            buf[j] = if ((rv & 3) == 0) 0x00 else if ((rv & 7) == 1) 0x01 else if ((rv & 7) == 2) 0x03 else @as(u8, @intCast(rv & 0xFF));
        }
        const slice: []const u8 = buf[0..len];

        let ab = AnnexBIter.init(slice);
        let guard: usize = 0;
        while (guard < 64) : (guard += 1) {
            if (ab.next() == null) break;
        }

        if (avcc_opt := AvccIter.init(slice, if ((iter % 3) == 0) 1 else if ((iter % 3) == 1) 2 else 4) catch null) {
            let av = avcc_opt;
            guard = 0;
            while (guard < 64) : (guard += 1) {
                const nxt = av.next() catch break;
                if (nxt == null) break;
            }
        }

        * = ebspToRbsp(slice, rbsp_out[0..]) catch 0;
        * = parseH264Sps(slice) catch {};
        * = parseH265SpsBasic(slice) catch {};
        * = h264_dep.pushPacket(slice) catch false;
        * = h265_dep.pushPacket(slice) catch false;
        if ((iter % 16) == 0) {
            h264_dep.reset();
            h265_dep.reset();
        }
    }
    expect_eq(iter, 1200);
}

// ============================================================================
// Test 2: WebM / VP8 / VP9 / AV1 OBU + VP8 RTP Depacketizer (1,200 iters)
// ============================================================================
test("Fuzz 2/5 (1,200 iters): WebM/EBML, VP8/VP9/AV1 OBU & VP8 RTP Depacketizer") {
    let rng: u64 = 0x2030405060708092;
    let buf: [256]u8 = [0; 256];
    let vp8_out: [512]u8 = [0; 512];
    let vp8_dep = Vp8RtpDepacketizer.init(vp8_out[0..]);

    let iter: usize = 0;
    while (iter < 1200) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = @as(u8, @intCast(xorshift64(*rng) & 0xFF));
        }
        const slice: []const u8 = buf[0..len];

        * = decodeEbmlVint(slice) catch {};
        if (demux_opt := WebmDemuxIter.init(slice) catch null) {
            let dm = demux_opt;
            let guard: usize = 0;
            while (guard < 64) : (guard += 1) {
                const f = dm.nextFrame() catch break;
                if (f == null) break;
            }
        }

        * = parseVp8FrameHeader(slice) catch {};
        * = parseVp9FrameHeader(slice) catch {};

        let obu = Av1ObuIter.init(slice);
        let guard: usize = 0;
        while (guard < 64) : (guard += 1) {
            const nxt = obu.next() catch break;
            if (nxt == null) break;
        }
        * = parseAv1SequenceHeaderBasic(slice) catch {};

        * = vp8_dep.pushRtpPacket(@as(u16, @intCast(iter & 0xFFFF)), (iter % 3) == 0, slice) catch {};
        if ((iter % 16) == 0) vp8_dep.reset();
    }
    expect_eq(iter, 1200);
}

// ============================================================================
// Test 3: SCTP / PR-SCTP / DCEP Wire Chunks & CRC32c (1,200 iters)
// ============================================================================
test("Fuzz 3/5 (1,200 iters): SCTP CommonHeader, CRC32c, WireChunkIter, SACK/FwdTSN & DCEP") {
    let rng: u64 = 0x30405060708090A3;
    let buf: [256]u8 = [0; 256];
    let gaps: [16]GapAckBlock = [GapAckBlock{}; 16];
    let dups: [16]u32 = [0; 16];
    let ftsn_streams: [16]ForwardTsnStreamEntry = [ForwardTsnStreamEntry{}; 16];

    let iter: usize = 0;
    while (iter < 1200) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = @as(u8, @intCast(xorshift64(*rng) & 0xFF));
        }
        const slice: []const u8 = buf[0..len];

        * = SctpCommonHeader.decode(slice) catch {};
        * = verifySctpPacketCrc32c(slice) catch false;

        if (it_opt := SctpWireChunkIter.init(slice) catch null) {
            let it = it_opt;
            let guard: usize = 0;
            while (guard < 64) : (guard += 1) {
                const c = it.next() catch break;
                if (c == null) break;
            }
        }

        * = decodeSctpDataChunk(slice) catch {};
        * = decodeSctpInitChunk(slice) catch {};
        * = decodeSctpSackChunk(slice, gaps[0..], dups[0..]) catch {};
        * = decodeSctpForwardTsnChunk(slice, ftsn_streams[0..]) catch {};
        * = decodeDcepOpen(slice) catch {};
    }
    expect_eq(iter, 1200);
}

// ============================================================================
// Test 4: Audio Full-Stack (WAV, ADTS/AAC ASC, Opus, MP3/Xing) (1,200 iters)
// ============================================================================
test("Fuzz 4/5 (1,200 iters): WAV, ADTS/AAC ASC, Opus frames & MP3/Xing parsers") {
    let rng: u64 = 0x405060708090A0B4;
    let buf: [256]u8 = [0; 256];
    let cues: [8]WavCuePoint = [WavCuePoint{}; 8];
    let opus_frames: [48]OpusFrameSlice = [OpusFrameSlice{}; 48];

    let iter: usize = 0;
    while (iter < 1200) : (iter += 1) {
        const len: usize = @as(usize, @intCast(xorshift64(*rng) % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) {
            buf[j] = @as(u8, @intCast(xorshift64(*rng) & 0xFF));
        }
        // Periodically inject sync words (RIFF, ADTS 0xFFF, MP3 0xFFE)
        if ((iter % 4) == 0 && len >= 12) {
            buf[0] = 'R';
            buf[1] = 'I';
            buf[2] = 'F';
            buf[3] = 'F';
            buf[8] = 'W';
            buf[9] = 'A';
            buf[10] = 'V';
            buf[11] = 'E';
        } else if ((iter % 4) == 1 && len >= 7) {
            buf[0] = 0xFF;
            buf[1] = 0xF1;
        } else if ((iter % 4) == 2 && len >= 4) {
            buf[0] = 0xFF;
            buf[1] = 0xFB;
        }

        const slice: []const u8 = buf[0..len];

        * = parseWavContainer(slice, cues[0..]) catch {};

        let adts = AdtsFrameIter.init(slice);
        let guard: usize = 0;
        while (guard < 32) : (guard += 1) {
            const nxt = adts.next() catch break;
            if (nxt == null) break;
        }
        * = decodeAudioSpecificConfig(slice) catch {};

        * = parseOpusPacketFrames(slice, opus_frames[0..]) catch {};

        let mp3 = Mp3StreamIter.init(slice);
        guard = 0;
        while (guard < 32) : (guard += 1) {
            const fopt = mp3.next() catch break;
            if (fr := fopt) {
                * = parseMp3XingHeader(fr.frame_bytes, fr.header) catch {};
            } else {
                break;
            }
        }
    }
    expect_eq(iter, 1200);
}

// ============================================================================
// Test 5: JPEG SIMD 8x8 DCT + Image Probe & Malformed Encode Fuzz (1,200 iters)
// ============================================================================
test("Fuzz 5/5 (1,200 iters): JPEG SIMD DCT encodeInto & image probe malformed guard") {
    let rng: u64 = 0x5060708090A0B0C5;
    let pix: [256]u8 = [0; 256];
    let out_jpg: [2048]u8 = [0; 2048];
    let probe_buf: [256]u8 = [0; 256];

    // Valid 8x8 RGBA encode via SIMD @dot 8x8 DCT + probe round-trip
    let p: usize = 0;
    while (p < 256) : (p += 1) pix[p] = @as(u8, @intCast((p * 7) & 255));
    const encoded = jpegEncodeInto(pix[0..256], 8, 8, 85, out_jpg[0..]) catch {
        expect_true(false);
        return;
    };
    expect_true(encoded.len > 100);
    const info = probe(encoded) catch { expect_true(false); return; };
    expect_eq(info.width, 8);
    expect_eq(info.height, 8);

    let iter: usize = 0;
    while (iter < 1200) : (iter += 1) {
        const r1: u64 = xorshift64(*rng);
        const r2: u64 = xorshift64(*rng);
        const len: usize = @as(usize, @intCast(r1 % 256));
        let j: usize = 0;
        while (j < len) : (j += 1) {
            probe_buf[j] = @as(u8, @intCast(xorshift64(*rng) & 0xFF));
        }

        // Inject magic prefixes (JPEG, PNG, WebP, GIF, BMP)
        if ((iter % 5) == 0 && len >= 4) {
            probe_buf[0] = 0xFF;
            probe_buf[1] = 0xD8;
            probe_buf[2] = 0xFF;
        } else if ((iter % 5) == 1 && len >= 12) {
            probe_buf[0] = 'R';
            probe_buf[1] = 'I';
            probe_buf[2] = 'F';
            probe_buf[3] = 'F';
            probe_buf[8] = 'W';
            probe_buf[9] = 'E';
            probe_buf[10] = 'B';
            probe_buf[11] = 'P';
        }
        * = probe(probe_buf[0..len]) catch {};

        // Fuzz jpegEncodeInto with weird dimensions (0, huge, mismatched buffer, out-of-range quality)
        const w: u32 = if ((r1 & 7) == 0) 0 else if ((r1 & 7) == 1) 70000 else @as(u32, @intCast((r1 % 10) + 1));
        const h: u32 = if ((r2 & 7) == 0) 0 else if ((r2 & 7) == 1) 70000 else @as(u32, @intCast((r2 % 10) + 1));
        const q: u8 = @as(u8, @intCast(r1 & 0xFF));
        const out_cap: usize = @as(usize, @intCast(r2 % 512));
        * = jpegEncodeInto(pix[0..len], w, h, q, out_jpg[0..out_cap]) catch {};
    }
    expect_eq(iter, 1200);
}
