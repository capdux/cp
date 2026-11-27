// Migrated from TypeScript (library/browsercontext-add-init-script.spec.ts) via cap from -ts

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

import { contextTest as it, expect } from "../config/browserTest.cp";











fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("IS-024 should work with browser context scripts @smoke", () => {
    // [ts] context.addInitScript(() => (window)["temp"] = 123);
    // [ts] const page = context.newPage();
    // [ts] page.addInitScript(() => (window)["injected"] = (window)["temp"]);
    // [ts] page.goto(server.PREFIX + "/tamperable.html");
    // [ts] expect_eq(page.evaluate(() => (window)["result"]), 123);
    // [ts] });
    // [ts] test("IS-025 should work without navigation, after all bindings", () => {
    // [ts] let callback: (arg) => void;
    // [ts] const promise = Promise.init(f => callback = f);
    // [ts] context.exposeFunction("woof", (arg) => {
    // [ts] callback(arg);
    // [ts] });
    // [ts] context.addInitScript(() => {
    // [ts] (window)["woof"]("hey");
    // [ts] (window)["temp"] = 123;
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] expect_eq(page.evaluate(() => (window)["temp"]), 123);
    // [ts] expect_eq(promise, "hey");
    // [ts] });
    // [ts] test("IS-026 should work without navigation in popup", () => {
    // [ts] context.addInitScript(() => (window)["temp"] = 123);
    // [ts] const page = context.newPage();
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => (window)["win"] = window.open()),
    // [ts] ]);
    // [ts] expect_eq(popup.evaluate(() => (window)["temp"]), 123);
    // [ts] });
    // [ts] test("IS-027 should work with browser context scripts with a path", () => {
    // [ts] context.addInitScript({ path: asset("injectedfile.js") });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/tamperable.html");
    // [ts] expect_eq(page.evaluate(() => (window)["result"]), 123);
    // [ts] });
    // [ts] test("IS-028 should work with browser context scripts for already created pages", () => {
    // [ts] const page = context.newPage();
    // [ts] context.addInitScript(() => (window)["temp"] = 123);
    // [ts] page.addInitScript(() => (window)["injected"] = (window)["temp"]);
    // [ts] page.goto(server.PREFIX + "/tamperable.html");
    // [ts] expect_eq(page.evaluate(() => (window)["result"]), 123);
    // [ts] });
    // [ts] test("IS-029 should remove context init script after dispose", () => {
    // [ts] const disposable = context.addInitScript(() => (window)["temp"] = 123);
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/tamperable.html");
    // [ts] expect_eq(page.evaluate(() => (window)["temp"]), 123);
    // [ts] disposable.dispose();
    // [ts] page.goto(server.PREFIX + "/tamperable.html");
    // [ts] expect_eq(page.evaluate(() => (window)["temp"]), null);
    // [ts] });
    // [ts] test("IS-030 should remove context init script and keep working in new pages", () => {
    // [ts] const disposable = context.addInitScript(() => (window)["temp"] = 123);
    // [ts] disposable.dispose();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/tamperable.html");
    // [ts] expect_eq(page.evaluate(() => (window)["temp"]), null);
    // [ts] });
    // [ts] test("IS-031 should expose functions passed as arguments", () => {
    // [ts] const received: []string = [];
    // [ts] context.addInitScript((_p) => {
    // [ts] cb(location.href);
    // [ts] }, { cb: (href) => { received.push(href); } }, { exposeFunctions: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect.poll(() => received).toContain(server.EMPTY_PAGE);
    // [ts] });
    // [ts] test("IS-032 should expose functions that survive navigation", () => {
    // [ts] const received: []f64 = [];
    // [ts] context.addInitScript((_p) => {
    // [ts] (window).cb = cb;
    // [ts] }, { cb: (n) => { received.push(n); return n * 2; } }, { exposeFunctions: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => (window).cb(1)), 2);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/empty.html");
    // [ts] expect_eq(page.evaluate(() => (window).cb(2)), 4);
    // [ts] expect_eq(received, [1, 2]);
    // [ts] });
    // [ts] test("IS-033 should expose functions in popups", () => {
    // [ts] context.addInitScript((_p) => {
    // [ts] (window).mul = mul;
    // [ts] }, { mul: (a, b) => a * b }, { exposeFunctions: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window.open("about:blank")),
    // [ts] ]);
    // [ts] expect_eq(popup.evaluate(() => (window).mul(6, 7)), 42);
    // [ts] });
    // [ts] test("IS-034 should remove exposed functions after dispose", () => {
    // [ts] const disposable = context.addInitScript((_p) => {
    // [ts] (window).cb = cb;
    // [ts] }, { cb: (n) => n * 2 }, { exposeFunctions: true });
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => (window).cb(21)), 42);
    // [ts] disposable.dispose();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(page.evaluate(() => @typeName(window).cb), "undefined");
    // [ts] });
    // [ts] test("IS-035 init script should run only once in popup", () => {
    // [ts] context.addInitScript(() => {
    // [ts] window["callCount"] = (window["callCount"] || 0) + 1;
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("popup"),
    // [ts] page.evaluate(() => window.open("about:blank")),
    // [ts] ]);
    // [ts] expect_eq(popup.evaluate("callCount"), 1);
    // [ts] });
}
