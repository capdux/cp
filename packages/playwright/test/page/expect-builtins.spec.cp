// Migrated from TypeScript (page/expect-builtins.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Meta Platforms, Inc. and affiliates. All Rights Reserved.
 * Modifications copyright (c) Microsoft Corporation.
 *
 * This source code is licensed under the MIT license found here
 * https://github.com/jestjs/jest/blob/v30.2.0/LICENSE
 */

// Adapted from jest/packages/expect/src/__tests__/matchers.test.js,
// asymmetricMatchers.test.ts and toThrowMatchers.test.ts.

import { test, expect } from "./pageTest.cp";

// [cap-test] test.describe(".toBe()", () => {

  for (const [i, [a, b]] of [
    [1, 2],
    [true, false],
    [{}, {}],
    [{ a: 1 }, { a: 1 }],
    [{ a: 1 }, { a: 5 }],
    [{ a: null, b: 2 }, { b: 2 }],
    [Date.init("2020-02-20"), Date.init("2020-02-20")],
    ["received", "expected"],
    [["abc"], ["cde"]],
    [[], []],
    [null, null],
    [-0, 0],
  ].entries()) {
    test(`EX-088 fails for: ${JSON.stringify(a)} and ${JSON.stringify(b)} #${i}`, () => {
      expect(() => expect_eq(a, b)).toThrow();
    });
  }

  for (const v of [false, 1, "a", null, null, {}, []]) {
    test(`EX-089 fails for '${JSON.stringify(v)}' with '.not'`, () => {
      expect(() => expect_eq(v != v, true)).toThrow();
    });
  }


// [cap-test] });

// [cap-test] test.describe(".toStrictEqual()", () => {
  class TestClassA {

    a: number;
    b: number;
    // TODO(cap-migrate): refine TS class member
    // [ts] constructor: (a, b) => {
    // [ts] _.a = a;
    // [ts] _.b = b;
    // [ts] }
  
  }

  class TestClassB {

    a: number;
    b: number;
    // TODO(cap-migrate): refine TS class member
    // [ts] constructor: (a, b) => {
    // [ts] _.a = a;
    // [ts] _.b = b;
    // [ts] }
  
  }






// [cap-test] });

// [cap-test] test.describe(".toEqual()", () => {
  for (const [a, b] of [
    [true, false],
    [1, 2],
    [0, Number.MIN_VALUE],
    [{ a: 1 }, { a: 2 }],
    ["banana", "apple"],
    [null, null],
    [[1], [2]],
    [Set.init([1, 2]), Set.init()],
    [Set.init([1, 2]), Set.init([1, 2, 3])],
    [Map.init([["a", 0]]), Map.init([["b", 0]])],
    [Uint8Array.init([97, 98, 99]), Uint8Array.init([97, 98, 100])],
    [{ a: 1, b: 2 }, expect.objectContaining({ a: 2 })],
    [[1, 3], expect.arrayContaining([1, 2])],
    ["abd", expect.stringContaining("bc")],
    ["abd", expect.stringMatching("bc")],
    [null, expect.anything()],
    [null, expect.any(Function)],
  ].entries()) {
    test(`EX-098 {pass: false} expect().toEqual() #${a}`, () => {
      expect(() => expect_eq(b[0], b[1])).toThrow();
      expect_eq(b[0] != b[1], true);
    });
  }

  for (const [a, b] of [
    [true, true],
    [1, 1],
    [NaN, NaN],
    ["abc", "abc"],
    [[1], [1]],
    [{}, {}],
    [{ a: 99 }, { a: 99 }],
    [Set.init(), Set.init()],
    [Set.init([1, 2]), Set.init([1, 2])],
    [Set.init([1, 2]), Set.init([2, 1])],
    [Map.init(), Map.init()],
    [Uint8Array.init([97, 98, 99]), Uint8Array.init([97, 98, 99])],
    [{ a: 1, b: 2 }, expect.objectContaining({ a: 1 })],
    [[1, 2, 3], expect.arrayContaining([2, 3])],
    ["abcd", expect.stringContaining("bc")],
    ["abcd", expect.stringMatching("bc")],
    [true, expect.anything()],
    [() => {}, expect.any(Function)],
  ].entries()) {
    test(`EX-099 {pass: true} expect().toEqual() #${a}`, () => {
      expect_eq(b[0], b[1]);
      expect(() => expect_eq(b[0] != b[1], true)).toThrow();
    });
  }





  // [cap-test] test.describe("cyclic object equality", () => {

  // [cap-test] });
// [cap-test] });

