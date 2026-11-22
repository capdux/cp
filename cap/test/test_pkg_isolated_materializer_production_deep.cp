// cap/test/test_pkg_isolated_materializer_production_deep.cp
// 深度生产级回归测试：cap:pkg Isolated Install 物化策略与错误自愈协调器 (lib/pkg/isolated_materializer.cp)

import { test } from "cap:test";
import {
    ISOLATED_MAX_SKIP_DIRS,
    IsolatedLinkerMode,
    MaterializePlatform,
    MaterializeBackend,
    MaterializeErrno,
    MaterializeRecoveryAction,
    resolveDefaultLinkerMode,
    parseLinkerModeFlag,
    defaultMaterializeBackend,
    fallbackMaterializeBackend,
    decideMaterializeRecovery,
    resolveMaterializeDestPath,
    IsolatedMaterializer,
} from "cap:pkg";

test("isolated_materializer: default linker table & CLI --linker flag parsing") {
    // 对齐 docs/pm/isolated-installs.mdx 决策矩阵
    if (resolveDefaultLinkerMode(1, true) != IsolatedLinkerMode.Isolated) return err.V1WorkspaceMustBeIsolated;
    if (resolveDefaultLinkerMode(1, false) != IsolatedLinkerMode.Hoisted) return err.V1SingleMustBeHoisted;
    if (resolveDefaultLinkerMode(0, true) != IsolatedLinkerMode.Hoisted) return err.V0WorkspaceMustBeHoisted;
    if (resolveDefaultLinkerMode(0, false) != IsolatedLinkerMode.Hoisted) return err.V0SingleMustBeHoisted;

    if ((parseLinkerModeFlag("isolated")?) != IsolatedLinkerMode.Isolated) return err.ParseIsolatedFailed;
    if ((parseLinkerModeFlag("hoisted")?) != IsolatedLinkerMode.Hoisted) return err.ParseHoistedFailed;

    let bad_flag = false;
    * = parseLinkerModeFlag("pnpm") catch {
        bad_flag = true;
        IsolatedLinkerMode.Hoisted;
    };
    if (!bad_flag) return err.UnknownLinkerMustFail;
}

test("isolated_materializer: platform backend selection & fallback chain") {
    if (defaultMaterializeBackend(MaterializePlatform.MacOS) != MaterializeBackend.Clonefile) {
        return err.MacOsMustPreferClonefile;
    }
    if (defaultMaterializeBackend(MaterializePlatform.Linux) != MaterializeBackend.Hardlink) {
        return err.LinuxMustPreferHardlink;
    }
    if (defaultMaterializeBackend(MaterializePlatform.Windows) != MaterializeBackend.Hardlink) {
        return err.WinMustPreferHardlink;
    }

    if (b1 := fallbackMaterializeBackend(MaterializeBackend.Clonefile)) {
        if (b1 != MaterializeBackend.Hardlink) return err.ClonefileFallbackMustBeHardlink;
    } else {
        return err.MissingClonefileFallback;
    }
    if (b2 := fallbackMaterializeBackend(MaterializeBackend.Hardlink)) {
        if (b2 != MaterializeBackend.Copyfile) return err.HardlinkFallbackMustBeCopyfile;
    } else {
        return err.MissingHardlinkFallback;
    }
    if (fallbackMaterializeBackend(MaterializeBackend.Copyfile) != null) {
        return err.CopyfileHasNoFallback;
    }
}

test("isolated_materializer: EEXIST/ENOENT single-retry guard & no-parent failure") {
    // 首次 EEXIST -> DeleteTreeAndRetry；重试仍 EEXIST -> Fail
    const d_exist1 = decideMaterializeRecovery(MaterializeBackend.Clonefile, MaterializeErrno.Exist, true, false);
    if (d_exist1.action != MaterializeRecoveryAction.DeleteTreeAndRetry || !d_exist1.should_increment_retry) {
        return err.FirstExistMustRetry;
    }
    const d_exist2 = decideMaterializeRecovery(MaterializeBackend.Clonefile, MaterializeErrno.Exist, true, true);
    if (d_exist2.action != MaterializeRecoveryAction.Fail) return err.SecondExistMustFail;

    // 首次 ENOENT 且有父目录 -> MakeParentDirAndRetry；无父目录或已重试 -> Fail
    const d_noent1 = decideMaterializeRecovery(MaterializeBackend.Hardlink, MaterializeErrno.Noent, true, false);
    if (d_noent1.action != MaterializeRecoveryAction.MakeParentDirAndRetry || !d_noent1.should_increment_retry) {
        return err.FirstNoentWithParentMustRetry;
    }
    const d_noent_noparent = decideMaterializeRecovery(MaterializeBackend.Hardlink, MaterializeErrno.Noent, false, false);
    if (d_noent_noparent.action != MaterializeRecoveryAction.Fail) return err.NoParentNoentMustFail;

    const d_noent2 = decideMaterializeRecovery(MaterializeBackend.Hardlink, MaterializeErrno.Noent, true, true);
    if (d_noent2.action != MaterializeRecoveryAction.Fail) return err.SecondNoentMustFail;

    const d_access = decideMaterializeRecovery(MaterializeBackend.Hardlink, MaterializeErrno.Access, true, false);
    if (d_access.action != MaterializeRecoveryAction.Fail) return err.AccessErrMustFail;
}

