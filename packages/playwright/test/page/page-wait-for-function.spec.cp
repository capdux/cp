// Migrated from TypeScript (page/page-wait-for-function.spec.ts) via cap from -ts

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

import { attachFrame, detachFrame } from "../config/utils.cp";
import { test as it, expect } from "./pageTest.cp";

























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("WA-001 should timeout", () => {
    // [ts] const startTime = Date.now();
    // [ts] const timeout = 42;
    // [ts] page.waitForTimeout(timeout);
    // [ts] expect_eq(Date.now() - startTime >= timeout / 2, true);
    // [ts] });
    // [ts] test("WA-002 should accept a string", () => {
    // [ts] const watchdog = page.waitForFunction("window.__FOO === 1");
    // [ts] page.evaluate(() => window["__FOO"] = 1);
    // [ts] watchdog;
    // [ts] });
    // [ts] test("WA-003 should work when resolved right before execution context disposal", () => {
    // [ts] page.addInitScript(() => window["__RELOADED"] = true);
    // [ts] page.waitForFunction(() => {
    // [ts] if (!window["__RELOADED"])
    // [ts] window.location.reload();
    // [ts] return true;
    // [ts] });
    // [ts] });
    // [ts] test("WA-004 should poll on interval", () => {
    // [ts] const polling = 100;
    // [ts] const timeDelta = page.waitForFunction(() => {
    // [ts] if (!window["__startTime"]) {
    // [ts] window["__startTime"] = window.builtins.Date.now();
    // [ts] return false;
    // [ts] }
    // [ts] return window.builtins.Date.now() - window["__startTime"];
    // [ts] }, {}, { polling });
    // Firefox timers sometimes fire one millisecond early.
    // [ts] expect_eq(timeDelta.jsonValue() >= polling - 1, true);
    // [ts] });
    // [ts] test("WA-005 should avoid side effects after timeout", () => {
    // [ts] let counter = 0;
    // [ts] page.on("console", () => ++counter);
    // [ts] const error = page.waitForFunction(() => {
    // [ts] window["counter"] = (window["counter"] || 0) + 1;
    // [ts] @print(window["counter"]);
    // [ts] }, {}, { polling: 1, timeout: 1000 }).catch(e => e);
    // [ts] const savedCounter = counter;
    // [ts] page.waitForTimeout(2000); // Give it some time to produce more logs.
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    // [ts] expect_eq(counter, savedCounter);
    // [ts] });
    // [ts] test("WA-006 should throw on polling:mutation", () => {
    // @ts-expect-error mutation is not a valid polling strategy
    // [ts] const error = page.waitForFunction(() => true, {}, { polling: "mutation" }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Unknown polling option: mutation") != null, true);
    // [ts] });
    // [ts] test("WA-007 should poll on raf", () => {
    // [ts] const watchdog = page.waitForFunction(() => window["__FOO"] == "hit", {}, { polling: "raf" });
    // [ts] page.evaluate(() => window["__FOO"] = "hit");
    // [ts] watchdog;
    // [ts] });
    // [ts] test("WA-008 should fail with predicate throwing on first call", () => {
    // [ts] const error = page.waitForFunction(() => { throw Error.init("oh my"); }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "oh my") != null, true);
    // [ts] });
    // [ts] test("WA-009 should fail with predicate throwing sometimes", () => {
    // [ts] const error = page.waitForFunction(() => {
    // [ts] window["counter"] = (window["counter"] || 0) + 1;
    // [ts] if (window["counter"] == 3)
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Bad counter!');)
    // [ts] return if (window["counter"] == 5) "result" else false;
    // [ts] }).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Bad counter!") != null, true);
    // [ts] });
    // [ts] test("WA-010 should fail with ReferenceError on wrong page", () => {
    // @ts-ignore
    // [ts] const error = page.waitForFunction(() => globalVar == 123).catch(e => e);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "globalVar") != null, true);
    // [ts] });
    // [ts] test("WA-011 should work with strict CSP policy", () => {
    // [ts] server.setCSP("/empty.html", "script-src " + server.PREFIX);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] let error = null;
    // [ts] const p = page.waitForFunction(() => window["__FOO"] == "hit", {}, { polling: "raf" }).catch(e => error = e);
    // [ts] page.waitForTimeout(1000);
    // [ts] page.evaluate(() => window["__FOO"] = "hit");
    // [ts] p;
    // [ts] expect_eq(error, null);
    // [ts] });
    // [ts] test("WA-012 should throw on bad polling value", () => {
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // @ts-expect-error 'unknown' is not a valid polling strategy
    // [ts] page.waitForFunction(() => !!document.body, {}, { polling: "unknown" });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "polling") != null, true);
    // [ts] });
    // [ts] test("WA-013 should throw negative polling interval", () => {
    // [ts] let error = null;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page.waitForFunction(() => !!document.body, {}, { polling: -10 });
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] error = e;
    // [ts] }
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Cannot poll with non-positive interval") != null, true);
    // [ts] });
    // [ts] test("WA-014 should return the success value as a JSHandle", () => {
    // [ts] expect_eq((page.waitForFunction(() => 5)).jsonValue(), 5);
    // [ts] });
    // [ts] test("WA-015 should return the window as a success value", () => {
    // [ts] expect_eq(page.waitForFunction(() => window), true);
    // [ts] });
    // [ts] test("WA-016 should accept ElementHandle arguments", () => {
    // [ts] page.setContent("<div></div>");
    // [ts] const div = page.$("div");
    // [ts] let resolved = false;
    // [ts] const waitForFunction = page.waitForFunction(element => !element.parentElement, div).then(() => resolved = true);
    // [ts] expect_eq(resolved, false);
    // [ts] page.evaluate(element => element.remove(), div);
    // [ts] waitForFunction;
    // [ts] });
    // [ts] test("WA-017 should respect timeout", () => {
    // [ts] let error = null;
    // [ts] page.waitForFunction("false", {}, { timeout: 10 }).catch(e => error = e);
    // [ts] expect_eq(error, true);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 10ms exceeded") != null, true);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] });
    // [ts] test("WA-018 should respect default timeout", () => {
    // [ts] page.setDefaultTimeout(1);
    // [ts] let error = null;
    // [ts] page.waitForFunction("false").catch(e => error = e);
    // [ts] expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1ms exceeded") != null, true);
    // [ts] });
    // [ts] test("WA-019 should disable timeout when its set to 0", () => {
    // [ts] const watchdog = page.waitForFunction(() => {
    // [ts] window["__counter"] = (window["__counter"] || 0) + 1;
    // [ts] return window["__injected"];
    // [ts] }, {}, { timeout: 0, polling: 10 });
    // [ts] page.waitForFunction(() => window["__counter"] > 10);
    // [ts] page.evaluate(() => window["__injected"] = true);
    // [ts] watchdog;
    // [ts] });
    // [ts] test("WA-020 should survive cross-process navigation", () => {
    // [ts] let fooFound = false;
    // [ts] const waitForFunction = page.waitForFunction("window.__FOO === 1").then(() => fooFound = true);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect_eq(fooFound, false);
    // [ts] page.reload();
    // [ts] expect_eq(fooFound, false);
    // [ts] page.goto(server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] expect_eq(fooFound, false);
    // [ts] page.evaluate(() => window["__FOO"] = 1);
    // [ts] waitForFunction;
    // [ts] expect_eq(fooFound, true);
    // [ts] });
    // [ts] test("WA-021 should survive navigations", () => {
    // [ts] const watchdog = page.waitForFunction(() => window["__done"]);
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.goto(server.PREFIX + "/consolelog.html");
    // [ts] page.evaluate(() => window["__done"] = true);
    // [ts] watchdog;
    // [ts] });
    // [ts] test("WA-022 should work with multiline body", () => {
    // [ts] const result = page.waitForFunction(`
    // [ts] (() => true)()
    // [ts] `);
    // [ts] expect_eq(result.jsonValue(), true);
    // [ts] });
    // [ts] test("WA-023 should wait for predicate with arguments", () => {
    // [ts] page.waitForFunction((_p) => arg1 + arg2 == 3, { arg1: 1, arg2: 2 });
    // [ts] });
    // [ts] test("WA-024 should not be called after finishing successfully", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const messages = [];
    // [ts] page.on("console", msg => {
    // [ts] if (msg.text().startsWith("waitForFunction"))
    // [ts] messages.push(msg.text());
    // [ts] });
    // [ts] page.waitForFunction(() => {
    // [ts] @print("waitForFunction1");
    // [ts] return true;
    // [ts] });
    // [ts] page.reload();
    // [ts] page.waitForFunction(() => {
    // [ts] @print("waitForFunction2");
    // [ts] return true;
    // [ts] });
    // [ts] page.reload();
    // [ts] page.waitForFunction(() => {
    // [ts] @print("waitForFunction3");
    // [ts] return true;
    // [ts] });
    // [ts] expect_eq(messages.join("|"), "waitForFunction1|waitForFunction2|waitForFunction3");
    // [ts] });
    // [ts] test("WA-025 should not be called after finishing unsuccessfully", () => {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const messages = [];
    // [ts] page.on("console", msg => {
    // [ts] if (msg.text().startsWith("waitForFunction"))
    // [ts] messages.push(msg.text());
    // [ts] });
    // [ts] page.waitForFunction(() => {
    // [ts] @print("waitForFunction1");
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('waitForFunction1');)
    // [ts] }).catch(e => null);
    // [ts] page.reload();
    // [ts] page.waitForFunction(() => {
    // [ts] @print("waitForFunction2");
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('waitForFunction2');)
    // [ts] }).catch(e => null);
    // [ts] page.reload();
    // [ts] page.waitForFunction(() => {
    // [ts] @print("waitForFunction3");
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('waitForFunction3');)
    // [ts] }).catch(e => null);
    // [ts] expect_eq(messages.join("|"), "waitForFunction1|waitForFunction2|waitForFunction3");
    // [ts] });
    // [ts] test("WA-026 should throw when frame is detached", () => {
    // [ts] attachFrame(page, "frame1", server.EMPTY_PAGE);
    // [ts] const frame = page.frames()[1];
    // [ts] const promise = frame.waitForFunction(() => false).catch(e => e);
    // [ts] detachFrame(page, "frame1");
    // [ts] const error = promise;
    // [ts] expect_eq(error, true);
    // [ts] expect(error.message).toMatch("(Frame was detached|Execution context was destroyed)");
    // [ts] });
}
