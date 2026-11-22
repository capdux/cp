// Origin: Pure-CP Stdlib Test Suite (lib/media/hlsDash.cp) | Created: 2026-11-23
// Production Deep Verification Suite for RFC 8216 HLS/LL-HLS, MPEG-DASH SegmentTemplate & ABR Controller:
//   1. HLS Master Playlist with quoted-comma CODECS parse & `formatHlsMasterPlaylist` round-trip
//   2. HLS VOD Media Playlist with implicit `#EXT-X-BYTERANGE` offset accumulation, `#EXT-X-DISCONTINUITY` & AES-128
//   3. LL-HLS Live Media Playlist `#EXT-X-PART-INF` & `#EXT-X-PART` partial segments
//   4. MPEG-DASH `SegmentTemplate` expansion (`$RepresentationID$`, `$Number%05d$`, `$Time$`, `$$`)
//   5. ABR Dual-EWMA spike suppression, upgrade hysteresis cooldown & instant downgrade
//   6. ABR BOLA logarithmic buffer utility & low-buffer panic downgrade guard
//   7. Malformed M3U8 / template / config error guards & `cap.media` facade parity

import { test, expect_eq, expect_true } from "cap:test";
import cap from "cap";
import {
    StreamManifestError,
    HlsPlaylistKind,
    HlsVariant,
    HlsVariantSpec,
    parseHlsMasterPlaylist,
    formatHlsMasterPlaylist,
    HlsByteRange,
    HlsSegment,
    HlsSegmentSpec,
    HlsPartialSegment,
    HlsMediaPlaylistInfo,
    parseHlsMediaPlaylist,
    formatHlsMediaPlaylist,
    DashTemplateParams,
    expandDashSegmentTemplate,
    AbrStrategy,
    AbrLevel,
    AbrController,
} from "cap:media";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

