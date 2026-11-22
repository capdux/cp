// cap/test/test_shell_ls_opts_production_deep.cp
// 深度生产级回归测试：cap:shell POSIX/GNU `ls` 选项解析、权限/尺寸格式化与排序器 (lib/shell/ls_opts.cp)

import { test } from "cap:test";
import {
    SHELL_LS_MAX_PATHS,
    LsSortKey,
    LsEntryKind,
    LsOpts,
    LsPlan,
    LsEntryMeta,
    formatLsMode,
    formatLsHumanSize,
    sortLsEntries,
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

test("ls_opts: default '.', bundled short flags, long flags & '--' sentinel") {
    const empty: [0]([]const u8) = [];
    const p_def = LsPlan.parse(empty[0..])?;
    if (p_def.pathsSlice().len != 1 || !bytesEq(p_def.pathsSlice()[0], ".")) {
        return err.DefaultDotPathFailed;
    }

    const args: [5]([]const u8) = ["-lahtrF", "--group-directories-first", "src", "--", "-weird-dir"];
    const plan = LsPlan.parse(args[0..])?;
    if (!plan.opts.long_listing || !plan.opts.show_all || !plan.opts.human_readable ||
        !plan.opts.reverse_order || !plan.opts.classify || !plan.opts.group_directories_first ||
        plan.opts.sort_key != LsSortKey.Mtime)
    {
        return err.FlagsMismatch;
    }
    const paths = plan.pathsSlice();
    if (paths.len != 2 || !bytesEq(paths[0], "src") || !bytesEq(paths[1], "-weird-dir")) {
        return err.PathsMismatch;
    }

    // -l 与 -1 后出现者胜出；-f 启用 show_all 且禁用排序
    const args_1: [3]([]const u8) = ["-l", "-1", "-f"];
    const p1 = LsPlan.parse(args_1[0..])?;
    if (p1.opts.long_listing || !p1.opts.one_per_line || !p1.opts.show_all || p1.opts.sort_key != LsSortKey.None) {
        return err.OverridePrecedenceFailed;
    }
}

test("ls_opts: hidden dotfile, '.'/'..' & backup '~' filtering rules") {
    const o_default = LsOpts{};
    if (o_default.shouldIncludeName(".") || o_default.shouldIncludeName("..") ||
        o_default.shouldIncludeName(".env") || !o_default.shouldIncludeName("main.cp"))
    {
        return err.DefaultFilterFailed;
    }

    // -A 显示隐藏文件，但排除 '.' 和 '..'
    const o_almost = LsOpts{ show_almost_all: true };
    if (o_almost.shouldIncludeName(".") || o_almost.shouldIncludeName("..") ||
        !o_almost.shouldIncludeName(".env"))
    {
        return err.AlmostAllFilterFailed;
    }

    // -a 显示包含 '.' 和 '..'；-B 忽略 '~' 结尾的备份文件
    const o_all_nobak = LsOpts{ show_all: true, ignore_backups: true };
    if (!o_all_nobak.shouldIncludeName(".") || !o_all_nobak.shouldIncludeName("..") ||
        o_all_nobak.shouldIncludeName("notes.txt~") || !o_all_nobak.shouldIncludeName("notes.txt"))
    {
        return err.AllAndBackupFilterFailed;
    }
}

test("ls_opts: POSIX 10-char mode string formatting including special bits") {
    let buf: [16]u8 = [0; 16];

    const n1 = formatLsMode(LsEntryKind.Directory, 0o755, buf[0..])?;
    if (!bytesEq(buf[0..n1], "drwxr-xr-x")) return err.DirModeMismatch;

    const n2 = formatLsMode(LsEntryKind.File, 0o644, buf[0..])?;
    if (!bytesEq(buf[0..n2], "-rw-r--r--")) return err.FileModeMismatch;

    const n3 = formatLsMode(LsEntryKind.Symlink, 0o777, buf[0..])?;
    if (!bytesEq(buf[0..n3], "lrwxrwxrwx")) return err.SymlinkModeMismatch;

    // setuid + setgid + sticky（含执行位 vs 无执行位大小写区分）
    const n4 = formatLsMode(LsEntryKind.Directory, 0o7755, buf[0..])?;
    if (!bytesEq(buf[0..n4], "drwsr-sr-t")) return err.SpecialExecBitsMismatch;

    const n5 = formatLsMode(LsEntryKind.File, 0o7644, buf[0..])?;
    if (!bytesEq(buf[0..n5], "-rwSr-Sr-T")) return err.SpecialNoExecBitsMismatch;
}

test("ls_opts: integer human-readable size formatting & classify indicators") {
    let buf: [16]u8 = [0; 16];

    const n0 = formatLsHumanSize(0, buf[0..])?;
    if (!bytesEq(buf[0..n0], "0")) return err.Size0Mismatch;

    const n1 = formatLsHumanSize(512, buf[0..])?;
    if (!bytesEq(buf[0..n1], "512")) return err.Size512Mismatch;

    const n2 = formatLsHumanSize(1536, buf[0..])?; // 1.5K
    if (!bytesEq(buf[0..n2], "1.5K")) return err.Size1536Mismatch;

    const n3 = formatLsHumanSize(1024 * 1024, buf[0..])?; // 1.0M
    if (!bytesEq(buf[0..n3], "1.0M")) return err.Size1MMismatch;

    const n4 = formatLsHumanSize(15 * 1024 * 1024, buf[0..])?; // 15M
    if (!bytesEq(buf[0..n4], "15M")) return err.Size15MMismatch;

    // -F 与 -p 指示后缀
    const o_F = LsOpts{ classify: true };
    const o_p = LsOpts{ slash_indicator: true };
    const e_dir = LsEntryMeta{ name: "src", kind: LsEntryKind.Directory, mode: 0o755 };
    const e_bin = LsEntryMeta{ name: "run", kind: LsEntryKind.File, mode: 0o755 };
    const e_lnk = LsEntryMeta{ name: "latest", kind: LsEntryKind.Symlink, mode: 0o777 };

    if (e_dir.indicatorChar(o_F).? != '/' || e_bin.indicatorChar(o_F).? != '*' || e_lnk.indicatorChar(o_F).? != '@') {
        return err.ClassifyIndicatorFailed;
    }
    if (e_dir.indicatorChar(o_p).? != '/' || e_bin.indicatorChar(o_p) != null) {
        return err.SlashIndicatorFailed;
    }
}

test("ls_opts: in-place sorting by Name, Size (-S), Mtime (-t), reverse (-r) & dirs-first") {
    let items: [4]LsEntryMeta = [
        LsEntryMeta{ name: "z.txt", kind: LsEntryKind.File, size: 100, mtime_sec: 1000 },
        LsEntryMeta{ name: "alpha", kind: LsEntryKind.Directory, size: 4096, mtime_sec: 500 },
        LsEntryMeta{ name: "big.bin", kind: LsEntryKind.File, size: 9000, mtime_sec: 2000 },
        LsEntryMeta{ name: "beta", kind: LsEntryKind.Directory, size: 4096, mtime_sec: 1500 },
    ];

    // 1. --group-directories-first + 默认名称升序
    sortLsEntries(items[0..], LsOpts{ group_directories_first: true, sort_key: LsSortKey.Name });
    if (!bytesEq(items[0].name, "alpha") || !bytesEq(items[1].name, "beta") ||
        !bytesEq(items[2].name, "big.bin") || !bytesEq(items[3].name, "z.txt"))
    {
        return err.GroupDirsFirstSortFailed;
    }

    // 2. -S 按大小降序（big.bin 9000 -> alpha 4096 -> beta 4096 -> z.txt 100）
    sortLsEntries(items[0..], LsOpts{ sort_key: LsSortKey.Size });
    if (!bytesEq(items[0].name, "big.bin") || !bytesEq(items[3].name, "z.txt")) {
        return err.SizeSortFailed;
    }

    // 3. -t -r 按修改时间升序（最旧 alpha 500 -> z.txt 1000 -> beta 1500 -> big.bin 2000）
    sortLsEntries(items[0..], LsOpts{ sort_key: LsSortKey.Mtime, reverse_order: true });
    if (!bytesEq(items[0].name, "alpha") || !bytesEq(items[1].name, "z.txt") ||
        !bytesEq(items[2].name, "beta") || !bytesEq(items[3].name, "big.bin"))
    {
        return err.MtimeReverseSortFailed;
    }
}

test("ls_opts: unsorted (-U/-f) preserves original order round-trip") {
    let items: [3]LsEntryMeta = [
        LsEntryMeta{ name: "c", size: 1 },
        LsEntryMeta{ name: "a", size: 99 },
        LsEntryMeta{ name: "b", size: 50 },
    ];
    sortLsEntries(items[0..], LsOpts{ sort_key: LsSortKey.None });
    if (!bytesEq(items[0].name, "c") || !bytesEq(items[1].name, "a") || !bytesEq(items[2].name, "b")) {
        return err.UnsortedModifiedOrder;
    }
}

test("ls_opts: malformed options, TooManyPaths & BufferTooSmall boundary guards") {
    const bad_short: [1]([]const u8) = ["-Z"];
    let caught_short = false;
    * = LsPlan.parse(bad_short[0..]) catch {
        caught_short = true;
    };
    if (!caught_short) return err.ExpectedIllegalShort;

    const bad_long: [1]([]const u8) = ["--bogus-opt"];
    let caught_long = false;
    * = LsPlan.parse(bad_long[0..]) catch {
        caught_long = true;
    };
    if (!caught_long) return err.ExpectedIllegalLong;

    if (SHELL_LS_MAX_PATHS != 32) return err.UnexpectedMaxPaths;
    let many: [33]([]const u8) = ["dir"; 33];
    let caught_many = false;
    * = LsPlan.parse(many[0..]) catch {
        caught_many = true;
    };
    if (!caught_many) return err.ExpectedTooManyPaths;

    let tiny_mode: [5]u8 = [0; 5];
    let caught_mode_buf = false;
    * = formatLsMode(LsEntryKind.File, 0o644, tiny_mode[0..]) catch {
        caught_mode_buf = true;
    };
    if (!caught_mode_buf) return err.ExpectedModeBufferTooSmall;

    let tiny_sz: [2]u8 = [0; 2];
    let caught_sz_buf = false;
    * = formatLsHumanSize(1536, tiny_sz[0..]) catch {
        caught_sz_buf = true;
    };
    if (!caught_sz_buf) return err.ExpectedSizeBufferTooSmall;
}
