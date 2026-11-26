// cap/test/testCapSpdx.cp
// Comprehensive unit tests for @cap/spdx

import { test, expect_eq } from "cap:test";
import {
    LicenseCategory,
    LicenseInfo,
    lookupLicense,
    validateExpression,
    isPermissiveOnly,
    satisfies,
} from "../../packages/@cap/spdx/src/index.cp";

fn main() {
    test("1. @cap/spdx — lookupLicense metadata & OSI approval across categories", () => {
        const mit = lookupLicense("MIT");
        expect_eq(mit != null, true);
        if (mit != null) {
            const mitInfo = mit ?? LicenseInfo{ id: "", category: LicenseCategory.Unknown, isOsiApproved: false };
            expect_eq(mitInfo.id, "MIT");
            expect_eq(mitInfo.category == LicenseCategory.Permissive, true);
            expect_eq(mitInfo.isOsiApproved, true);
        }

        const unlic = lookupLicense("Unlicense");
        expect_eq(unlic != null, true);
        if (unlic != null) {
            const unlicInfo = unlic ?? LicenseInfo{ id: "", category: LicenseCategory.Unknown, isOsiApproved: false };
            expect_eq(unlicInfo.category == LicenseCategory.PublicDomain, true);
            expect_eq(unlicInfo.isOsiApproved, false);
        }

        const mpl = lookupLicense("MPL-2.0");
        expect_eq(mpl != null, true);
        if (mpl != null) {
            const mplInfo = mpl ?? LicenseInfo{ id: "", category: LicenseCategory.Unknown, isOsiApproved: false };
            expect_eq(mplInfo.category == LicenseCategory.WeakCopyleft, true);
            expect_eq(mplInfo.isOsiApproved, true);
        }

        const gpl = lookupLicense("GPL-2.0+");
        expect_eq(gpl != null, true);
        if (gpl != null) {
            const gplInfo = gpl ?? LicenseInfo{ id: "", category: LicenseCategory.Unknown, isOsiApproved: false };
            expect_eq(gplInfo.category == LicenseCategory.StrongCopyleft, true);
            expect_eq(gplInfo.isOsiApproved, true);
        }

        const agpl = lookupLicense("AGPL-3.0-only");
        expect_eq(agpl != null, true);
        if (agpl != null) {
            const agplInfo = agpl ?? LicenseInfo{ id: "", category: LicenseCategory.Unknown, isOsiApproved: false };
            expect_eq(agplInfo.category == LicenseCategory.NetworkCopyleft, true);
            expect_eq(agplInfo.isOsiApproved, true);
        }

        expect_eq(lookupLicense("Not-A-Real-License") == null, true);
    });

    test("2. @cap/spdx — validateExpression valid compound expressions, WITH, +, LicenseRef", () => {
        const r1 = validateExpression("(Apache-2.0 WITH LLVM-exception OR MIT) AND ISC");
        expect_eq(r1.valid, true);
        expect_eq(r1.allKnown, true);
        expect_eq(r1.licenseCount, 3 as usize);
        expect_eq(r1.licenses[0], "Apache-2.0");
        expect_eq(r1.licenses[1], "MIT");
        expect_eq(r1.licenses[2], "ISC");

        const r2 = validateExpression("GPL-2.0+ OR LGPL-2.1-or-later");
        expect_eq(r2.valid, true);
        expect_eq(r2.allKnown, true);
        expect_eq(r2.licenseCount, 2 as usize);
        expect_eq(r2.licenses[0], "GPL-2.0+");
        expect_eq(r2.licenses[1], "LGPL-2.1-or-later");

        const r3 = validateExpression("MIT OR LicenseRef-Proprietary-1.0");
        expect_eq(r3.valid, true);
        expect_eq(r3.allKnown, false);
        expect_eq(r3.licenseCount, 2 as usize);

        const r4 = validateExpression("DocumentRef-spdx-tool-1.2:LicenseRef-MIT-Style-2");
        expect_eq(r4.valid, true);
        expect_eq(r4.allKnown, false);
        expect_eq(r4.licenseCount, 1 as usize);
    });

    test("3. @cap/spdx — validateExpression rejects malformed syntax", () => {
        expect_eq(validateExpression("").valid, false);
        expect_eq(validateExpression("   ").valid, false);
        expect_eq(validateExpression("MIT OR").valid, false);
        expect_eq(validateExpression("AND MIT").valid, false);
        expect_eq(validateExpression("(MIT OR ISC").valid, false);
        expect_eq(validateExpression("MIT OR ISC)").valid, false);
        expect_eq(validateExpression("()").valid, false);
        expect_eq(validateExpression("MIT Apache-2.0").valid, false);
        expect_eq(validateExpression("MIT WITH").valid, false);
        expect_eq(validateExpression("MIT WITH OR").valid, false);
        expect_eq(validateExpression("MIT +").valid, false);
        expect_eq(validateExpression("GPL-2.0++").valid, false);
        expect_eq(validateExpression("LicenseRef-Foo+").valid, false);
        expect_eq(validateExpression("MIT or ISC").valid, false);
    });

    test("4. @cap/spdx — isPermissiveOnly with operator precedence (AND > OR)", () => {
        expect_eq(isPermissiveOnly("MIT"), true);
        expect_eq(isPermissiveOnly("CC0-1.0"), true);
        expect_eq(isPermissiveOnly("Unlicense AND BlueOak-1.0.0"), true);
        expect_eq(isPermissiveOnly("MIT OR GPL-3.0-only"), true);
        expect_eq(isPermissiveOnly("MIT AND GPL-3.0-only"), false);
        expect_eq(isPermissiveOnly("(Apache-2.0 OR MIT) AND ISC"), true);
        expect_eq(isPermissiveOnly("(Apache-2.0 OR GPL-3.0-only) AND BSD-3-Clause"), true);
        // Precedence: MIT OR (GPL-3.0-only AND AGPL-3.0-only) => true
        expect_eq(isPermissiveOnly("MIT OR GPL-3.0-only AND AGPL-3.0-only"), true);
        // Precedence: (MIT OR GPL-3.0-only) AND AGPL-3.0-only => false
        expect_eq(isPermissiveOnly("(MIT OR GPL-3.0-only) AND AGPL-3.0-only"), false);
        expect_eq(isPermissiveOnly("MPL-2.0"), false);
        expect_eq(isPermissiveOnly("Custom-Unknown-1.0"), false);
        expect_eq(isPermissiveOnly("MIT OR"), false);
    });

    test("5. @cap/spdx — satisfies against allowlists (AND/OR/parentheses)", () => {
        const allowPerm: [3]string = ["MIT", "Apache-2.0", "ISC"];
        expect_eq(satisfies("MIT", allowPerm[0..3]), true);
        expect_eq(satisfies("BSD-3-Clause", allowPerm[0..3]), false);
        expect_eq(satisfies("BSD-3-Clause OR MIT", allowPerm[0..3]), true);
        expect_eq(satisfies("(Apache-2.0 OR GPL-3.0-only) AND ISC", allowPerm[0..3]), true);
        expect_eq(satisfies("Apache-2.0 AND BSD-2-Clause", allowPerm[0..3]), false);
        // Precedence: ISC OR (BSD-3-Clause AND GPL-3.0-only) => true
        expect_eq(satisfies("ISC OR BSD-3-Clause AND GPL-3.0-only", allowPerm[0..3]), true);
        // Precedence: (ISC OR BSD-3-Clause) AND GPL-3.0-only => false
        expect_eq(satisfies("(ISC OR BSD-3-Clause) AND GPL-3.0-only", allowPerm[0..3]), false);
    });

    test("6. @cap/spdx — satisfies with WITH exceptions and + / -or-later equivalence", () => {
        const allowExc: [3]string = [
            "Apache-2.0 WITH LLVM-exception",
            "GPL-2.0-or-later",
            "LGPL-3.0+",
        ];
        expect_eq(satisfies("Apache-2.0 WITH LLVM-exception", allowExc[0..3]), true);
        expect_eq(satisfies("Apache-2.0  WITH  LLVM-exception", allowExc[0..3]), true);
        expect_eq(satisfies("Apache-2.0 WITH Classpath-exception-2.0", allowExc[0..3]), false);
        expect_eq(satisfies("GPL-2.0+", allowExc[0..3]), true);
        expect_eq(satisfies("LGPL-3.0-or-later", allowExc[0..3]), true);
        expect_eq(satisfies("GPL-2.0+ AND Apache-2.0 WITH LLVM-exception", allowExc[0..3]), true);
        expect_eq(satisfies("MIT OR (GPL-2.0+ AND LGPL-3.0-or-later)", allowExc[0..3]), true);
    });

    test("7. @cap/spdx — deduplication of repeated licenses & additional metadata coverage", () => {
        const res = validateExpression("(MIT OR MIT) AND (Zlib OR BSL-1.0 OR 0BSD OR PostgreSQL OR curl OR FTL OR WTFPL OR CC-BY-4.0 OR CC-BY-SA-4.0 OR OpenSSL OR NCSA OR Python-2.0 OR EUPL-1.2 OR EPL-2.0 OR CDDL-1.0 OR Artistic-2.0)");
        expect_eq(res.valid, true);
        expect_eq(res.allKnown, true);
        expect_eq(res.licenseCount, 17 as usize);
        expect_eq(res.getLicense(0 as usize) ?? "", "MIT");
        expect_eq(res.getLicense(99 as usize) == null, true);
    });
}
