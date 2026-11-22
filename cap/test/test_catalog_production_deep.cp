// Source: cp/cap/test/test_catalog_production_deep.cp (Production deep tests for cap:catalog)

import std from "std";
import { test, expect_eq } from "cap:test";
import {
    CatalogError,
    CatalogEntry,
    CatalogMap,
    isCatalogProtocol,
    isValidCatalogName,
    parseCatalogRef,
    formatCatalogRef,
} from "cap:catalog";

test("1. Protocol parsing, validation & formatting (catalog: vs catalog:<group>)", () => {
    expect_eq(isCatalogProtocol("catalog:"), true);
    expect_eq(isCatalogProtocol("catalog:testing"), true);
    expect_eq(isCatalogProtocol("^19.0.0"), false);

    if (r_def := parseCatalogRef("catalog:")) {
        expect_eq(r_def.is_default, true);
        expect_eq(r_def.catalog_name.len, 0);
    } else { expect_eq(true, false); }

    if (r_named := parseCatalogRef("catalog:testing")) {
        expect_eq(r_named.is_default, false);
        expect_eq(std.mem.eql(u8, r_named.catalog_name, "testing"), true);
    } else { expect_eq(true, false); }

    let buf: [32]u8 = [0; 32];
    let out: []u8 = buf[0..];
    if (f1 := formatCatalogRef("", *out)) {
        expect_eq(std.mem.eql(u8, f1, "catalog:"), true);
    } else { expect_eq(true, false); }
    if (f2 := formatCatalogRef("react18", *out)) {
        expect_eq(std.mem.eql(u8, f2, "catalog:react18"), true);
    } else { expect_eq(true, false); }

    expect_eq(isValidCatalogName("testing-v2"), true);
    expect_eq(isValidCatalogName(""), false);
    expect_eq(isValidCatalogName("bad/name"), false);
    expect_eq(isValidCatalogName("bad name"), false);
    expect_eq(parseCatalogRef("catalog:bad:name") == null, true);
});

test("2. Default & named catalogs isolation, upsert & resolveSpecifier passthrough", () => {
    let storage: [8]CatalogEntry = [{ catalog_name: "", package_name: "", version: "" }; 8];
    let map = CatalogMap.init(storage[0..]);
    expect_eq(map.hasAny(), false);

    expect_eq(map.putDefault("react", "^19.0.0"), CatalogError.ok);
    expect_eq(map.putDefault("react-dom", "^19.0.0"), CatalogError.ok);
    expect_eq(map.putNamed("legacy", "react", "^18.2.0"), CatalogError.ok);
    expect_eq(map.putNamed("testing", "jest", "30.0.0"), CatalogError.ok);

    expect_eq(map.hasAny(), true);
    expect_eq(map.len(), 4);
    expect_eq(map.defaultCount(), 2);
    expect_eq(map.groupCount("legacy"), 1);

    // Same package resolves differently across default vs named catalog
    if (v1 := map.resolveSpecifier("react", "catalog:")) {
        expect_eq(std.mem.eql(u8, v1, "^19.0.0"), true);
    } else { expect_eq(true, false); }

    if (v2 := map.resolveSpecifier("react", "catalog:legacy")) {
        expect_eq(std.mem.eql(u8, v2, "^18.2.0"), true);
    } else { expect_eq(true, false); }

    // Non-catalog specifiers pass through untouched
    if (v3 := map.resolveSpecifier("lodash", "^4.17.21")) {
        expect_eq(std.mem.eql(u8, v3, "^4.17.21"), true);
    } else { expect_eq(true, false); }

    // Upsert updates version in-place without increasing len
    expect_eq(map.upsert("", "react", "^19.1.0"), CatalogError.ok);
    expect_eq(map.len(), 4);
    if (v4 := map.get("", "react")) {
        expect_eq(std.mem.eql(u8, v4, "^19.1.0"), true);
    } else { expect_eq(true, false); }
});

test("3. Deterministic sortEntries & formatLockfileSection canonical JSON output", () => {
    let storage: [6]CatalogEntry = [{ catalog_name: "", package_name: "", version: "" }; 6];
    let map = CatalogMap.init(storage[0..]);

    // Insert in reverse/unsorted order
    expect_eq(map.putNamed("testing", "vitest", "^2.0.0"), CatalogError.ok);
    expect_eq(map.putDefault("zod", "^3.23.0"), CatalogError.ok);
    expect_eq(map.putNamed("build", "esbuild", "0.24.0"), CatalogError.ok);
    expect_eq(map.putNamed("testing", "jest", "30.0.0"), CatalogError.ok);
    expect_eq(map.putDefault("axios", "^1.7.0"), CatalogError.ok);

    map.sortEntries();

    let json_b: [256]u8 = [0; 256];
    let json_s: []u8 = json_b[0..];
    if (section := map.formatLockfileSection(*json_s)) {
        const expected = "{\"catalog\":{\"axios\":\"^1.7.0\",\"zod\":\"^3.23.0\"},\"catalogs\":{\"build\":{\"esbuild\":\"0.24.0\"},\"testing\":{\"jest\":\"30.0.0\",\"vitest\":\"^2.0.0\"}}}";
        expect_eq(std.mem.eql(u8, section, expected), true);
    } else {
        expect_eq(true, false);
    }
});

test("4. Missing entries & unknown catalog groups return null cleanly", () => {
    let storage: [4]CatalogEntry = [{ catalog_name: "", package_name: "", version: "" }; 4];
    let map = CatalogMap.init(storage[0..]);
    expect_eq(map.putDefault("react", "^19.0.0"), CatalogError.ok);

    expect_eq(map.get("", "vue") == null, true);
    expect_eq(map.get("nonexistent", "react") == null, true);
    expect_eq(map.resolveSpecifier("vue", "catalog:") == null, true);
    expect_eq(map.resolveSpecifier("react", "catalog:nonexistent") == null, true);
    expect_eq(map.resolveSpecifier("react", "catalog:bad/group") == null, true);
});

test("5. Boundary defense: duplicates, nested catalog refs, invalid names, capacity full & short output buffer", () => {
    let storage: [2]CatalogEntry = [{ catalog_name: "", package_name: "", version: "" }; 2];
    let map = CatalogMap.init(storage[0..]);

    // Invalid inputs
    expect_eq(map.putDefault("", "^1.0.0"), CatalogError.invalid_package_name);
    expect_eq(map.putDefault("bad pkg", "^1.0.0"), CatalogError.invalid_package_name);
    expect_eq(map.putDefault("react", ""), CatalogError.invalid_version);
    expect_eq(map.putDefault("react", "catalog:other"), CatalogError.nested_catalog_ref);
    expect_eq(map.putNamed("", "react", "^1.0.0"), CatalogError.invalid_catalog_name);
    expect_eq(map.putNamed("bad/group", "react", "^1.0.0"), CatalogError.invalid_catalog_name);

    // Duplicate & capacity full
    expect_eq(map.putDefault("a", "1.0.0"), CatalogError.ok);
    expect_eq(map.putDefault("a", "2.0.0"), CatalogError.duplicate_entry);
    expect_eq(map.putDefault("b", "1.0.0"), CatalogError.ok);
    expect_eq(map.putDefault("c", "1.0.0"), CatalogError.capacity_full);

    // Short output buffers
    let tiny_b: [4]u8 = [0; 4];
    let tiny: []u8 = tiny_b[0..];
    expect_eq(formatCatalogRef("testing", *tiny) == null, true);
    expect_eq(map.formatLockfileSection(*tiny) == null, true);
});
