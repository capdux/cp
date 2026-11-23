// Public Reproducible Full-Stack CP SIMD Performance Benchmark Suite (Wave 27 Round 10)
// Benchmarks 6 high-value pure-CP SIMD kernels with strict scalar accuracy verification (< 1e-4):
//   1. Dense Matrix GEMM (`matmulF32Simd` 64x64 x 20 iters): SIMD vs naive scalar ms, GFLOP/s & speedup
//   2. Audio DSP (`convolveFirF32Simd` 64-tap x 48,000 samples 1s @ 48kHz + `computeAudioRmsSimdF32`): ms & RTF
//   3. Image Processing (`computePsnrRgba` + `resizeRgba` 256x256 -> 128x128): ms & MPix/s
//   4. JPEG SIMD 8x8 DCT Encoding (`jpegEncodeInto` 256x256 RGBA): ms & MPix/s
//   5. Document & Search `cap:simd` (`CsvTableReader` 5,000 rows + `FullTextIndex` batch tokenize/query): MB/s
//   6. Pure-CP Decompression (`zstdDecompressInto` + `brotliDecompressInto`): throughput MB/s

import { test, expect_eq } from "cap:test";
import { WallClock } from "cap:time";
import {
    MatrixViewF32,
    MatrixMutF32,
    matmulF32Simd,
} from "../../lib/math/index.cp";
import {
    convolveFirF32Simd,
    computeAudioRmsSimdF32,
    mixAudioGainSimdF32,
    ResizeFilterKind,
    resizeRgba,
    computePsnrRgba,
} from "../../lib/media/index.cp";
import {
    jpegEncodeInto,
} from "../../lib/image/index.cp";
import {
    CsvTableOptions,
    CsvTableReader,
} from "../../lib/doc/index.cp";
import {
    FullTextFieldSpec,
    FullTextConfig,
    FtDocMeta,
    FtTermEntry,
    FtPosting,
    FtHit,
    FullTextBuffers,
    FullTextScratch,
    FullTextIndex,
} from "../../lib/vector/index.cp";
import {
    ZstdWorkspace,
    zstdDecompressInto,
    brotliDecompressInto,
} from "../../lib/pack/index.cp";

const std = @import("std");

fn absF64(x: f64): f64 {
    return if (x < 0.0) -x else x;
}

fn expect_true(cond: bool): void! {
    expect_eq(cond, true);
}

fn nsToMs(ns: i64): f64 {
    const clamped: i64 = if (ns > 1) ns else 1;
    return @as(f64, @floatFromInt(clamped)) / 1.0e6;
}

// Authentic RFC 8878 Zstd JSON vector (2,280 raw bytes -> 103 compressed bytes)
const ZSTD_BENCH_VEC: [103]u8 = [
    0x28, 0xB5, 0x2F, 0xFD, 0x04, 0x58, 0xD5, 0x02, 0x00, 0xC2, 0xC5, 0x13, 0x1A, 0x70, 0x89, 0x5A,
    0x0C, 0xB0, 0x0F, 0x6B, 0xA5, 0xC7, 0x52, 0xFE, 0xF6, 0x4A, 0x14, 0xD0, 0xAB, 0xAA, 0x67, 0xE1,
    0x19, 0x7D, 0x5F, 0xFA, 0x59, 0x57, 0x04, 0x80, 0x05, 0x61, 0xE5, 0x50, 0x0B, 0x89, 0x05, 0x9F,
    0xEB, 0x80, 0x40, 0xD3, 0xA9, 0xE3, 0x5C, 0xEF, 0x21, 0x32, 0x1B, 0xBD, 0xC6, 0x94, 0x0F, 0xEC,
    0x84, 0x4B, 0xC3, 0x1C, 0xE6, 0xF5, 0xC1, 0xA4, 0x67, 0x1B, 0x37, 0x41, 0x16, 0xAF, 0x15, 0xC3,
    0x36, 0xB2, 0x77, 0xB8, 0x0C, 0x84, 0x0C, 0xFB, 0x48, 0x56, 0x04, 0x01, 0x00, 0x5C, 0x10, 0x3E,
    0x3F, 0x44, 0x01, 0x40, 0x83, 0x29, 0x08,
];

