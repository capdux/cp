import { test, expect_eq } from "cap:test";
import {
    yn,
    ynLenient,
    ynDefault,
} from "../../packages/@cap/yn/src/index.cp";

fn main() {
    test("1. @cap/yn — recognizes standard truthy values case-insensitively", () => {
        expect_eq(yn("y") ?? false, true);
        expect_eq(yn("YES") ?? false, true);
        expect_eq(yn("True") ?? false, true);
        expect_eq(yn("1") ?? false, true);
        expect_eq(yn("  on \n") ?? false, true);
    });

    test("2. @cap/yn — recognizes standard falsy values case-insensitively", () => {
        expect_eq(yn("n") ?? true, false);
        expect_eq(yn("NO") ?? true, false);
        expect_eq(yn("False") ?? true, false);
        expect_eq(yn("0") ?? true, false);
        expect_eq(yn("\toff ") ?? true, false);
    });

    test("3. @cap/yn — returns null for unrecognized values", () => {
        expect_eq(yn("") == null, true);
        expect_eq(yn("maybe") == null, true);
        expect_eq(yn("2") == null, true);
        expect_eq(yn("yes please") == null, true);
    });

    test("4. @cap/yn — ynLenient accepts common keyboard typos", () => {
        expect_eq(ynLenient("yse") ?? false, true);
        expect_eq(ynLenient("TURE") ?? false, true);
        expect_eq(ynLenient("noo") ?? true, false);
        expect_eq(ynLenient("flase") ?? true, false);
        expect_eq(ynLenient("unknown") == null, true);
    });

    test("5. @cap/yn — ynDefault falls back cleanly on unrecognized strings", () => {
        expect_eq(ynDefault("yes", false), true);
        expect_eq(ynDefault("no", true), false);
        expect_eq(ynDefault("invalid", true), true);
        expect_eq(ynDefault("", false), false);
    });

    test("6. @cap/yn — rejects overly long strings immediately", () => {
        expect_eq(yn("yesyesyesyesyes") == null, true);
    });
}
