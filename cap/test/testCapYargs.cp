// cap/test/testCapYargs.cp
// Comprehensive unit tests for @cap/yargs (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    OptionSpec,
    YargsApp,
    YargsResult,
} from "../../packages/@cap/yargs/src/index.cp";

fn containsSubstr(haystack: string, needle: string): bool {
    if (needle.len == 0) {
        return true;
    }
    if (needle.len > haystack.len) {
        return false;
    }
    let i: usize = 0;
    while (i + needle.len <= haystack.len) {
        let j: usize = 0;
        let ok = true;
        while (j < needle.len) {
            if (haystack[i + j] != needle[j]) {
                ok = false;
                break;
            }
            j = j + 1;
        }
        if (ok) {
            return true;
        }
        i = i + 1;
    }
    return false;
}

fn main() {
    test("1. @cap/yargs — subcommand match, option parsing & default fallbacks", () => {
        let app = YargsApp.new("cap-cli", "1.2.3");
        app.command("serve", "Start the server");
        app.command("build", "Build project");
        app.option(OptionSpec.intOpt("port", 112, "Port to listen on", 3000, false));
        app.option(OptionSpec.strOpt("host", 0, "Bind host", "127.0.0.1", false));
        app.option(OptionSpec.boolOpt("verbose", 118, "Verbose logs", false));

        const argv: [1]string = ["serve"];
        const res: YargsResult = app.parse(argv[0..1]);
        let buf: [64]u8 = undefined;

        expect_eq(res.ok, true);
        expect_eq(res.hasCommand(), true);
        const cLen = res.commandName(buf[0..64]);
        expect_eq(buf[0..cLen], "serve");

        // Defaults automatically injected
        expect_eq(res.getInt("port", 0), 3000 as i64);
        expect_eq(res.getBool("verbose", true), false);
        const hLen = res.getString("host", buf[0..64]);
        expect_eq(buf[0..(hLen as usize)], "127.0.0.1");
    });

    test("2. @cap/yargs — short alias -p 8080 maps to canonical long option port", () => {
        let app = YargsApp.new("cap-cli", "1.0.0");
        app.option(OptionSpec.intOpt("port", 112, "Listen port", 3000, false));
        app.option(OptionSpec.strOpt("config", 99, "Config path", "cap.json", false));

        const argv: [4]string = ["-p", "8080", "-c", "prod.json"];
        const res = app.parse(argv[0..4]);
        let buf: [64]u8 = undefined;

        expect_eq(res.ok, true);
        expect_eq(res.has("port"), true);
        expect_eq(res.getInt("port", 0), 8080 as i64);
        const cLen = res.getString("config", buf[0..64]);
        expect_eq(buf[0..(cLen as usize)], "prod.json");
    });

    test("3. @cap/yargs — required option validation failure & success", () => {
        let app = YargsApp.new("deploy", "2.0.0");
        app.option(OptionSpec.strOpt("token", 116, "API token", "", true));
        app.option(OptionSpec.strOpt("env", 101, "Target env", "staging", false));

        // Missing required option -> ok == false & localized error via @cap/y18n
        const missingArgv: [2]string = ["--env", "prod"];
        const failRes = app.parse(missingArgv[0..2]);
        expect_eq(failRes.ok, false);
        let errBuf: [128]u8 = undefined;
        const eLen = failRes.errorMessage(errBuf[0..128]);
        expect_eq(errBuf[0..eLen], "Missing required argument: token");

        // Provided via short alias `-t` -> ok == true
        const okArgv: [2]string = ["-t", "secret-xyz"];
        const okRes = app.parse(okArgv[0..2]);
        expect_eq(okRes.ok, true);
        let valBuf: [64]u8 = undefined;
        const vLen = okRes.getString("token", valBuf[0..64]);
        expect_eq(valBuf[0..(vLen as usize)], "secret-xyz");
    });

    test("4. @cap/yargs — --help / -h & --version / -v flag detection", () => {
        let app = YargsApp.new("mytool", "3.4.5");
        app.option(OptionSpec.strOpt("token", 116, "Required token", "", true));

        const h1: [1]string = ["--help"];
        const r1 = app.parse(h1[0..1]);
        expect_eq(r1.show_help, true);
        expect_eq(r1.show_version, false);
        expect_eq(r1.ok, true);

        const h2: [1]string = ["-h"];
        expect_eq(app.parse(h2[0..1]).show_help, true);

        const v1: [1]string = ["--version"];
        const r2 = app.parse(v1[0..1]);
        expect_eq(r2.show_version, true);
        expect_eq(r2.show_help, false);
        expect_eq(r2.ok, true);

        const v2: [1]string = ["-v"];
        expect_eq(app.parse(v2[0..1]).show_version, true);
    });

    test("5. @cap/yargs — renderHelp via @cap/cliui & @cap/y18n multi-column layout", () => {
        let app = YargsApp.new("cap-cli", "1.0.0");
        app.usage("Usage: cap-cli <command> [options]");
        app.command("serve", "Start HTTP server");
        app.command("test", "Run test suite");
        app.option(OptionSpec.intOpt("port", 112, "Listen port", 3000, false));
        app.option(OptionSpec.strOpt("token", 116, "Auth token", "", true));

        let helpBuf: [4096]u8 = undefined;
        const n = app.renderHelp(80, helpBuf[0..4096]);
        const helpStr = helpBuf[0..n];

        expect_eq(n > 100, true);
        expect_eq(containsSubstr(helpStr, "Usage: cap-cli <command> [options]"), true);
        expect_eq(containsSubstr(helpStr, "Commands:"), true);
        expect_eq(containsSubstr(helpStr, "cap-cli serve"), true);
        expect_eq(containsSubstr(helpStr, "Start HTTP server"), true);
        expect_eq(containsSubstr(helpStr, "Options:"), true);
        expect_eq(containsSubstr(helpStr, "-p, --port"), true);
        expect_eq(containsSubstr(helpStr, "[number]"), true);
        expect_eq(containsSubstr(helpStr, "-t, --token"), true);
        expect_eq(containsSubstr(helpStr, "[required]"), true);
        expect_eq(containsSubstr(helpStr, "-h, --help"), true);
        expect_eq(containsSubstr(helpStr, "-v, --version"), true);
    });

    test("6. @cap/yargs — multiple commands & empty argv safety", () => {
        let app = YargsApp.new("ctl", "0.1.0");
        expect_eq(app.command("init", "Initialize workspace"), true);
        expect_eq(app.command("deploy", "Deploy service"), true);
        expect_eq(app.command("status", "Inspect status"), true);
        expect_eq(app.option(OptionSpec.boolOpt("dry-run", 0, "Dry run mode", false)), true);

        const dummy: [1]string = ["unused"];
        const emptyRes = app.parse(dummy[0..0]);
        expect_eq(emptyRes.ok, true);
        expect_eq(emptyRes.hasCommand(), false);
        expect_eq(emptyRes.show_help, false);
        expect_eq(emptyRes.show_version, false);
        expect_eq(emptyRes.getBool("dry-run", true), false);

        // Unknown command does not match registered command
        const unkArgv: [1]string = ["unknown-cmd"];
        const unkRes = app.parse(unkArgv[0..1]);
        expect_eq(unkRes.hasCommand(), false);
        expect_eq(unkRes.positionalCount(), 1 as usize);

        expect_eq(ARG_BOOL, 1 as u8);
        expect_eq(ARG_INT, 2 as u8);
        expect_eq(ARG_STRING, 3 as u8);
    });
}
