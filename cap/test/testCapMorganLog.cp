import { test, expect_eq } from "cap:test";
import {
    MORGAN_TINY,
    MORGAN_DEV,
    MORGAN_SHORT,
    MORGAN_COMMON,
    MORGAN_COMBINED,
    HttpLogEvent,
    formatMorganLog,
    formatPreset,
} from "../../packages/morgan/src/index.cp";

fn main() {
    test("1. @cap/morgan-log — tiny & dev presets with microsecond response time", () => {
        const ev = HttpLogEvent {
            remote_addr: "127.0.0.1",
            remote_user: "alice",
            date: "27/Nov/2026:05:30:00 +0000",
            method: "GET",
            url: "/api/v1/users",
            http_version: "1.1",
            status: 200,
            content_length: 1024,
            has_content_length: true,
            referrer: "https://cap.dev/",
            user_agent: "CapClient/1.0",
            response_time_us: 12345,
        };
        let out: [256]u8 = undefined;

        const n1 = formatPreset("tiny", ev, out[0..256]);
        expect_eq(out[0..n1], "GET /api/v1/users 200 1024 - 12.345 ms");

        const n2 = formatPreset("dev", ev, out[0..256]);
        expect_eq(out[0..n2], "GET /api/v1/users 200 12.345 ms - 1024");
    });

    test("2. @cap/morgan-log — combined preset (Apache Combined Log Format)", () => {
        const ev = HttpLogEvent {
            remote_addr: "10.0.0.5",
            remote_user: "bob",
            date: "27/Nov/2026:05:30:00 +0000",
            method: "POST",
            url: "/login",
            http_version: "2.0",
            status: 201,
            content_length: 88,
            has_content_length: true,
            referrer: "https://app.example.com/",
            user_agent: "Mozilla/5.0",
            response_time_us: 4500,
        };
        let out: [256]u8 = undefined;

        const n = formatPreset("combined", ev, out[0..256]);
        expect_eq(
            out[0..n],
            "10.0.0.5 - bob [27/Nov/2026:05:30:00 +0000] \"POST /login HTTP/2.0\" 201 88 \"https://app.example.com/\" \"Mozilla/5.0\"",
        );
    });

    test("3. @cap/morgan-log — common & short presets", () => {
        const ev = HttpLogEvent {
            remote_addr: "::1",
            remote_user: "",
            date: "27/Nov/2026:05:35:00 +0000",
            method: "DELETE",
            url: "/items/7",
            http_version: "1.1",
            status: 204,
            content_length: 0,
            has_content_length: false,
            referrer: "",
            user_agent: "",
            response_time_us: 705,
        };
        let out: [256]u8 = undefined;

        const n1 = formatPreset("common", ev, out[0..256]);
        expect_eq(
            out[0..n1],
            "::1 - - [27/Nov/2026:05:35:00 +0000] \"DELETE /items/7 HTTP/1.1\" 204 -",
        );

        const n2 = formatPreset("short", ev, out[0..256]);
        expect_eq(
            out[0..n2],
            "::1 - DELETE /items/7 HTTP/1.1 204 - - 0.705 ms",
        );
    });

    test("4. @cap/morgan-log — Missing fields render as Apache '-' placeholder", () => {
        const ev = HttpLogEvent {
            remote_addr: "",
            remote_user: "",
            date: "",
            method: "",
            url: "",
            http_version: "",
            status: 0,
            content_length: 0,
            has_content_length: false,
            referrer: "",
            user_agent: "",
            response_time_us: 0,
        };
        let out: [256]u8 = undefined;

        const n = formatMorganLog(MORGAN_COMBINED, ev, out[0..256]);
        expect_eq(out[0..n], "- - - [-] \"- - HTTP/-\" - - \"-\" \"-\"");
    });

    test("5. @cap/morgan-log — Response time zero-padding edge cases (0us, 7us, 50us, 1005us)", () => {
        let ev = HttpLogEvent {
            remote_addr: "127.0.0.1",
            remote_user: "",
            date: "",
            method: "GET",
            url: "/",
            http_version: "1.1",
            status: 200,
            content_length: 0,
            has_content_length: true,
            referrer: "",
            user_agent: "",
            response_time_us: 7,
        };
        let out: [64]u8 = undefined;

        const n1 = formatMorganLog(":response-time ms", ev, out[0..64]);
        expect_eq(out[0..n1], "0.007 ms");

        ev.response_time_us = 50;
        const n2 = formatMorganLog(":response-time ms", ev, out[0..64]);
        expect_eq(out[0..n2], "0.050 ms");

        ev.response_time_us = 1005;
        const n3 = formatMorganLog(":response-time ms", ev, out[0..64]);
        expect_eq(out[0..n3], "1.005 ms");
    });

    test("6. @cap/morgan-log — Custom format string & buffer truncation safety", () => {
        const ev = HttpLogEvent {
            remote_addr: "192.168.1.10",
            remote_user: "",
            date: "",
            method: "PUT",
            url: "/upload",
            http_version: "1.1",
            status: 202,
            content_length: 4096,
            has_content_length: true,
            referrer: "",
            user_agent: "curl/8.0",
            response_time_us: 250000,
        };
        let out: [128]u8 = undefined;

        const n1 = formatPreset("[:method] :url -> status=:status bytes=:res[content-length] (:response-time ms)", ev, out[0..128]);
        expect_eq(out[0..n1], "[PUT] /upload -> status=202 bytes=4096 (250.000 ms)");

        // Truncated small buffer never overflows
        let tiny_buf: [10]u8 = undefined;
        const n2 = formatMorganLog(MORGAN_TINY, ev, tiny_buf[0..10]);
        expect_eq(n2, 10);
        expect_eq(tiny_buf[0..10], "PUT /uploa");
    });
}
