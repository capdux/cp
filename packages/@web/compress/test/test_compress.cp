// Regression Test Suite for `@web/compress`
import { App, AppMethod, Context, Request, Response } from "../../../../lib/web/app.cp";
import {
    CompressConfig,
    EncodingKind,
    negotiateEncoding,
    encodeGzipStored,
    registerCompress,
} from "../src/index.cp";

fn strEql(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (a[i] != b[i]) return false;
    }
    return true;
}

fn hSmall(ctx: *Context): Response {
    * = ctx;
    return Response.text(200, "tiny");
}

fn hLarge(ctx: *Context): Response {
    * = ctx;
    return Response.text(
        200,
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA" ++
        "BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB" ++
        "CCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCCC" ++
        "DDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDDD" ++
        "EEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE",
    );
}

fn main() {
    // 1. Accept-Encoding negotiation & q=0 rejection
    if (negotiateEncoding("gzip, deflate", true) != EncodingKind.Gzip) {
        @panic("Test 1a failed: expected Gzip");
    }
    if (negotiateEncoding("gzip, deflate", false) != EncodingKind.Deflate) {
        @panic("Test 1b failed: expected Deflate when prefer_gzip=false");
    }
    if (negotiateEncoding("gzip;q=0, deflate;q=0.8", true) != EncodingKind.Deflate) {
        @panic("Test 1c failed: gzip q=0 should fall back to deflate");
    }
    if (negotiateEncoding("gzip;q=0, *;q=0", true) != EncodingKind.None) {
        @panic("Test 1d failed: all encodings q=0 should return None");
    }
    if (negotiateEncoding("deflate;q=0.5, gzip;q=0.9", false) != EncodingKind.Gzip) {
        @panic("Test 1e failed: higher q-value should win");
    }

    // 2. RFC 1952 Gzip stored-block encoding verification ("hello")
    // Known IEEE CRC32("hello") = 0x3610A686 -> LE bytes: 0x86, 0xA6, 0x10, 0x36
    let gz_buf: [64]u8 = [0; 64];
    const gz_len = encodeGzipStored("hello", gz_buf[0..]) ?? 0;
    if (gz_len == 0) @panic("Test 2a failed: encodeGzipStored returned null");
    // 10B header + 5B stored block hdr + 5B data + 8B trailer = 28 bytes
    if (gz_len != 28) @panic("Test 2b failed: unexpected gzip frame length");
    if (gz_buf[0] != 0x1F || gz_buf[1] != 0x8B || gz_buf[2] != 0x08) {
        @panic("Test 2c failed: invalid gzip magic/method");
    }
    // Stored block header: BFINAL=1, LEN=5, NLEN=0xFFFA
    if (gz_buf[10] != 0x01 || gz_buf[11] != 0x05 || gz_buf[12] != 0x00 || gz_buf[13] != 0xFA || gz_buf[14] != 0xFF) {
        @panic("Test 2d failed: invalid RFC 1951 stored block header");
    }
    // Trailer CRC32 = 0x3610A686 (LE) & ISIZE = 5 (LE)
    if (gz_buf[20] != 0x86 || gz_buf[21] != 0xA6 || gz_buf[22] != 0x10 || gz_buf[23] != 0x36) {
        @panic("Test 2e failed: IEEE CRC32 mismatch");
    }
    if (gz_buf[24] != 0x05 || gz_buf[25] != 0x00 || gz_buf[26] != 0x00 || gz_buf[27] != 0x00) {
        @panic("Test 2f failed: ISIZE mismatch");
    }

    // 3. App middleware integration
    let app = App.init();
    * = registerCompress(*app, CompressConfig{ min_bytes: 256, prefer_gzip: true });
    * = app.get("/small", hSmall);
    * = app.get("/large", hLarge);

    // 3a. Small response skipped
    let s_req = Request.init(AppMethod.GET, "/small");
    * = s_req.withHeader("Accept-Encoding", "gzip");
    const s_resp = app.dispatch(s_req);
    if (s_resp.status != 200) @panic("Test 3a failed: status mismatch");
    if (s_resp.header("Content-Encoding") != null) @panic("Test 3b failed: small body should not be compressed");
    if (!strEql(s_resp.body, "tiny")) @panic("Test 3c failed: small body altered");

    // 3b. Large response compressed with gzip + Vary header
    let l_req = Request.init(AppMethod.GET, "/large");
    * = l_req.withHeader("Accept-Encoding", "gzip, deflate");
    const l_resp = app.dispatch(l_req);
    if (l_resp.status != 200) @panic("Test 4a failed: status mismatch");
    if (!strEql(l_resp.header("Content-Encoding") ?? "", "gzip")) {
        @panic("Test 4b failed: Content-Encoding: gzip missing");
    }
    if (!strEql(l_resp.header("Vary") ?? "", "Accept-Encoding")) {
        @panic("Test 4c failed: Vary: Accept-Encoding missing");
    }
    if (l_resp.body.len < 20 || l_resp.body[0] != 0x1F || l_resp.body[1] != 0x8B) {
        @panic("Test 4d failed: response body is not a valid gzip frame");
    }

    // 3c. Client rejects gzip (q=0) -> uncompressed
    let ng_req = Request.init(AppMethod.GET, "/large");
    * = ng_req.withHeader("Accept-Encoding", "gzip;q=0");
    const ng_resp = app.dispatch(ng_req);
    if (ng_resp.header("Content-Encoding") != null) {
        @panic("Test 5 failed: gzip;q=0 should not be compressed");
    }

    @print("PASS @web/compress (5/5 tests)");
}