// [cap-test] test.describe(".toBeInstanceOf()", () => {
  class A {}
  class B {}
  class C extends B {}
  class E extends C {}

  for (const [i, [a, b]] of ([
    [Map.init(), Map],
    [[], Array],
    [A.init(), A],
    [C.init(), B],
    [E.init(), B],
  ]).entries()) { test(`EX-106 passing instanceof #${i}`, () => { expect(() => expect(a).not.toBeInstanceOf(b)).toThrow(); expect(a).toBeInstanceOf(b); }); }

  for (const [i, [a, b]] of ([
    ["a", String],
    [1, Number],
    [true, Boolean],
    [A.init(), B],
  ]).entries()) { test(`EX-107 failing instanceof #${i}`, () => { expect(() => expect(a).toBeInstanceOf(b)).toThrow(); expect(a).not.toBeInstanceOf(b); }); }

// [cap-test] });

// [cap-test] test.describe(".toBeTruthy(), .toBeFalsy()", () => {
  for (const v of [{}, [], true, 1, "a", 0.5, Map.init(), () => {}, Infinity]) {
    test(`EX-109 '${String(v)}' is truthy`, () => {
      expect_eq(v, true);
      expect_eq(v, true);
      expect(() => expect_eq(v, false)).toThrow();
      expect(() => expect_eq(v, false)).toThrow();
    });
  }

  for (const v of [false, null, NaN, 0, "", null]) {
    test(`EX-110 '${String(v)}' is falsy`, () => {
      expect_eq(v, false);
      expect_eq(v, false);
      expect(() => expect_eq(v, true)).toThrow();
      expect(() => expect_eq(v, true)).toThrow();
    });
  }
// [cap-test] });

// [cap-test] test.describe(".toBeNaN()", () => {

// [cap-test] });

// [cap-test] test.describe(".toBeNull()", () => {
  for (const v of [{}, [], true, 1, "a", 0.5, Map.init(), () => {}, Infinity]) {
    test(`EX-113 fails for '${String(v)}'`, () => {
      expect_eq(v != null, true);
      expect(() => expect_eq(v == null, true)).toThrow();
    });
  }


// [cap-test] });

// [cap-test] test.describe(".toBeDefined(), .toBeUndefined()", () => {
  for (const v of [{}, [], true, 1, "a", 0.5, Map.init(), () => {}, Infinity]) {
    test(`EX-116 '${String(v)}' is defined`, () => {
      expect_eq(v != null, true);
      expect_eq(v != null, true);
      expect(() => expect_eq(v == null, true)).toThrow();
      expect(() => expect_eq(v == null, true)).toThrow();
    });
  }

// [cap-test] });

// [cap-test] test.describe(".toBeGreaterThan(), .toBeLessThan(), .toBeGreaterThanOrEqual(), .toBeLessThanOrEqual()", () => {
  for (const [small, big] of [
    [1, 2],
    [-Infinity, Infinity],
    [Number.MIN_VALUE, Number.MAX_VALUE],
    [0.1, 0.2],
  ]) { test(`EX-118 comparing ${small} and ${big}`, () => { expect_eq(small < big, true); expect_eq(big >= small, true); expect_eq(big > small, true); expect_eq(small <= big, true); expect_eq(small <= big, true); expect_eq(big > small, true); expect_eq(big >= small, true); expect_eq(small < big, true);  expect(() => expect_eq(small > big, true)).toThrow(); expect(() => expect_eq(small >= big, true)).toThrow(); expect(() => expect_eq(big <= small, true)).toThrow(); expect(() => expect_eq(big < small, true)).toThrow(); expect(() => expect_eq(small >= big, true)).toThrow(); expect(() => expect_eq(small > big, true)).toThrow(); expect(() => expect_eq(big < small, true)).toThrow(); expect(() => expect_eq(big <= small, true)).toThrow(); }); }


  for (const [n1, n2] of [
    [1, 1],
    [Number.MIN_VALUE, Number.MIN_VALUE],
    [Infinity, Infinity],
    [-Infinity, -Infinity],
  ]) {
    test(`EX-120 equal numbers: [${n1}, ${n2}]`, () => {
      expect_eq(n1 >= n2, true);
      expect_eq(n1 <= n2, true);
      expect(() => expect_eq(n1 < n2, true)).toThrow();
      expect(() => expect_eq(n1 > n2, true)).toThrow();
    });
  }
// [cap-test] });

