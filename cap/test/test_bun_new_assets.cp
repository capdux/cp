// cap/test/test_bun_new_assets.cp
// Comprehensive E2E Regression Suite for Newly Absorbed High-Value Bun Assets:
// 1. cap:errno  (SystemErrno cross-platform POSIX/Linux error codes & predicates)
// 2. cap:logger (LogLevel, Loc, Range, Source, DiagMsg, Log plain & ANSI formatting)
// 3. cap:string (wcwidth & stringWidth / Bun.stringWidth parity)

const std = @import("std");
import { test, expect_eq } from "cap:test";
import { SystemErrno } from "cap:errno";
import { LogLevel, Loc, Range, LineCol, Source, DiagMsg, Log } from "cap:logger";
import { wcwidth, stringWidth, StringWidth } from "cap:string";

fn main() {
    test("1. cap:errno — SystemErrno fromCode/toCode, name, description & predicates", () => {
        // Positive & negative Linux syscall return conversion
        const e_noent = SystemErrno.fromCode(2);
        expect_eq(e_noent == SystemErrno.ENOENT, true);
        expect_eq(SystemErrno.fromCode(-2) == SystemErrno.ENOENT, true);
        expect_eq(e_noent.toCode(), 2 as i32);
        expect_eq(std.mem.eql(u8, e_noent.name, "ENOENT"), true);
        expect_eq(std.mem.eql(u8, e_noent.codeName(), "ENOENT"), true);
        expect_eq(std.mem.eql(u8, SystemErrno.name(e_noent), "ENOENT"), true);
        expect_eq(std.mem.eql(u8, e_noent.description(), "No such file or directory"), true);
        expect_eq(e_noent.isNotFound(), true);
        expect_eq(e_noent.isRetryable(), false);
        expect_eq(e_noent.isPermissionDenied(), false);

        // Retryable errors
        const e_again = SystemErrno.fromCode(11);
        expect_eq(e_again == SystemErrno.EAGAIN, true);
        expect_eq(e_again.isRetryable(), true);
        expect_eq(SystemErrno.EINTR.isRetryable(), true);
        expect_eq(SystemErrno.EWOULDBLOCK.isRetryable(), true);
        expect_eq(SystemErrno.ETIMEDOUT.isRetryable(), true);

        // Permission denied errors
        expect_eq(SystemErrno.EPERM.isPermissionDenied(), true);
        expect_eq(SystemErrno.EACCES.isPermissionDenied(), true);
        expect_eq(SystemErrno.EROFS.isPermissionDenied(), true);

        // Network errors
        const e_refused = SystemErrno.fromCode(-111);
        expect_eq(e_refused == SystemErrno.ECONNREFUSED, true);
        expect_eq(std.mem.eql(u8, e_refused.codeName(), "ECONNREFUSED"), true);
        expect_eq(e_refused.isNetworkError(), true);
        expect_eq(SystemErrno.EADDRINUSE.isNetworkError(), true);
        expect_eq(SystemErrno.ECONNRESET.isNetworkError(), true);

        // Success & Unknown
        expect_eq(SystemErrno.fromCode(0).isOk(), true);
        const e_unk = SystemErrno.fromCode(9999);
        expect_eq(e_unk == SystemErrno.UNKNOWN, true);
        expect_eq(std.mem.eql(u8, e_unk.codeName(), "UNKNOWN"), true);
    });

    test("2. cap:logger — Source lineColOf, Loc, Range & DiagMsg Plain/ANSI Formatting", () => {
        const code = "let alpha = 10;\nlet beta = unknown_var + 1;\nreturn beta;\n";
        const src = Source.init("src/app.cp", code);

        // Locate "unknown_var" on line 2 (byte offset 27, len 11)
        const target_off: usize = std.mem.indexOf(u8, code, "unknown_var").?;
        const lc = src.lineColOf(target_off);
        expect_eq(lc.line, 2 as usize);
        expect_eq(lc.col, 12 as usize);
        expect_eq(std.mem.eql(u8, src.lineTextOf(target_off), "let beta = unknown_var + 1;"), true);

        const rng = Range.init(target_off as i32, 11);
        expect_eq(rng.isEmpty(), false);
        expect_eq(std.mem.eql(u8, rng.sliceOf(code), "unknown_var"), true);

        const loc_empty = Loc.empty();
        expect_eq(loc_empty.isEmpty(), true);

        const msg = DiagMsg.fromSource(
            LogLevel.Err,
            src,
            rng,
            "undefined identifier 'unknown_var'",
            "did you mean 'alpha'?",
        );
        expect_eq(msg.line, 2 as usize);
        expect_eq(msg.col, 12 as usize);

        let plain_buf: [512]u8 = [0; 512];
        const plain = msg.formatPlain(plain_buf[0..512]);
        expect_eq(std.mem.indexOf(u8, plain, "error: undefined identifier 'unknown_var'") != null, true);
        expect_eq(std.mem.indexOf(u8, plain, "--> src/app.cp:2:12") != null, true);
        expect_eq(std.mem.indexOf(u8, plain, "^~~~~~~~~~~") != null, true);
        expect_eq(std.mem.indexOf(u8, plain, "= note: did you mean 'alpha'?") != null, true);

        let ansi_buf: [512]u8 = [0; 512];
        const ansi = msg.formatAnsi(ansi_buf[0..512]);
        expect_eq(std.mem.indexOf(u8, ansi, "\x1b[1;31merror\x1b[0m") != null, true);
    });

    test("3. cap:logger — Log Accumulator, Level Filtering, Counters & formatAll", () => {
        const code = "const x = 1;\nconst y = 2;\n";
        const src = Source.init("main.cp", code);

        let log = Log.init(LogLevel.Warn);
        log.addDebug("filtered out debug");
        log.addInfo("filtered out info");
        expect_eq(log.count, 0 as usize);
        expect_eq(log.hasErrors(), false);

        log.addWarning(src, Loc.init(6), "unused variable 'x'");
        log.addErrorRange(src, Range.init(19, 1), "reassignment to const 'y'", "change to 'let y'");

        expect_eq(log.warnings, 1 as usize);
        expect_eq(log.errors, 1 as usize);
        expect_eq(log.count, 2 as usize);
        expect_eq(log.hasErrors(), true);

        let out_buf: [1024]u8 = [0; 1024];
        const report = log.formatAll(out_buf[0..1024], false);
        expect_eq(std.mem.indexOf(u8, report, "warn: unused variable 'x'") != null, true);
        expect_eq(std.mem.indexOf(u8, report, "error: reassignment to const 'y'") != null, true);

        log.clear();
        expect_eq(log.count, 0 as usize);
        expect_eq(log.errors, 0 as usize);
        expect_eq(log.warnings, 0 as usize);
    });

    test("4. cap:string — wcwidth & stringWidth (Bun.stringWidth Parity)", () => {
        // wcwidth codepoint checks
        expect_eq(wcwidth('A' as u32), 1 as u8);
        expect_eq(wcwidth(0x000A), 0 as u8); // LF control char
        expect_eq(wcwidth(0x0301), 0 as u8); // Combining acute accent
        expect_eq(wcwidth(0x200D), 0 as u8); // ZWJ
        expect_eq(wcwidth(0x00AD), 1 as u8); // Soft hyphen (terminal width 1)
        expect_eq(wcwidth(0x4E2D), 2 as u8); // '中'
        expect_eq(wcwidth(0xFF01), 2 as u8); // Fullwidth '！'
        expect_eq(wcwidth(0x1F680), 2 as u8); // '🚀'

        // ASCII & CJK mixed width (also verifies default arg `count_ansi = false`)
        expect_eq(stringWidth("Hello, CP!"), 10 as usize);
        expect_eq(stringWidth("你好CP"), 6 as usize); // 2*2 + 2 = 6
        expect_eq(stringWidth("Hello 世界 🚀", false), 13 as usize); // 6 + 4 + 1 + 2 = 13

        // Combining diacritic ('e' + U+0301 combining acute => width 1)
        expect_eq(stringWidth("cafe\xCC\x81", false), 4 as usize);

        // Emoji ZWJ family sequence ("👨‍👩‍👧" => 2 columns)
        expect_eq(stringWidth("👨‍👩‍👧", false), 2 as usize);

        // ANSI CSI color escape skipping vs counting
        const colored = "\x1b[1;32m成功\x1b[0m: OK";
        expect_eq(stringWidth(colored, false), 8 as usize); // "成功" (4) + ": OK" (4)
        expect_eq(StringWidth.measure(colored), 8 as usize);
        expect_eq(stringWidth(colored, true) > 8, true);

        // ANSI OSC hyperlink sequence skipping
        const link = "\x1b]8;;https://cp.dev\x07CP文档\x1b]8;;\x07";
        expect_eq(stringWidth(link, false), 6 as usize); // "CP" (2) + "文档" (4)
    });
}
