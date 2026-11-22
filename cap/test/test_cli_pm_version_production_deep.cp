import { test, expect } from "cap:test";
import {
    PmVersionType,
    PmSemver,
    PmVersionOptions,
    parsePmVersionArg,
    normalizeGitTagVersion,
    computeNextPmVersion,
    formatPmCommitMessage,
    formatPmGitTag,
} from "cap:cli";

fn eqBytes(a: []const u8, b: []const u8): bool {
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

test("cli pm_version: argument parsing across all 9 version strategies") {
    expect((parsePmVersionArg("patch")?).version_type == .patch);
    expect((parsePmVersionArg("minor")?).version_type == .minor);
    expect((parsePmVersionArg("major")?).version_type == .major);
    expect((parsePmVersionArg("prepatch")?).version_type == .prepatch);
    expect((parsePmVersionArg("preminor")?).version_type == .preminor);
    expect((parsePmVersionArg("premajor")?).version_type == .premajor);
    expect((parsePmVersionArg("prerelease")?).version_type == .prerelease);
    expect((parsePmVersionArg("from-git")?).version_type == .from_git);
    expect(eqBytes(PmVersionType.from_git.asKeyword(), "from-git"));

    const spec1 = parsePmVersionArg("v2.4.6-rc.1+build.42")?;
    expect(spec1.version_type == .specific);
    expect(eqBytes(spec1.specific_version, "2.4.6-rc.1+build.42"));

    const spec2 = parsePmVersionArg("=1.0.0")?;
    expect(spec2.version_type == .specific);
    expect(eqBytes(spec2.specific_version, "1.0.0"));
}

test("cli pm_version: patch, minor, major and prepatch/preminor/premajor bumps") {
    const opts = PmVersionOptions.defaults();
    let buf: [64]u8 = [0; 64];

    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("patch")?, opts, buf[0..])?], "1.2.4"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("minor")?, opts, buf[0..])?], "1.3.0"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("major")?, opts, buf[0..])?], "2.0.0"));

    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("prepatch")?, opts, buf[0..])?], "1.2.4-0"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("preminor")?, opts, buf[0..])?], "1.3.0-0"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("premajor")?, opts, buf[0..])?], "2.0.0-0"));

    let beta_opts = PmVersionOptions.defaults();
    beta_opts.preid = "beta";
    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("prepatch")?, beta_opts, buf[0..])?], "1.2.4-beta.0"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("preminor")?, beta_opts, buf[0..])?], "1.3.0-beta.0"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3", parsePmVersionArg("premajor")?, beta_opts, buf[0..])?], "2.0.0-beta.0"));

    // 未指定 --preid 时继承已有预发布首段标识符
    expect(eqBytes(buf[0..computeNextPmVersion("1.2.3-alpha.5", parsePmVersionArg("preminor")?, opts, buf[0..])?], "1.3.0-alpha.0"));
}

test("cli pm_version: prerelease full transition matrix") {
    const pre_arg = parsePmVersionArg("prerelease")?;
    const opts = PmVersionOptions.defaults();
    let beta_opts = PmVersionOptions.defaults();
    beta_opts.preid = "beta";
    let rc_opts = PmVersionOptions.defaults();
    rc_opts.preid = "rc";

    let buf: [64]u8 = [0; 64];
    expect(eqBytes(buf[0..computeNextPmVersion("1.0.0", pre_arg, opts, buf[0..])?], "1.0.1-0"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.0.0", pre_arg, beta_opts, buf[0..])?], "1.0.1-beta.0"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.0.1-0", pre_arg, opts, buf[0..])?], "1.0.1-1"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.0.1-0", pre_arg, rc_opts, buf[0..])?], "1.0.1-rc.1"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.0.1-alpha.3", pre_arg, opts, buf[0..])?], "1.0.1-alpha.4"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.0.1-alpha", pre_arg, opts, buf[0..])?], "1.0.1-alpha.1"));
    expect(eqBytes(buf[0..computeNextPmVersion("1.0.1-alpha.2", pre_arg, beta_opts, buf[0..])?], "1.0.1-beta.3"));
}

