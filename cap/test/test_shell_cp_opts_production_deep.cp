import { test, expect } from "cap:test";
import {
    CpOpts,
    CpPlan,
} from "cap:shell";

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

test("shell cp_opts: short/long flags, last-wins overwrite policy and -a archive semantics") {
    const a1: [4][]const u8 = ["-rfv", "-n", "src.txt", "dst.txt"];
    const o1 = CpOpts.parse(a1[0..])?;
    expect(o1.recursive);
    expect(o1.verbose);
    expect(o1.overwrite == .no_clobber);
    expect(!o1.allowsOverwrite());
    expect(o1.source_count == 1);
    expect(eqBytes(o1.destination, "dst.txt"));

    // 后出现的 -f 覆盖先前的 -n 和 -i
    const a2: [5][]const u8 = ["--no-clobber", "--interactive", "--force", "a", "b"];
    const o2 = CpOpts.parse(a2[0..])?;
    expect(o2.overwrite == .force);
    expect(o2.allowsOverwrite());

    // -a 等价 -RPp
    const a3: [3][]const u8 = ["-a", "dir_a", "dir_b"];
    const o3 = CpOpts.parse(a3[0..])?;
    expect(o3.recursive);
    expect(o3.preserve);
    expect(o3.symlink_policy == .no_dereference);
}

test("shell cp_opts: POSIX symlink follow matrix (-H / -L / -P / default)") {
    // 非递归默认跟随符号链接
    const a_def: [2][]const u8 = ["a", "b"];
    const o_def = CpOpts.parse(a_def[0..])?;
    expect(o_def.effectiveFollowSymlink(true));
    expect(o_def.effectiveFollowSymlink(false));

    // 递归默认不跟随符号链接
    const a_rec: [3][]const u8 = ["-R", "a", "b"];
    const o_rec = CpOpts.parse(a_rec[0..])?;
    expect(!o_rec.effectiveFollowSymlink(true));
    expect(!o_rec.effectiveFollowSymlink(false));

    // -H 仅跟随命令行顶层参数
    const a_h: [3][]const u8 = ["-RH", "a", "b"];
    const o_h = CpOpts.parse(a_h[0..])?;
    expect(o_h.effectiveFollowSymlink(true));
    expect(!o_h.effectiveFollowSymlink(false));

    // -L 始终跟随
    const a_l: [3][]const u8 = ["-RL", "a", "b"];
    const o_l = CpOpts.parse(a_l[0..])?;
    expect(o_l.effectiveFollowSymlink(true));
    expect(o_l.effectiveFollowSymlink(false));

    // -P 始终不跟随
    const a_p: [3][]const u8 = ["-P", "a", "b"];
    const o_p = CpOpts.parse(a_p[0..])?;
    expect(!o_p.effectiveFollowSymlink(true));
    expect(!o_p.effectiveFollowSymlink(false));
}

test("shell cp_opts: CpPlan single/multi-source target resolution and double-dash terminator") {
    let out: [128]u8 = [0; 128];

    // 单文件重命名
    const a1: [2][]const u8 = ["src/a.txt", "dst/b.txt"];
    const p1 = CpPlan.build(CpOpts.parse(a1[0..])?, false)?;
    expect(eqBytes(out[0..p1.resolveTargetFor(0, false, out[0..])?], "dst/b.txt"));

    // 多源拷贝到带尾斜杠目录
    const a2: [4][]const u8 = ["-v", "src/a.txt", "src/b.txt", "out/dir///"];
    const p2 = CpPlan.build(CpOpts.parse(a2[0..])?, true)?;
    expect(eqBytes(out[0..p2.resolveTargetFor(0, false, out[0..])?], "out/dir/a.txt"));
    expect(eqBytes(out[0..p2.resolveTargetFor(1, false, out[0..])?], "out/dir/b.txt"));

    // 目标为根目录 '/' 不产生双斜杠
    const a3: [2][]const u8 = ["tmp/hello.txt", "/"];
    const p3 = CpPlan.build(CpOpts.parse(a3[0..])?, true)?;
    expect(eqBytes(out[0..p3.resolveTargetFor(0, false, out[0..])?], "/hello.txt"));

    // '--' 终止符允许以 '-' 开头的文件名
    const a4: [4][]const u8 = ["-p", "--", "-src.txt", "-dst.txt"];
    const o4 = CpOpts.parse(a4[0..])?;
    expect(o4.preserve);
    if (s0 := o4.sourceAt(0)) { expect(eqBytes(s0, "-src.txt")); } else { expect(false); }
    expect(o4.sourceAt(1) == null);
    expect(eqBytes(o4.destination, "-dst.txt"));
}

