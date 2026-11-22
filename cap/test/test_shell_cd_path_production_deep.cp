import { test, expect, expect_eq } from "cap:test";
import {
    SHELL_CD_MAX_PATH,
    SHELL_CD_MAX_SEGMENTS,
    ShellCdError,
    CdTargetKind,
    CdTarget,
    normalizePosixPath,
    resolveCdTarget,
    formatCdError,
    ShellDirState,
} from "cap:shell";

fn strEql(a: []const u8, b: []const u8): bool {
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

test("shell cd_path: CdTarget.parseArgs handles 0 args, ~, ~/sub, -, path, and too many args", fn()! {
    expect_eq(SHELL_CD_MAX_PATH, 1024);
    expect_eq(SHELL_CD_MAX_SEGMENTS, 64);

    const empty_args: [0][]const u8 = [];
    const t0 = CdTarget.parseArgs(empty_args[0..])?;
    expect(t0.kind == .Home);
    expect(strEql(t0.raw_path, ""));

    const tilde_args: [1][]const u8 = ["~"];
    const t1 = CdTarget.parseArgs(tilde_args[0..])?;
    expect(t1.kind == .Home);
    expect(strEql(t1.raw_path, ""));

    const tilde_sub: [1][]const u8 = ["~/projects/cp"];
    const t2 = CdTarget.parseArgs(tilde_sub[0..])?;
    expect(t2.kind == .Home);
    expect(strEql(t2.raw_path, "projects/cp"));

    const dash_args: [1][]const u8 = ["-"];
    const t3 = CdTarget.parseArgs(dash_args[0..])?;
    expect(t3.kind == .Previous);

    const path_args: [1][]const u8 = ["../lib"];
    const t4 = CdTarget.parseArgs(path_args[0..])?;
    expect(t4.kind == .Path);
    expect(strEql(t4.raw_path, "../lib"));

    const two_args: [2][]const u8 = ["a", "b"];
    let hit_too_many = false;
    if (_ := CdTarget.parseArgs(two_args[0..])) {} else { hit_too_many = true; }
    expect(hit_too_many);
});

test("shell cd_path: normalizePosixPath collapses slashes, dots, and clamps root", fn()! {
    let buf: [128]u8 = [0; 128];

    const n1 = normalizePosixPath("///usr//local/./bin/../lib/", buf[0..])?;
    expect(strEql(buf[0..n1], "/usr/local/lib"));

    const n2 = normalizePosixPath("/../../../etc/hosts", buf[0..])?;
    expect(strEql(buf[0..n2], "/etc/hosts"));

    const n3 = normalizePosixPath("/../..", buf[0..])?;
    expect(strEql(buf[0..n3], "/"));

    const n4 = normalizePosixPath("a/b/../../../c/./d", buf[0..])?;
    expect(strEql(buf[0..n4], "../c/d"));

    const n5 = normalizePosixPath("./a/..", buf[0..])?;
    expect(strEql(buf[0..n5], "."));

    const n6 = normalizePosixPath("", buf[0..])?;
    expect(strEql(buf[0..n6], "."));
});

test("shell cd_path: resolveCdTarget handles Home, Previous, Path, and missing env guards", fn()! {
    let buf: [128]u8 = [0; 128];

    const n_home = resolveCdTarget("/tmp", "/var", "/home/alice", CdTarget{ kind: .Home, raw_path: "dev/../docs" }, buf[0..])?;
    expect(strEql(buf[0..n_home], "/home/alice/docs"));

    const n_prev = resolveCdTarget("/tmp", "/var/log/", "/home/alice", CdTarget{ kind: .Previous, raw_path: "-" }, buf[0..])?;
    expect(strEql(buf[0..n_prev], "/var/log"));

    const n_abs = resolveCdTarget("/tmp", "", "/home/alice", CdTarget{ kind: .Path, raw_path: "/opt//cp/" }, buf[0..])?;
    expect(strEql(buf[0..n_abs], "/opt/cp"));

    const n_rel = resolveCdTarget("/workspace/app", "", "/home/alice", CdTarget{ kind: .Path, raw_path: "../lib/./shell" }, buf[0..])?;
    expect(strEql(buf[0..n_rel], "/workspace/lib/shell"));

    // Empty HOME / empty OLDPWD guards
    let errs: usize = 0;
    if (_ := resolveCdTarget("/tmp", "", "", CdTarget{ kind: .Home, raw_path: "" }, buf[0..])) {} else { errs += 1; }
    if (_ := resolveCdTarget("/tmp", "", "/home/alice", CdTarget{ kind: .Previous, raw_path: "-" }, buf[0..])) {} else { errs += 1; }
    expect_eq(errs, 2);
});

test("shell cd_path: ShellDirState multi-step navigation and cd - toggle", fn()! {
    let st = ShellDirState.init("/workspace/app/")?;
    expect(strEql(st.cwd(), "/workspace/app"));
    expect(strEql(st.prevCwd(), ""));

    // cd ../shared/./lib
    const a1: [1][]const u8 = ["../shared/./lib"];
    const p1 = st.applyCd(a1[0..], "/home/dev")?;
    expect(strEql(p1, "/workspace/shared/lib"));
    expect(strEql(st.prevCwd(), "/workspace/app"));

    // cd - toggles back to /workspace/app
    const dash: [1][]const u8 = ["-"];
    const p2 = st.applyCd(dash[0..], "/home/dev")?;
    expect(strEql(p2, "/workspace/app"));
    expect(strEql(st.prevCwd(), "/workspace/shared/lib"));

    // cd - toggles forward again
    const p3 = st.applyCd(dash[0..], "/home/dev")?;
    expect(strEql(p3, "/workspace/shared/lib"));

    // cd ~/docs
    const a_home: [1][]const u8 = ["~/docs"];
    const p4 = st.applyCd(a_home[0..], "/home/dev")?;
    expect(strEql(p4, "/home/dev/docs"));

    // cd (0 args -> $HOME)
    const no_args: [0][]const u8 = [];
    const p5 = st.applyCd(no_args[0..], "/home/dev")?;
    expect(strEql(p5, "/home/dev"));
});

test("shell cd_path: formatCdError emits Bun-compatible diagnostics", fn()! {
    let buf: [128]u8 = [0; 128];

    const n1 = formatCdError(.TooManyArguments, "", buf[0..])?;
    expect(strEql(buf[0..n1], "cd: too many arguments\n"));

    const n2 = formatCdError(.EmptyHome, "", buf[0..])?;
    expect(strEql(buf[0..n2], "cd: HOME not set\n"));

    const n3 = formatCdError(.NoPreviousDir, "", buf[0..])?;
    expect(strEql(buf[0..n3], "cd: OLDPWD not set\n"));

    const n4 = formatCdError(.PathTooLong, "", buf[0..])?;
    expect(strEql(buf[0..n4], "cd: file name too long\n"));

    const n5 = formatCdError(.BufferTooSmall, "/bad/path", buf[0..])?;
    expect(strEql(buf[0..n5], "cd: not a directory: /bad/path\n"));
});

test("shell cd_path: BufferTooSmall guards on normalizePosixPath and formatCdError", fn()! {
    let tiny: [4]u8 = [0; 4];
    let errs: usize = 0;

    if (_ := normalizePosixPath("/usr/local/bin", tiny[0..])) {} else { errs += 1; }
    if (_ := formatCdError(.TooManyArguments, "", tiny[0..])) {} else { errs += 1; }
    expect_eq(errs, 2);
});
