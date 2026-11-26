// cap/test/testCapTargetTriple.cp
// Comprehensive unit tests for @cap/target-triple

import { test, expect_eq } from "cap:test";
import {
    ArchKind,
    OsKind,
    EnvKind,
    TargetTriple,
    parseTargetTriple,
    exeSuffix,
    dylibSuffix,
} from "../../packages/@cap/target-triple/src/index.cp";

fn main() {
    test("1. @cap/target-triple — 4-part Linux & Windows triples", () => {
        const lin: TargetTriple = parseTargetTriple("x86_64-unknown-linux-gnu");
        expect_eq(lin.valid, true);
        expect_eq(lin.arch, ArchKind.X86_64);
        expect_eq(lin.os, OsKind.Linux);
        expect_eq(lin.env, EnvKind.Gnu);
        expect_eq(lin.pointer_width, 64 as u8);
        expect_eq(lin.is_little_endian, true);
        expect_eq(exeSuffix(lin), "");
        expect_eq(dylibSuffix(lin), ".so");

        const win: TargetTriple = parseTargetTriple("x86_64-pc-windows-msvc");
        expect_eq(win.valid, true);
        expect_eq(win.os, OsKind.Windows);
        expect_eq(win.env, EnvKind.Msvc);
        expect_eq(exeSuffix(win), ".exe");
        expect_eq(dylibSuffix(win), ".dll");
    });

    test("2. @cap/target-triple — 3-part Apple Darwin triple", () => {
        const mac: TargetTriple = parseTargetTriple("aarch64-apple-darwin");
        expect_eq(mac.valid, true);
        expect_eq(mac.arch, ArchKind.Aarch64);
        expect_eq(mac.raw_vendor, "apple");
        expect_eq(mac.os, OsKind.Darwin);
        expect_eq(mac.pointer_width, 64 as u8);
        expect_eq(exeSuffix(mac), "");
        expect_eq(dylibSuffix(mac), ".dylib");
    });

    test("3. @cap/target-triple — Zig-style 3-part arch-os-abi triple", () => {
        const musl: TargetTriple = parseTargetTriple("x86_64-linux-musl");
        expect_eq(musl.valid, true);
        expect_eq(musl.arch, ArchKind.X86_64);
        expect_eq(musl.os, OsKind.Linux);
        expect_eq(musl.env, EnvKind.Musl);
        expect_eq(musl.raw_vendor, "unknown");
    });

    test("4. @cap/target-triple — 2-part & 3-part WebAssembly triples", () => {
        const wasi: TargetTriple = parseTargetTriple("wasm32-wasi");
        expect_eq(wasi.valid, true);
        expect_eq(wasi.arch, ArchKind.Wasm32);
        expect_eq(wasi.os, OsKind.Wasi);
        expect_eq(wasi.pointer_width, 32 as u8);
        expect_eq(exeSuffix(wasi), ".wasm");

        const bare: TargetTriple = parseTargetTriple("wasm32-unknown-unknown");
        expect_eq(bare.valid, true);
        expect_eq(bare.os, OsKind.None);
        expect_eq(exeSuffix(bare), ".wasm");
    });

    test("5. @cap/target-triple — big-endian architectures", () => {
        const s390: TargetTriple = parseTargetTriple("s390x-unknown-linux-gnu");
        expect_eq(s390.valid, true);
        expect_eq(s390.arch, ArchKind.S390x);
        expect_eq(s390.is_little_endian, false);
        expect_eq(s390.pointer_width, 64 as u8);

        const ppcLe: TargetTriple = parseTargetTriple("powerpc64le-unknown-linux-gnu");
        expect_eq(ppcLe.is_little_endian, true);
    });

    test("6. @cap/target-triple — invalid triples rejected", () => {
        expect_eq(parseTargetTriple("").valid, false);
        expect_eq(parseTargetTriple("x86_64").valid, false);
        expect_eq(parseTargetTriple("x86_64--linux").valid, false);
        expect_eq(parseTargetTriple("a-b-c-d-e").valid, false);
    });
}