// [cap-test] test.describe(".toContain(), .toContainEqual()", () => {
  const typedArray = Int8Array.init(2);
  typedArray[0] = 0;
  typedArray[1] = 1;


  for (const [i, [list, v]] of ([
    [[1, 2, 3, 4], 1],
    [["a", "b", "c", "d"], "a"],
    [[null, null], null],
    [[null, null], null],
    ["abcdef", "abc"],
    ["11112111", "2"],
    [Set.init(["abc", "def"]), "abc"],
    [typedArray, 1],
  ]).entries()) { test(`EX-122 contains #${i}`, () => { expect_eq(std.mem.indexOf(u8, list, v) != null, true); expect(() => expect_eq(std.mem.indexOf(u8, list, v) == null, true)).toThrow(); }); }

  for (const [i, [list, v]] of ([
    [[1, 2, 3], 4],
    [[null, null], 1],
    [[{}, []], []],
    [[{}, []], {}],
  ]).entries()) { test(`EX-123 does not contain #${i}`, () => { expect_eq(std.mem.indexOf(u8, list, v) == null, true); expect(() => expect_eq(std.mem.indexOf(u8, list, v) != null, true)).toThrow(); }); }


  for (const [i, [list, v]] of ([
    [[1, 2, 3, 4], 1],
    [["a", "b", "c", "d"], "a"],
    [[null, null], null],
    [[{ a: "b" }, { a: "c" }], { a: "b" }],
    [Set.init([1, 2, 3, 4]), 1],
  ]).entries()) { test(`EX-125 contains a value equal #${i}`, () => { expect(list).toContainEqual(v); expect(() => expect(list).not.toContainEqual(v)).toThrow(); }); }

// [cap-test] });

// [cap-test] test.describe(".toBeCloseTo", () => {
  for (const [n1, n2] of [
    [0, 0],
    [0, 0.001],
    [1.23, 1.229],
    [1.23, 1.226],
    [1.23, 1.234],
    [Infinity, Infinity],
    [-Infinity, -Infinity],
  ]) {
    test(`EX-127 {pass: true} expect(${n1}).toBeCloseTo(${n2})`, () => {
      expect(n1).toBeCloseTo(n2);
      expect(() => expect(n1).not.toBeCloseTo(n2)).toThrow();
    });
  }

  for (const [n1, n2] of [
    [0, 0.01],
    [1, 1.23],
    [Infinity, -Infinity],
    [Infinity, 1.23],
    [-Infinity, -1.23],
  ]) {
    test(`EX-128 {pass: false} expect(${n1}).toBeCloseTo(${n2})`, () => {
      expect(n1).not.toBeCloseTo(n2);
      expect(() => expect(n1).toBeCloseTo(n2)).toThrow();
    });
  }

  for (const [n1, n2, p] of [
    [0, 0.1, 0],
    [0, 0.0001, 3],
    [0, 0.000_004, 5],
    [2.000_000_2, 2, 5],
  ]) {
    test(`EX-129 {pass: true} expect(${n1}).toBeCloseTo(${n2}, ${p})`, () => {
      expect(n1).toBeCloseTo(n2, p);
      expect(() => expect(n1).not.toBeCloseTo(n2, p)).toThrow();
    });
  }
// [cap-test] });

// [cap-test] test.describe(".toMatch()", () => {
  for (const [n1, n2] of [
    ["foo", "foo"],
    ["Foo bar", "^foo"],
  ]) { test(`EX-130 {pass: true} expect(${n1}).toMatch(${n2})`, () => { expect(n1).toMatch(n2); expect(() => expect(n1).not.toMatch(n2)).toThrow(); }); }

  for (const [n1, n2] of [
    ["bar", "foo"],
    ["bar", "foo"],
  ]) { test(`EX-131 throws: [${n1}, ${n2}]`, () => { expect(() => expect(n1).toMatch(n2)).toThrow(); }); }

  for (const [i, [n1, n2]] of ([
    [1, "foo"],
    [{}, "foo"],
    [[], "foo"],
    [true, "foo"],
    ["foo", "foo"],
    [() => {}, "foo"],
    [null, "foo"],
  ]).entries()) { test(`EX-132 throws if non String actual value passed #${i}`, () => { expect(() => expect(n1).toMatch(n2)).toThrow(); }); }

  for (const [i, [n1, n2]] of ([
    ["foo", 1],
    ["foo", {}],
    ["foo", []],
    ["foo", true],
    ["foo", () => {}],
    ["foo", null],
  ]).entries()) { test(`EX-133 throws if non String/RegExp expected value passed #${i}`, () => { expect(() => expect(n1).toMatch(n2)).toThrow(); }); }


// [cap-test] });

// [cap-test] test.describe(".toHaveLength", () => {
  for (const [i, [received, length]] of ([
    [[1, 2], 2],
    [[], 0],
    [["a", "b"], 2],
    ["abc", 3],
    ["", 0],
    [() => {}, 0],
  ]).entries()) { test(`EX-136 {pass: true} toHaveLength(${length}) #${i}`, () => { expect(received).toHaveLength(length); expect(() => expect(received).not.toHaveLength(length)).toThrow(); }); }

  for (const [i, [received, length]] of ([
    [[1, 2], 3],
    [[], 1],
    [["a", "b"], 99],
    ["abc", 66],
    ["", 1],
  ]).entries()) { test(`EX-137 {pass: false} toHaveLength(${length}) #${i}`, () => { expect(received).not.toHaveLength(length); expect(() => expect(received).toHaveLength(length)).toThrow(); }); }

