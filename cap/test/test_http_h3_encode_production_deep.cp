import std from "std";
import { test, expect_eq } from "cap:test";
import {
    H3EncodeError,
    H3InputHeader,
    H3Header,
    H3RequestMeta,
    H3EncodedRequest,
    encodeH3RequestHeaders,
} from "cap:http";

fn expectEncodeErr(
    meta: H3RequestMeta,
    in_hdrs: []const H3InputHeader,
    out_hdrs: []H3Header,
    lower_buf: []u8,
    expected: H3EncodeError,
): bool {
    * = encodeH3RequestHeaders(meta, in_hdrs, out_hdrs, lower_buf) catch |e| {
        return e == expected;
    };
    return false;
}

fn main() {
    test("http_h3_encode: pseudo-headers ordering, Host promotion & default path", () => {
        const in_hdrs: [3]H3InputHeader = [
            H3InputHeader{ name: "Host", value: "override.example.com" },
            H3InputHeader{ name: "Accept", value: "application/json" },
            H3InputHeader{ name: "X-Custom-Trace", value: "abc-123" },
        ];
        let out_hdrs: [16]H3Header = [H3Header{}; 16];
        let lower_buf: [128]u8 = [0; 128];

        const meta = H3RequestMeta{
            method: "POST",
            scheme: "https",
            url_host: "url.example.com",
            session_hostname: "session.example.com",
            path: "",
            has_inline_body: true,
        };
        const res: H3EncodedRequest = encodeH3RequestHeaders(meta, in_hdrs[0..], out_hdrs[0..], lower_buf[0..]) catch unreachable;

        expect_eq(res.header_count, 6 as usize);
        expect_eq(res.dropped_forbidden, 0 as usize);
        expect_eq(res.end_stream, false);
        expect_eq(res.lower_bytes_used, "x-custom-trace".len);

        expect_eq(std.mem.eql(u8, out_hdrs[0].name, ":method"), true);
        expect_eq(std.mem.eql(u8, out_hdrs[0].value, "POST"), true);
        expect_eq(out_hdrs[0].qpack_index, 20 as i16);

        expect_eq(std.mem.eql(u8, out_hdrs[1].name, ":scheme"), true);
        expect_eq(out_hdrs[1].qpack_index, 23 as i16);

        expect_eq(std.mem.eql(u8, out_hdrs[2].name, ":authority"), true);
        expect_eq(std.mem.eql(u8, out_hdrs[2].value, "override.example.com"), true);
        expect_eq(out_hdrs[2].qpack_index, 0 as i16);

        expect_eq(std.mem.eql(u8, out_hdrs[3].name, ":path"), true);
        expect_eq(std.mem.eql(u8, out_hdrs[3].value, "/"), true);
        expect_eq(out_hdrs[3].qpack_index, 1 as i16);

        expect_eq(std.mem.eql(u8, out_hdrs[4].name, "accept"), true);
        expect_eq(out_hdrs[4].isIndexed(), true);
        expect_eq(out_hdrs[4].qpack_index, 29 as i16);

        expect_eq(std.mem.eql(u8, out_hdrs[5].name, "x-custom-trace"), true);
        expect_eq(out_hdrs[5].isIndexed(), false);
    });

    test("http_h3_encode: filters RFC 9114 forbidden hop-by-hop headers & allows te: trailers", () => {
        const in_hdrs: [7]H3InputHeader = [
            H3InputHeader{ name: "Connection", value: "keep-alive" },
            H3InputHeader{ name: "Keep-Alive", value: "timeout=5" },
            H3InputHeader{ name: "Proxy-Connection", value: "keep-alive" },
            H3InputHeader{ name: "Transfer-Encoding", value: "chunked" },
            H3InputHeader{ name: "Upgrade", value: "websocket" },
            H3InputHeader{ name: "TE", value: "gzip" },
            H3InputHeader{ name: "TE", value: " trailers " },
        ];
        let out_hdrs: [16]H3Header = [H3Header{}; 16];
        let lower_buf: [64]u8 = [0; 64];

        const meta = H3RequestMeta{
            url_host: "api.example.com",
            path: "/v1/items",
        };
        const res: H3EncodedRequest = encodeH3RequestHeaders(meta, in_hdrs[0..], out_hdrs[0..], lower_buf[0..]) catch unreachable;

        expect_eq(res.dropped_forbidden, 6 as usize);
        expect_eq(res.header_count, 5 as usize);
        expect_eq(res.end_stream, true);
        expect_eq(std.mem.eql(u8, out_hdrs[4].name, "te"), true);
    });

    test("http_h3_encode: authority fallback chain url_host -> session_hostname -> MissingAuthority", () => {
        const dummy: [1]H3InputHeader = [H3InputHeader{ name: "Accept", value: "*/*" }];
        let out_hdrs: [8]H3Header = [H3Header{}; 8];
        let lower_buf: [32]u8 = [0; 32];

        const res1: H3EncodedRequest = encodeH3RequestHeaders(
            H3RequestMeta{ session_hostname: "fallback.local", is_streaming: true },
            dummy[0..0],
            out_hdrs[0..],
            lower_buf[0..],
        ) catch unreachable;
        expect_eq(std.mem.eql(u8, out_hdrs[2].value, "fallback.local"), true);
        expect_eq(res1.end_stream, false);

        expect_eq(expectEncodeErr(
            H3RequestMeta{},
            dummy[0..0],
            out_hdrs[0..],
            lower_buf[0..],
            H3EncodeError.MissingAuthority,
        ), true);
    });

    test("http_h3_encode: rejects empty or pseudo header names in input", () => {
        let out_hdrs: [8]H3Header = [H3Header{}; 8];
        let lower_buf: [32]u8 = [0; 32];
        const meta = H3RequestMeta{ url_host: "example.com" };

        const bad_pseudo: [1]H3InputHeader = [H3InputHeader{ name: ":path", value: "/evil" }];
        expect_eq(expectEncodeErr(meta, bad_pseudo[0..], out_hdrs[0..], lower_buf[0..], H3EncodeError.InvalidHeaderName), true);

        const bad_empty: [1]H3InputHeader = [H3InputHeader{ name: "", value: "x" }];
        expect_eq(expectEncodeErr(meta, bad_empty[0..], out_hdrs[0..], lower_buf[0..], H3EncodeError.InvalidHeaderName), true);
    });

    test("http_h3_encode: buffer & header slot exhaustion protection", () => {
        const in_hdrs: [1]H3InputHeader = [H3InputHeader{ name: "X-Very-Long-Custom-Header", value: "1" }];
        const meta = H3RequestMeta{ url_host: "example.com" };

        let tiny_hdrs: [3]H3Header = [H3Header{}; 3];
        let lower_buf: [64]u8 = [0; 64];
        expect_eq(expectEncodeErr(meta, in_hdrs[0..], tiny_hdrs[0..], lower_buf[0..], H3EncodeError.HeaderSlotsExhausted), true);

        let ok_hdrs: [8]H3Header = [H3Header{}; 8];
        let tiny_lower: [4]u8 = [0; 4];
        expect_eq(expectEncodeErr(meta, in_hdrs[0..], ok_hdrs[0..], tiny_lower[0..], H3EncodeError.LowerBufferTooSmall), true);
    });
}
