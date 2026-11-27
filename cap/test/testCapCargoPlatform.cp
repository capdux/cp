import { test, expect_eq } from "cap:test";
import {
    TargetTriple,
    TargetEnv,
    CfgEvalResult,
    parseTargetTriple,
    evalCfgExpr,
} from "../../packages/cargo-platform/src/index.cp";

fn sliceEq(src: string, start: usize, len: usize, expected: string): bool {
    if (len != expected.len || start + len > src.len) {
        return false;
    }
    let i: usize = 0;
    while (i < len) {
        if (src[start + i] != expected[i]) {
            return false;
        }
        i = i + 1;
    }
    return true;
}

fn linuxX64(): TargetEnv {
    return TargetEnv {
        arch: "x86_64",
        os: "linux",
        family: "unix",
        env: "gnu",
        pointer_width: 64,
        is_big_endian: false,
    };
}

fn macArm64(): TargetEnv {
    return TargetEnv {
        arch: "aarch64",
        os: "macos",
        family: "unix",
        env: "",
        pointer_width: 64,
        is_big_endian: false,
    };
}

fn winX64(): TargetEnv {
    return TargetEnv {
        arch: "x86_64",
        os: "windows",
        family: "windows",
        env: "msvc",
        pointer_width: 64,
        is_big_endian: false,
    };
}

fn main() {
    test("1. @cap/cargo-platform — 3-part & 4-part parseTargetTriple", () => {
        let s4: string = "x86_64-unknown-linux-gnu";
        let t4: TargetTriple = parseTargetTriple(s4);
        expect_eq(t4.valid, true);
        expect_eq(t4.has_env, true);
        expect_eq(sliceEq(s4, t4.arch_start, t4.arch_len, "x86_64"), true);
        expect_eq(sliceEq(s4, t4.vendor_start, t4.vendor_len, "unknown"), true);
        expect_eq(sliceEq(s4, t4.os_start, t4.os_len, "linux"), true);
        expect_eq(sliceEq(s4, t4.env_start, t4.env_len, "gnu"), true);

        let s3: string = "aarch64-apple-darwin";
        let t3: TargetTriple = parseTargetTriple(s3);
        expect_eq(t3.valid, true);
        expect_eq(t3.has_env, false);
        expect_eq(sliceEq(s3, t3.arch_start, t3.arch_len, "aarch64"), true);
        expect_eq(sliceEq(s3, t3.vendor_start, t3.vendor_len, "apple"), true);
        expect_eq(sliceEq(s3, t3.os_start, t3.os_len, "darwin"), true);

        expect_eq(parseTargetTriple("x86_64-linux").valid, false);
        expect_eq(parseTargetTriple("x86_64--linux-gnu").valid, false);
    });

    test("2. @cap/cargo-platform — bare flags unix/windows & single key-value", () => {
        let lin: TargetEnv = linuxX64();
        let win: TargetEnv = winX64();

        let r1: CfgEvalResult = evalCfgExpr("unix", lin);
        expect_eq(r1.ok && r1.matched, true);
        let r2: CfgEvalResult = evalCfgExpr("windows", lin);
        expect_eq(r2.ok && !r2.matched, true);
        let r3: CfgEvalResult = evalCfgExpr("windows", win);
        expect_eq(r3.ok && r3.matched, true);

        let r4: CfgEvalResult = evalCfgExpr("target_os = \"linux\"", lin);
        expect_eq(r4.ok && r4.matched, true);
        let r5: CfgEvalResult = evalCfgExpr("target_env = \"msvc\"", win);
        expect_eq(r5.ok && r5.matched, true);
    });

    test("3. @cap/cargo-platform — nested cfg(all(unix, not(target_os = \"macos\")))", () => {
        let expr: string = "cfg(all(unix, not(target_os = \"macos\")))";
        let r_lin: CfgEvalResult = evalCfgExpr(expr, linuxX64());
        expect_eq(r_lin.ok && r_lin.matched, true);

        let r_mac: CfgEvalResult = evalCfgExpr(expr, macArm64());
        expect_eq(r_mac.ok && !r_mac.matched, true);

        let r_win: CfgEvalResult = evalCfgExpr(expr, winX64());
        expect_eq(r_win.ok && !r_win.matched, true);

        let any_expr: string = "cfg(any(target_arch = \"aarch64\", target_os = \"windows\"))";
        expect_eq(evalCfgExpr(any_expr, macArm64()).matched, true);
        expect_eq(evalCfgExpr(any_expr, winX64()).matched, true);
        expect_eq(evalCfgExpr(any_expr, linuxX64()).matched, false);
    });

    test("4. @cap/cargo-platform — target_pointer_width & target_endian", () => {
        let lin: TargetEnv = linuxX64();
        expect_eq(evalCfgExpr("cfg(all(target_pointer_width = \"64\", target_endian = \"little\"))", lin).matched, true);
        expect_eq(evalCfgExpr("target_pointer_width = \"32\"", lin).matched, false);
        expect_eq(evalCfgExpr("target_endian = \"big\"", lin).matched, false);

        let big_t: TargetEnv = TargetEnv {
            arch: "s390x",
            os: "linux",
            family: "unix",
            env: "gnu",
            pointer_width: 64,
            is_big_endian: true,
        };
        expect_eq(evalCfgExpr("target_endian = \"big\"", big_t).matched, true);
    });

    test("5. @cap/cargo-platform — empty all() == true & empty any() == false", () => {
        let lin: TargetEnv = linuxX64();
        let r_all: CfgEvalResult = evalCfgExpr("cfg(all())", lin);
        expect_eq(r_all.ok, true);
        expect_eq(r_all.matched, true);

        let r_any: CfgEvalResult = evalCfgExpr("cfg(any())", lin);
        expect_eq(r_any.ok, true);
        expect_eq(r_any.matched, false);
    });

    test("6. @cap/cargo-platform — unclosed paren, unclosed quote & syntax errors return ok=false", () => {
        let lin: TargetEnv = linuxX64();
        expect_eq(evalCfgExpr("cfg(all(unix", lin).ok, false);
        expect_eq(evalCfgExpr("target_os = \"linux", lin).ok, false);
        expect_eq(evalCfgExpr("unknown_fn(unix)", lin).ok, false);
        expect_eq(evalCfgExpr("unix extra", lin).ok, false);
        expect_eq(evalCfgExpr("", lin).ok, false);
    });
}