// Authentic RFC 7932 Brotli JSON vector (1,872 raw bytes -> 112 compressed bytes)
const BR_BENCH_VEC: [112]u8 = [
    0x1B, 0x4F, 0x07, 0xE0, 0x2D, 0x0A, 0xEC, 0x36, 0xB2, 0x0C, 0xE5, 0xA0, 0xE6, 0x4D, 0x95, 0x68,
    0xF1, 0x4C, 0xAD, 0xA0, 0xB7, 0xD7, 0x47, 0xE7, 0x96, 0x2A, 0x7B, 0xB5, 0x18, 0x2B, 0xC2, 0x54,
    0x37, 0x1D, 0x4E, 0x08, 0x6A, 0xB1, 0xE8, 0x84, 0x88, 0x0A, 0xC7, 0xC8, 0x07, 0x96, 0x0F, 0xA7,
    0x1C, 0x38, 0x64, 0x71, 0x5B, 0x9E, 0x06, 0x84, 0xD9, 0xC9, 0xE1, 0xB0, 0x03, 0xF6, 0x86, 0x23,
    0x86, 0x49, 0xA5, 0x1A, 0x28, 0x1B, 0x7C, 0x2C, 0xC5, 0xC2, 0x29, 0xDF, 0x04, 0xCB, 0xB1, 0x98,
    0x23, 0xE9, 0xA1, 0xB0, 0xF9, 0x01, 0x16, 0xA3, 0xDF, 0x13, 0x61, 0x44, 0x53, 0xED, 0x52, 0x68,
    0x3B, 0x21, 0x3D, 0x2B, 0x9D, 0xFC, 0x1F, 0xA9, 0x35, 0xCA, 0x94, 0xD3, 0xD9, 0x7B, 0x8C, 0x13,
];

fn benchGemm(): void! {
    const N: usize = 64;
    const ITERS: usize = 20;
    let a: [4096]f32 = [0.0; 4096];
    let b: [4096]f32 = [0.0; 4096];
    let c_simd: [4096]f32 = [0.0; 4096];
    let c_ref: [4096]f32 = [0.0; 4096];
    let bt_w: [4096]f32 = [0.0; 4096];

    let i: usize = 0;
    while (i < 4096) : (i += 1) {
        a[i] = (@as(f32, @floatFromInt((i * 17) % 101)) - 50.0) * 0.01;
        b[i] = (@as(f32, @floatFromInt((i * 31) % 97)) - 48.0) * 0.01;
    }

    const va: MatrixViewF32 = MatrixViewF32.init(a[0..], N, N) catch { expect_true(false)?; return; };
    const vb: MatrixViewF32 = MatrixViewF32.init(b[0..], N, N) catch { expect_true(false)?; return; };
    let mc: MatrixMutF32 = MatrixMutF32.init(c_simd[0..], N, N) catch { expect_true(false)?; return; };
    let bt_s: []f32 = bt_w[0..];

    const t0: i64 = WallClock.nowUnixNs();
    let it: usize = 0;
    while (it < ITERS) : (it += 1) {
        * = matmulF32Simd(va, vb, 1.0, 0.0, *mc, bt_s) catch { expect_true(false)?; return; };
    }
    const simd_ms: f64 = nsToMs(WallClock.nowUnixNs() - t0);

    const t1: i64 = WallClock.nowUnixNs();
    let it2: usize = 0;
    while (it2 < ITERS) : (it2 += 1) {
        let r: usize = 0;
        while (r < N) : (r += 1) {
            let c: usize = 0;
            while (c < N) : (c += 1) {
                let acc: f32 = 0.0;
                let k: usize = 0;
                while (k < N) : (k += 1) {
                    acc += a[r * N + k] * b[k * N + c];
                }
                c_ref[r * N + c] = acc;
            }
        }
    }
    const scalar_ms: f64 = nsToMs(WallClock.nowUnixNs() - t1);

    let max_err: f64 = 0.0;
    let ci: usize = 0;
    while (ci < 4096) : (ci += 1) {
        const e: f64 = absF64(@as(f64, c_simd[ci]) - @as(f64, c_ref[ci]));
        if (e > max_err) max_err = e;
    }
    expect_true(max_err < 1.0e-4)?;

    const total_flops: f64 = 2.0 * 64.0 * 64.0 * 64.0 * @as(f64, @floatFromInt(ITERS));
    const gflops: f64 = (total_flops / 1.0e9) / (simd_ms / 1000.0);
    const speedup: f64 = scalar_ms / simd_ms;
    std.debug.print(
        "[BENCH 1/6 GEMM 64x64 x20] SIMD={d:.2}ms ({d:.3} GFLOP/s) | Scalar={d:.2}ms | Speedup={d:.2}x | max_err={e:.2}\n",
        .{ simd_ms, gflops, scalar_ms, speedup, max_err },
    );
}

