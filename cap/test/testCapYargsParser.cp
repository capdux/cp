// cap/test/testCapYargsParser.cp
// Comprehensive unit tests for @cap/yargs-parser (6 test suites)

import { test, expect_eq } from "cap:test";
import {
    ARG_BOOL,
    ARG_INT,
    ARG_STRING,
    ParsedArgs,
    parseArgs,
} from "../../packages/@cap/yargs-parser/src/index.cp";

fn main() {
    test("1. @cap/yargs-parser — long options (--key=val, --key val, --flag, --no-flag)", () => {
        const argv: [6]string = [
            "--host=127.0.0.1",
            "--port",
            "8080",
            "--verbose",
            "--no-color",
            "--debug=false",
        ];
        const p: ParsedArgs = parseArgs(argv[0..6]);
        let buf: [64]u8 = undefined;

        expect_eq(p.has("host"), true);
        const hLen = p.getString("host", buf[0..64]);
        expect_eq(hLen, 9 as i64);
        expect_eq(buf[0..(hLen as usize)], "127.0.0.1");

        expect_eq(p.getInt("port", 0), 8080 as i64);
        expect_eq(p.getBool("verbose", false), true);
        expect_eq(p.getBool("color", true), false);
        expect_eq(p.getBool("debug", true), false);
        expect_eq(p.has("missing"), false);
        expect_eq(p.getInt("missing", 42), 42 as i64);
    });

    test("2. @cap/yargs-parser — short options, combined flags -abc & -p 3000", () => {
        const argv: [4]string = ["-abc", "-p", "3000", "-e=prod"];
        const p: ParsedArgs = parseArgs(argv[0..4]);
        let buf: [64]u8 = undefined;

        expect_eq(p.getBool("a", false), true);
        expect_eq(p.getBool("b", false), true);
        expect_eq(p.getBool("c", false), true);
        expect_eq(p.getInt("p", 0), 3000 as i64);

        const eLen = p.getString("e", buf[0..64]);
        expect_eq(buf[0..(eLen as usize)], "prod");
    });

    test("3. @cap/yargs-parser — @cap/camelcase & @cap/decamelize dual key lookup", () => {
        const argv: [3]string = ["--max-retries=5", "--no-dry-run", "--outputDir=/tmp/out"];
        const p: ParsedArgs = parseArgs(argv[0..3]);
        let buf: [64]u8 = undefined;

        // Query kebab option via camelCase and raw kebab-case
        expect_eq(p.has("max-retries"), true);
        expect_eq(p.has("maxRetries"), true);
        expect_eq(p.getInt("maxRetries", 0), 5 as i64);
        expect_eq(p.getInt("max-retries", 0), 5 as i64);

        // Negated kebab flag queried via camelCase
        expect_eq(p.has("dryRun"), true);
        expect_eq(p.getBool("dryRun", true), false);

        // CamelCase input queried via decamelized kebab-case
        expect_eq(p.has("output-dir"), true);
        const oLen = p.getString("output-dir", buf[0..64]);
        expect_eq(buf[0..(oLen as usize)], "/tmp/out");
    });

    test("4. @cap/yargs-parser — negative numbers (-10) & positional arguments", () => {
        const argv: [5]string = ["build", "-10", "--offset", "-42", "target.cp"];
        const p: ParsedArgs = parseArgs(argv[0..5]);
        let buf: [64]u8 = undefined;

        expect_eq(p.getInt("offset", 0), -42 as i64);
        expect_eq(p.has("1"), false);
        expect_eq(p.has("4"), false);

        expect_eq(p.positionalCount(), 3 as usize);
        const n0 = p.positionalAt(0, buf[0..64]);
        expect_eq(buf[0..n0], "build");
        const n1 = p.positionalAt(1, buf[0..64]);
        expect_eq(buf[0..n1], "-10");
        const n2 = p.positionalAt(2, buf[0..64]);
        expect_eq(buf[0..n2], "target.cp");
    });

    test("5. @cap/yargs-parser — -- terminator passthrough into rest & positionals", () => {
        const argv: [6]string = ["--mode=fast", "run", "--", "--raw-flag", "-x", "123"];
        const p: ParsedArgs = parseArgs(argv[0..6]);
        let buf: [64]u8 = undefined;

        expect_eq(p.has("mode"), true);
        expect_eq(p.has("raw-flag"), false);
        expect_eq(p.has("x"), false);

        expect_eq(p.restCount(), 3 as usize);
        const r0 = p.restAt(0, buf[0..64]);
        expect_eq(buf[0..r0], "--raw-flag");
        const r1 = p.restAt(1, buf[0..64]);
        expect_eq(buf[0..r1], "-x");
        const r2 = p.restAt(2, buf[0..64]);
        expect_eq(buf[0..r2], "123");

        // Positionals include `run` + 3 passthrough items
        expect_eq(p.positionalCount(), 4 as usize);
        const p0 = p.positionalAt(0, buf[0..64]);
        expect_eq(buf[0..p0], "run");
        const p1 = p.positionalAt(1, buf[0..64]);
        expect_eq(buf[0..p1], "--raw-flag");
    });

    test("6. @cap/yargs-parser — empty argv & last-wins duplicate flag override", () => {
        const one: [1]string = ["ignored"];
        const empty: ParsedArgs = parseArgs(one[0..0]);
        expect_eq(empty.entryCount(), 0 as usize);
        expect_eq(empty.positionalCount(), 0 as usize);
        expect_eq(empty.restCount(), 0 as usize);
        expect_eq(empty.has("any"), false);

        const dupArgv: [5]string = ["--port=3000", "--port=9090", "--verbose", "--no-verbose", "--maxRetries=8"];
        const p: ParsedArgs = parseArgs(dupArgv[0..5]);
        expect_eq(p.getInt("port", 0), 9090 as i64);
        expect_eq(p.getBool("verbose", true), false);
        expect_eq(p.getInt("max-retries", 0), 8 as i64);
        expect_eq(p.entryCount(), 3 as usize);
        expect_eq(ARG_BOOL, 1 as u8);
        expect_eq(ARG_INT, 2 as u8);
        expect_eq(ARG_STRING, 3 as u8);
    });
}
