import { test, expect_eq } from "cap:test";
import {
    STATUS_OK,
    STATUS_RESOURCE_EXHAUSTED,
    STATUS_ABORTED,
    STATUS_UNAVAILABLE,
    STATUS_UNAUTHENTICATED,
    GRPC_OK,
    GRPC_INCOMPLETE,
    GRPC_INVALID,
    GrpcFrameHeader,
    statusName,
    isRetryableStatus,
    encodeFrame,
    decodeHeader,
    parseTimeoutMicros,
    formatTimeoutMillis,
} from "../../packages/tonic/src/index.cp";

fn testEq(a: []const u8, b: []const u8) -> bool {
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

fn main() {
    test("1. @cap/grpc-frame — encodeFrame & decodeHeader uncompressed + compressed roundtrip", () => {
        const payload = "protobuf-bytes-payload";
        let buf: [64]u8 = undefined;

        const n1 = encodeFrame(false, payload, buf[0..64]);
        expect_eq(n1, payload.len + 5);
        let h = GrpcFrameHeader { compressed: false, payload_len: 0, total_len: 0 };
        expect_eq(decodeHeader(buf[0..n1], &h), GRPC_OK);
        expect_eq(h.compressed, false);
        expect_eq(h.payload_len, payload.len as u32);
        expect_eq(h.total_len, n1);
        expect_eq(testEq(buf[5..h.total_len], payload), true);

        const n2 = encodeFrame(true, payload, buf[0..64]);
        expect_eq(decodeHeader(buf[0..n2], &h), GRPC_OK);
        expect_eq(h.compressed, true);
        expect_eq(h.payload_len, payload.len as u32);
    });

    test("2. @cap/grpc-frame — Partial frame GRPC_INCOMPLETE & invalid flag=2 GRPC_INVALID", () => {
        let h = GrpcFrameHeader { compressed: false, payload_len: 0, total_len: 0 };
        const short_hdr: [3]u8 = [0, 0, 0];
        expect_eq(decodeHeader(short_hdr[0..3], &h), GRPC_INCOMPLETE);

        // Header declares 10 bytes but only 2 payload bytes present
        const partial: [7]u8 = [0, 0, 0, 0, 10, 0xAA, 0xBB];
        expect_eq(decodeHeader(partial[0..7], &h), GRPC_INCOMPLETE);
        expect_eq(h.payload_len, 10 as u32);
        expect_eq(h.total_len, 15 as usize);

        // Invalid compression flag = 2
        const bad_flag: [5]u8 = [2, 0, 0, 0, 0];
        expect_eq(decodeHeader(bad_flag[0..5], &h), GRPC_INVALID);
    });

    test("3. @cap/grpc-frame — All 17 statusName & isRetryableStatus", () => {
        expect_eq(testEq(statusName(STATUS_OK), "OK"), true);
        expect_eq(testEq(statusName(1), "CANCELLED"), true);
        expect_eq(testEq(statusName(4), "DEADLINE_EXCEEDED"), true);
        expect_eq(testEq(statusName(12), "UNIMPLEMENTED"), true);
        expect_eq(testEq(statusName(STATUS_UNAVAILABLE), "UNAVAILABLE"), true);
        expect_eq(testEq(statusName(STATUS_UNAUTHENTICATED), "UNAUTHENTICATED"), true);
        expect_eq(testEq(statusName(17), ""), true);

        expect_eq(isRetryableStatus(STATUS_UNAVAILABLE), true);
        expect_eq(isRetryableStatus(STATUS_RESOURCE_EXHAUSTED), true);
        expect_eq(isRetryableStatus(STATUS_ABORTED), true);
        expect_eq(isRetryableStatus(STATUS_OK), false);
        expect_eq(isRetryableStatus(STATUS_UNAUTHENTICATED), false);
    });

    test("4. @cap/grpc-frame — parseTimeoutMicros across all 6 units H/M/S/m/u/n", () => {
        expect_eq(parseTimeoutMicros("1H"), 3600000000 as i64);
        expect_eq(parseTimeoutMicros("2M"), 120000000 as i64);
        expect_eq(parseTimeoutMicros("2S"), 2000000 as i64);
        expect_eq(parseTimeoutMicros("500m"), 500000 as i64);
        expect_eq(parseTimeoutMicros("250u"), 250 as i64);
        expect_eq(parseTimeoutMicros("5000n"), 5 as i64);
    });

    test("5. @cap/grpc-frame — formatTimeoutMillis & 8-digit unit promotion", () => {
        let buf: [16]u8 = undefined;
        const n1 = formatTimeoutMillis(500, buf[0..16]);
        expect_eq(testEq(buf[0..n1], "500m"), true);

        const n2 = formatTimeoutMillis(0, buf[0..16]);
        expect_eq(testEq(buf[0..n2], "0m"), true);

        // Exceeds 8-digit ms (> 99,999,999 ms) -> promoted to seconds 'S'
        const n3 = formatTimeoutMillis(150000000, buf[0..16]);
        expect_eq(testEq(buf[0..n3], "150000S"), true);
    });

    test("6. @cap/grpc-frame — Invalid timeouts & undersized buffer guards", () => {
        expect_eq(parseTimeoutMicros(""), -1 as i64);
        expect_eq(parseTimeoutMicros("S"), -1 as i64);
        expect_eq(parseTimeoutMicros("100x"), -1 as i64);
        expect_eq(parseTimeoutMicros("12aS"), -1 as i64);
        expect_eq(parseTimeoutMicros("123456789m"), -1 as i64); // > 8 digits

        let tiny: [3]u8 = undefined;
        expect_eq(encodeFrame(false, "hello", tiny[0..3]), 0 as usize);
        expect_eq(formatTimeoutMillis(12345, tiny[0..3]), 0 as usize);
    });
}