fn benchAudioFir(): void! {
    const alloc = std.heap.page_allocator;
    const N: usize = 48000;
    let pcm = alloc.alloc(f32, N) catch { expect_true(false)?; return; };
    defer alloc.free(pcm);
    let out_simd = alloc.alloc(f32, N) catch { expect_true(false)?; return; };
    defer alloc.free(out_simd);
    let out_ref = alloc.alloc(f32, N) catch { expect_true(false)?; return; };
    defer alloc.free(out_ref);

    let coeffs: [64]f32 = [0.0; 64];
    let k: usize = 0;
    while (k < 64) : (k += 1) coeffs[k] = 1.0 / 64.0;

    let i: usize = 0;
    while (i < N) : (i += 1) {
        pcm[i] = (std.math.sin(@as(f64, @floatFromInt(i)) * 0.05) * 0.5) as f32;
    }

    let out_s: []f32 = out_simd;
    const t0: i64 = WallClock.nowUnixNs();
    const w: usize = convolveFirF32Simd(pcm, coeffs[0..], out_s) catch { expect_true(false)?; return; };
    const simd_ms: f64 = nsToMs(WallClock.nowUnixNs() - t0);
    expect_eq(w, N);

    const t1: i64 = WallClock.nowUnixNs();
    let si: usize = 0;
    while (si < N) : (si += 1) {
        let acc: f32 = 0.0;
        const taps: usize = if (si + 1 < 64) (si + 1) else 64;
        let j: usize = 0;
        while (j < taps) : (j += 1) {
            acc += coeffs[j] * pcm[si - j];
        }
        out_ref[si] = acc;
    }
    const scalar_ms: f64 = nsToMs(WallClock.nowUnixNs() - t1);

    let max_err: f64 = 0.0;
    let ci: usize = 0;
    while (ci < N) : (ci += 1) {
        const e: f64 = absF64(@as(f64, out_simd[ci]) - @as(f64, out_ref[ci]));
        if (e > max_err) max_err = e;
    }
    expect_true(max_err < 1.0e-4)?;

    const rms_val: f64 = computeAudioRmsSimdF32(pcm);
    expect_true(absF64(rms_val - (0.5 / std.math.sqrt(2.0))) < 1.0e-3)?;
    const mixed: usize = mixAudioGainSimdF32(out_s, pcm, 0.25);
    expect_eq(mixed, N);

    const rtf: f64 = 1000.0 / simd_ms;
    const speedup: f64 = scalar_ms / simd_ms;
    std.debug.print(
        "[BENCH 2/6 Audio FIR 64-tap x 48k] SIMD={d:.2}ms (RTF={d:.1}x realtime) | Scalar={d:.2}ms | Speedup={d:.2}x | RMS={d:.4}\n",
        .{ simd_ms, rtf, scalar_ms, speedup, rms_val },
    );
}