test("shell cp_opts: directory omission guard without -r/-R") {
    let out: [128]u8 = [0; 128];
    const a_no_r: [2][]const u8 = ["dir_src", "dir_dst"];
    const p_no_r = CpPlan.build(CpOpts.parse(a_no_r[0..])?, true)?;

    let omitted: usize = 0;
    * = p_no_r.resolveTargetFor(0, true, out[0..]) catch { omitted += 1; 0; };
    expect(omitted == 1);

    // 开启 -r 后正常规划
    const a_r: [3][]const u8 = ["-r", "dir_src/", "dir_dst"];
    const p_r = CpPlan.build(CpOpts.parse(a_r[0..])?, true)?;
    expect(eqBytes(out[0..p_r.resolveTargetFor(0, true, out[0..])?], "dir_dst/dir_src"));
}

test("shell cp_opts: self-copy and recursive-into-self cycle guards") {
    let out: [128]u8 = [0; 128];
    let blocked: usize = 0;

    // 文件自拷贝
    const s1: [2][]const u8 = ["a.txt", "a.txt"];
    const p1 = CpPlan.build(CpOpts.parse(s1[0..])?, false)?;
    * = p1.resolveTargetFor(0, false, out[0..]) catch { blocked += 1; 0; };

    // 目录拷贝到自身子目录
    const s2: [3][]const u8 = ["-r", "/app/src", "/app/src/sub"];
    const p2 = CpPlan.build(CpOpts.parse(s2[0..])?, true)?;
    * = p2.resolveTargetFor(0, true, out[0..]) catch { blocked += 1; 0; };

    // 仅尾斜杠差异也属于自拷贝
    const s3: [3][]const u8 = ["-r", "/app/src/", "/app/src"];
    const p3 = CpPlan.build(CpOpts.parse(s3[0..])?, false)?;
    * = p3.resolveTargetFor(0, true, out[0..]) catch { blocked += 1; 0; };

    expect(blocked == 3);

    // 相似前缀目录（/app/src -> /app/src2）不是子目录，应正常放行
    const s_ok: [3][]const u8 = ["-r", "/app/src", "/app/src2"];
    const p_ok = CpPlan.build(CpOpts.parse(s_ok[0..])?, false)?;
    expect(eqBytes(out[0..p_ok.resolveTargetFor(0, true, out[0..])?], "/app/src2"));
}

test("shell cp_opts: malformed args, non-dir destination guards and buffer guard") {
    let rejected: usize = 0;

    // 0 操作数 / 1 操作数 / 非法选项
    const e1: [1][]const u8 = ["-r"];
    * = CpOpts.parse(e1[0..]) catch { rejected += 1; CpOpts.defaults(); };
    const e2: [2][]const u8 = ["-r", "only_src"];
    * = CpOpts.parse(e2[0..]) catch { rejected += 1; CpOpts.defaults(); };
    const e3: [3][]const u8 = ["-Z", "a", "b"];
    * = CpOpts.parse(e3[0..]) catch { rejected += 1; CpOpts.defaults(); };
    const e4: [3][]const u8 = ["--bogus", "a", "b"];
    * = CpOpts.parse(e4[0..]) catch { rejected += 1; CpOpts.defaults(); };

    // 多源目标非目录 / 尾斜杠目标非目录
    const m1: [3][]const u8 = ["a", "b", "not_a_dir"];
    * = CpPlan.build(CpOpts.parse(m1[0..])?, false) catch { rejected += 1; CpPlan{ opts: CpOpts.defaults(), dest_is_dir: false }; };
    const m2: [2][]const u8 = ["a", "not_a_dir/"];
    * = CpPlan.build(CpOpts.parse(m2[0..])?, false) catch { rejected += 1; CpPlan{ opts: CpOpts.defaults(), dest_is_dir: false }; };

    // 缓冲区不足防护
    const ok_args: [2][]const u8 = ["source_file.txt", "destination_dir"];
    const ok_plan = CpPlan.build(CpOpts.parse(ok_args[0..])?, true)?;
    let tiny: [8]u8 = [0; 8];
    * = ok_plan.resolveTargetFor(0, false, tiny[0..]) catch { rejected += 1; 0; };

    expect(rejected == 7);
}
