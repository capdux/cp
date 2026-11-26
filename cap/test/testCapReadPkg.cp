import { test, expect_eq } from "cap:test";
import {
    ReadPkgResult,
    parsePackageJson,
    extractJsonStringField,
    extractJsonBoolField,
} from "../../packages/@cap/read-pkg/src/index.cp";

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
    test("1. @cap/read-pkg — extractJsonStringField and extractJsonBoolField skip nested objects", () => {
        let json: string = "{\"dependencies\":{\"name\":\"nested-dep\",\"private\":true},\"name\":\"@cap/read-pkg\",\"private\":false}";
        let name_opt: string? = extractJsonStringField(json, "name");
        expect_eq(name_opt != null, true);
        expect_eq(eqStr(name_opt ?? "", "@cap/read-pkg"), true);

        let priv_opt: bool? = extractJsonBoolField(json, "private");
        expect_eq(priv_opt != null, true);
        expect_eq(priv_opt ?? true, false);

        expect_eq(extractJsonStringField(json, "missing") == null, true);
        expect_eq(extractJsonBoolField(json, "name") == null, true);
    });

    test("2. @cap/read-pkg — parsePackageJson extracts and normalizes valid package", () => {
        let json: string = "{\"name\":\"@cap/read-pkg\",\"version\":\"  v1.2.3 \",\"license\":\"MIT\",\"repository\":\"cp-lang/cp\",\"main\":\"./src/index.cp\",\"private\":false}";
        let res: ReadPkgResult = parsePackageJson(json, true);
        expect_eq(res.parsed, true);
        expect_eq(eqStr(res.meta.name, "@cap/read-pkg"), true);
        expect_eq(eqStr(res.meta.version, "1.2.3"), true);
        expect_eq(eqStr(res.meta.license, "MIT"), true);
        expect_eq(eqStr(res.meta.repository, "cp-lang/cp"), true);
        expect_eq(eqStr(res.meta.main_entry, "src/index.cp"), true);
        expect_eq(res.meta.private_pkg, false);
        expect_eq(res.normalized.valid, true);
        expect_eq(res.normalized.has_warnings, false);
    });

    test("3. @cap/read-pkg — parsePackageJson with normalize=false preserves raw fields", () => {
        let json: string = "{\"name\":\"demo-pkg\",\"version\":\"v0.9.0\",\"main\":\"./dist/index.cp\"}";
        let res: ReadPkgResult = parsePackageJson(json, false);
        expect_eq(res.parsed, true);
        expect_eq(eqStr(res.meta.version, "v0.9.0"), true);
        expect_eq(eqStr(res.meta.main_entry, "./dist/index.cp"), true);
        expect_eq(res.normalized.valid, false);
    });

    test("4. @cap/read-pkg — supports object-form repository field", () => {
        let json: string = "{\"name\":\"my-lib\",\"version\":\"1.0.0\",\"license\":\"Apache-2.0\",\"repository\":{\"type\":\"git\",\"url\":\"https://github.com/cp-lang/cp.git\"}}";
        let res: ReadPkgResult = parsePackageJson(json, true);
        expect_eq(res.parsed, true);
        expect_eq(eqStr(res.meta.repository, "https://github.com/cp-lang/cp.git"), true);
        expect_eq(res.normalized.valid, true);
    });

    test("5. @cap/read-pkg — flags invalid package name or license during normalization", () => {
        let json: string = "{\"name\":\"Bad Name\",\"version\":\"1.0.0\",\"license\":\"InvalidLic\"}";
        let res: ReadPkgResult = parsePackageJson(json, true);
        expect_eq(res.parsed, true);
        expect_eq(res.normalized.valid, false);
        expect_eq(res.normalized.name_valid, false);
        expect_eq(res.normalized.license_valid, false);
    });

    test("6. @cap/read-pkg — rejects malformed or non-object JSON", () => {
        let r1: ReadPkgResult = parsePackageJson("", true);
        expect_eq(r1.parsed, false);

        let r2: ReadPkgResult = parsePackageJson("[\"not\", \"an\", \"object\"]", true);
        expect_eq(r2.parsed, false);

        let r3: ReadPkgResult = parsePackageJson("{\"name\":\"unclosed\"", true);
        expect_eq(r3.parsed, false);
    });
}