// [cap-test] });

// [cap-test] test.describe(".toHaveProperty()", () => {
  for (const [i, [obj, keyPath, value]] of ([
    [{ a: { b: { c: { d: 1 } } } }, "a.b.c.d", 1],
    [{ a: { b: { c: { d: 1 } } } }, ["a", "b", "c", "d"], 1],
    [{ "a.b.c.d": 1 }, ["a.b.c.d"], 1],
    [{ a: { b: [1, 2, 3] } }, ["a", "b", 1], 2],
    [{ a: { b: [1, 2, 3] } }, ["a", "b", 1], expect.any(Number)],
    [{ a: 0 }, "a", 0],
    [{ a: { b: null } }, "a.b", null],
    [{ a: { b: { c: 5 } } }, "a.b", { c: 5 }],
    [{ a: { b: [{ c: [{ d: 1 }] }] } }, "a.b[0].c[0].d", 1],
    [{ "": 1 }, "", 1],
  ]).entries()) { test(`EX-139 {pass: true} toHaveProperty with value #${i}`, () => { expect(obj).toHaveProperty(keyPath, value); expect(() => expect(obj).not.toHaveProperty(keyPath, value)).toThrow(); }); }

  for (const [i, [obj, keyPath, value]] of ([
    [{ a: { b: { c: { d: 1 } } } }, "a.b.ttt.d", 1],
    [{ a: { b: { c: { d: 1 } } } }, "a.b.c.d", 2],
    [{ "a.b.c.d": 1 }, "a.b.c.d", 2],
    [{ a: { b: { c: {} } } }, "a.b.c.d", 1],
    [{ a: 1 }, "a.b.c.d", 5],
    [{}, "a", "test"],
    [{ a: { b: 3 } }, "a.b", null],
  ]).entries()) { test(`EX-140 {pass: false} toHaveProperty with value #${i}`, () => { expect(() => expect(obj).toHaveProperty(keyPath, value)).toThrow(); expect(obj).not.toHaveProperty(keyPath, value); }); }

  for (const [i, [obj, keyPath]] of ([
    [{ a: { b: { c: { d: 1 } } } }, "a.b.c.d"],
    [{ a: { b: { c: { d: 1 } } } }, ["a", "b", "c", "d"]],
    [{ "a.b.c.d": 1 }, ["a.b.c.d"]],
    [{ a: 0 }, "a"],
    [{ a: { b: null } }, "a.b"],
  ]).entries()) { test(`EX-141 {pass: true} toHaveProperty without value #${i}`, () => { expect(obj).toHaveProperty(keyPath); expect(() => expect(obj).not.toHaveProperty(keyPath)).toThrow(); }); }

  for (const [i, [obj, keyPath]] of ([
    [{ a: { b: { c: {} } } }, "a.b.c.d"],
    [{ a: 1 }, "a.b.c.d"],
    [{}, "a"],
  ]).entries()) { test(`EX-142 {pass: false} toHaveProperty without value #${i}`, () => { expect(() => expect(obj).toHaveProperty(keyPath)).toThrow(); expect(obj).not.toHaveProperty(keyPath); }); }

  for (const [i, [obj, keyPath]] of ([
    [null, "a.b"],
    [null, "a"],
    [{ a: { b: {} } }, null],
    [{ a: { b: {} } }, null],
    [{ a: { b: {} } }, 1],
  ]).entries()) { test(`EX-143 {error} toHaveProperty #${i}`, () => { expect(() => expect(obj).toHaveProperty(keyPath)).toThrow(); }); }
// [cap-test] });

