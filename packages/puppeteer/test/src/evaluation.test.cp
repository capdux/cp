// Migrated from TypeScript (src/evaluation.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";
import { attachFrame, html } from "./utils.cp";

// [cap-test] describe("Evaluation specs", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.evaluate", () => {

  // [cap-test] });

  // [cap-test] describe("Page.evaluateOnNewDocument", () => {
  // [cap-test] });

  // [cap-test] describe("Page.removeScriptToEvaluateOnNewDocument", () => {
  // [cap-test] });

  // [cap-test] describe("Frame.evaluate", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return 7 * 3;
    // [ts] });
    // [ts] expect_eq(result, 21);
    // [ts] });
    // [ts] test("should transfer BigInt", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate((a) => {
    // [ts] return a;
    // [ts] }, BigInt(42));
    // [ts] expect_eq(result, BigInt(42));
    // [ts] });
    // [ts] test("should transfer NaN", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(a => {
    // [ts] return a;
    // [ts] }, NaN);
    // [ts] expect_eq(Object.is(result, NaN), true);
    // [ts] });
    // [ts] test("should transfer -0", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(a => {
    // [ts] return a;
    // [ts] }, -0);
    // [ts] expect_eq(Object.is(result, -0), true);
    // [ts] });
    // [ts] test("should transfer Infinity", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(a => {
    // [ts] return a;
    // [ts] }, Infinity);
    // [ts] expect_eq(Object.is(result, Infinity), true);
    // [ts] });
    // [ts] test("should transfer -Infinity", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(a => {
    // [ts] return a;
    // [ts] }, -Infinity);
    // [ts] expect_eq(Object.is(result, -Infinity), true);
    // [ts] });
    // [ts] test("should transfer arrays", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(
    // [ts] a => {
    // [ts] return a;
    // [ts] },
    // [ts] [1, 2, 3],
    // [ts] );
    // [ts] expect_eq(result, [1, 2, 3]);
    // [ts] });
    // [ts] test("should transfer arrays as arrays, not objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(
    // [ts] a => {
    // [ts] return Array.isArray(a);
    // [ts] },
    // [ts] [1, 2, 3],
    // [ts] );
    // [ts] expect_eq(result, true);
    // [ts] });
    // [ts] test("should transfer RegEx", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(a => {
    // [ts] return `Hello World!`.match(a)[1];
    // [ts] }, "Hello (.*)");
    // [ts] expect_eq(result, `World!`);
    // [ts] });
    // [ts] test("should modify global environment", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.evaluate(() => {
    // [ts] return ((globalThis).globalVar = 123);
    // [ts] });
    // [ts] expect_eq(page.evaluate("globalVar"), 123);
    // [ts] });
    // [ts] test("should evaluate in the page context", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/global-var.html");
    // [ts] expect_eq(page.evaluate("globalVar"), 123);
    // [ts] });
    // [ts] test("should replace symbols with undefined", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return ["__symbol_foo4", "foo"];
    // [ts] }),
    // [ts] ).toEqual([null, "foo"]);
    // [ts] });
    // [ts] test("should work with function shorthands", () => {
    // [ts] const {page} = getTestState();
    // [ts] const a = {
    // [ts] sum: (a, b) => {
    // [ts] return a + b;
    // [ts] },
    // [ts] mult: (a, b) => {
    // [ts] return a * b;
    // [ts] },
    // [ts] };
    // [ts] expect_eq(page.evaluate(a.sum, 1, 2), 3);
    // [ts] expect_eq(page.evaluate(a.mult, 2, 4), 8);
    // [ts] });
    // [ts] test("should work with function shorthands and nested arrow functions", () => {
    // [ts] const {page} = getTestState();
    // [ts] const a = {
    // [ts] sum: (a, b) => {
    // [ts] const _arrow = () => {};
    // [ts] _arrow();
    // [ts] return a + b;
    // [ts] },
    // [ts] };
    // [ts] expect_eq(page.evaluate(a.sum, 1, 2), 3);
    // [ts] });
    // [ts] test("should work with unicode chars", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(
    // [ts] a => {
    // [ts] return a["Ã¤Â¸Â­Ã¦ÂÂÃ¥Â­ÂÃ§Â¬Â¦"];
    // [ts] },
    // [ts] {
    // [ts] 中文字符: 42,
    // [ts] },
    // [ts] );
    // [ts] expect_eq(result, 42);
    // [ts] });
    // [ts] test("should throw when evaluation triggers reload", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluate(() => {
    // [ts] location.reload();
    // [ts] return Promise.init(() => {});
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "Execution context was destroyed", // Chrome
    // [ts] "no such frame", // Firefox
    // [ts] ]);
    // [ts] });
    // [ts] test("should await promise", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return Promise.resolve(8 * 7);
    // [ts] });
    // [ts] expect_eq(result, 56);
    // [ts] });
    // [ts] test("should work right after framenavigated", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] let frameEvaluation = null;
    // [ts] page.on("framenavigated", async frame => {
    // [ts] frameEvaluation = frame.evaluate(() => {
    // [ts] return 6 * 7;
    // [ts] });
    // [ts] });
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(frameEvaluation, 42);
    // [ts] });
    // [ts] test("should work from-inside an exposed function", () => {
    // [ts] const {page} = getTestState();
    // Setup inpage callback, which calls Page.evaluate
    // [ts] page.exposeFunction(
    // [ts] "callController",
    // [ts] (a, b) => {
    // [ts] return page.evaluate(
    // [ts] (a, b) => {
    // [ts] return a * b;
    // [ts] },
    // [ts] a,
    // [ts] b,
    // [ts] );
    // [ts] },
    // [ts] );
    // [ts] const result = page.evaluate(() => {
    // [ts] return (globalThis).callController(9, 3);
    // [ts] });
    // [ts] expect_eq(result, 27);
    // [ts] });
    // [ts] test("should reject promise with exception", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluate(() => {
    // @ts-expect-error we know the object doesn't exist
    // [ts] return notExistingObject.property;
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "notExistingObject") != null, true);
    // [ts] });
    // [ts] test("should support thrown strings as error messages", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluate(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw 'qwerty';)
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, "qwerty");
    // [ts] });
    // [ts] test("should support thrown numbers as error messages", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluate(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw 100500;)
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, 100500);
    // [ts] });
    // [ts] test("should support thrown platform objects as error messages", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluate(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new DOMException('some DOMException message');)
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "some DOMException message") != null, true);
    // [ts] });
    // [ts] test("should return complex objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] const object = {foo: "bar!"};
    // [ts] const result = page.evaluate(a => {
    // [ts] return a;
    // [ts] }, object);
    // [ts] expect_eq(result != object, true);
    // [ts] expect_eq(result, object);
    // [ts] });
    // [ts] test("should return BigInt", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return BigInt(42);
    // [ts] });
    // [ts] expect_eq(result, BigInt(42));
    // [ts] });
    // [ts] test("should return NaN", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return NaN;
    // [ts] });
    // [ts] expect_eq(Object.is(result, NaN), true);
    // [ts] });
    // [ts] test("should return -0", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return -0;
    // [ts] });
    // [ts] expect_eq(Object.is(result, -0), true);
    // [ts] });
    // [ts] test("should return Infinity", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return Infinity;
    // [ts] });
    // [ts] expect_eq(Object.is(result, Infinity), true);
    // [ts] });
    // [ts] test("should return -Infinity", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return -Infinity;
    // [ts] });
    // [ts] expect_eq(Object.is(result, -Infinity), true);
    // [ts] });
    // [ts] test("should return RegEx", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return /(.*)/;
    // [ts] });
    // [ts] expect_eq(result == RegExp, true);
    // [ts] });
    // [ts] test("should accept \"null\" as one of multiple parameters", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(
    // [ts] (a, b) => {
    // [ts] return Object.is(a, null) && Object.is(b, "foo");
    // [ts] },
    // [ts] null,
    // [ts] "foo",
    // [ts] );
    // [ts] expect_eq(result, true);
    // [ts] });
    // [ts] test("should properly serialize null fields", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return {a: null};
    // [ts] }),
    // [ts] ).toEqual({});
    // [ts] });
    // [ts] test("should return undefined for non-serializable objects", () => {
    // [ts] const {page} = getTestState();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return window;
    // [ts] }),
    // [ts] ).toBe(null);
    // [ts] });
    // [ts] test("should return promise as empty object", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] return {
    // [ts] promise: Promise.init(resolve => {
    // [ts] setTimeout(resolve, 1000);
    // [ts] }),
    // [ts] };
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] promise: {},
    // [ts] });
    // [ts] });
    // [ts] test("should work for circular object", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] const a: Map<string, string> = {
    // [ts] c: 5,
    // [ts] d: {
    // [ts] foo: "bar",
    // [ts] },
    // [ts] };
    // [ts] const b = {a};
    // [ts] a["b"] = b;
    // [ts] return a;
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] c: 5,
    // [ts] d: {
    // [ts] foo: "bar",
    // [ts] },
    // [ts] b: {
    // [ts] a: null,
    // [ts] },
    // [ts] });
    // [ts] });
    // [ts] test("should accept a string", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate("1 + 2");
    // [ts] expect_eq(result, 3);
    // [ts] });
    // [ts] test("should accept a string with semi colons", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate("1 + 5;");
    // [ts] expect_eq(result, 6);
    // [ts] });
    // [ts] test("should accept a string with comments", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate("2 + 5;\n// do some math!");
    // [ts] expect_eq(result, 7);
    // [ts] });
    // [ts] test("should accept element handle as an argument", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section>42</section>`);
    // [ts] using element = (page.$("section"));
    // [ts] const text = page.evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }, element);
    // [ts] expect_eq(text, "42");
    // [ts] });
    // [ts] test("should throw if underlying element was disposed", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(html`<section>39</section>`);
    // [ts] using element = (page.$("section"));
    // [ts] expect_eq(element, true);
    // We want to dispose early.
    // [ts] element.dispose();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluate(e => {
    // [ts] return e.textContent;
    // [ts] }, element)
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "JSHandle is disposed") != null, true);
    // [ts] });
    // [ts] test("should throw if elementHandles are from other frames", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] using bodyHandle = page.frames()[1].$("body");
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluate(body => {
    // [ts] return body?.innerHTML;
    // [ts] }, bodyHandle)
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(error, true);
    // [ts] expect(error.message).atLeastOneToContain([
    // [ts] "JSHandles can be evaluated only in the context they were created",
    // [ts] "Trying to evaluate JSHandle from different frames. Usually this means you're using a handle from a page on a different page.",
    // [ts] ]);
    // [ts] });
    // [ts] test("should simulate a user gesture", () => {
    // [ts] const {page} = getTestState();
    // [ts] const result = page.evaluate(() => {
    // [ts] document.body.appendChild(document.createTextNode("test"));
    // [ts] document.execCommand("selectAll");
    // [ts] return document.execCommand("copy");
    // [ts] });
    // [ts] expect_eq(result, true);
    // [ts] });
    // [ts] test("should not throw an error when evaluation does a navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/one-style.html");
    // [ts] const onRequest = server.waitForRequest("/empty.html");
    // [ts] const result = page.evaluate(() => {
    // [ts] (window).location = "/empty.html";
    // [ts] return [42];
    // [ts] });
    // [ts] expect_eq(result, [42]);
    // [ts] onRequest;
    // [ts] });
    // [ts] test("should transfer 100Mb of data from page to node.js", () => {
    // [ts] _.timeout(25_000);
    // [ts] const {page} = getTestState();
    // [ts] const a = page.evaluate(() => {
    // [ts] return Array(100 * 1024 * 1024 + 1).join("a");
    // [ts] });
    // [ts] expect_eq(a.length, 100 * 1024 * 1024);
    // [ts] });
    // [ts] test("should throw error with detailed information on exception inside promise", () => {
    // [ts] const {page} = getTestState();
    // [ts] let error: Error;
    // [ts] page
    // [ts] .evaluate(() => {
    // [ts] return Promise.init(() => {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Error in promise');)
    // [ts] });
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Error in promise") != null, true);
    // [ts] });
    // [ts] test("should return properly serialize objects with unknown type fields", () => {
    // [ts] const {page} = getTestState();
    // [ts] page.setContent(
    // [ts] html`<img
    // [ts] src="data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNkYPhfDwAChwGA60e6kgAAAABJRU5ErkJggg=="
    // [ts] />`,
    // [ts] );
    // [ts] const result = page.evaluate(() => {
    // [ts] const image = document.querySelector("img");
    // [ts] const imageBitmap = createImageBitmap(image);
    // [ts] return {
    // [ts] a: "foo",
    // [ts] b: imageBitmap,
    // [ts] };
    // [ts] });
    // [ts] expect: (result) => {
    // [ts] a: "foo",
    // [ts] b: null,
    // [ts] });
    // [ts] });
    // [ts] test("should evaluate before anything else on the page", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.evaluateOnNewDocument(() => {
    // [ts] (globalThis).injected = 123;
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/tamperable.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe(123);
    // [ts] });
    // [ts] test("should work with CSP", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] server.setCSP("/empty.html", "script-src " + server.PREFIX);
    // [ts] page.evaluateOnNewDocument(() => {
    // [ts] (globalThis).injected = 123;
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).injected;
    // [ts] }),
    // [ts] ).toBe(123);
    // Make sure CSP works.
    // [ts] page.addScriptTag({content: "window.e = 10;"}).catch(error => {
    // [ts] return void error;
    // [ts] });
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (window).e;
    // [ts] }),
    // [ts] ).toBe(null);
    // [ts] });
    // [ts] test("should remove new document script", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] const {identifier} = page.evaluateOnNewDocument(() => {
    // [ts] (globalThis).injected = 123;
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/tamperable.html");
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result;
    // [ts] }),
    // [ts] ).toBe(123);
    // [ts] page.removeScriptToEvaluateOnNewDocument(identifier);
    // [ts] page.reload();
    // [ts] expect(
    // [ts] page.evaluate(() => {
    // [ts] return (globalThis).result || null;
    // [ts] }),
    // [ts] ).toBe(null);
    // [ts] });
    // [ts] test("should have different execution contexts", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] page.frames()[0].evaluate(() => {
    // [ts] return ((globalThis).FOO = "foo");
    // [ts] });
    // [ts] page.frames()[1].evaluate(() => {
    // [ts] return ((globalThis).FOO = "bar");
    // [ts] });
    // [ts] expect(
    // [ts] page.frames()[0].evaluate(() => {
    // [ts] return (globalThis).FOO;
    // [ts] }),
    // [ts] ).toBe("foo");
    // [ts] expect(
    // [ts] page.frames()[1].evaluate(() => {
    // [ts] return (globalThis).FOO;
    // [ts] }),
    // [ts] ).toBe("bar");
    // [ts] });
    // [ts] test("should have correct execution contexts", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] expect(page.frames()).toHaveLength(2);
    // [ts] expect(
    // [ts] page.frames()[0].evaluate(() => {
    // [ts] return document.body.textContent.trim();
    // [ts] }),
    // [ts] ).toBe("");
    // [ts] expect(
    // [ts] page.frames()[1].evaluate(() => {
    // [ts] return document.body.textContent.trim();
    // [ts] }),
    // [ts] ).toBe(`Hi, I'm frame`);
    // [ts] });
    // [ts] test("should execute after cross-site navigation", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const mainFrame = page.mainFrame();
    // [ts] expect(
    // [ts] mainFrame.evaluate(() => {
    // [ts] return window.location.href;
    // [ts] }),
    // [ts] ).toContain("localhost");
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect(
    // [ts] mainFrame.evaluate(() => {
    // [ts] return window.location.href;
    // [ts] }),
    // [ts] ).toContain("127");
    // [ts] });
}