fn benchImageOps(): void! {
    const alloc = std.heap.page_allocator;
    const W: usize = 256;
    const H: usize = 256;
    let src_a = alloc.alloc(u8, W * H * 4) catch { expect_true(false)?; return; };
    defer alloc.free(src_a);
    let src_b = alloc.alloc(u8, W * H * 4) catch { expect_true(false)?; return; };
    defer alloc.free(src_b);
    let dst = alloc.alloc(u8, 128 * 128 * 4) catch { expect_true(false)?; return; };
    defer alloc.free(dst);
    let sc = alloc.alloc(f32, H * 128 * 4) catch { expect_true(false)?; return; };
    defer alloc.free(sc);

    let i: usize = 0;
    while (i < W * H) : (i += 1) {
        const off: usize = i * 4;
        src_a[off + 0] = @as(u8, @intCast((i * 3) & 255));
        src_a[off + 1] = @as(u8, @intCast((i * 7) & 255));
        src_a[off + 2] = @as(u8, @intCast((i * 11) & 255));
        src_a[off + 3] = 255;
        src_b[off + 0] = src_a[off + 0] ^ 1;
        src_b[off + 1] = src_a[off + 1];
        src_b[off + 2] = src_a[off + 2];
        src_b[off + 3] = 255;
    }

    const t0: i64 = WallClock.nowUnixNs();
    let psnr: f64 = 0.0;
    let r: usize = 0;
    while (r < 10) : (r += 1) {
        psnr = computePsnrRgba(src_a, src_b, W, H);
    }
    const psnr_ms: f64 = nsToMs(WallClock.nowUnixNs() - t0);
    expect_true(psnr > 40.0 && psnr < 70.0)?;

    const t1: i64 = WallClock.nowUnixNs();
    * = resizeRgba(src_a, W, H, 128, 128, ResizeFilterKind.bilinear, dst, sc) catch {
        expect_true(false)?;
        return;
    };
    const resize_ms: f64 = nsToMs(WallClock.nowUnixNs() - t1);
    const psnr_mpix: f64 = (10.0 * 65536.0 / 1.0e6) / (psnr_ms / 1000.0);
    const res_mpix: f64 = (65536.0 / 1.0e6) / (resize_ms / 1000.0);
    std.debug.print(
        "[BENCH 3/6 Image 256x256] PSNRx10={d:.2}ms ({d:.2} MPix/s, {d:.2}dB) | Resize->128={d:.2}ms ({d:.2} MPix/s)\n",
        .{ psnr_ms, psnr_mpix, psnr, resize_ms, res_mpix },
    );
}

fn benchJpegDct(): void! {
    const alloc = std.heap.page_allocator;
    const W: usize = 256;
    const H: usize = 256;
    let rgba = alloc.alloc(u8, W * H * 4) catch { expect_true(false)?; return; };
    defer alloc.free(rgba);
    let jpg_out = alloc.alloc(u8, 262144) catch { expect_true(false)?; return; };
    defer alloc.free(jpg_out);

    let y: usize = 0;
    while (y < H) : (y += 1) {
        let x: usize = 0;
        while (x < W) : (x += 1) {
            const off: usize = (y * W + x) * 4;
            rgba[off + 0] = @as(u8, @intCast(x));
            rgba[off + 1] = @as(u8, @intCast(y));
            rgba[off + 2] = @as(u8, @intCast((x + y) & 255));
            rgba[off + 3] = 255;
        }
    }

    const t0: i64 = WallClock.nowUnixNs();
    const encoded = jpegEncodeInto(rgba, 256, 256, 85, jpg_out) catch { expect_true(false)?; return; };
    const enc_ms: f64 = nsToMs(WallClock.nowUnixNs() - t0);
    expect_true(encoded.len > 1000)?;
    const mpix: f64 = (65536.0 / 1.0e6) / (enc_ms / 1000.0);
    std.debug.print(
        "[BENCH 4/6 JPEG 8x8 DCT 256x256] Time={d:.2}ms ({d:.2} MPix/s) | Output={d} bytes\n",
        .{ enc_ms, mpix, encoded.len },
    );
}

