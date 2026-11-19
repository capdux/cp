import {
    gzip,
    gunzip,
    deflate,
    inflate,
    deflateRaw,
    inflateRaw,
    Gzip,
    Gunzip,
    DeflateCodec,
    InflateCodec,
    GzipHeader,
    RleCodec,
    GZIP_MAGIC_ID1,
    GZIP_MAGIC_ID2,
} from "cap:pack";

pub fn main()! {
    // -------------------------------------------------------------------------
    // Test 1: Standard Gzip & Gunzip Roundtrip + Header Validation
    // -------------------------------------------------------------------------
    let msg1 = "CP Language native high-performance Gzip compression powered by vendor/zlib static libz.a!";
    let comp1 = gzip(msg1, 6);
    @print("Test 1: Compressed len = {d}, original len = {d}\n", comp1.len, msg1.len);

    if (comp1.len < 18) {
        @print("FAIL: compressed length too small for Gzip!\n");
        return;
    }
    if (comp1[0] != GZIP_MAGIC_ID1 || comp1[1] != GZIP_MAGIC_ID2) {
        @print("FAIL: missing RFC 1952 magic bytes 0x1f 0x8b!\n");
        return;
    }

    let parsed_hdr = GzipHeader.parse(comp1);
    if (parsed_hdr == null) {
        @print("FAIL: GzipHeader.parse failed!\n");
        return;
    }

    let decomp1 = gunzip(comp1);
    if (decomp1 == null) {
        @print("FAIL: gunzip returned null!\n");
        return;
    }
    let res1 = decomp1.?;
    if (res1.len != msg1.len) {
        @print("FAIL: decompressed length mismatch: {d} vs {d}\n", res1.len, msg1.len);
        return;
    }
    if (res1 != msg1) {
        @print("FAIL: decompressed content mismatch!\n");
        return;
    }
    @print("PASS 1: Standard Gzip & Gunzip roundtrip succeeded.\n");

    // -------------------------------------------------------------------------
    // Test 2: Large Data & Dynamic Buffer Growth in Gunzip
    // -------------------------------------------------------------------------
    // Construct a repeated payload of > 4096 bytes to force dynamic buffer growth
    let chunk = "CP_GZIP_SCALE_TEST_STRING_0123456789_ABCDEF_";
    let large_buf: [4500]u8 = undefined;
    let pos: usize = 0;
    while (pos + chunk.len <= 4500) {
        @memcpy(large_buf[pos .. pos + chunk.len], chunk);
        pos += chunk.len;
    }
    let large_src = large_buf[0..pos];

    let comp_large = gzip(large_src, 9);
    @print("Test 2: Large payload original = {d}, compressed = {d}\n", large_src.len, comp_large.len);

    let decomp_large = gunzip(comp_large);
    if (decomp_large == null) {
        @print("FAIL: large gunzip returned null!\n");
        return;
    }
    let res_large = decomp_large.?;
    if (res_large.len != large_src.len) {
        @print("FAIL: large decompressed length mismatch: {d} vs {d}\n", res_large.len, large_src.len);
        return;
    }
    if (res_large != large_src) {
        @print("FAIL: large decompressed content mismatch!\n");
        return;
    }
    @print("PASS 2: Large payload dynamic buffer growth verified.\n");

    // -------------------------------------------------------------------------
    // Test 3: RFC 1950 Zlib Deflate & Inflate Roundtrip
    // -------------------------------------------------------------------------
    let msg3 = "RFC 1950 Zlib Deflate format with 2-byte header and Adler32 checksum.";
    let comp3 = deflate(msg3, 6);
    let decomp3 = inflate(comp3);
    if (decomp3 == null) {
        @print("FAIL: inflate returned null!\n");
        return;
    }
    if (decomp3.? != msg3) {
        @print("FAIL: inflate content mismatch!\n");
        return;
    }
    @print("PASS 3: RFC 1950 Zlib Deflate & Inflate roundtrip verified.\n");

    // -------------------------------------------------------------------------
    // Test 4: RFC 1951 Raw Deflate & Inflate Raw Roundtrip
    // -------------------------------------------------------------------------
    let msg4 = "RFC 1951 Raw Deflate format without any container headers or trailers.";
    let comp4 = deflateRaw(msg4, 6);
    let decomp4 = inflateRaw(comp4);
    if (decomp4 == null) {
        @print("FAIL: inflateRaw returned null!\n");
        return;
    }
    if (decomp4.? != msg4) {
        @print("FAIL: inflateRaw content mismatch!\n");
        return;
    }
    @print("PASS 4: RFC 1951 Raw Deflate & Inflate Raw roundtrip verified.\n");

    // -------------------------------------------------------------------------
    // Test 5: Object-Oriented APIs (Gzip, Gunzip, DeflateCodec, InflateCodec)
    // -------------------------------------------------------------------------
    let msg5 = "Object-Oriented API testing for Gzip, Gunzip, DeflateCodec and InflateCodec.";
    let comp5 = Gzip.compress(msg5, 6);
    let decomp5 = Gunzip.decompress(comp5);
    if (decomp5 == null || decomp5.? != msg5) {
        @print("FAIL: Gzip / Gunzip OO roundtrip failed!\n");
        return;
    }

    let comp5_def = DeflateCodec.compress(msg5, 6);
    let decomp5_def = InflateCodec.decompress(comp5_def);
    if (decomp5_def == null || decomp5_def.? != msg5) {
        @print("FAIL: DeflateCodec / InflateCodec OO roundtrip failed!\n");
        return;
    }
    @print("PASS 5: Object-Oriented APIs verified.\n");

    // -------------------------------------------------------------------------
    // Test 6: RleCodec Backward Compatibility
    // -------------------------------------------------------------------------
    let rle_input = "AAAAABBBBBCCCCCDDDDDEEEEEFFFFF";
    let rle_comp: [128]u8 = undefined;
    let rle_comp_len = RleCodec.compress(rle_input, &rle_comp);
    let rle_decomp: [128]u8 = undefined;
    let rle_decomp_len = RleCodec.decompress(rle_comp[0..rle_comp_len], &rle_decomp);

    if (rle_decomp[0..rle_decomp_len] != rle_input) {
        @print("FAIL: RleCodec roundtrip mismatch!\n");
        return;
    }
    @print("PASS 6: RleCodec backward compatibility verified.\n");

    @print("\n🎉 ALL GZIP & PACK COMPRESSION TESTS PASSED (100% SUCCESS)!\n");
}