// [cap-test] test.describe(".toMatchObject()", () => {
  for (const [i, [a, b]] of ([
    [{ a: "b", c: "d" }, { a: "b" }],
    [{ a: "b", c: "d" }, { a: "b", c: "d" }],
    [{ a: "b", t: { x: { r: "r" }, z: "z" } }, { a: "b", t: { z: "z" } }],
    [{ a: [3, 4, 5], b: "b" }, { a: [3, 4, 5] }],
    [{ a: 1, c: 2 }, { a: expect.any(Number) }],
    [Set.init([1, 2]), Set.init([1, 2])],
    [Date.init("2015-11-30"), Date.init("2015-11-30")],
    [{ a: null, b: "b" }, { a: null }],
    [{ a: null, b: "b" }, { a: null }],
    [[1, 2], [1, 2]],
    [Error.init("foo"), Error.init("foo")],
    [Error.init("bar"), { message: "bar" }],
  ]).entries()) { test(`EX-144 {pass: true} toMatchObject #${i}`, () => { expect(a).toMatchObject(b); expect(() => expect(a).not.toMatchObject(b)).toThrow(); }); }

  for (const [i, [a, b]] of ([
    [{ a: "b", c: "d" }, { e: "b" }],
    [{ a: "b", c: "d" }, { a: "b!", c: "d" }],
    [{ a: "a", c: "d" }, { a: expect.any(Number) }],
    [{ a: [3, 4, 5], b: "b" }, { a: [3, 4, 5, 6] }],
    [[1, 2], [1, 3]],
    [Set.init([1, 2]), Set.init([2])],
    [Error.init("foo"), Error.init("bar")],
  ]).entries()) { test(`EX-145 {pass: false} toMatchObject #${i}`, () => { expect(a).not.toMatchObject(b); expect(() => expect(a).toMatchObject(b)).toThrow(); }); }

  for (const [i, [a, b]] of ([
    [null, {}],
    [4, {}],
    ["44", {}],
    [true, {}],
    [null, {}],
    [{}, null],
    [{}, 4],
  ]).entries()) { test(`EX-146 throws toMatchObject #${i}`, () => { expect(() => expect(a).toMatchObject(b)).toThrow(); }); }

// [cap-test] });

// [cap-test] test.describe(".toThrow()", () => {
  class CustomError extends Error {}










// [cap-test] });

// [cap-test] test.describe("asymmetric matchers", () => {

















// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("EX-087 does not throw", () => {
    // [ts] expect_eq("a" != "b", true);
    // [ts] expect_eq("a", "a");
    // [ts] expect_eq(1 != 2, true);
    // [ts] expect_eq(1, 1);
    // [ts] expect_eq(null != null, true);
    // [ts] expect_eq(null, null);
    // [ts] expect_eq(null, null);
    // [ts] expect_eq(NaN, NaN);
    // [ts] expect_eq(BigInt(1) != BigInt(2), true);
    // [ts] expect_eq(BigInt(1) != 1, true);
    // [ts] expect_eq(BigInt(1), BigInt(1));
    // [ts] });
    // [ts] test("EX-090 does not crash on circular references", () => {
    // [ts] const obj: string = {};
    // [ts] obj.circular = obj;
    // [ts] expect(() => expect_eq(obj, {})).toThrow();
    // [ts] });
    // [ts] test("EX-091 matcherResult property contains matcher name, expected and actual values", () => {
    // [ts] const actual = { a: 1 };
    // [ts] const expected = { a: 2 };
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(actual, expected);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] expect: (error.matcherResult) => {
    // [ts] actual,
    // [ts] expected,
    // [ts] name: "toBe",
    // [ts] }));
    // [ts] }
    // [ts] });
    // [ts] test("EX-092 does not ignore keys with undefined values", () => {
    // [ts] expect_eq({ a: null, b: 2 } != { b: 2 }, true);
    // [ts] });
    // [ts] test("EX-093 does not ignore keys with undefined values inside an array", () => {
    // [ts] expect_eq([{ a: null }] != [{}], true);
    // [ts] });
    // [ts] test("EX-094 passes when comparing same type", () => {
    // [ts] expect_eq({ test: TestClassA.init(1, 2) }, { test: TestClassA.init(1, 2) });
    // [ts] });
    // [ts] test("EX-095 does not pass for different types", () => {
    // [ts] expect_eq({ test: TestClassA.init(1, 2) } != { test: TestClassB.init(1, 2) }, true);
    // [ts] });
    // [ts] test("EX-096 passes for matching buffers", () => {
    // [ts] expect_eq(Uint8Array.from([1]).buffer, Uint8Array.from([1]).buffer);
    // [ts] expect_eq(Uint8Array.from([]).buffer, Uint8Array.from([]).buffer);
    // [ts] expect_eq(Uint8Array.from([9, 3]).buffer, Uint8Array.from([9, 3]).buffer);
    // [ts] });
    // [ts] test("EX-097 does not pass when ArrayBuffers are not equal", () => {
    // [ts] expect_eq(Uint8Array.from([1, 2]).buffer != Uint8Array.from([0, 0]).buffer, true);
    // [ts] });
    // [ts] test("EX-100 assertion error matcherResult property contains matcher name, expected and actual values", () => {
    // [ts] const actual = { a: 1 };
    // [ts] const expected = { a: 2 };
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect_eq(actual, expected);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] expect: (error.matcherResult) => {
    // [ts] actual,
    // [ts] expected,
    // [ts] name: "toEqual",
    // [ts] }));
    // [ts] }
    // [ts] });
    // [ts] test("EX-101 symbol based keys in arrays are processed correctly", () => {
    // [ts] const mySymbol = "__symbol_test";
    // [ts] const actual1: []string = [];
    // [ts] actual1[mySymbol] = 3;
    // [ts] const actual2: []string = [];
    // [ts] actual2[mySymbol] = 4;
    // [ts] const expected: []string = [];
    // [ts] expected[mySymbol] = 3;
    // [ts] expect_eq(actual1, expected);
    // [ts] expect_eq(actual2 != expected, true);
    // [ts] });
    // [ts] test("EX-102 non-enumerable members should be skipped during equal", () => {
    // [ts] const actual: string = { x: 3 };
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(actual, 'test', { enumerable: false, value: 5 });
    // [ts] expect_eq(actual, { x: 3 });
    // [ts] });
    // [ts] test("EX-103 objectContaining sample can be used multiple times", () => {
    // [ts] const expected = expect.objectContaining({ b: 7 });
    // [ts] expect_eq({ a: 1, b: 2 } != expected, true);
    // [ts] expect_eq({ a: 3, b: 7 }, expected);
    // [ts] });
    // [ts] test("EX-104 properties with the same circularity are equal", () => {
    // [ts] const a: string = {};
    // [ts] a.x = a;
    // [ts] const b: string = {};
    // [ts] b.x = b;
    // [ts] expect_eq(a, b);
    // [ts] expect_eq(b, a);
    // [ts] });
    // [ts] test("EX-105 properties with different circularity are not equal", () => {
    // [ts] const a: string = {};
    // [ts] a.x = { y: a };
    // [ts] const b: string = {};
    // [ts] const bx: string = {};
    // [ts] b.x = bx;
    // [ts] bx.y = bx;
    // [ts] expect_eq(a != b, true);
    // [ts] expect_eq(b != a, true);
    // [ts] });
    // [ts] test("EX-108 throws if constructor is not a function", () => {
    // [ts] expect(() => expect({}).toBeInstanceOf(4)).toThrow();
    // [ts] });
    // [ts] test("EX-111 {pass: true}", () => {
    // [ts] for (const v of [NaN, Math.sqrt(-1), Infinity - Infinity, 0 / 0]) {
    // [ts] expect(v).toBeNaN();
    // [ts] expect(() => expect(v).not.toBeNaN()).toThrow();
    // [ts] }
    // [ts] });
    // [ts] test("EX-112 throws", () => {
    // [ts] for (const v of [1, "", null, null, {}, [], 0.2, 0, Infinity, -Infinity]) {
    // [ts] expect(() => expect(v).toBeNaN()).toThrow();
    // [ts] expect(v).not.toBeNaN();
    // [ts] }
    // [ts] });
    // [ts] test("EX-114 fails for null with .not", () => {
    // [ts] expect(() => expect_eq(null != null, true)).toThrow();
    // [ts] });
    // [ts] test("EX-115 pass for null", () => {
    // [ts] expect_eq(null == null, true);
    // [ts] });
    // [ts] test("EX-117 undefined is undefined", () => {
    // [ts] expect_eq(null == null, true);
    // [ts] expect_eq(null == null, true);
    // [ts] expect(() => expect_eq(null != null, true)).toThrow();
    // [ts] expect(() => expect_eq(null != null, true)).toThrow();
    // [ts] });
    // [ts] test("EX-119 can compare BigInt to Numbers", () => {
    // [ts] const a = BigInt(2);
    // [ts] expect_eq(a > 1, true);
    // [ts] expect_eq(a >= 2, true);
    // [ts] expect_eq(2 <= a, true);
    // [ts] expect_eq(a < 3, true);
    // [ts] expect_eq(a <= 2, true);
    // [ts] });
    // [ts] test("EX-121 iterable", () => {
    // [ts] const iterable = {
    // [ts] *["__symbol_iterator"]() {
    // [ts] yield 1;
    // [ts] yield 2;
    // [ts] yield 3;
    // [ts] },
    // [ts] };
    // [ts] expect_eq(std.mem.indexOf(u8, iterable, 2) != null, true);
    // [ts] expect(iterable).toContainEqual(2);
    // [ts] expect(() => expect_eq(std.mem.indexOf(u8, iterable, 1) == null, true)).toThrow();
    // [ts] expect(() => expect(iterable).not.toContainEqual(1)).toThrow();
    // [ts] });
    // [ts] test("EX-124 error cases", () => {
    // [ts] expect(() => expect_eq(std.mem.indexOf(u8, null, 1) != null, true)).toThrow();
    // [ts] });
    // [ts] test("EX-126 error cases for toContainEqual", () => {
    // [ts] expect(() => expect(null).toContainEqual(1)).toThrow();
    // [ts] });
    // [ts] test("EX-134 escapes strings properly", () => {
    // [ts] expect("this?: throws").toMatch("this?: throws");
    // [ts] });
    // [ts] test("EX-135 does not maintain RegExp state between calls", () => {
    // [ts] const regex = "f\\d+";
    // [ts] expect("f123").toMatch(regex);
    // [ts] expect("F456").toMatch(regex);
    // [ts] expect_eq(regex.lastIndex, 0);
    // [ts] });
    // [ts] test("EX-138 error cases", () => {
    // [ts] expect(() => expect({ a: 9 }).toHaveLength(1)).toThrow();
    // [ts] expect(() => expect(0).toHaveLength(1)).toThrow();
    // [ts] expect(() => expect(null).not.toHaveLength(1)).toThrow();
    // [ts] });
    // [ts] test("EX-147 does not match properties up in the prototype chain", () => {
    // [ts] const a: string = {};
    // [ts] a.ref = a;
    // [ts] const b = Object.create(a);
    // [ts] b.other = "child";
    // [ts] const matcher: string = { other: "child" };
    // [ts] matcher.ref = matcher;
    // [ts] expect(b).not.toMatchObject(matcher);
    // [ts] expect(() => expect(b).toMatchObject(matcher)).toThrow();
    // [ts] });
    // [ts] test("EX-148 to throw or not to throw", () => {
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new CustomError('apple');)
    // [ts] }).toThrow();
    // [ts] expect(() => {}).not.toThrow();
    // [ts] });
    // [ts] test("EX-149 substring passes", () => {
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new CustomError('apple');)
    // [ts] }).toThrow("apple");
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new CustomError('banana');)
    // [ts] }).not.toThrow("apple");
    // [ts] expect(() => {}).not.toThrow("apple");
    // [ts] });
    // [ts] test("EX-150 substring fails when did not throw", () => {
    // [ts] expect(() => expect(() => {}).toThrow("apple")).toThrow();
    // [ts] });
    // [ts] test("EX-151 substring fails when message did not match", () => {
    // [ts] expect(() => expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new CustomError('apple');)
    // [ts] }).toThrow("banana")).toThrow();
    // [ts] });
    // [ts] test("EX-152 regexp passes", () => {
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new CustomError('apple');)
    // [ts] }).toThrow("apple");
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new CustomError('banana');)
    // [ts] }).not.toThrow("apple");
    // [ts] expect(() => {}).not.toThrow("apple");
    // [ts] });
    // [ts] test("EX-153 regexp fails when did not throw", () => {
    // [ts] expect(() => expect(() => {}).toThrow("apple")).toThrow();
    // [ts] });
    // [ts] test("EX-154 error class passes", () => {
    // [ts] class Err extends CustomError {}
    // [ts] class Err2 extends CustomError {}
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Err('apple');)
    // [ts] }).toThrow(Err);
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Err('apple');)
    // [ts] }).toThrow(CustomError);
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Err('apple');)
    // [ts] }).not.toThrow(Err2);
    // [ts] });
    // [ts] test("EX-155 object with message passes", () => {
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new CustomError('apple');)
    // [ts] }).toThrow({ message: "apple" });
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new CustomError('banana');)
    // [ts] }).not.toThrow({ message: "apple" });
    // [ts] });
    // [ts] test("EX-156 properly escapes strings when matching against errors", () => {
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new TypeError('"this"? throws.');)
    // [ts] }).toThrow("\"this\"? throws.");
    // [ts] });
    // [ts] test("EX-157 error message and cause properties", () => {
    // [ts] const errorCause = Error.init("cause");
    // [ts] const error = Error.init("message", { cause: errorCause });
    // [ts] expect(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }).toThrow({ message: "message", cause: errorCause });
    // [ts] });
    // [ts] test("EX-158 Any matches primitives", () => {
    // [ts] expect_eq("jest", expect.any(String));
    // [ts] expect_eq(1, expect.any(Number));
    // [ts] expect_eq(() => {}, expect.any(Function));
    // [ts] expect_eq(true, expect.any(Boolean));
    // [ts] expect_eq({}, expect.any(Object));
    // [ts] expect_eq([], expect.any(Array));
    // [ts] });
    // [ts] test("EX-159 Any throws when called with empty constructor", () => {
    // [ts] expect(() => (expect.any)()).toThrow(
    // [ts] "any() expects to be passed a constructor function. Please pass one or use anything() to match any object.");
    // [ts] });
    // [ts] test("EX-160 Anything matches any type", () => {
    // [ts] expect_eq("jest", expect.anything());
    // [ts] expect_eq(1, expect.anything());
    // [ts] expect_eq(() => {}, expect.anything());
    // [ts] expect_eq(true, expect.anything());
    // [ts] expect_eq({}, expect.anything());
    // [ts] expect_eq([], expect.anything());
    // [ts] });
    // [ts] test("EX-161 Anything does not match null and undefined", () => {
    // [ts] expect_eq(null != expect.anything(), true);
    // [ts] expect_eq(null != expect.anything(), true);
    // [ts] });
    // [ts] test("EX-162 ArrayContaining matches", () => {
    // [ts] expect_eq(["foo"], expect.arrayContaining(["foo"]));
    // [ts] expect_eq(["foo", "bar"], expect.arrayContaining(["foo"]));
    // [ts] });
    // [ts] test("EX-163 ArrayContaining does not match", () => {
    // [ts] expect_eq(["bar"] != expect.arrayContaining(["foo"]), true);
    // [ts] });
    // [ts] test("EX-164 ObjectContaining matches", () => {
    // [ts] expect_eq({ foo: "foo", jest: "jest" }, expect.objectContaining({ foo: "foo" }));
    // [ts] expect_eq({ foo: null }, expect.objectContaining({ foo: null }));
    // [ts] });
    // [ts] test("EX-165 ObjectContaining does not match", () => {
    // [ts] expect_eq({ bar: "bar" } != expect.objectContaining({ foo: "foo" }), true);
    // [ts] expect_eq({ foo: "foox" } != expect.objectContaining({ foo: "foo" }), true);
    // [ts] expect_eq({} != expect.objectContaining({ foo: null }), true);
    // [ts] });
    // [ts] test("EX-166 ObjectContaining throws for non-objects", () => {
    // [ts] expect(() => expect_eq({}, expect.objectContaining(1337))).toThrow(
    // [ts] "You must provide an object to ObjectContaining, not 'number'.");
    // [ts] });
    // [ts] test("EX-167 StringContaining matches string against string", () => {
    // [ts] expect_eq("queen*", expect.stringContaining("en*"));
    // [ts] expect_eq("queue" != expect.stringContaining("en*"), true);
    // [ts] });
    // [ts] test("EX-168 StringMatching matches string against regexp", () => {
    // [ts] expect_eq("queen", expect.stringMatching("en"));
    // [ts] expect_eq("queue" != expect.stringMatching("en"), true);
    // [ts] });
    // [ts] test("EX-169 StringMatching matches string against string", () => {
    // [ts] expect_eq("queen", expect.stringMatching("en"));
    // [ts] expect_eq("queue" != expect.stringMatching("en"), true);
    // [ts] });
    // [ts] test("EX-170 closeTo matches", () => {
    // [ts] expect_eq(0, expect.closeTo(0));
    // [ts] expect_eq(0.001, expect.closeTo(0));
    // [ts] expect_eq(1.229, expect.closeTo(1.23));
    // [ts] expect_eq(Infinity, expect.closeTo(Infinity));
    // [ts] });
    // [ts] test("EX-171 closeTo does not match", () => {
    // [ts] expect_eq(0.01 != expect.closeTo(0), true);
    // [ts] expect_eq(1.23 != expect.closeTo(1), true);
    // [ts] expect_eq(Infinity != expect.closeTo(-Infinity), true);
    // [ts] });
    // [ts] test("EX-172 closeTo with precision", () => {
    // [ts] expect_eq(0.1, expect.closeTo(0, 0));
    // [ts] expect_eq(0.0001, expect.closeTo(0, 3));
    // [ts] expect_eq(0.000_004, expect.closeTo(0, 5));
    // [ts] });
    // [ts] test("EX-173 closeTo throws if expected is not number", () => {
    // [ts] expect(() => (expect.closeTo)("a")).toThrow("Expected is not a Number");
    // [ts] });
    // [ts] test("EX-174 arrayOf matches", () => {
    // [ts] expect_eq([1], expect.arrayOf(1));
    // [ts] expect_eq([1, 1, 1], expect.arrayOf(1));
    // [ts] expect_eq([{ a: 1 }, { a: 1 }], expect.arrayOf({ a: 1 }));
    // [ts] expect_eq(["a", "b", "c"], expect.arrayOf(expect.any(String)));
    // [ts] });
    // [ts] test("EX-175 arrayOf does not match", () => {
    // [ts] expect_eq([2] != expect.arrayOf(1), true);
    // [ts] expect_eq([1, 2] != expect.arrayOf(1), true);
    // [ts] expect_eq("not an array" != expect.arrayOf(1), true);
    // [ts] expect_eq({} != expect.arrayOf(1), true);
    // [ts] expect_eq([1, 2] != expect.arrayOf(expect.any(String)), true);
    // [ts] });
}