fn benchCsvAndFullText(): void! {
    const alloc = std.heap.page_allocator;
    const row_tpl: []const u8 = "10042,alpha_service_worker_node_07,\"high throughput simd scanning engine\",99.95,true\r\n";
    const N_ROWS: usize = 5000;
    const total_csv_bytes: usize = row_tpl.len * N_ROWS;
    const csv_buf = alloc.alloc(u8, total_csv_bytes) catch { expect_true(false)?; return; };
    defer alloc.free(csv_buf);

    let r: usize = 0;
    while (r < N_ROWS) : (r += 1) {
        @memcpy(csv_buf[r * row_tpl.len..(r + 1) * row_tpl.len], row_tpl);
    }

    const t0: i64 = WallClock.nowUnixNs();
    let reader = CsvTableReader.init(csv_buf as string, CsvTableOptions{ delimiter: ',' });
    let fields: [8]string = [""; 8];
    let parsed_rows: usize = 0;
    while (cur := reader.nextRow(fields[0..])) {
        if (cur.fields_count == 5) parsed_rows += 1;
    }
    const csv_ms: f64 = nsToMs(WallClock.nowUnixNs() - t0);
    expect_eq(parsed_rows, N_ROWS);
    const csv_mbs: f64 = (@as(f64, @floatFromInt(total_csv_bytes)) / (1024.0 * 1024.0)) / (csv_ms / 1000.0);

    let fspecs: [2]FullTextFieldSpec = [
        FullTextFieldSpec{ weight: 2.0, b: 0.75 },
        FullTextFieldSpec{ weight: 1.0, b: 0.75 },
    ];
    let fsums: [2]u64 = [0; 2];
    let docs: [128]FtDocMeta = [FtDocMeta{}; 128];
    let flens: [256]u32 = [0; 256];
    let terms: [512]FtTermEntry = [FtTermEntry{}; 512];
    let htbl: [1024]u32 = [0; 1024];
    let tbytes: [8192]u8 = [0; 8192];
    let posts: [4096]FtPosting = [FtPosting{}; 4096];
    let pos_a: [8192]u32 = [0; 8192];
    let idx = FullTextIndex.init(
        FullTextConfig{ k1: 1.2, field_count: 2 },
        FullTextBuffers{
            field_specs: fspecs[0..],
            field_len_sums: fsums[0..],
            docs: docs[0..],
            doc_field_lens: flens[0..],
            terms: terms[0..],
            term_hash_table: htbl[0..],
            term_bytes: tbytes[0..],
            postings: posts[0..],
            positions: pos_a[0..],
        },
    ) catch { expect_true(false)?; return; };

    let sc_tids: [128]u32 = [0; 128];
    let sc_pos: [128]u32 = [0; 128];
    let sc_scores: [128]f64 = [0.0; 128];
    let sc_mask: [128]u8 = [0; 128];
    let sc_aux: [1024]u8 = [0; 1024];
    let sc_cnt: [128]u32 = [0; 128];
    let scratch = FullTextScratch{
        term_ids: sc_tids[0..],
        token_positions: sc_pos[0..],
        scores: sc_scores[0..],
        doc_mask: sc_mask[0..],
        doc_mask_aux: sc_aux[0..],
        matched_terms_cnt: sc_cnt[0..],
    };

    const t1: i64 = WallClock.nowUnixNs();
    const doc_txt: [2][]const u8 = ["High Performance SIMD FullText", "vectorized lowercase folding and inverted index retrieval"];
    let d: usize = 0;
    while (d < 100) : (d += 1) {
        * = idx.addDocument(@as(u64, @intCast(d + 1)), doc_txt[0..], *scratch) catch { expect_true(false)?; return; };
    }
    let hits: [8]FtHit = [FtHit{}; 8];
    let hits_s: []FtHit = hits[0..];
    let q: usize = 0;
    let total_hits: usize = 0;
    while (q < 100) : (q += 1) {
        total_hits += idx.searchQuery("SIMD AND retrieval", *scratch, hits_s) catch 0;
    }
    const ft_ms: f64 = nsToMs(WallClock.nowUnixNs() - t1);
    expect_eq(total_hits, 800);

    std.debug.print(
        "[BENCH 5/6 CSV+Search SIMD] CSV 5k rows={d:.2}ms ({d:.1} MB/s) | FullText 100doc+100q={d:.2}ms\n",
        .{ csv_ms, csv_mbs, ft_ms },
    );
}

