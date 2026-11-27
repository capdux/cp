import { test, expect_eq } from "cap:test";
import { hasFlag, findFlagIndex, getFlagValue, isFlagEnabled } from "../../packages/has-flag/src/index.cp";

fn main() {
    test("1. @cap/has-flag — Long and short flags with auto-prefix", () => {
        let argv: [4]string = ["-f", "--unicorn", "--foo=bar", "--rainbow"];
        let slice = argv[0..4];
        expect_eq(hasFlag("f", slice), true);
        expect_eq(hasFlag("-f", slice), true);
        expect_eq(hasFlag("unicorn", slice), true);
        expect_eq(hasFlag("--unicorn", slice), true);
        expect_eq(hasFlag("foo=bar", slice), true);
        expect_eq(hasFlag("foo", slice), true);
        expect_eq(hasFlag("missing", slice), false);
    });

    test("2. @cap/has-flag — Respects double-dash terminator", () => {
        let argv: [4]string = ["--before", "--", "--after", "-x"];
        let slice = argv[0..4];
        expect_eq(hasFlag("before", slice), true);
        expect_eq(hasFlag("after", slice), false);
        expect_eq(hasFlag("x", slice), false);
        expect_eq(findFlagIndex("before", slice), 0);
        expect_eq(findFlagIndex("after", slice), -1);
    });

    test("3. @cap/has-flag — Rejects wrong value on explicit equals", () => {
        let argv: [2]string = ["--mode=prod", "--port=8080"];
        let slice = argv[0..2];
        expect_eq(hasFlag("mode=prod", slice), true);
        expect_eq(hasFlag("mode=dev", slice), false);
        expect_eq(hasFlag("port=8080", slice), true);
        expect_eq(hasFlag("port=80", slice), false);
    });

    test("4. @cap/has-flag — getFlagValue extracts equals and space-separated values", () => {
        let argv: [4]string = ["--host=localhost", "--port", "3000", "--flag"];
        let slice = argv[0..4];
        let buf: [32]u8 = undefined;
        let n1 = getFlagValue("host", slice, buf[0..32]);
        expect_eq(n1, 9);
        expect_eq(buf[0], 108);
        expect_eq(buf[8], 116);

        let n2 = getFlagValue("port", slice, buf[0..32]);
        expect_eq(n2, 4);
        expect_eq(buf[0], 51);
        expect_eq(buf[3], 48);

        let n3 = getFlagValue("flag", slice, buf[0..32]);
        expect_eq(n3, -1);
    });

    test("5. @cap/has-flag — isFlagEnabled handles --flag and --no-flag overrides", () => {
        let argv1: [2]string = ["--color", "--no-color"];
        expect_eq(isFlagEnabled("color", argv1[0..2], true), false);

        let argv2: [2]string = ["--no-color", "--color"];
        expect_eq(isFlagEnabled("color", argv2[0..2], false), true);

        let argv3: [3]string = ["--color", "--", "--no-color"];
        expect_eq(isFlagEnabled("color", argv3[0..3], false), true);
    });

    test("6. @cap/has-flag — Handles empty flag and empty argv safely", () => {
        let empty: [1]string = [""];
        expect_eq(hasFlag("color", empty[0..0]), false);
        let argv: [1]string = ["--color"];
        expect_eq(hasFlag("", argv[0..1]), false);
        expect_eq(findFlagIndex("", argv[0..1]), -1);
        expect_eq(isFlagEnabled("color", empty[0..0], true), true);
    });
}
