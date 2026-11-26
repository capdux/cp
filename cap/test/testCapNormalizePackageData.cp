import { test, expect_eq } from "cap:test";
import {
    PackageMetadata,
    NormalizedPackageResult,
    expandRepoShorthand,
    normalizeEntryPath,
    cleanVersionString,
    normalizePackageData,
} from "../../packages/@cap/normalize-package-data/src/index.cp";

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
    test("1. @cap/normalize-package-data — expandRepoShorthand expands host shorthands and preserves URLs", () => {
        let buf: [128]u8 = undefined;

        let n1: usize = expandRepoShorthand("cp-lang/cp", buf[0..128]);
        expect_eq(eqStr(buf[0..n1], "https://github.com/cp-lang/cp.git"), true);

        let n2: usize = expandRepoShorthand("github:foo/bar", buf[0..128]);
        expect_eq(eqStr(buf[0..n2], "https://github.com/foo/bar.git"), true);

        let n3: usize = expandRepoShorthand("gitlab:org/proj", buf[0..128]);
        expect_eq(eqStr(buf[0..n3], "https://gitlab.com/org/proj.git"), true);

        let n4: usize = expandRepoShorthand("bitbucket:team/app", buf[0..128]);
        expect_eq(eqStr(buf[0..n4], "https://bitbucket.org/team/app.git"), true);

        let n5: usize = expandRepoShorthand("https://example.com/repo.git", buf[0..128]);
        expect_eq(eqStr(buf[0..n5], "https://example.com/repo.git"), true);
    });

    test("2. @cap/normalize-package-data — normalizeEntryPath strips leading ./ and extra slashes", () => {
        expect_eq(eqStr(normalizeEntryPath("./src/index.cp"), "src/index.cp"), true);
        expect_eq(eqStr(normalizeEntryPath("././dist/app.cp/"), "dist/app.cp"), true);
        expect_eq(eqStr(normalizeEntryPath("///lib/main.cp///"), "lib/main.cp"), true);
        expect_eq(eqStr(normalizeEntryPath("  ./index.cp  "), "index.cp"), true);
    });

    test("3. @cap/normalize-package-data — cleanVersionString strips whitespace and leading v/=", () => {
        expect_eq(eqStr(cleanVersionString("  v1.2.3  "), "1.2.3"), true);
        expect_eq(eqStr(cleanVersionString("=v2.0.0-alpha.1"), "2.0.0-alpha.1"), true);
        expect_eq(eqStr(cleanVersionString("V3.4.5"), "3.4.5"), true);
        expect_eq(eqStr(cleanVersionString("0.1.0"), "0.1.0"), true);
    });

    test("4. @cap/normalize-package-data — valid public package passes with zero warnings", () => {
        let meta: PackageMetadata = PackageMetadata {
            name: "@cap/normalize-package-data",
            version: "v1.0.0",
            license: "MIT",
            repository: "cp-lang/cp",
            main_entry: "./src/index.cp",
            private_pkg: false,
        };
        let res: NormalizedPackageResult = normalizePackageData(meta);
        expect_eq(res.valid, true);
        expect_eq(res.name_valid, true);
        expect_eq(res.license_valid, true);
        expect_eq(res.has_warnings, false);
        expect_eq(res.warning_count, 0);
    });

    test("5. @cap/normalize-package-data — invalid name and legacy license are flagged", () => {
        let meta: PackageMetadata = PackageMetadata {
            name: "Invalid Uppercase Name",
            version: "1.0.0",
            license: "BSD",
            repository: "",
            main_entry: "src/index.cp",
            private_pkg: false,
        };
        let res: NormalizedPackageResult = normalizePackageData(meta);
        expect_eq(res.valid, false);
        expect_eq(res.name_valid, false);
        expect_eq(res.license_valid, false);
        expect_eq(res.has_warnings, true);
        expect_eq(res.warning_count >= 2, true);
    });

    test("6. @cap/normalize-package-data — private package tolerates missing license and repository with warnings", () => {
        let meta: PackageMetadata = PackageMetadata {
            name: "internal-tool",
            version: "",
            license: "",
            repository: "",
            main_entry: "./src/main.cp",
            private_pkg: true,
        };
        let res: NormalizedPackageResult = normalizePackageData(meta);
        expect_eq(res.valid, true);
        expect_eq(res.name_valid, true);
        expect_eq(res.license_valid, true);
        expect_eq(res.has_warnings, true);
        expect_eq(res.warning_count, 2);
    });
}