test("HLS Master: quoted-comma CODECS parsing & formatHlsMasterPlaylist round-trip") {
    const master =
        "#EXTM3U\n" ++
        "#EXT-X-VERSION:7\n" ++
        "#EXT-X-STREAM-INF:BANDWIDTH=1280000,AVERAGE-BANDWIDTH=1000000,RESOLUTION=640x360,FRAME-RATE=29.970,CODECS=\"avc1.4d401e,mp4a.40.2\"\n" ++
        "low/index.m3u8\n" ++
        "#EXT-X-STREAM-INF:BANDWIDTH=5500000,RESOLUTION=1920x1080,FRAME-RATE=60.000,CODECS=\"avc1.640028,mp4a.40.2\"\n" ++
        "high/index.m3u8\n";

    let vars: [4]HlsVariant = [HlsVariant{}; 4];
    let vars_s: []HlsVariant = vars[0..];
    const n = cap.media.parseHlsMasterPlaylist(master, *vars_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(n, @as(usize, 2));
    expect_eq(vars[0].bandwidth_bps, @as(u64, 1280000));
    expect_eq(vars[0].average_bandwidth_bps, @as(u64, 1000000));
    expect_eq(vars[0].width, @as(u32, 640));
    expect_eq(vars[0].height, @as(u32, 360));
    expect_eq(vars[0].frame_rate_milli, @as(u32, 29970));
    expect_true(bytesEq(vars[0].codecs(master), "avc1.4d401e,mp4a.40.2"));
    expect_true(bytesEq(vars[0].uri(master), "low/index.m3u8"));

    expect_eq(vars[1].bandwidth_bps, @as(u64, 5500000));
    expect_eq(vars[1].width, @as(u32, 1920));
    expect_eq(vars[1].height, @as(u32, 1080));
    expect_true(bytesEq(vars[1].uri(master), "high/index.m3u8"));

    // Format back and re-parse to confirm round-trip fidelity
    const specs: [2]HlsVariantSpec = [
        HlsVariantSpec{
            bandwidth_bps: vars[0].bandwidth_bps,
            average_bandwidth_bps: vars[0].average_bandwidth_bps,
            width: vars[0].width,
            height: vars[0].height,
            frame_rate_milli: vars[0].frame_rate_milli,
            codecs: vars[0].codecs(master),
            uri: vars[0].uri(master),
        },
        HlsVariantSpec{
            bandwidth_bps: vars[1].bandwidth_bps,
            width: vars[1].width,
            height: vars[1].height,
            frame_rate_milli: vars[1].frame_rate_milli,
            codecs: vars[1].codecs(master),
            uri: vars[1].uri(master),
        },
    ];
    let out_buf: [512]u8 = [0; 512];
    let out_s: []u8 = out_buf[0..];
    const fmt_len = formatHlsMasterPlaylist(specs[0..], *out_s) catch 0;
    expect_true(fmt_len > 80);

    let vars2: [4]HlsVariant = [HlsVariant{}; 4];
    let vars2_s: []HlsVariant = vars2[0..];
    const n2 = parseHlsMasterPlaylist(out_buf[0..fmt_len], *vars2_s) catch 0;
    expect_eq(n2, @as(usize, 2));
    expect_eq(vars2[0].bandwidth_bps, @as(u64, 1280000));
    expect_eq(vars2[1].height, @as(u32, 1080));
    expect_true(bytesEq(vars2[0].codecs(out_buf[0..fmt_len]), "avc1.4d401e,mp4a.40.2"));
}

test("HLS Media VOD: implicit BYTERANGE offsets, DISCONTINUITY, AES-128 & format round-trip") {
    const media =
        "#EXTM3U\n" ++
        "#EXT-X-VERSION:4\n" ++
        "#EXT-X-TARGETDURATION:6\n" ++
        "#EXT-X-MEDIA-SEQUENCE:100\n" ++
        "#EXT-X-PLAYLIST-TYPE:VOD\n" ++
        "#EXT-X-KEY:METHOD=AES-128,URI=\"https://cdn.example/key.bin\"\n" ++
        "#EXTINF:6.000,\n" ++
        "#EXT-X-BYTERANGE:1000@500\n" ++
        "movie.ts\n" ++
        "#EXTINF:5.500,\n" ++
        "#EXT-X-BYTERANGE:2000\n" ++
        "movie.ts\n" ++
        "#EXT-X-DISCONTINUITY\n" ++
        "#EXTINF:4.250,\n" ++
        "#EXT-X-BYTERANGE:800\n" ++
        "movie.ts\n" ++
        "#EXT-X-ENDLIST\n";

    let segs: [8]HlsSegment = [HlsSegment{}; 8];
    let parts: [4]HlsPartialSegment = [HlsPartialSegment{}; 4];
    let segs_s: []HlsSegment = segs[0..];
    let parts_s: []HlsPartialSegment = parts[0..];

    const info = parseHlsMediaPlaylist(media, *segs_s, *parts_s) catch {
        expect_true(false);
        return;
    };
    expect_true(info.kind == HlsPlaylistKind.media_vod);
    expect_true(info.end_list);
    expect_eq(info.version, @as(u32, 4));
    expect_eq(info.target_duration_sec, @as(u32, 6));
    expect_eq(info.media_sequence_base, @as(u64, 100));
    expect_eq(info.segment_count, @as(usize, 3));
    expect_eq(info.total_duration_ms, @as(u64, 15750));

    // Verify implicit BYTERANGE offset accumulation: 500 -> 1500 -> 3500
    expect_eq(segs[0].media_sequence, @as(u64, 100));
    expect_eq(segs[0].byte_range.offset, @as(u64, 500));
    expect_eq(segs[0].byte_range.length, @as(u64, 1000));
    expect_true(segs[0].key_method_aes128);

    expect_eq(segs[1].media_sequence, @as(u64, 101));
    expect_eq(segs[1].byte_range.offset, @as(u64, 1500));
    expect_eq(segs[1].byte_range.length, @as(u64, 2000));

    expect_eq(segs[2].media_sequence, @as(u64, 102));
    expect_true(segs[2].discontinuity);
    expect_eq(segs[2].byte_range.offset, @as(u64, 3500));
    expect_eq(segs[2].byte_range.length, @as(u64, 800));

    // Format back and re-parse
    const s_specs: [2]HlsSegmentSpec = [
        HlsSegmentSpec{ duration_ms: 6000, has_byte_range: true, byte_range: HlsByteRange{ length: 1000, offset: 500, has_offset: true }, uri: "seg0.ts" },
        HlsSegmentSpec{ duration_ms: 4500, discontinuity: true, uri: "seg1.ts" },
    ];
    let out_buf: [512]u8 = [0; 512];
    let out_s: []u8 = out_buf[0..];
    const m_len = formatHlsMediaPlaylist(info, s_specs[0..], *out_s) catch 0;
    expect_true(m_len > 60);

    const info2 = parseHlsMediaPlaylist(out_buf[0..m_len], *segs_s, *parts_s) catch {
        expect_true(false);
        return;
    };
    expect_eq(info2.segment_count, @as(usize, 2));
    expect_eq(info2.total_duration_ms, @as(u64, 10500));
    expect_true(segs[1].discontinuity);
}

test("LL-HLS Live: #EXT-X-PART-INF & #EXT-X-PART low-latency partial segments") {
    const llhls =
        "#EXTM3U\n" ++
        "#EXT-X-VERSION:9\n" ++
        "#EXT-X-TARGETDURATION:4\n" ++
        "#EXT-X-PART-INF:PART-TARGET=0.500\n" ++
        "#EXT-X-MEDIA-SEQUENCE:50\n" ++
        "#EXT-X-PART:DURATION=0.500,URI=\"seg50.0.m4s\",INDEPENDENT=YES\n" ++
        "#EXT-X-PART:DURATION=0.500,URI=\"seg50.1.m4s\"\n" ++
        "#EXTINF:2.000,\n" ++
        "seg50.m4s\n" ++
        "#EXT-X-PART:DURATION=0.450,URI=\"seg51.0.m4s\",INDEPENDENT=YES\n";

    let segs: [4]HlsSegment = [HlsSegment{}; 4];
    let parts: [8]HlsPartialSegment = [HlsPartialSegment{}; 8];
    let segs_s: []HlsSegment = segs[0..];
    let parts_s: []HlsPartialSegment = parts[0..];

    const info = parseHlsMediaPlaylist(llhls, *segs_s, *parts_s) catch {
        expect_true(false);
        return;
    };
    expect_true(info.kind == HlsPlaylistKind.media_live);
    expect_true(!info.end_list);
    expect_eq(info.part_target_ms, @as(u64, 500));
    expect_eq(info.segment_count, @as(usize, 1));
    expect_eq(info.part_count, @as(usize, 3));

    expect_eq(parts[0].duration_ms, @as(u64, 500));
    expect_true(parts[0].independent);
    expect_true(bytesEq(parts[0].uri(llhls), "seg50.0.m4s"));

    expect_true(!parts[1].independent);
    expect_eq(parts[2].duration_ms, @as(u64, 450));
    expect_true(parts[2].independent);
    expect_true(bytesEq(parts[2].uri(llhls), "seg51.0.m4s"));
}

test("MPEG-DASH: SegmentTemplate expansion with zero-padding, Bandwidth, Time & $$ escape") {
    let buf: [128]u8 = [0; 128];
    let buf_s: []u8 = buf[0..];

    const params = DashTemplateParams{
        representation_id: "video_1080p",
        number: 42,
        bandwidth_bps: 5_000_000,
        time: 96000,
    };
    const tmpl = "cdn/$RepresentationID$/seg_$Number%05d$_bw$Bandwidth$_t$Time%08d$_$$.m4s";
    const written = cap.media.expandDashSegmentTemplate(tmpl, params, *buf_s) catch {
        expect_true(false);
        return;
    };
    expect_true(bytesEq(
        buf[0..written],
        "cdn/video_1080p/seg_00042_bw5000000_t00096000_$.m4s",
    ));
}

test("ABR: Dual-EWMA spike suppression, upgrade hysteresis & instant downgrade") {
    const levels: [4]AbrLevel = [
        AbrLevel{ level_id: 0, bitrate_bps: 500_000, width: 480, height: 270 },
        AbrLevel{ level_id: 1, bitrate_bps: 1_500_000, width: 854, height: 480 },
        AbrLevel{ level_id: 2, bitrate_bps: 3_000_000, width: 1280, height: 720 },
        AbrLevel{ level_id: 3, bitrate_bps: 6_000_000, width: 1920, height: 1080 },
    ];

    let abr = AbrController.init(AbrStrategy.throughput_ewma, 0.85, 2_000, 20_000, 5_000) catch {
        expect_true(false);
        return;
    };

    // Baseline 2 Mbps -> safety budget = 1.7 Mbps -> picks level 1 (1.5 Mbps)
    abr.recordDownload(500_000, 2000); // 2 Mbps
    const d1 = abr.selectLevel(levels[0..], 10_000, 10_000);
    expect_eq(d1.selected_index, @as(usize, 1));

    // Single brief 10 Mbps spike: slow_ewma damps it -> conservative est stays well below 6 Mbps
    abr.recordDownload(2_500_000, 2000); // 10 Mbps spike
    const est_after_spike = abr.estimatedBandwidthBps();
    expect_true(est_after_spike < 4_000_000);

    // Sustained 10 Mbps pushes slow_ewma above 7.5 Mbps, but within 5000ms cooldown -> hysteresis hold!
    let k: usize = 0;
    while (k < 15) : (k += 1) {
        abr.recordDownload(2_500_000, 2000);
    }
    const d_hold = abr.selectLevel(levels[0..], 12_000, 12_000); // only 2000ms since last switch
    expect_true(d_hold.reason_hysteresis_hold);
    expect_eq(d_hold.selected_index, @as(usize, 1));

    // After cooldown expires (t = 16_000 >= 10_000 + 5_000) -> upgrades to top level 3
    const d_up = abr.selectLevel(levels[0..], 12_000, 16_000);
    expect_true(!d_up.reason_hysteresis_hold);
    expect_eq(d_up.selected_index, @as(usize, 3));

    // Sudden bandwidth crash to 600 kbps -> fast_ewma drops rapidly & downgrades immediately without cooldown!
    let m: usize = 0;
    while (m < 8) : (m += 1) {
        abr.recordDownload(150_000, 2000); // 600 kbps
    }
    const d_down = abr.selectLevel(levels[0..], 10_000, 17_000);
    expect_true(d_down.selected_index <= 1);
}

test("ABR: BOLA buffer utility & low-buffer emergency panic downgrade") {
    const levels: [3]AbrLevel = [
        AbrLevel{ level_id: 0, bitrate_bps: 600_000, width: 640, height: 360 },
        AbrLevel{ level_id: 1, bitrate_bps: 2_000_000, width: 1280, height: 720 },
        AbrLevel{ level_id: 2, bitrate_bps: 5_000_000, width: 1920, height: 1080 },
    ];

    let bola = AbrController.init(AbrStrategy.bola_buffer, 0.85, 2_000, 20_000, 0) catch {
        expect_true(false);
        return;
    };
    const low_buf = bola.selectLevel(levels[0..], 4_000, 1_000);
    const high_buf = bola.selectLevel(levels[0..], 19_000, 2_000);
    expect_true(high_buf.selected_index >= low_buf.selected_index);

    // Hybrid safe with huge bandwidth (20 Mbps) but critically low buffer (1500ms <= 2000ms) -> panic drop to 0!
    let hyb = AbrController.init(AbrStrategy.hybrid_safe, 0.85, 2_000, 20_000, 0) catch {
        expect_true(false);
        return;
    };
    hyb.recordDownload(5_000_000, 2000); // 20 Mbps
    const panic = hyb.selectLevel(levels[0..], 1_500, 5_000);
    expect_true(panic.reason_downgrade_panic);
    expect_eq(panic.selected_index, @as(usize, 0));
}

test("StreamManifestError: malformed M3U8, invalid template & config guards") {
    let vars: [2]HlsVariant = [HlsVariant{}; 2];
    let vars_s: []HlsVariant = vars[0..];
    let got_missing: bool = false;
    * = parseHlsMasterPlaylist("#EXT-X-VERSION:3\n", *vars_s) catch |e| {
        got_missing = (e == StreamManifestError.MissingExtM3u);
    };
    expect_true(got_missing);

    // Implicit BYTERANGE without prior offset rejected
    let segs: [2]HlsSegment = [HlsSegment{}; 2];
    let parts: [2]HlsPartialSegment = [HlsPartialSegment{}; 2];
    let segs_s: []HlsSegment = segs[0..];
    let parts_s: []HlsPartialSegment = parts[0..];
    let got_bad_br: bool = false;
    * = parseHlsMediaPlaylist("#EXTM3U\n#EXT-X-BYTERANGE:500\n#EXTINF:2.0,\na.ts\n", *segs_s, *parts_s) catch |e| {
        got_bad_br = (e == StreamManifestError.InvalidByteRange);
    };
    expect_true(got_bad_br);

    // Unclosed DASH template '$' rejected
    let out: [32]u8 = [0; 32];
    let out_s: []u8 = out[0..];
    let got_bad_tmpl: bool = false;
    * = expandDashSegmentTemplate("seg_$Number", DashTemplateParams{}, *out_s) catch |e| {
        got_bad_tmpl = (e == StreamManifestError.InvalidTemplate);
    };
    expect_true(got_bad_tmpl);

    // Invalid ABR config (panic >= max)
    let got_bad_cfg: bool = false;
    * = AbrController.init(AbrStrategy.hybrid_safe, 0.85, 10_000, 5_000, 1_000) catch |e| {
        got_bad_cfg = (e == StreamManifestError.InvalidConfig);
    };
    expect_true(got_bad_cfg);
}
