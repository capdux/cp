// cap/test/testTestRunnerExpectKeys.cp
// Production-grade deep tests for cap:test_runner expectKeys.cp (Bun toContainAllKeys & key/value matchers)

import { test, assert, assertEq } from "cap:test";
import {
    ExpectKvEntry,
    toBeOneOfStr,
    toBeOneOfI64,
    toContainKey,
    toContainKeys,
    toContainAllKeys,
    toContainAnyKeys,
    toContainValue,
    toContainValues,
    toContainAllValues,
    toContainAnyValues,
    ExpectKeysMatcher,
} from "cap:test_runner";

test("ExpectKeys: toContainKey / toContainKeys / toContainAllKeys / toContainAnyKeys", () => {
    const obj: [3]ExpectKvEntry = [
        ExpectKvEntry{ key: "id", val: "42" },
        ExpectKvEntry{ key: "name", val: "alice" },
        ExpectKvEntry{ key: "role", val: "admin" },
    ];

    assert(toContainKey(obj[0..], "name"));
    assert(!toContainKey(obj[0..], "email"));

    // toContainKeys allows extra keys in obj
    const sub: [2][]const u8 = ["role", "id"];
    assert(toContainKeys(obj[0..], sub[0..]));
    assert(!toContainAllKeys(obj[0..], sub[0..])); // length mismatch

    // toContainAllKeys is order-independent exact key set match
    const exact: [3][]const u8 = ["role", "id", "name"];
    assert(toContainAllKeys(obj[0..], exact[0..]));

    // Duplicate key in expected_keys must not fool toContainAllKeys
    const dup_expected: [3][]const u8 = ["id", "id", "name"];
    assert(!toContainAllKeys(obj[0..], dup_expected[0..]));

    // toContainAnyKeys
    const any_hit: [2][]const u8 = ["missing", "role"];
    const any_miss: [2][]const u8 = ["x", "y"];
    assert(toContainAnyKeys(obj[0..], any_hit[0..]));
    assert(!toContainAnyKeys(obj[0..], any_miss[0..]));
});

test("ExpectKeys: value matchers and multiset multiplicity in toContainAllValues", () => {
    const obj: [3]ExpectKvEntry = [
        ExpectKvEntry{ key: "a", val: "red" },
        ExpectKvEntry{ key: "b", val: "red" },
        ExpectKvEntry{ key: "c", val: "blue" },
    ];

    assert(toContainValue(obj[0..], "blue"));
    assert(!toContainValue(obj[0..], "green"));

    const sub_vals: [2][]const u8 = ["blue", "red"];
    assert(toContainValues(obj[0..], sub_vals[0..]));

    // Exact multiset match (order-independent)
    const good_multi: [3][]const u8 = ["blue", "red", "red"];
    assert(toContainAllValues(obj[0..], good_multi[0..]));

    // Same distinct set but wrong multiplicity must fail
    const bad_multi: [3][]const u8 = ["blue", "blue", "red"];
    assert(!toContainAllValues(obj[0..], bad_multi[0..]));

    const any_vals: [2][]const u8 = ["yellow", "blue"];
    const no_vals: [2][]const u8 = ["yellow", "green"];
    assert(toContainAnyValues(obj[0..], any_vals[0..]));
    assert(!toContainAnyValues(obj[0..], no_vals[0..]));
});

test("ExpectKeys: toBeOneOfStr and toBeOneOfI64 membership checks", () => {
    const statuses: [3][]const u8 = ["pending", "running", "done"];
    assert(toBeOneOfStr("running", statuses[0..]));
    assert(!toBeOneOfStr("cancelled", statuses[0..]));

    const codes: [4]i64 = [200, 201, 204, 304];
    assert(toBeOneOfI64(204, codes[0..]));
    assert(!toBeOneOfI64(500, codes[0..]));
});

test("ExpectKeys: empty objects and empty expected lists edge cases", () => {
    const empty_obj: [0]ExpectKvEntry = [];
    const empty_keys: [0][]const u8 = [];
    const non_empty: [1][]const u8 = ["a"];

    assert(toContainAllKeys(empty_obj[0..], empty_keys[0..]));
    assert(toContainKeys(empty_obj[0..], empty_keys[0..]));
    assert(!toContainAnyKeys(empty_obj[0..], empty_keys[0..]));
    assert(!toContainAnyKeys(empty_obj[0..], non_empty[0..]));
    assert(toContainAllValues(empty_obj[0..], empty_keys[0..]));
    assert(!toBeOneOfStr("a", empty_keys[0..]));
});

test("ExpectKeysMatcher: one-shot negate() and pass/fail accounting", () => {
    const obj: [2]ExpectKvEntry = [
        ExpectKvEntry{ key: "k1", val: "v1" },
        ExpectKvEntry{ key: "k2", val: "v2" },
    ];
    const exact: [2][]const u8 = ["k2", "k1"];
    const wrong: [2][]const u8 = ["k1", "k3"];

    let m = ExpectKeysMatcher.init();
    assert(m.checkContainAllKeys(obj[0..], exact[0..]));
    assert(!m.checkContainAllKeys(obj[0..], wrong[0..]));

    // One-shot negation inverts the next check and automatically resets
    m.negate();
    assert(m.checkContainKey(obj[0..], "missing"));
    assert(!m.negated); // auto-reset

    // Normal positive check after auto-reset
    assert(m.checkContainKey(obj[0..], "k1"));

    const allowed: [2][]const u8 = ["v1", "v2"];
    assert(m.checkBeOneOfStr("v2", allowed[0..]));

    assertEq(m.assertions, 5);
    assertEq(m.passed, 4);
    assertEq(m.failed, 1);
});

test("ExpectKeysMatcher: checkContainKeys / checkContainAnyKeys / checkContainAllValues", () => {
    const obj: [2]ExpectKvEntry = [
        ExpectKvEntry{ key: "host", val: "localhost" },
        ExpectKvEntry{ key: "port", val: "8080" },
    ];
    const req_keys: [1][]const u8 = ["host"];
    const any_keys: [2][]const u8 = ["tls", "port"];
    const vals: [2][]const u8 = ["8080", "localhost"];

    let m = ExpectKeysMatcher.init();
    assert(m.checkContainKeys(obj[0..], req_keys[0..]));
    assert(m.checkContainAnyKeys(obj[0..], any_keys[0..]));
    assert(m.checkContainAllValues(obj[0..], vals[0..]));
    assertEq(m.passed, 3);
    assertEq(m.failed, 0);
});