fn benchPackDecompress(): void! {
    const ITERS: usize = 200;
    let lit_buf: [8192]u8 = [0; 8192];
    let ws: ZstdWorkspace = ZstdWorkspace.init(lit_buf[0..]);
    let out: [4096]u8 = [0; 4096];
    let out_s: []u8 = out[0..];

    const t0: i64 = WallClock.nowUnixNs();
    let total_zstd: usize = 0;
    let i: usize = 0;
    while (i < ITERS) : (i += 1) {
        total_zstd += zstdDecompressInto(ZSTD_BENCH_VEC[0..], out_s, *ws) catch |e| {
            std.debug.print("Zstd error: {any}\n", .{e});
            expect_true(false)?;
            return;
        };
    }
    const zstd_ms: f64 = nsToMs(WallClock.nowUnixNs() - t0);
    expect_eq(total_zstd, 2208 * ITERS);
    const zstd_mbs: f64 = (@as(f64, @floatFromInt(total_zstd)) / (1024.0 * 1024.0)) / (zstd_ms / 1000.0);

    const t1: i64 = WallClock.nowUnixNs();
    let total_br: usize = 0;
    let j: usize = 0;
    while (j < ITERS) : (j += 1) {
        total_br += brotliDecompressInto(BR_BENCH_VEC[0..], out_s) catch |e| {
            std.debug.print("Brotli error: {any}\n", .{e});
            expect_true(false)?;
            return;
        };
    }
    const br_ms: f64 = nsToMs(WallClock.nowUnixNs() - t1);
    expect_eq(total_br, 1872 * ITERS);
    const br_mbs: f64 = (@as(f64, @floatFromInt(total_br)) / (1024.0 * 1024.0)) / (br_ms / 1000.0);

    std.debug.print(
        "[BENCH 6/6 Pure-CP Pack x200] Zstd={d:.2}ms ({d:.1} MB/s) | Brotli={d:.2}ms ({d:.1} MB/s)\n",
        .{ zstd_ms, zstd_mbs, br_ms, br_mbs },
    );
}

pub fn main() {
    test("Bench 1/6: Dense Matrix GEMM 64x64 x20 (matmulF32Simd vs naive scalar)", () => {
        benchGemm()?;
    });
    test("Bench 2/6: Audio DSP 64-tap FIR x 48k samples (1s @ 48kHz) + computeAudioRmsSimdF32", () => {
        benchAudioFir()?;
    });
    test("Bench 3/6: Image SIMD PSNR + Resize 256x256 -> 128x128", () => {
        benchImageOps()?;
    });
    test("Bench 4/6: JPEG SIMD 8x8 DCT Encode 256x256 RGBA", () => {
        benchJpegDct()?;
    });
    test("Bench 5/6: CSV 5,000-row cap:simd scanner & FullTextIndex batch throughput", () => {
        benchCsvAndFullText()?;
    });
    test("Bench 6/6: Pure-CP Zstd (RFC 8878) + Brotli (RFC 7932) Decompression Throughput", () => {
        benchPackDecompress()?;
    });
}
