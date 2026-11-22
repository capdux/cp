// cap/test/test_shell_yes_stream_production_deep.cp
// 深度生产级回归测试：cap:shell `yes` 8KiB 块倍增生成器与协同让出状态机 (lib/shell/yes_stream.cp)

import { test } from "cap:test";
import {
    YES_BUFSIZ,
    YES_HALF_BUFSIZ,
    YES_CHUNKS_PER_TICK,
    YesState,
    YesTickAction,
    computeYesLineLen,
    computeYesLayout,
    fillYesChunk,
    YesStreamState,
} from "cap:shell";

test("shell_yes: default no-args generates full 8KiB slab of 'y\\n'") {
    const empty: [0]([]const u8) = [];
    const line_len = computeYesLineLen(empty[0..])?;
    if (line_len != 2) return err.ExpectedDefaultLineLen2;

    const layout = computeYesLayout(empty[0..], YES_BUFSIZ)?;
    if (layout.copies_per_chunk != 4096 || layout.chunk_len != 8192) {
        return err.UnexpectedDefaultLayout;
    }

    let ys = YesStreamState.init(empty[0..])?;
    const chunk = ys.chunkSlice();
    if (chunk.len != 8192 || ys.copiesPerChunk() != 4096) return err.UnexpectedDefaultChunk;
    // 首尾与中段均严格为 "y\n"
    if (chunk[0] != 'y' || chunk[1] != '\n' ||
        chunk[4096] != 'y' || chunk[4097] != '\n' ||
        chunk[8190] != 'y' || chunk[8191] != '\n') {
        return err.DefaultPatternMismatch;
    }
}

test("shell_yes: multi-arg space join & whole-line alignment without splitting") {
    const args: [3]([]const u8) = ["alpha", "beta", "gamma"];
    // "alpha beta gamma\n" => 5 + 1 + 4 + 1 + 5 + 1 = 17 bytes
    const line_len = computeYesLineLen(args[0..])?;
    if (line_len != 17) return err.ExpectedLineLen17;

    let ys = YesStreamState.init(args[0..])?;
    const chunk = ys.chunkSlice();
    // 8192 / 17 = 481 copies => 481 * 17 = 8177 bytes（绝不截断半行）
    if (ys.copiesPerChunk() != 481 || chunk.len != 8177) {
        return err.WholeLineAlignmentMismatch;
    }
    if (chunk[0] != 'a' || chunk[5] != ' ' || chunk[10] != ' ' || chunk[16] != '\n') {
        return err.FirstLineContentMismatch;
    }
    // 末行结尾同样以 '\n' 结束，且开头为 'a'
    const last_off = 480 * 17;
    if (chunk[last_off] != 'a' || chunk[chunk.len - 1] != '\n') {
        return err.LastLineContentMismatch;
    }
}

test("shell_yes: single line > HALF_BUFSIZ emits exactly 1 copy without doubling") {
    let big: [5000]u8 = ['z'; 5000];
    const args: [1]([]const u8) = [big[0..]];
    // line_len = 5001 > 4096 (YES_HALF_BUFSIZ)
    const layout = computeYesLayout(args[0..], YES_BUFSIZ)?;
    if (layout.line_len != 5001 || layout.copies_per_chunk != 1 || layout.chunk_len != 5001) {
        return err.ExpectedSingleCopyAboveHalfBufsiz;
    }

    let ys = YesStreamState.init(args[0..])?;
    const chunk = ys.chunkSlice();
    if (chunk.len != 5001 || chunk[0] != 'z' || chunk[4999] != 'z' || chunk[5000] != '\n') {
        return err.BigLineContentMismatch;
    }
}

