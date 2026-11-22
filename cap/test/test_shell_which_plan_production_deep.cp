// cap/test/test_shell_which_plan_production_deep.cp
// 深度生产级回归测试：cap:shell 跨平台 `which` PATH 扫描与退出码评估引擎 (lib/shell/which_plan.cp)

import { test } from "cap:test";
import {
    SHELL_WHICH_MAX_ARGS,
    WhichPlatform,
    PathEnvIter,
    WhichPlan,
    isDirectCommandPath,
    joinDirAndCommand,
    formatWhichNotFound,
    resolveWhichSingle,
    evaluateWhichExitCode,
} from "cap:shell";

fn bytesEq(a: []const u8, b: []const u8): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) return false;
        i += 1;
    }
    return true;
}

fn mockPosixProbe(path: []const u8): bool {
    return bytesEq(path, "/usr/local/bin/bun") ||
        bytesEq(path, "/usr/bin/git") ||
        bytesEq(path, "/workspace/local-tool") ||
        bytesEq(path, "/workspace/bin/run.sh") ||
        bytesEq(path, "/opt/custom/cp");
}

fn mockWinProbe(path: []const u8): bool {
    return bytesEq(path, "C:\\Tools\\bun.exe") ||
        bytesEq(path, "D:\\proj\\script.cmd");
}

test("which_plan: PathEnvIter POSIX ':' & Windows ';' splitting with empty-segment '.' normalization") {
    // POSIX: 头部空段、中间空段、尾部空段均等价于 "."
    let it = PathEnvIter.init(":/usr/bin::/bin:", WhichPlatform.Posix);
    if (!bytesEq(it.next().?, ".")) return err.PosixLeadingEmptyFailed;
    if (!bytesEq(it.next().?, "/usr/bin")) return err.PosixSeg1Failed;
    if (!bytesEq(it.next().?, ".")) return err.PosixMiddleEmptyFailed;
    if (!bytesEq(it.next().?, "/bin")) return err.PosixSeg3Failed;
    if (!bytesEq(it.next().?, ".")) return err.PosixTrailingEmptyFailed;
    if (it.next() != null) return err.PosixExpectedEnd;

    // 空 PATH 直接结束
    let it_empty = PathEnvIter.init("", WhichPlatform.Posix);
    if (it_empty.next() != null) return err.EmptyPathShouldYieldNull;

    // Windows ';' 分隔
    let it_win = PathEnvIter.init("C:\\Windows;C:\\Tools", WhichPlatform.Windows);
    if (!bytesEq(it_win.next().?, "C:\\Windows")) return err.WinSeg0Failed;
    if (!bytesEq(it_win.next().?, "C:\\Tools")) return err.WinSeg1Failed;
    if (it_win.next() != null) return err.WinExpectedEnd;
}

test("which_plan: PATH lookup priority, cwd empty-segment resolution & direct path bypass") {
    const path_env = "/usr/local/bin::/usr/bin";

    // 1. 常规 PATH 查找命中
    const r_bun = resolveWhichSingle("bun", path_env, "/workspace", WhichPlatform.Posix, mockPosixProbe)?;
    if (!r_bun.found || !bytesEq(r_bun.path(), "/usr/local/bin/bun")) return err.BunLookupFailed;

    // 2. PATH 中空段解析为 cwd (/workspace)
    const r_local = resolveWhichSingle("local-tool", path_env, "/workspace", WhichPlatform.Posix, mockPosixProbe)?;
    if (!r_local.found || !bytesEq(r_local.path(), "/workspace/local-tool")) return err.CwdSegmentLookupFailed;

    // 3. 相对直接路径跳过 PATH，相对 cwd 解析
    if (!isDirectCommandPath("bin/run.sh", WhichPlatform.Posix)) return err.DirectPathDetectFailed;
    const r_rel = resolveWhichSingle("bin/run.sh", "", "/workspace", WhichPlatform.Posix, mockPosixProbe)?;
    if (!r_rel.found || !bytesEq(r_rel.path(), "/workspace/bin/run.sh")) return err.DirectRelLookupFailed;

    // 4. 绝对路径直接探测
    const r_abs = resolveWhichSingle("/opt/custom/cp", "", "/workspace", WhichPlatform.Posix, mockPosixProbe)?;
    if (!r_abs.found || !bytesEq(r_abs.path(), "/opt/custom/cp")) return err.DirectAbsLookupFailed;

    // 5. 未命中返回 found == false
    const r_miss = resolveWhichSingle("nonexistent", path_env, "/workspace", WhichPlatform.Posix, mockPosixProbe)?;
    if (r_miss.found || r_miss.path().len != 0) return err.ExpectedNotFound;
}

