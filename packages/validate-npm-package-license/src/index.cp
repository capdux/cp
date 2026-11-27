// packages/@cap/validate-npm-package-license/src/index.cp
// Zero-allocation npm package license field validator for CP.
// Clean-room MIT pure CP implementation reusing `@cap/spdx-expression-parse`.

import { strEq, startsWith, isSpace } from "cap:ascii";
import {
    isValidSpdxExpression,
    parseSpdxExpression,
    SpdxAst,
    SPDX_NODE_LICENSE,
    SPDX_NODE_AND,
    SPDX_NODE_OR,
} from "../../spdx-expression-parse/src/index.cp";
import { lookupLicense } from "../../spdx/src/index.cp";

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

/// Returns true if `license` is valid for legacy/existing packages.
pub fn isValidOldLicense(license: string): bool {
    let res: LicenseValidation = validateLicense(license);
    return res.valid_for_old_packages;
}

/// Returns true if a single SPDX license identifier is OSI-approved.
pub fn isOsiApprovedLicense(license_id: string): bool {
    const info_opt = lookupLicense(license_id);
    if (info_opt == null) {
        return false;
    }
    const info = info_opt ?? return false;
    return info.isOsiApproved;
}

/// Returns true if a single SPDX license identifier is FSF-libre.
pub fn isFsfLibreLicense(license_id: string): bool {
    return strEq(license_id, "MIT")
        || strEq(license_id, "MIT-0")
        || strEq(license_id, "Apache-2.0")
        || strEq(license_id, "BSD-2-Clause")
        || strEq(license_id, "BSD-3-Clause")
        || strEq(license_id, "ISC")
        || strEq(license_id, "Zlib")
        || strEq(license_id, "Unlicense")
        || strEq(license_id, "CC0-1.0")
        || strEq(license_id, "0BSD")
        || strEq(license_id, "BSL-1.0")
        || strEq(license_id, "WTFPL")
        || strEq(license_id, "MPL-2.0")
        || strEq(license_id, "EPL-2.0")
        || strEq(license_id, "Artistic-2.0")
        || strEq(license_id, "GPL-2.0-only")
        || strEq(license_id, "GPL-2.0-or-later")
        || strEq(license_id, "GPL-3.0-only")
        || strEq(license_id, "GPL-3.0-or-later")
        || strEq(license_id, "LGPL-2.1-only")
        || strEq(license_id, "LGPL-2.1-or-later")
        || strEq(license_id, "LGPL-3.0-only")
        || strEq(license_id, "LGPL-3.0-or-later")
        || strEq(license_id, "AGPL-3.0-only")
        || strEq(license_id, "AGPL-3.0-or-later");
}

fn vnlEvalOsiAst(ast: SpdxAst, idx: i32, depth: usize): bool {
    if (idx < 0 || depth >= 32) {
        return false;
    }
    const u_idx = idx as usize;
    if (u_idx >= ast.node_count) {
        return false;
    }
    const node = ast.nodes[u_idx];
    if (node.kind == SPDX_NODE_LICENSE) {
        return isOsiApprovedLicense(node.license);
    }
    if (node.kind == SPDX_NODE_AND) {
        return vnlEvalOsiAst(ast, node.left, depth + 1) && vnlEvalOsiAst(ast, node.right, depth + 1);
    }
    if (node.kind == SPDX_NODE_OR) {
        return vnlEvalOsiAst(ast, node.left, depth + 1) || vnlEvalOsiAst(ast, node.right, depth + 1);
    }
    return false;
}

fn vnlEvalFsfAst(ast: SpdxAst, idx: i32, depth: usize): bool {
    if (idx < 0 || depth >= 32) {
        return false;
    }
    const u_idx = idx as usize;
    if (u_idx >= ast.node_count) {
        return false;
    }
    const node = ast.nodes[u_idx];
    if (node.kind == SPDX_NODE_LICENSE) {
        return isFsfLibreLicense(node.license);
    }
    if (node.kind == SPDX_NODE_AND) {
        return vnlEvalFsfAst(ast, node.left, depth + 1) && vnlEvalFsfAst(ast, node.right, depth + 1);
    }
    if (node.kind == SPDX_NODE_OR) {
        return vnlEvalFsfAst(ast, node.left, depth + 1) || vnlEvalFsfAst(ast, node.right, depth + 1);
    }
    return false;
}

/// Evaluates whether an SPDX expression (`AND` / `OR` / `WITH`) satisfies OSI-approved licensing.
pub fn isOsiApprovedExpression(expr: string): bool {
    const ast = parseSpdxExpression(expr);
    if (!ast.valid || ast.root < 0) {
        return false;
    }
    return vnlEvalOsiAst(ast, ast.root, 0);
}

/// Evaluates whether an SPDX expression (`AND` / `OR` / `WITH`) satisfies FSF-libre licensing.
pub fn isFsfLibreExpression(expr: string): bool {
    const ast = parseSpdxExpression(expr);
    if (!ast.valid || ast.root < 0) {
        return false;
    }
    return vnlEvalFsfAst(ast, ast.root, 0);
}

/// Extended license inspection result including OSI/FSF flags and SPDX AST metadata.
pub struct LicenseDetails {
    pub valid_for_new_packages: bool,
    pub valid_for_old_packages: bool,
    pub spdx: bool,
    pub unlicensed: bool,
    pub in_file: string,
    pub osi_approved: bool,
    pub fsf_libre: bool,
    pub has_compound_expr: bool,
    pub has_exception: bool,
}

/// Performs full validation and SPDX AST inspection (OSI, FSF, `AND`/`OR`, `WITH`).
pub fn inspectLicense(license: string): LicenseDetails {
    const base = validateLicense(license);
    let osi = false;
    let fsf = false;
    let compound = false;
    let has_exc = false;

    if (base.spdx) {
        const ast = parseSpdxExpression(license);
        if (ast.valid && ast.root >= 0) {
            osi = vnlEvalOsiAst(ast, ast.root, 0);
            fsf = vnlEvalFsfAst(ast, ast.root, 0);
            let i: usize = 0;
            while (i < ast.node_count) {
                const n = ast.nodes[i];
                if (n.kind == SPDX_NODE_AND || n.kind == SPDX_NODE_OR) {
                    compound = true;
                }
                if (n.exception.len > 0) {
                    has_exc = true;
                }
                i += 1;
            }
        }
    }

    return LicenseDetails {
        valid_for_new_packages: base.valid_for_new_packages,
        valid_for_old_packages: base.valid_for_old_packages,
        spdx: base.spdx,
        unlicensed: base.unlicensed,
        in_file: base.in_file,
        osi_approved: osi,
        fsf_libre: fsf,
        has_compound_expr: compound,
        has_exception: has_exc,
    };
}
