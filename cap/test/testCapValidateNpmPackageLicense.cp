import { test, expect_eq } from "cap:test";
import {
    LicenseValidation,
    validateLicense,
    isValidNewLicense,
} from "../../packages/validate-npm-package-license/src/index.cp";

fn eqStr(a: string, b: string): bool {
    if (a.len != b.len) {
        return false;
    }
    let i: usize = 0;
    while (i < a.len) {
        if (a[i] != b[i]) {
            return false;
        }
        i += 1;
    }
    return true;
}

fn main() {
    test("1. @cap/validate-npm-package-license — accepts valid SPDX expressions", () => {
        let r1: LicenseValidation = validateLicense("MIT");
        expect_eq(r1.valid_for_new_packages, true);
        expect_eq(r1.valid_for_old_packages, true);
        expect_eq(r1.spdx, true);
        expect_eq(r1.unlicensed, false);
        expect_eq(eqStr(r1.in_file, ""), true);

        let r2: LicenseValidation = validateLicense("(MIT OR Apache-2.0) AND BSD-3-Clause");
        expect_eq(r2.valid_for_new_packages, true);
        expect_eq(r2.valid_for_old_packages, true);
        expect_eq(r2.spdx, true);
    });

    test("2. @cap/validate-npm-package-license — accepts UNLICENSED", () => {
        let r: LicenseValidation = validateLicense("UNLICENSED");
        expect_eq(r.valid_for_new_packages, true);
        expect_eq(r.valid_for_old_packages, true);
        expect_eq(r.spdx, false);
        expect_eq(r.unlicensed, true);
        expect_eq(eqStr(r.in_file, ""), true);
    });

    test("3. @cap/validate-npm-package-license — accepts SEE LICENSE IN <filename>", () => {
        let r1: LicenseValidation = validateLicense("SEE LICENSE IN LICENSE.txt");
        expect_eq(r1.valid_for_new_packages, true);
        expect_eq(r1.valid_for_old_packages, true);
        expect_eq(r1.spdx, false);
        expect_eq(r1.unlicensed, false);
        expect_eq(eqStr(r1.in_file, "LICENSE.txt"), true);

        // Empty filename or filename with spaces is rejected.
        let r2: LicenseValidation = validateLicense("SEE LICENSE IN ");
        expect_eq(r2.valid_for_new_packages, false);
        expect_eq(r2.valid_for_old_packages, false);

        let r3: LicenseValidation = validateLicense("SEE LICENSE IN my license.txt");
        expect_eq(r3.valid_for_new_packages, false);
        expect_eq(r3.valid_for_old_packages, false);
    });

    test("4. @cap/validate-npm-package-license — handles legacy old-package values", () => {
        let r1: LicenseValidation = validateLicense("BSD");
        expect_eq(r1.valid_for_new_packages, false);
        expect_eq(r1.valid_for_old_packages, true);
        expect_eq(r1.spdx, false);

        let r2: LicenseValidation = validateLicense("MIT/X11");
        expect_eq(r2.valid_for_new_packages, false);
        expect_eq(r2.valid_for_old_packages, true);

        let r3: LicenseValidation = validateLicense("WTFPL");
        expect_eq(r3.valid_for_new_packages, false);
        expect_eq(r3.valid_for_old_packages, true);
    });

    test("5. @cap/validate-npm-package-license — rejects empty and invalid strings", () => {
        let r1: LicenseValidation = validateLicense("");
        expect_eq(r1.valid_for_new_packages, false);
        expect_eq(r1.valid_for_old_packages, false);

        let r2: LicenseValidation = validateLicense("Proprietary-Custom");
        expect_eq(r2.valid_for_new_packages, false);
        expect_eq(r2.valid_for_old_packages, false);

        let r3: LicenseValidation = validateLicense("MIT or Apache-2.0");
        expect_eq(r3.valid_for_new_packages, false);
        expect_eq(r3.valid_for_old_packages, false);
    });

    test("6. @cap/validate-npm-package-license — isValidNewLicense helper", () => {
        expect_eq(isValidNewLicense("ISC"), true);
        expect_eq(isValidNewLicense("UNLICENSED"), true);
        expect_eq(isValidNewLicense("SEE LICENSE IN LICENSE.md"), true);
        expect_eq(isValidNewLicense("BSD"), false);
        expect_eq(isValidNewLicense("MIT/X11"), false);
        expect_eq(isValidNewLicense(""), false);
    });
}
