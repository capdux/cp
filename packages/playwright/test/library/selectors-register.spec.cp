// Migrated from TypeScript (library/selectors-register.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { browserTest as it, expect } from "../config/browserTest.cp";

fn createTagSelector() {
    // TODO(cap-migrate): refine TS body
    // [ts] return ({ query(root, selector) { return root.querySelector(selector); }, queryAll(root, selector) { return Array.from(root.querySelectorAll(selector)); } });
}







fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("SE-160 should work", () => {
    // Register one engine before creating context.
    // [ts] playwright.selectors.register("tag", `(${createTagSelector.toString()})()`);
    // [ts] const context = browser.newContext();
    // Register another engine after creating context.
    // [ts] playwright.selectors.register("tag2", `(${createTagSelector.toString()})()`);
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<div><span></span></div><div></div>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('tag=DIV', e => e.nodeName)).toBe('DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('tag=SPAN', e => e.nodeName)).toBe('SPAN');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('tag=DIV', es => es.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('tag2=DIV', e => e.nodeName)).toBe('DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('tag2=SPAN', e => e.nodeName)).toBe('SPAN');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('tag2=DIV', es => es.length)).toBe(2);
    // Selector names are case-sensitive.
    // [ts] const error = page.$("tAG=DIV").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Unknown engine \"tAG\" while parsing selector tAG=DIV") != null, true);
    // [ts] context.close();
    // [ts] });
    // [ts] test("SE-161 should work when registered on global", () => {
    // [ts] test(mode == "driver", "We expect registering selectors on the right Playwright instance.");
    // [ts] require("@playwright/test").selectors.register("oop-tag", `(${createTagSelector.toString()})()`);
    // [ts] const context = browser.newContext();
    // Register another engine after creating context.
    // [ts] require("@playwright/test").selectors.register("oop-tag2", `(${createTagSelector.toString()})()`);
    // [ts] const page = context.newPage();
    // [ts] page.setContent("<div><span></span></div><div></div>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('oop-tag=DIV', e => e.nodeName)).toBe('DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('oop-tag=SPAN', e => e.nodeName)).toBe('SPAN');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('oop-tag=DIV', es => es.length)).toBe(2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('oop-tag2=DIV', e => e.nodeName)).toBe('DIV');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('oop-tag2=SPAN', e => e.nodeName)).toBe('SPAN');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('oop-tag2=DIV', es => es.length)).toBe(2);
    // [ts] context.close();
    // [ts] });
    // [ts] test("SE-162 should work with path", () => {
    // [ts] playwright.selectors.register("foo", { path: asset("sectionselectorengine.js") });
    // [ts] const page = browser.newPage();
    // [ts] page.setContent("<section></section>");
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('foo=whatever', e => e.nodeName)).toBe('SECTION');
    // [ts] page.close();
    // [ts] });
    // [ts] test("SE-163 should work in main and isolated world", () => {
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] return window["__answer"];
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] return if (window["__answer"]) [window["__answer"], document.body, document.documentElement] else [];
    // [ts] }
    // [ts] });
    // [ts] playwright.selectors.register("main", createDummySelector);
    // [ts] playwright.selectors.register("isolated", createDummySelector, { contentScript: true });
    // [ts] const page = browser.newPage();
    // [ts] page.setContent("<div><span><section></section></span></div>");
    // [ts] page.evaluate(() => window["__answer"] = document.querySelector("span"));
    // Works in main if asked.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('main=ignored', e => e.nodeName)).toBe('SPAN');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('css=div >> main=ignored', e => e.nodeName)).toBe('SPAN');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('main=ignored', es => window['__answer'] !== undefined)).toBe(true);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('main=ignored', es => es.filter(e => e).length)).toBe(3);
    // Works in isolated by default.
    // [ts] expect_eq(page.$("isolated=ignored"), null);
    // [ts] expect_eq(page.$("css=div >> isolated=ignored"), null);
    // $$eval always works in main, to avoid adopting nodes one by one.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('isolated=ignored', es => window['__answer'] !== undefined)).toBe(true);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$$eval('isolated=ignored', es => es.filter(e => e).length)).toBe(3);
    // At least one engine in main forces all to be in main.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('main=ignored >> isolated=ignored', e => e.nodeName)).toBe('SPAN');
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('isolated=ignored >> main=ignored', e => e.nodeName)).toBe('SPAN');
    // Can be chained to css.
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('main=ignored >> css=section', e => e.nodeName)).toBe('SECTION');
    // [ts] page.close();
    // [ts] });
    // [ts] test("SE-164 should handle errors", () => {
    // [ts] const page = browser.newPage();
    // [ts] let error = page.$("neverregister=ignored").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Unknown engine \"neverregister\" while parsing selector neverregister=ignored") != null, true);
    // [ts] const createDummySelector = () => ({
    // [ts] query: (root, selector) => {
    // [ts] return root.querySelector("dummy");
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] return Array.from(root.querySelectorAll("dummy"));
    // [ts] }
    // [ts] });
    // [ts] error = playwright.selectors.register("$", createDummySelector).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Selector engine name may only contain [a-zA-Z0-9_] characters") != null, true);
    // Selector names are case-sensitive.
    // [ts] playwright.selectors.register("dummy", createDummySelector);
    // [ts] playwright.selectors.register("duMMy", createDummySelector);
    // [ts] error = playwright.selectors.register("dummy", createDummySelector).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"dummy\" selector engine has been already registered") != null, true);
    // [ts] error = playwright.selectors.register("css", createDummySelector).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "\"css\" is a predefined selector engine") != null, true);
    // [ts] page.close();
    // [ts] });
    // [ts] test("SE-165 should throw \"already registered\" error when registering", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/36467" } }, (_p) => {
    // [ts] playwright.selectors.register("alreadyRegistered", createTagSelector);
    // [ts] expect(playwright.selectors.register("alreadyRegistered", createTagSelector)).rejects.toThrowError("\"alreadyRegistered\" selector engine has been already registered");
    // [ts] });
    // [ts] test("SE-166 should not rely on engines working from the root", () => {
    // [ts] const createValueEngine = () => ({
    // [ts] query: (root, selector) => {
    // [ts] return if (root && root.value.includes(selector)) root else null;
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] return if (root && root.value.includes(selector)) [root] else [];
    // [ts] },
    // [ts] });
    // [ts] playwright.selectors.register("__value", createValueEngine);
    // [ts] const page = browser.newPage();
    // [ts] page.setContent(`<input id=input1 value=value1><input id=input2 value=value2>`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('input >> __value=value2', e => e.id)).toBe('input2');
    // [ts] page.close();
    // [ts] });
    // [ts] test("SE-167 should throw a nice error if the selector returns a bad value", () => {
    // [ts] const createFakeEngine = () => ({
    // [ts] query: (root, selector) => {
    // [ts] return [document.body];
    // [ts] },
    // [ts] queryAll: (root, selector) => {
    // [ts] return [[document.body]];
    // [ts] },
    // [ts] });
    // [ts] playwright.selectors.register("__fake", createFakeEngine);
    // [ts] const page = browser.newPage();
    // [ts] const error = page.$("__fake=value2").catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Expected a Node but got [object Array]") != null, true);
    // [ts] page.close();
    // [ts] });
}
