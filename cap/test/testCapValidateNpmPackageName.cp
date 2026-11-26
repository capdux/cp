import { test, expect_eq } from "cap:test";
import {
    ERR_EMPTY, ERR_LEADING_DOT, ERR_LEADING_UNDERSCORE, ERR_WHITESPACE,
    ERR_INVALID_SCOPE, ERR_NON_URL_SAFE, ERR_BLACKLISTED,
    WARN_UPPERCASE, WARN_SPECIAL_CHARS, WARN_CORE_MODULE,
    validatePackageName, isValidNewPackageName
} from "../../packages/@cap/validate-npm-package-name/src/index.cp";

fn main() {
    test("1. @cap/validate-npm-package-name — accepts valid unscoped and scoped package names", () => {
        expect_eq(isValidNewPackageName("some-package"), true);
        expect_eq(isValidNewPackageName("example.com"), true);
        expect_eq(isValidNewPackageName("@cap/validate-npm-package-name"), true);
        expect_eq(isValidNewPackageName("@scope/pkg_1.2-beta"), true);
    });

    test("2. @cap/validate-npm-package-name — rejects empty, whitespace, leading dot/underscore", () => {
        expect_eq(validatePackageName("").error_code, ERR_EMPTY);
        expect_eq(validatePackageName(" foo").error_code, ERR_WHITESPACE);
        expect_eq(validatePackageName(".hidden").error_code, ERR_LEADING_DOT);
        expect_eq(validatePackageName("_internal").error_code, ERR_LEADING_UNDERSCORE);
    });

    test("3. @cap/validate-npm-package-name — rejects malformed scoped names & non-URL-safe chars", () => {
        expect_eq(validatePackageName("@scopeonly").error_code, ERR_INVALID_SCOPE);
        expect_eq(validatePackageName("@a/b/c").error_code, ERR_INVALID_SCOPE);
        expect_eq(validatePackageName("@/pkg").error_code, ERR_INVALID_SCOPE);
        expect_eq(validatePackageName("bad name").error_code, ERR_NON_URL_SAFE);
        expect_eq(validatePackageName("foo?bar").error_code, ERR_NON_URL_SAFE);
    });

    test("4. @cap/validate-npm-package-name — rejects blacklisted names case-insensitively", () => {
        let r1 = validatePackageName("node_modules");
        expect_eq(r1.valid_for_old, false);
        expect_eq(r1.error_code, ERR_BLACKLISTED);
        expect_eq(validatePackageName("Favicon.ICO").error_code, ERR_BLACKLISTED);
    });

    test("5. @cap/validate-npm-package-name — uppercase & special chars warn (valid_for_old=true, valid_for_new=false)", () => {
        let r1 = validatePackageName("MyPackage");
        expect_eq(r1.valid_for_old, true);
        expect_eq(r1.valid_for_new, false);
        expect_eq(r1.warning_code, WARN_UPPERCASE);

        let r2 = validatePackageName("cool~pkg!");
        expect_eq(r2.valid_for_old, true);
        expect_eq(r2.valid_for_new, false);
        expect_eq(r2.warning_code, WARN_SPECIAL_CHARS);
    });

    test("6. @cap/validate-npm-package-name — Node core module names trigger WARN_CORE_MODULE", () => {
        let r = validatePackageName("http");
        expect_eq(r.valid_for_old, true);
        expect_eq(r.valid_for_new, false);
        expect_eq(r.warning_code, WARN_CORE_MODULE);
    });
}
