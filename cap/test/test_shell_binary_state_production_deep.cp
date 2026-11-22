// cap/test/test_shell_binary_state_production_deep.cp
// 深度生产级回归测试：cap:shell POSIX `&&` / `||` 短路二元逻辑链状态机 (lib/shell/binary_state.cp)

import { test } from "cap:test";
import {
    SHELL_BINARY_MAX_TERMS,
    BinaryLogicOp,
    BinaryStepKind,
    shouldShortCircuitBinary,
    ShellBinaryStateMachine,
} from "cap:shell";

test("shell_binary_state: pure short-circuit predicate truth table") {
    if (shouldShortCircuitBinary(BinaryLogicOp.And, 0)) return err.AndZeroMustNotSkip;
    if (!shouldShortCircuitBinary(BinaryLogicOp.And, 1)) return err.AndNonZeroMustSkip;
    if (!shouldShortCircuitBinary(BinaryLogicOp.And, 127)) return err.And127MustSkip;
    if (!shouldShortCircuitBinary(BinaryLogicOp.Or, 0)) return err.OrZeroMustSkip;
    if (shouldShortCircuitBinary(BinaryLogicOp.Or, 1)) return err.OrNonZeroMustNotSkip;
    if (shouldShortCircuitBinary(BinaryLogicOp.None, 99)) return err.NoneMustNeverSkip;
}

test("shell_binary_state: POSIX equal-precedence left-associative AND-OR chain") {
    // false(10) && skip(20) || recover(30) && finish(40) || skip(50)
    let bm = ShellBinaryStateMachine.init();
    * = bm.setFirstTerm(10, false)?;
    * = bm.pushTerm(BinaryLogicOp.And, 20, false)?;
    * = bm.pushTerm(BinaryLogicOp.Or, 30, false)?;
    * = bm.pushTerm(BinaryLogicOp.And, 40, false)?;
    * = bm.pushTerm(BinaryLogicOp.Or, 50, false)?;

    const s1 = bm.nextStep()?;
    if (s1.kind != BinaryStepKind.RunTerm || s1.term_id != 10) return err.ExpectedTerm10;
    * = bm.onTermDone(2)?;

    // 20 被 && 短路，直接执行 30
    const s2 = bm.nextStep()?;
    if (s2.kind != BinaryStepKind.RunTerm || s2.term_id != 30 || s2.skipped_count != 1) {
        return err.ExpectedTerm30AfterSkip;
    }
    * = bm.onTermDone(0)?;

    // 30 成功 -> 执行 40
    const s3 = bm.nextStep()?;
    if (s3.kind != BinaryStepKind.RunTerm || s3.term_id != 40) return err.ExpectedTerm40;
    * = bm.onTermDone(0)?;

    // 40 成功 -> 50 被 || 短路，整个链以退出码 0 结束
    const done = bm.nextStep()?;
    if (done.kind != BinaryStepKind.Completed ||
        done.exit_code != 0 ||
        !bm.isDone() ||
        bm.executedCount() != 3 ||
        bm.skippedCount() != 2) {
        return err.ChainDoneSummaryMismatch;
    }
}

test("shell_binary_state: pure && failure propagates exact non-zero exit code") {
    let bm = ShellBinaryStateMachine.init();
    * = bm.setFirstTerm(1, false)?;
    * = bm.pushTerm(BinaryLogicOp.And, 2, false)?;
    * = bm.pushTerm(BinaryLogicOp.And, 3, false)?;

    * = bm.nextStep()?; // term 1
    * = bm.onTermDone(42)?;

    const done = bm.nextStep()?;
    if (done.kind != BinaryStepKind.Completed ||
        done.exit_code != 42 ||
        bm.finalExitCode() != 42 ||
        bm.executedCount() != 1 ||
        bm.skippedCount() != 2) {
        return err.AndFailureExitPropagationMismatch;
    }
}

test("shell_binary_state: pure || fallback chain executes until first zero exit") {
    let bm = ShellBinaryStateMachine.init();
    * = bm.setFirstTerm(100, false)?;
    * = bm.pushTerm(BinaryLogicOp.Or, 200, false)?;
    * = bm.pushTerm(BinaryLogicOp.Or, 300, false)?;

    const a = bm.nextStep()?;
    if (a.term_id != 100) return err.Expected100;
    * = bm.onTermDone(1)?;

    const b = bm.nextStep()?;
    if (b.term_id != 200) return err.Expected200;
    * = bm.onTermDone(0)?; // 成功 -> 300 被短路

    const done = bm.nextStep()?;
    if (done.kind != BinaryStepKind.Completed || done.exit_code != 0 || bm.skippedCount() != 1) {
        return err.OrFallbackMismatch;
    }
}

test("shell_binary_state: empty child nodes resolve to exit code 0 (Bun makeChild == null)") {
    let bm = ShellBinaryStateMachine.init();
    // empty_left && real_right(77) && empty_tail
    * = bm.setFirstTerm(0, true)?;
    * = bm.pushTerm(BinaryLogicOp.And, 77, false)?;
    * = bm.pushTerm(BinaryLogicOp.And, 0, true)?;

    // 首项为空直接视为 0 -> 立即进入 77
    const s1 = bm.nextStep()?;
    if (s1.kind != BinaryStepKind.RunTerm || s1.term_id != 77) return err.ExpectedReal77;
    * = bm.onTermDone(0)?;

    // 末项为空直接视为 0 -> 结束
    const done = bm.nextStep()?;
    if (done.kind != BinaryStepKind.Completed || done.exit_code != 0 || bm.executedCount() != 1) {
        return err.EmptyTailCompletionMismatch;
    }
}

test("shell_binary_state: protocol guards & fixed-capacity 32-term bound") {
    let empty_bm = ShellBinaryStateMachine.init();
    let empty_err = false;
    * = empty_bm.nextStep() catch { empty_err = true; };
    if (!empty_err) return err.EmptyChainMustFail;

    // 未设置首项时不能 pushTerm
    let no_first_err = false;
    * = empty_bm.pushTerm(BinaryLogicOp.And, 1, false) catch {
        no_first_err = true;
        0;
    };
    if (!no_first_err) return err.PushBeforeFirstMustFail;

    // 协议守卫：TermStillPending / NotAwaitingTerm / AlreadyStarted
    let bm = ShellBinaryStateMachine.init();
    * = bm.setFirstTerm(1, false)?;
    let not_await = false;
    * = bm.onTermDone(0) catch {
        not_await = true;
        false;
    };
    if (!not_await) return err.NotAwaitingMustFail;

    * = bm.nextStep()?;
    let pending = false;
    * = bm.nextStep() catch { pending = true; };
    if (!pending) return err.PendingStepMustFail;

    let started = false;
    * = bm.pushTerm(BinaryLogicOp.And, 2, false) catch {
        started = true;
        0;
    };
    if (!started) return err.PushAfterStartMustFail;

    // 容量上限 32 项
    let full_bm = ShellBinaryStateMachine.init();
    * = full_bm.setFirstTerm(0, false)?;
    let i: usize = 1;
    while (i < SHELL_BINARY_MAX_TERMS) {
        * = full_bm.pushTerm(BinaryLogicOp.And, i as u32, false)?;
        i += 1;
    }
    let overflow = false;
    * = full_bm.pushTerm(BinaryLogicOp.And, 999, false) catch {
        overflow = true;
        0;
    };
    if (!overflow) return err.TooManyTermsMustFail;
}
