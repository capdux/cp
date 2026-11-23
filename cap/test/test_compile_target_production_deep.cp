// Source: cp/cap/test/test_compile_target_production_deep.cp (Production deep tests for cap:compile_target)

import { test, expect_eq } from "cap:test";
import {
    TargetOs,
    TargetArch,
    TargetLibc,
    CompileTarget,
} from "cap:cli";

test("compile_target: platform aliases, prefixes and canonical formatting") {
    let buf: [64]u8 = [0; 64];

    const t1 = CompileTarget.parse("bun-macos-aarch64").?;
    expect_eq(t1.os == .darwin, true);
    expect_eq(t1.arch == .arm64, true);
    expect_eq(t1.libc == .default, true);
    expect_eq(t1.baseline, false);
    expect_eq(t1.isSupported(), true);
    expect_eq(std.mem.eql(u8, t1.toCanonical(buf[0..]).?, "darwin-arm64"), true);

    const t2 = CompileTarget.parse("cap-win32-amd64-baseline").?;
    expect_eq(t2.os == .windows, true);
    expect_eq(t2.arch == .x64, true);
    expect_eq(t2.baseline, true);
    expect_eq(t2.isSupported(), true);
    expect_eq(std.mem.eql(u8, t2.toCanonical(buf[0..]).?, "windows-x64-baseline"), true);

    const t3 = CompileTarget.parse("LINUX-X86_64-MUSL-BASELINE").?;
    expect_eq(t3.os == .linux, true);
    expect_eq(t3.arch == .x64, true);
    expect_eq(t3.libc == .musl, true);
    expect_eq(t3.baseline, true);
    expect_eq(t3.isSupported(), true);
    expect_eq(std.mem.eql(u8, t3.toCanonical(buf[0..]).?, "linux-x64-musl-baseline"), true);

    const t4 = CompileTarget.parse("osx-x64-modern").?;
    expect_eq(t4.os == .darwin, true);
    expect_eq(t4.arch == .x64, true);
    expect_eq(t4.baseline, false);
    expect_eq(t4.isSupported(), true);

    const t5 = CompileTarget.parse("win-ia32").?;
    expect_eq(t5.os == .windows, true);
    expect_eq(t5.arch == .x86, true);
    expect_eq(t5.isSupported(), true);
}

test("compile_target: Zig triple generation across all supported OS/Arch/Libc") {
    let buf: [64]u8 = [0; 64];

    const linux_musl = CompileTarget.parse("linux-x64-musl-baseline").?;
    expect_eq(std.mem.eql(u8, linux_musl.toZigTriple(buf[0..]).?, "x86_64-linux-musl"), true);

    const linux_default = CompileTarget.parse("linux-arm64").?;
    expect_eq(std.mem.eql(u8, linux_default.toZigTriple(buf[0..]).?, "aarch64-linux-gnu"), true);

    const linux_android = CompileTarget.parse("linux-arm64-android").?;
    expect_eq(linux_android.isSupported(), true);
    expect_eq(std.mem.eql(u8, linux_android.toZigTriple(buf[0..]).?, "aarch64-linux-android"), true);

    const darwin_arm = CompileTarget.parse("darwin-arm64").?;
    expect_eq(std.mem.eql(u8, darwin_arm.toZigTriple(buf[0..]).?, "aarch64-macos"), true);

    const win_x64 = CompileTarget.parse("windows-x64").?;
    expect_eq(std.mem.eql(u8, win_x64.toZigTriple(buf[0..]).?, "x86_64-windows-msvc"), true);

    const freebsd_x64 = CompileTarget.parse("freebsd-x64").?;
    expect_eq(freebsd_x64.isSupported(), true);
    expect_eq(std.mem.eql(u8, freebsd_x64.toZigTriple(buf[0..]).?, "x86_64-freebsd"), true);

    const wasm_t = CompileTarget.parse("wasm32-wasi").?;
    expect_eq(wasm_t.isSupported(), true);
    expect_eq(std.mem.eql(u8, wasm_t.toZigTriple(buf[0..]).?, "wasm32-wasi"), true);
}

test("compile_target: isSupported validation matrix rejects invalid combinations") {
    // darwin-musl / darwin-glibc 均非法
    const darwin_musl = CompileTarget.parse("darwin-x64-musl").?;
    expect_eq(darwin_musl.isSupported(), false);

    const darwin_glibc = CompileTarget.parse("darwin-arm64-glibc").?;
    expect_eq(darwin_glibc.isSupported(), false);

    // arm64-baseline / x86-baseline 均非法（baseline 仅限 x64）
    const arm64_base = CompileTarget.parse("linux-arm64-baseline").?;
    expect_eq(arm64_base.isSupported(), false);

    const darwin_arm_base = CompileTarget.parse("darwin-arm64-baseline").?;
    expect_eq(darwin_arm_base.isSupported(), false);

    // windows-musl / linux-msvc 均非法
    const win_musl = CompileTarget.parse("windows-x64-musl").?;
    expect_eq(win_musl.isSupported(), false);

    const linux_msvc = CompileTarget.parse("linux-x64-msvc").?;
    expect_eq(linux_msvc.isSupported(), false);

    // wasm + x64 架构不匹配非法
    const wasm_x64 = CompileTarget.parse("wasm-x64").?;
    expect_eq(wasm_x64.isSupported(), false);
}

test("compile_target: exeSuffix and equality") {
    const win = CompileTarget.parse("windows-x64").?;
    const linux = CompileTarget.parse("linux-x64").?;
    const mac = CompileTarget.parse("darwin-arm64").?;
    const mac_alias = CompileTarget.parse("cap-macos-aarch64").?;

    expect_eq(std.mem.eql(u8, win.exeSuffix(), ".exe"), true);
    expect_eq(std.mem.eql(u8, linux.exeSuffix(), ""), true);
    expect_eq(std.mem.eql(u8, mac.exeSuffix(), ""), true);
    expect_eq(mac.eql(mac_alias), true);
    expect_eq(mac.eql(linux), false);
}

test("compile_target: malformed inputs and small output buffer guards") {
    expect_eq(CompileTarget.parse("") == null, true);
    expect_eq(CompileTarget.parse("   ") == null, true);
    expect_eq(CompileTarget.parse("bun-") == null, true);
    expect_eq(CompileTarget.parse("cap-") == null, true);
    expect_eq(CompileTarget.parse("linux") == null, true);
    expect_eq(CompileTarget.parse("x64") == null, true);
    expect_eq(CompileTarget.parse("linux--x64") == null, true);
    expect_eq(CompileTarget.parse("linux-x64-") == null, true);
    expect_eq(CompileTarget.parse("linux-darwin-x64") == null, true);
    expect_eq(CompileTarget.parse("linux-x64-arm64") == null, true);
    expect_eq(CompileTarget.parse("linux-x64-unknown") == null, true);

    const t = CompileTarget.parse("linux-x64-musl-baseline").?;
    let tiny: [4]u8 = [0; 4];
    expect_eq(t.toCanonical(tiny[0..]) == null, true);
    expect_eq(t.toZigTriple(tiny[0..]) == null, true);
}
