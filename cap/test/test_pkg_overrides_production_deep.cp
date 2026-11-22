// Production Deep Tests for cap:pkg OverrideMap (lib/pkg/overrides.cp)
import { test, expect_eq } from "cap:test";
import {
    MAX_OVERRIDE_RULES,
    OverrideError,
    OverrideSourceKind,
    RootDepSpec,
    OverrideRule,
    normalizeOverrideKey,
    OverrideMap,
    hashPackageName,
} from "cap:pkg";

test("pkg overrides: npm overrides JSON object with plain, dot-object, and $ref expansion", fn() {
    const root_deps = [
        RootDepSpec{ name: "react", spec: "^19.1.0" },
        RootDepSpec{ name: "@types/node", spec: "^22.0.0" },
    ];

    let map = OverrideMap.init();
    map.parseOverridesObject(
        "{ \"lodash\": \"4.17.21\", \"react\": \"$react\", \"chalk\": { \".\": \"5.3.0\" }, \"@types/node\": \"$@types/node\" }",
        root_deps[0..],
    ) catch unreachable;

    expect_eq(map.count(), 4);
    expect_eq(map.get("lodash") ?? "", "4.17.21");
    expect_eq(map.get("react") ?? "", "^19.1.0");
    expect_eq(map.get("chalk") ?? "", "5.3.0");
    expect_eq(map.get("@types/node") ?? "", "^22.0.0");
    expect_eq(map.get("nonexistent") == null, true);

    if (r := map.getRule("react")) {
        expect_eq(r.from_dollar_ref, true);
        expect_eq(r.source_kind == .npm_override, true);
    } else {
        unreachable;
    }

    // Hash-based lookup matches Bun OverrideMap.get(name_hash)
    const lodash_hash = hashPackageName("lodash");
    if (by_h := map.getByHash(lodash_hash)) {
        expect_eq(by_h.name, "lodash");
        expect_eq(by_h.spec, "4.17.21");
    } else {
        unreachable;
    }
});

test("pkg overrides: yarn resolutions **/ stripping, scoped packages, and deterministic sort + JSON", fn() {
    const empty_deps: [0]RootDepSpec = [];
    let map = OverrideMap.init();
    map.parseResolutionsObject(
        "{ \"**/zod\": \"3.23.8\", \"@babel/core\": \"7.26.0\", \"**/axios\": \"1.7.9\" }",
        empty_deps[0..],
    ) catch unreachable;

    expect_eq(map.count(), 3);
    expect_eq(map.get("zod") ?? "", "3.23.8");
    expect_eq(map.get("@babel/core") ?? "", "7.26.0");
    expect_eq(map.get("axios") ?? "", "1.7.9");

    if (z := map.getRule("zod")) {
        expect_eq(z.source_kind == .yarn_resolution, true);
        expect_eq(z.from_dollar_ref, false);
    } else {
        unreachable;
    }

    map.sort();
    const sorted = map.items();
    expect_eq(sorted[0].name, "@babel/core");
    expect_eq(sorted[1].name, "axios");
    expect_eq(sorted[2].name, "zod");

    let json_buf: [256]u8 = undefined;
    const json_out = map.toJson(json_buf[0..]) catch unreachable;
    expect_eq(json_out, "{\"@babel/core\":\"7.26.0\",\"axios\":\"1.7.9\",\"zod\":\"3.23.8\"}");
});

test("pkg overrides: duplicate key overwrites in place & normalizeOverrideKey edge cases", fn() {
    const empty_deps: [0]RootDepSpec = [];
    let map = OverrideMap.init();
    map.addOverride("semver", "7.5.0", empty_deps[0..]) catch unreachable;
    map.addOverride("semver", "7.6.3", empty_deps[0..]) catch unreachable;
    expect_eq(map.count(), 1);
    expect_eq(map.get("semver") ?? "", "7.6.3");

    expect_eq(normalizeOverrideKey("**/pkg", true) catch unreachable, "pkg");
    expect_eq(normalizeOverrideKey("**/@scope/pkg", true) catch unreachable, "@scope/pkg");

    let empty_err = false;
    * = normalizeOverrideKey("**/", true) catch { empty_err = true; };
    expect_eq(empty_err, true);

    let bad_scope = false;
    * = normalizeOverrideKey("@onlyscope", false) catch { bad_scope = true; };
    expect_eq(bad_scope, true);

    let empty_scope_name = false;
    * = normalizeOverrideKey("@scope/", false) catch { empty_scope_name = true; };
    expect_eq(empty_scope_name, true);
});

test("pkg overrides: rejects nested overrides, nested resolutions, and patch: protocol", fn() {
    const empty_deps: [0]RootDepSpec = [];
    let map = OverrideMap.init();

    // Nested resolution path `foo/bar`
    let nested_unscoped = false;
    * = map.addResolution("foo/bar", "1.0.0", empty_deps[0..]) catch { nested_unscoped = true; };
    expect_eq(nested_unscoped, true);

    // Nested scoped resolution `@ns/hello/world`
    let nested_scoped = false;
    * = map.addResolution("@ns/hello/world", "1.0.0", empty_deps[0..]) catch { nested_scoped = true; };
    expect_eq(nested_scoped, true);

    // Nested npm override object `{ ".": "1.0.0", "bar": "2.0.0" }`
    let nested_obj = false;
    * = map.parseOverridesObject("{\"foo\":{\".\":\"1.0.0\",\"bar\":\"2.0.0\"}}", empty_deps[0..]) catch {
        nested_obj = true;
    };
    expect_eq(nested_obj, true);

    // `patch:` override rejected
    let patch_err = false;
    * = map.addOverride("lodash", "patch:lodash@4.17.21#patches/lodash.patch", empty_deps[0..]) catch {
        patch_err = true;
    };
    expect_eq(patch_err, true);

    expect_eq(map.count(), 0);
});

test("pkg overrides: unresolved $ref, empty value, and malformed JSON guards", fn() {
    const root_deps = [
        RootDepSpec{ name: "react", spec: "18.3.1" },
    ];
    let map = OverrideMap.init();

    let unresolved = false;
    * = map.addOverride("vue", "$vue", root_deps[0..]) catch { unresolved = true; };
    expect_eq(unresolved, true);

    let bare_dollar = false;
    * = map.addOverride("react", "$", root_deps[0..]) catch { bare_dollar = true; };
    expect_eq(bare_dollar, true);

    let empty_val = false;
    * = map.addOverride("react", "", root_deps[0..]) catch { empty_val = true; };
    expect_eq(empty_val, true);

    let bad_json = false;
    * = map.parseOverridesObject("not-an-object", root_deps[0..]) catch { bad_json = true; };
    expect_eq(bad_json, true);

    // Empty JSON object is valid and yields 0 entries
    map.parseOverridesObject("  {  }  ", root_deps[0..]) catch unreachable;
    expect_eq(map.count(), 0);
});

test("pkg overrides: buffer-too-small guard & MAX_OVERRIDE_RULES capacity bound", fn() {
    expect_eq(MAX_OVERRIDE_RULES, 64);
    const empty_deps: [0]RootDepSpec = [];
    let map = OverrideMap.init();
    map.addOverride("express", "4.21.0", empty_deps[0..]) catch unreachable;

    let tiny: [4]u8 = undefined;
    let oob = false;
    * = map.toJson(tiny[0..]) catch { oob = true; };
    expect_eq(oob, true);

    // Empty map serializes cleanly to "{}"
    let empty_map = OverrideMap.init();
    let small: [4]u8 = undefined;
    const empty_json = empty_map.toJson(small[0..]) catch unreachable;
    expect_eq(empty_json, "{}");
});
