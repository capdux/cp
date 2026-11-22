// cap/test/test_pkg_install_progress_production_deep.cp
// 深度生产级回归测试：cap:pkg 包安装 5 阶段进度条渲染与节点计数器 (lib/pkg/install_progress.cp)

import { test } from "cap:test";
import {
    InstallProgressStage,
    stagePlainLabel,
    stageEmojiPrefix,
    stageBannerLabel,
    InstallProgressTracker,
} from "cap:pkg";

test("install_progress: 5-stage plain vs ANSI banner labels match Bun ProgressStrings") {
    const dl_plain = stageBannerLabel(InstallProgressStage.Download, false);
    const dl_ansi = stageBannerLabel(InstallProgressStage.Download, true);
    if (dl_plain.len != 10 || dl_plain[9] != '\n') return err.DownloadPlainMustEndNewline;
    if (dl_ansi.len <= 10 || dl_ansi[dl_ansi.len - 1] == '\n') return err.DownloadAnsiNoNewline;

    // Save 阶段在无 ANSI 模式下不追加换行（对齐 Bun save_no_emoji）
    const save_plain = stageBannerLabel(InstallProgressStage.Save, false);
    if (save_plain.len != 15 || save_plain[14] != 'e') return err.SavePlainNoNewline;

    const ex_plain = stagePlainLabel(InstallProgressStage.Extract);
    if (ex_plain.len != 22) return err.ExtractPlainLenMismatch;
    if (stageEmojiPrefix(InstallProgressStage.Install).len == 0) return err.EmptyEmojiPrefix;
}

test("install_progress: idempotent startIfNone, stage transitions & finish snap") {
    let pt = InstallProgressTracker.init(false, false);

    // 首次启动返回 true，二次调用幂等返回 false
    const started1 = pt.startIfNone(20, 15, 5)?;
    const started2 = pt.startIfNone(20, 10, 5)?;
    if (!started1 || started2 || !pt.isActive()) return err.StartIdempotencyFailed;
    // total = 20 + 5 = 25, completed = 20 - 15 = 5 => 20%
    if (pt.estimatedTotal() != 25 || pt.completed() != 5 || pt.percentComplete() != 20) {
        return err.InitialCountsMismatch;
    }

    // 切换到 Extract -> Install -> Save
    * = pt.transitionStage(InstallProgressStage.Extract, 20, 10, 5)?;
    if (pt.currentStage() != InstallProgressStage.Extract || pt.completed() != 10 || pt.percentComplete() != 40) {
        return err.ExtractStageMismatch;
    }

    * = pt.transitionStage(InstallProgressStage.Install, 20, 5, 5)?;
    let line: [128]u8 = [0; 128];
    const n = pt.formatStatusLine(line[0..])?;
    // "Installing [15/25]" == 18 bytes
    if (n != 18 || line[0] != 'I' || line[11] != '[' || line[17] != ']') {
        return err.StatusLineFormatMismatch;
    }

    // finish 将 completed 对齐至 estimatedTotal 并标记 finished
    if (!pt.finish() || pt.finish()) return err.FinishIdempotencyFailed;
    if (!pt.isFinished() || pt.isActive() || pt.completed() != 25 || pt.percentComplete() != 100) {
        return err.FinishStateMismatch;
    }
}

test("install_progress: incremental setNodeName preserves emoji prefix in ANSI mode") {
    let pt = InstallProgressTracker.init(true, false);
    const emoji = stageEmojiPrefix(InstallProgressStage.Download);
    const n1 = pt.setNodeName("Resolving", emoji, true)?;
    if (n1 != emoji.len + 9) return err.FirstAnsiNodeNameLenMismatch;

    // is_first=false 复用前导 emoji 字节，仅更新尾部名称
    const n2 = pt.setNodeName("lodash@4.17.21", "", false)?;
    if (n2 != emoji.len + 14) return err.SecondAnsiNodeNameLenMismatch;
    const nm = pt.nodeName();
    if (nm[0] != ' ' || nm[1] != ' ' || nm[emoji.len] != 'l' || nm[nm.len - 1] != '1') {
        return err.EmojiPrefixPreservationFailed;
    }
}

test("install_progress: --silent mode suppresses activation cleanly") {
    let pt = InstallProgressTracker.init(true, true);
    const started = pt.startIfNone(10, 5, 0)?;
    if (started || pt.isActive() || pt.refreshCount() != 0) {
        return err.SilentModeMustNotActivate;
    }
}

test("install_progress: zero-total percentComplete guard & error boundaries") {
    let pt = InstallProgressTracker.init(false, false);
    if (pt.percentComplete() != 0) return err.IdlePercentMustBeZero;

    // 未启动调用 transitionStage -> NotStarted
    let not_started = false;
    * = pt.transitionStage(InstallProgressStage.Install, 1, 0, 0) catch {
        not_started = true;
        0;
    };
    if (!not_started) return err.ExpectedNotStarted;

    // pending > total -> PendingExceedsTotal
    let bad_pending = false;
    * = pt.startIfNone(5, 6, 0) catch {
        bad_pending = true;
        false;
    };
    if (!bad_pending) return err.ExpectedPendingExceedsTotal;

    // 0 任务启动后直接 finish -> 100% 且不除零
    * = pt.startIfNone(0, 0, 0)?;
    if (pt.percentComplete() != 0) return err.ZeroTotalActiveShouldBe0;
    * = pt.finish();
    if (pt.percentComplete() != 100) return err.ZeroTotalFinishedShouldBe100;
}

test("install_progress: NameTooLong & BufferTooSmall safety guards") {
    let pt = InstallProgressTracker.init(false, false);
    let long_name: [140]u8 = ['a'; 140];
    let too_long = false;
    * = pt.setNodeName(long_name[0..], "", true) catch {
        too_long = true;
        0;
    };
    if (!too_long) return err.ExpectedNameTooLong;

    * = pt.startIfNone(10, 2, 0)?;
    let tiny: [5]u8 = [0; 5];
    let buf_small = false;
    * = pt.formatStatusLine(tiny[0..]) catch {
        buf_small = true;
        0;
    };
    if (!buf_small) return err.ExpectedBufferTooSmall;
}