test("which_plan: Windows PATH & drive-letter direct path resolution") {
    const r_win = resolveWhichSingle("bun.exe", "C:\\Windows;C:\\Tools", "D:\\proj", WhichPlatform.Windows, mockWinProbe)?;
    if (!r_win.found || !bytesEq(r_win.path(), "C:\\Tools\\bun.exe")) return err.WinPathLookupFailed;

    if (!isDirectCommandPath("D:\\proj\\script.cmd", WhichPlatform.Windows)) return err.WinDirectDetectFailed;
    const r_abs = resolveWhichSingle("D:\\proj\\script.cmd", "", "C:\\", WhichPlatform.Windows, mockWinProbe)?;
    if (!r_abs.found || !bytesEq(r_abs.path(), "D:\\proj\\script.cmd")) return err.WinDirectLookupFailed;
}

test("which_plan: multi-arg plan parsing, flags (-a/-s/--), & Bun exit code contract") {
    const path_env = "/usr/local/bin:/usr/bin";

    // 1. 零参数按 Bun which.cp 返回退出码 1
    const empty_args: [0]([]const u8) = [];
    const p_empty = WhichPlan.parse(empty_args[0..])?;
    if (!p_empty.isEmptyArgs()) return err.ExpectedEmptyPlan;
    const rc_empty = evaluateWhichExitCode(p_empty, path_env, "/workspace", WhichPlatform.Posix, mockPosixProbe)?;
    if (rc_empty != 1) return err.EmptyArgsShouldExit1;

    // 2. 全部命中返回退出码 0
    const ok_args: [4]([]const u8) = ["-as", "--", "bun", "git"];
    const p_ok = WhichPlan.parse(ok_args[0..])?;
    if (!p_ok.all_matches || !p_ok.silent || p_ok.argsSlice().len != 2) return err.PlanFlagsMismatch;
    const rc_ok = evaluateWhichExitCode(p_ok, path_env, "/workspace", WhichPlatform.Posix, mockPosixProbe)?;
    if (rc_ok != 0) return err.AllFoundShouldExit0;

    // 3. 部分命中、任一未命中返回退出码 1
    const partial_args: [2]([]const u8) = ["bun", "missing-cmd"];
    const p_partial = WhichPlan.parse(partial_args[0..])?;
    const rc_partial = evaluateWhichExitCode(p_partial, path_env, "/workspace", WhichPlatform.Posix, mockPosixProbe)?;
    if (rc_partial != 1) return err.PartialMissingShouldExit1;
}

test("which_plan: joinDirAndCommand trailing slash normalization & not-found formatting") {
    let buf: [128]u8 = [0; 128];

    const n1 = joinDirAndCommand("/usr/bin///", "git", WhichPlatform.Posix, buf[0..])?;
    if (!bytesEq(buf[0..n1], "/usr/bin/git")) return err.JoinTrailingSlashFailed;

    const n2 = joinDirAndCommand("/", "sh", WhichPlatform.Posix, buf[0..])?;
    if (!bytesEq(buf[0..n2], "/sh")) return err.JoinRootFailed;

    const n3 = formatWhichNotFound("foobar", buf[0..])?;
    if (!bytesEq(buf[0..n3], "foobar not found\n")) return err.NotFoundFmtMismatch;
}

test("which_plan: malformed command names rejected (empty, NUL, newline)") {
    const bad_cmds: [3]([]const u8) = ["", "bad\x00cmd", "bad\ncmd"];
    let i: usize = 0;
    while (i < bad_cmds.len) {
        let caught = false;
        * = resolveWhichSingle(bad_cmds[i], "/usr/bin", "/", WhichPlatform.Posix, mockPosixProbe) catch {
            caught = true;
        };
        if (!caught) return err.ExpectedInvalidCommand;
        i += 1;
    }
}

test("which_plan: TooManyArgs & BufferTooSmall boundary guards") {
    if (SHELL_WHICH_MAX_ARGS != 32) return err.UnexpectedMaxArgs;
    let many: [33]([]const u8) = ["bun"; 33];
    let caught_many = false;
    * = WhichPlan.parse(many[0..]) catch {
        caught_many = true;
    };
    if (!caught_many) return err.ExpectedTooManyArgs;

    let tiny: [4]u8 = [0; 4];
    let caught_join = false;
    * = joinDirAndCommand("/usr/local/bin", "bun", WhichPlatform.Posix, tiny[0..]) catch {
        caught_join = true;
    };
    if (!caught_join) return err.ExpectedJoinBufferTooSmall;

    let caught_fmt = false;
    * = formatWhichNotFound("longcmd", tiny[0..]) catch {
        caught_fmt = true;
    };
    if (!caught_fmt) return err.ExpectedFmtBufferTooSmall;
}