test("cli pm_version: specific version, allow-same-version guard and git tag normalization") {
    let opts = PmVersionOptions.defaults();
    let buf: [64]u8 = [0; 64];

    // 默认禁止设置与当前版本相同的版本号（含忽略 +build metadata）
    let same_err: usize = 0;
    * = computeNextPmVersion("1.2.3", parsePmVersionArg("1.2.3")?, opts, buf[0..]) catch { same_err += 1; 0; };
    * = computeNextPmVersion("1.2.3+a", parsePmVersionArg("v1.2.3+b")?, opts, buf[0..]) catch { same_err += 1; 0; };
    expect(same_err == 2);

    // 开启 allow_same_version 放行
    opts.allow_same_version = true;
    const n = computeNextPmVersion("1.2.3", parsePmVersionArg("v1.2.3")?, opts, buf[0..])?;
    expect(eqBytes(buf[0..n], "1.2.3"));

    // normalizeGitTagVersion 规范化
    expect(eqBytes(normalizeGitTagVersion("  v3.1.4-rc.2\n")?, "3.1.4-rc.2"));
    expect(eqBytes(normalizeGitTagVersion("=0.9.0")?, "0.9.0"));

    // from-git 在纯计算函数无外部 tag 时返回 FromGitRequiresTag
    let fg_err: usize = 0;
    * = computeNextPmVersion("1.0.0", parsePmVersionArg("from-git")?, opts, buf[0..]) catch { fg_err += 1; 0; };
    expect(fg_err == 1);
}

test("cli pm_version: commit message %s substitution, git tag formatting and buffer guard") {
    let buf: [128]u8 = [0; 128];

    // 空模板默认 v<version>
    expect(eqBytes(buf[0..formatPmCommitMessage("", "1.2.4", buf[0..])?], "v1.2.4"));
    // 单个与多个 %s 替换
    expect(eqBytes(buf[0..formatPmCommitMessage("Release beta: %s", "1.0.1-beta.0", buf[0..])?], "Release beta: 1.0.1-beta.0"));
    expect(eqBytes(buf[0..formatPmCommitMessage("chore(%s): bump to v%s", "2.0.0", buf[0..])?], "chore(2.0.0): bump to v2.0.0"));
    // 无 %s 原样保留
    expect(eqBytes(buf[0..formatPmCommitMessage("manual release", "2.0.0", buf[0..])?], "manual release"));

    // formatPmGitTag 不重复叠加 v
    expect(eqBytes(buf[0..formatPmGitTag("1.5.0", buf[0..])?], "v1.5.0"));
    expect(eqBytes(buf[0..formatPmGitTag("v1.5.0", buf[0..])?], "v1.5.0"));

    // 缓冲区不足防护
    let tiny: [4]u8 = [0; 4];
    const small_res = formatPmCommitMessage("Release %s", "1.2.3", tiny[0..]) catch {
        expect(true);
        return;
    };
    * = small_res;
    expect(false);
}

test("cli pm_version: malformed input guards, overflow guard and semver round-trip") {
    let rejected: usize = 0;
    let buf: [64]u8 = [0; 64];
    const opts = PmVersionOptions.defaults();

    // 非法参数与畸形 Semver
    * = parsePmVersionArg("") catch { rejected += 1; parsePmVersionArg("patch")?; };
    * = parsePmVersionArg("invalid-bump") catch { rejected += 1; parsePmVersionArg("patch")?; };
    * = PmSemver.parse("01.2.3") catch { rejected += 1; PmSemver.parse("0.0.0")?; };
    * = PmSemver.parse("1.2") catch { rejected += 1; PmSemver.parse("0.0.0")?; };
    * = PmSemver.parse("1.2.3.4") catch { rejected += 1; PmSemver.parse("0.0.0")?; };
    * = PmSemver.parse("1.2.3-") catch { rejected += 1; PmSemver.parse("0.0.0")?; };
    * = PmSemver.parse("1.2.3+") catch { rejected += 1; PmSemver.parse("0.0.0")?; };
    * = PmSemver.parse("1.2.3-alpha..1") catch { rejected += 1; PmSemver.parse("0.0.0")?; };
    // 非法 --preid
    let bad_opts = PmVersionOptions.defaults();
    bad_opts.preid = "bad preid!";
    * = computeNextPmVersion("1.0.0", parsePmVersionArg("prerelease")?, bad_opts, buf[0..]) catch { rejected += 1; 0; };
    // u32 递进溢出防护
    * = computeNextPmVersion("1.0.4294967295", parsePmVersionArg("patch")?, opts, buf[0..]) catch { rejected += 1; 0; };

    expect(rejected == 10);

    // Round-trip 幂等性
    const v1 = PmSemver.parse("v10.20.30-beta.4+exp.sha.5114f85")?;
    const n1 = v1.format(buf[0..])?;
    expect(eqBytes(buf[0..n1], "10.20.30-beta.4+exp.sha.5114f85"));
    const v2 = PmSemver.parse(buf[0..n1])?;
    expect(v1.eqlCoreAndPre(v2));
    expect(eqBytes(v1.build, v2.build));
}
