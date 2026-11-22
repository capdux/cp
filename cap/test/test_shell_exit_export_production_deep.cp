// cap/test/test_shell_exit_export_production_deep.cp
// Deep production tests for cap:shell POSIX exit code & export declaration codec (exit_export.cp)

import { test, expect_eq, expect_true, expect_false } from "cap:test";
import {
    SHELL_EXIT_SUCCESS,
    SHELL_EXIT_GENERAL_ERROR,
    SHELL_EXIT_MISUSE,
    SHELL_EXIT_NOT_EXECUTABLE,
    SHELL_EXIT_NOT_FOUND,
    SHELL_EXIT_SIGNAL_BASE,
    isValidShellIdentifier,
    parseExitCodeStr,
    parseExitArgs,
    exitCodeFromSignal,
    isSignalExitCode,
    parseExportArg,
    formatExportLine,
} from "cap:shell";

test("shell.exit_export: parseExitCodeStr mod 256 positive, negative & arbitrary huge ints", () => {
    expect_eq(SHELL_EXIT_SUCCESS, 0);
    expect_eq(SHELL_EXIT_GENERAL_ERROR, 1);
    expect_eq(SHELL_EXIT_MISUSE, 2);
    expect_eq(SHELL_EXIT_NOT_EXECUTABLE, 126);
    expect_eq(SHELL_EXIT_NOT_FOUND, 127);
    expect_eq(SHELL_EXIT_SIGNAL_BASE, 128);

    const c0 = parseExitCodeStr("0") catch { expect_true(false); return; };
    const c42 = parseExitCodeStr("42") catch { expect_true(false); return; };
    const c255 = parseExitCodeStr("255") catch { expect_true(false); return; };
    const c256 = parseExitCodeStr("256") catch { expect_true(false); return; };
    const c257 = parseExitCodeStr("257") catch { expect_true(false); return; };
    const c1000 = parseExitCodeStr("1000") catch { expect_true(false); return; };
    const cplus = parseExitCodeStr("+7") catch { expect_true(false); return; };
    const cneg1 = parseExitCodeStr("-1") catch { expect_true(false); return; };
    const cneg256 = parseExitCodeStr("-256") catch { expect_true(false); return; };
    // Huge integer > u64 max: 256 * 10^25 + 13 -> mod 256 == 13 (10^25 is divisible by 256 since 10^8 % 256 == 0)
    const chuge = parseExitCodeStr("10000000000000000000000013") catch { expect_true(false); return; };

    expect_eq(c0, 0);
    expect_eq(c42, 42);
    expect_eq(c255, 255);
    expect_eq(c256, 0);
    expect_eq(c257, 1);
    expect_eq(c1000, 232);
    expect_eq(cplus, 7);
    expect_eq(cneg1, 255);
    expect_eq(cneg256, 0);
    expect_eq(chuge, 13);
});

test("shell.exit_export: parseExitArgs 0/1/many arguments and signal exit codes", () => {
    const empty: [0][]const u8 = [];
    const one: [1][]const u8 = ["130"];
    const two: [2][]const u8 = ["0", "1"];

    const r0 = parseExitArgs(empty[0..], 7) catch { expect_true(false); return; };
    expect_eq(r0, 7);

    const r1 = parseExitArgs(one[0..], 0) catch { expect_true(false); return; };
    expect_eq(r1, 130);
    expect_true(isSignalExitCode(r1));

    // SIGINT (2) -> 130, SIGTERM (15) -> 143
    expect_eq(exitCodeFromSignal(2), 130);
    expect_eq(exitCodeFromSignal(15), 143);
    expect_false(isSignalExitCode(128));
    expect_false(isSignalExitCode(0));

    let too_many = false;
    * = parseExitArgs(two[0..], 0) catch { too_many = true; };
    expect_true(too_many);
});

test("shell.exit_export: isValidShellIdentifier POSIX rules", () => {
    expect_true(isValidShellIdentifier("PATH"));
    expect_true(isValidShellIdentifier("_PRIVATE_1"));
    expect_true(isValidShellIdentifier("a"));
    expect_true(isValidShellIdentifier("foo_bar_99"));

    expect_false(isValidShellIdentifier(""));
    expect_false(isValidShellIdentifier("1ABC"));
    expect_false(isValidShellIdentifier("FOO-BAR"));
    expect_false(isValidShellIdentifier("FOO.BAR"));
    expect_false(isValidShellIdentifier("FOO=BAR"));
    expect_false(isValidShellIdentifier(" A"));
});

test("shell.exit_export: parseExportArg assignment, empty value & mark-only forms", () => {
    const e1 = parseExportArg("NODE_ENV=production") catch { expect_true(false); return; };
    expect_eq(e1.name, "NODE_ENV");
    expect_eq(e1.value, "production");
    expect_true(e1.has_value);

    // Value containing '=' signs is preserved intact
    const e2 = parseExportArg("FLAGS=--a=1 --b=2") catch { expect_true(false); return; };
    expect_eq(e2.name, "FLAGS");
    expect_eq(e2.value, "--a=1 --b=2");
    expect_true(e2.has_value);

    // Empty assigned value
    const e3 = parseExportArg("EMPTY_VAR=") catch { expect_true(false); return; };
    expect_eq(e3.name, "EMPTY_VAR");
    expect_eq(e3.value, "");
    expect_true(e3.has_value);

    // Mark-only export without '='
    const e4 = parseExportArg("EXISTING_VAR") catch { expect_true(false); return; };
    expect_eq(e4.name, "EXISTING_VAR");
    expect_eq(e4.value, "");
    expect_false(e4.has_value);
});

test("shell.exit_export: formatExportLine and malformed input error guards", () => {
    let buf: [64]u8 = [0; 64];
    const line = formatExportLine(buf[0..], "APP_PORT", "8080") catch { expect_true(false); return; };
    expect_eq(line, "APP_PORT=8080\n");

    // Non-numeric exit strings rejected
    let b1 = false;
    * = parseExitCodeStr("") catch { b1 = true; };
    expect_true(b1);

    let b2 = false;
    * = parseExitCodeStr("abc") catch { b2 = true; };
    expect_true(b2);

    let b3 = false;
    * = parseExitCodeStr("-") catch { b3 = true; };
    expect_true(b3);

    let b4 = false;
    * = parseExitCodeStr("12x") catch { b4 = true; };
    expect_true(b4);

    // Invalid export identifiers rejected
    let b5 = false;
    * = parseExportArg("") catch { b5 = true; };
    expect_true(b5);

    let b6 = false;
    * = parseExportArg("=value") catch { b6 = true; };
    expect_true(b6);

    let b7 = false;
    * = parseExportArg("9BAD=1") catch { b7 = true; };
    expect_true(b7);

    let b8 = false;
    * = parseExportArg("BAD-NAME") catch { b8 = true; };
    expect_true(b8);

    // BufferTooSmall & invalid identifier on formatExportLine
    let tiny: [5]u8 = [0; 5];
    let b9 = false;
    * = formatExportLine(tiny[0..], "KEY", "VALUE") catch { b9 = true; };
    expect_true(b9);

    let b10 = false;
    * = formatExportLine(buf[0..], "1BAD", "val") catch { b10 = true; };
    expect_true(b10);
});
