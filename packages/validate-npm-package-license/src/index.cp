// packages/@cap/validate-npm-package-license/src/index.cp
// Zero-allocation npm package license field validator for CP.
// Clean-room MIT pure CP implementation reusing `@cap/spdx-expression-parse`.

import { strEq, startsWith, isSpace } from "cap:ascii";
import { isValidSpdxExpression } from "../../spdx-expression-parse/src/index.cp";

/// Result of validating a `package.json` / `cap.json` license field.
pub struct LicenseValidation {
    pub valid_for_new_packages: bool,
    pub valid_for_old_packages: bool,
    pub spdx: bool,
    pub unlicensed: bool,
    pub in_file: string,
}

fn vnlContainsWhitespace(s: string): bool {
    let i: usize = 0;
    while (i < s.len) {
        if (isSpace(s[i])) {
            return true;
        }
        i += 1;
    }
    return false;
}

fn vnlIsLegacyLicense(license: string): bool {
    return strEq(license, "BSD")
        || strEq(license, "MIT/X11")
        || strEq(license, "WTFPL");
}

/// Validate an npm/cap package `license` field value.
pub fn validateLicense(license: string): LicenseValidation {
    if (license.len == 0) {
        return LicenseValidation {
            valid_for_new_packages: false,
            valid_for_old_packages: false,
            spdx: false,
            unlicensed: false,
            in_file: "",
        };
    }

    if (strEq(license, "UNLICENSED")) {
        return LicenseValidation {
            valid_for_new_packages: true,
            valid_for_old_packages: true,
            spdx: false,
            unlicensed: true,
            in_file: "",
        };
    }

    let see_prefix: string = "SEE LICENSE IN ";
    if (startsWith(license, see_prefix)) {
        let filename: string = license[see_prefix.len .. license.len];
        if (filename.len > 0 && !vnlContainsWhitespace(filename)) {
            return LicenseValidation {
                valid_for_new_packages: true,
                valid_for_old_packages: true,
                spdx: false,
                unlicensed: false,
                in_file: filename,
            };
        }
        return LicenseValidation {
            valid_for_new_packages: false,
            valid_for_old_packages: false,
            spdx: false,
            unlicensed: false,
            in_file: "",
        };
    }

    if (vnlIsLegacyLicense(license)) {
        return LicenseValidation {
            valid_for_new_packages: false,
            valid_for_old_packages: true,
            spdx: false,
            unlicensed: false,
            in_file: "",
        };
    }

    if (isValidSpdxExpression(license)) {
        return LicenseValidation {
            valid_for_new_packages: true,
            valid_for_old_packages: true,
            spdx: true,
            unlicensed: false,
            in_file: "",
        };
    }

    return LicenseValidation {
        valid_for_new_packages: false,
        valid_for_old_packages: false,
        spdx: false,
        unlicensed: false,
        in_file: "",
    };
}

/// Returns true if `license` is valid for newly published packages.
pub fn isValidNewLicense(license: string): bool {
    let res: LicenseValidation = validateLicense(license);
    return res.valid_for_new_packages;
}