test("isolated_materializer: stateful fallback Clonefile -> Hardlink -> Copyfile & counters") {
    let mat = IsolatedMaterializer.initForPlatform(MaterializePlatform.MacOS);
    if (mat.currentBackend() != MaterializeBackend.Clonefile) return err.InitialBackendMismatch;

    // 1 次成功 clone
    const r0 = mat.recordAttempt(MaterializeErrno.Ok, true, false);
    if (r0.action != MaterializeRecoveryAction.Succeeded) return err.ExpectedCloneOk;

    // ENOTSUP -> 自动降级至 Hardlink
    const r1 = mat.recordAttempt(MaterializeErrno.Notsup, true, false);
    if (r1.action != MaterializeRecoveryAction.FallbackBackend || mat.currentBackend() != MaterializeBackend.Hardlink) {
        return err.ExpectedFallbackToHardlink;
    }

    // EEXIST 重试后成功 hardlink
    * = mat.recordAttempt(MaterializeErrno.Exist, true, false);
    * = mat.recordAttempt(MaterializeErrno.Ok, true, true);

    // EXDEV -> 自动降级至 Copyfile
    const r2 = mat.recordAttempt(MaterializeErrno.Exdev, true, false);
    if (r2.action != MaterializeRecoveryAction.FallbackBackend || mat.currentBackend() != MaterializeBackend.Copyfile) {
        return err.ExpectedFallbackToCopyfile;
    }
    * = mat.recordAttempt(MaterializeErrno.Ok, true, false);

    // Copyfile 再遇 EXDEV 无可降级 -> Fail
    const r3 = mat.recordAttempt(MaterializeErrno.Exdev, true, false);
    if (r3.action != MaterializeRecoveryAction.Fail) return err.CopyfileExdevMustFail;

    const st = mat.stats();
    if (st.cloned_count != 1 ||
        st.hardlinked_count != 1 ||
        st.copied_count != 1 ||
        st.exist_retries != 1 ||
        st.backend_fallbacks != 2 ||
        st.failed_count != 1) {
        return err.MaterializeStatsMismatch;
    }
}

test("isolated_materializer: destination path resolution (global virtual store abs vs project rel)") {
    let out: [128]u8 = [0; 128];

    // 绝对路径（如 global virtual store）绝不叠加 cwd
    const abs_dest = "/home/user/.bun/install/cache/pkg@1.0.0";
    const n_abs = resolveMaterializeDestPath("/workspace/app", abs_dest, out[0..])?;
    if (n_abs != abs_dest.len || out[0] != '/' || out[1] != 'h') return err.AbsDestMustNotPrefixCwd;

    // Windows 盘符绝对路径同样不叠加 cwd
    const win_dest = "C:\\Users\\dev\\.bun\\cache\\pkg";
    const n_win = resolveMaterializeDestPath("D:\\project", win_dest, out[0..])?;
    if (n_win != win_dest.len || out[0] != 'C') return err.WinAbsDestMustNotPrefixCwd;

    // 相对路径正确拼接 cwd（自动去重 cwd 尾部斜杠）
    const n_rel = resolveMaterializeDestPath("/workspace/app/", "node_modules/.bun/foo", out[0..])?;
    // "/workspace/app" (14) + "/" (1) + "node_modules/.bun/foo" (21) = 36
    if (n_rel != 36 || out[14] != '/' || out[15] != 'n') return err.RelDestJoinMismatch;

    // 缓冲区过小拦截
    let tiny: [8]u8 = [0; 8];
    let small_err = false;
    * = resolveMaterializeDestPath("/workspace/app", "node_modules/pkg", tiny[0..]) catch {
        small_err = true;
        0;
    };
    if (!small_err) return err.SmallBufMustFail;
}

test("isolated_materializer: skip_dirnames dedup & capacity limit") {
    let mat = IsolatedMaterializer.initWithBackend(MaterializeBackend.Hardlink);

    * = mat.addSkipDir("node_modules")?;
    * = mat.addSkipDir(".git")?;
    // 重复添加幂等
    const c_dup = mat.addSkipDir("node_modules")?;
    if (c_dup != 2 || !mat.shouldSkipDir("node_modules") || !mat.shouldSkipDir(".git") || mat.shouldSkipDir("src")) {
        return err.SkipDirCheckFailed;
    }

    // 填满 16 个槽位，第 17 个拦截
    let nm: [4]u8 = ['d', '_', '0', '0'];
    let i: usize = 2;
    while (i < ISOLATED_MAX_SKIP_DIRS) {
        nm[2] = ('0' as u8) + ((i / 10) as u8);
        nm[3] = ('0' as u8) + ((i % 10) as u8);
        * = mat.addSkipDir(nm[0..])?;
        i += 1;
    }
    let overflow = false;
    * = mat.addSkipDir("extra_dir") catch {
        overflow = true;
        0;
    };
    if (!overflow) return err.SkipDirOverflowMustFail;
}