test("shell_yes: cooperative yield every 4 chunks (32KiB) & event-loop resume") {
    const empty: [0]([]const u8) = [];
    let ys = YesStreamState.init(empty[0..])?;

    // 第一轮：连续 4 次 WriteChunk，第 5 次必须 YieldToEventLoop
    let step: usize = 0;
    while (step < YES_CHUNKS_PER_TICK) {
        if (ys.nextStep() != YesTickAction.WriteChunk) return err.ExpectedWriteChunk;
        ys.onChunkWritten(ys.chunkSlice().len);
        step += 1;
    }
    if (ys.nextStep() != YesTickAction.YieldToEventLoop) return err.ExpectedYieldAfter4Chunks;
    if (ys.currentState() != YesState.Suspended) return err.ExpectedSuspendedState;
    if (ys.totalChunksEmitted() != 4 || ys.totalBytesEmitted() != 32768 || ys.totalLinesEmitted() != 16384) {
        return err.FirstTickCountersMismatch;
    }

    // 处于 Suspended 时再次 nextStep 仍保持 YieldToEventLoop
    if (ys.nextStep() != YesTickAction.YieldToEventLoop) return err.StillSuspendedMustYield;

    // 唤醒后再跑一轮 4 块
    if (!ys.resumeFromEventLoop()) return err.ResumeShouldSucceed;
    if (ys.resumeFromEventLoop()) return err.ResumeWhenRunningShouldBeFalse;

    step = 0;
    while (step < YES_CHUNKS_PER_TICK) {
        if (ys.nextStep() != YesTickAction.WriteChunk) return err.ExpectedSecondTickWrite;
        ys.onChunkWritten(ys.chunkSlice().len);
        step += 1;
    }
    if (ys.nextStep() != YesTickAction.YieldToEventLoop) return err.ExpectedSecondTickYield;
    if (ys.totalChunksEmitted() != 8 || ys.totalBytesEmitted() != 65536) {
        return err.SecondTickCountersMismatch;
    }
}

test("shell_yes: EPIPE graceful completion vs fatal write error exit code") {
    const empty: [0]([]const u8) = [];
    let ys_pipe = YesStreamState.init(empty[0..])?;
    * = ys_pipe.nextStep();
    const ec_pipe = ys_pipe.onWriteError(true);
    if (ec_pipe != 0 || ys_pipe.exitCode() != 0 || ys_pipe.currentState() != YesState.Done) {
        return err.BrokenPipeShouldExitZero;
    }
    if (ys_pipe.nextStep() != YesTickAction.Done) return err.DoneStateMustReturnDone;

    let ys_err = YesStreamState.init(empty[0..])?;
    * = ys_err.nextStep();
    const ec_err = ys_err.onWriteError(false);
    if (ec_err != 1 || ys_err.exitCode() != 1 || ys_err.currentState() != YesState.Failed) {
        return err.FatalWriteErrShouldExitOne;
    }
    if (ys_err.nextStep() != YesTickAction.WriteError) return err.FailedStateMustReturnWriteError;
}

test("shell_yes: small external buffer fill & BufferTooSmall / LineTooLong boundaries") {
    const args: [1]([]const u8) = ["ok"]; // "ok\n" = 3 bytes
    let small: [10]u8 = [0; 10];
    // 10 / 3 = 3 copies = 9 bytes
    const n = fillYesChunk(args[0..], small[0..])?;
    if (n != 9 || small[0] != 'o' || small[2] != '\n' || small[6] != 'o' || small[8] != '\n') {
        return err.SmallBufFillMismatch;
    }

    // 缓冲小于单行长度 (2 < 3) -> BufferTooSmall
    let tiny: [2]u8 = [0; 2];
    let too_small = false;
    * = fillYesChunk(args[0..], tiny[0..]) catch {
        too_small = true;
        0;
    };
    if (!too_small) return err.ExpectedBufferTooSmall;

    // 单行长度超过 YES_BUFSIZ (8193 > 8192) -> LineTooLong
    let huge: [8192]u8 = ['x'; 8192];
    const huge_args: [1]([]const u8) = [huge[0..]];
    let line_too_long = false;
    * = YesStreamState.init(huge_args[0..]) catch {
        line_too_long = true;
        YesStreamState{};
    };
    if (!line_too_long) return err.ExpectedLineTooLong;
}
