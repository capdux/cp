// cap/test/test_syntax_orthogonality.cp
// Regression suite for CP Orthogonal Modifiers, First-Class JSON/std, @c {}, and TLS Identity Verification

import { test, expect_eq } from "cap:test";
import { isSafeAltName, matchDnsName } from "cap:crypto";

pub @c {
    fn getpid(): i32;
}

pub enum CompressionKind: u8 {
    RawDeflate = 0,
    Zlib = 1,
    Gzip = 2,

    static fn defaultKind(): CompressionKind {
        return .Gzip;
    }

    fn isGzip(): bool {
        return _ == .Gzip;
    }
}

pub struct PacketMeta {
    seq: u32 = 0;
    tag: string = "init";
}

pub class LinearBuf {
    pub items: []const u8 = [];
    pub cursor: usize = 0;

    pub fn init(*_, start: usize) {
        _.cursor = start;
    }

    pub fn isRewound(): bool {
        return _.cursor == 0;
    }

    pub fn step(*_, delta: *usize): usize {
        // delta: *usize is mutated by value -> auto-deduced as local value copy!
        delta += 1;
        _.cursor += delta;
        return _.cursor;
    }
}

pub fn mutatePacketMeta(meta: *PacketMeta) {
    // meta: *PacketMeta mutates fields -> auto-deduced as mutable reference borrow!
    meta.seq += 10;
    meta.tag = "updated";
}

fn main(): void! {
    test("1. Orthogonal receiver (*_ vs omitted _) & v: *T value-copy vs borrow auto-deduction", () => {
        let b = new LinearBuf(5);
        expect_eq(b.isRewound(), false);

        let orig_step: usize = 2;
        let next_pos = b.step(orig_step);
        expect_eq(next_pos, 8 as usize);
        // orig_step in caller must remain 2 because step(delta: *usize) reassigned delta by value
        expect_eq(orig_step, 2 as usize);

        let meta = PacketMeta{ seq: 5, tag: "init" };
        mutatePacketMeta(&meta);
        expect_eq(meta.seq, 15 as u32);
        expect_eq(meta.tag, "updated");
    });

    test("2. Enum default pub methods, static fn, = [] slice literal, and * = expr discard", () => {
        const k = CompressionKind.defaultKind();
        expect_eq(k.isGzip(), true);
        let b = new LinearBuf(0);
        expect_eq(b.items.len, 0 as usize);
        unsafe {
            * = getpid();
        }
    });

    test("3. Built-in first-class JSON (stringify & parse<T>) and pre-registered std without import", () => {
        const meta = PacketMeta{ seq: 42, tag: "hello" };
        const encoded = JSON.stringify(meta);
        const decoded = JSON.parse<PacketMeta>(encoded) catch PacketMeta{ seq: 0, tag: "err" };
        expect_eq(decoded.seq, 42 as u32);
        expect_eq(decoded.tag, "hello");
        expect_eq(std.ascii.eqlIgnoreCase("Content-Type", "content-type"), true);
    });

    test("4. BoringSSL X.509 DNS wildcard & SAN identity verification (cap:crypto)", () => {
        expect_eq(isSafeAltName("*.example.com", false), true);
        expect_eq(isSafeAltName("bad name.com", false), false);
        expect_eq(matchDnsName("*.example.com", "api.example.com"), true);
        expect_eq(matchDnsName("*.example.com", "sub.api.example.com"), false);
        expect_eq(matchDnsName("*.example.com", "example.com"), false);
        expect_eq(matchDnsName("Example.COM", "example.com"), true);
    });

    test("5. Built-in RegExp with [2048]u8 stackFallback & 16-byte SIMD literal prefix fast-skip", () => {
        const re_lit = /https:\/\/api\.example/;
        expect_eq(re_lit.test("GET https://api.example.com/v1 HTTP/1.1"), true);
        expect_eq(re_lit.test("GET http://other.org/v1 HTTP/1.1"), false);

        const re_num = new RegExp("user_[0-9]+", "");
        expect_eq(re_num.test("id=user_98765&active=1"), true);
        expect_eq(re_num.test("id=guest_abc"), false);
        expect_eq(re_num.exec("id=user_42&ok=1"), "user_42");
    });
}
