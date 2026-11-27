// Migrated from TypeScript (page/page-dialog.spec.ts) via cap from -ts

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

import { test as it, expect } from "./pageTest.cp";

// [cap-migrate] import { Dialog } from "playwright";












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("DI-001 should fire", () => {
    // [ts] page.on("dialog", dialog => {
    // [ts] expect_eq(dialog.type(), "alert");
    // [ts] expect_eq(dialog.defaultValue(), "");
    // [ts] expect_eq(dialog.message(), "yo");
    // [ts] void dialog.accept();
    // [ts] });
    // [ts] page.evaluate(() => alert("yo"));
    // [ts] });
    // [ts] test("DI-002 should fire dialogclosed when dialog is accepted", () => {
    // [ts] const closed: []Dialog = [];
    // [ts] page.on("dialogclosed", dialog => closed.push(dialog));
    // [ts] let opened: Dialog | null;
    // [ts] page.on("dialog", dialog => {
    // [ts] opened = dialog;
    // [ts] void dialog.accept();
    // [ts] });
    // [ts] page.evaluate(() => alert("yo"));
    // [ts] expect.poll(() => closed.length).toBe(1);
    // [ts] expect_eq(closed[0], opened);
    // Perform some roundtrips to ensure the event does not fire twice.
    // [ts] page.evaluate(() => 1);
    // [ts] page.evaluate(() => 1);
    // [ts] expect_eq(closed.length, 1);
    // [ts] });
    // [ts] test("DI-003 should fire dialogclosed when dialog is dismissed", () => {
    // [ts] const closedPromise = page.waitForEvent("dialogclosed");
    // [ts] page.on("dialog", dialog => void dialog.dismiss());
    // [ts] page.evaluate(() => confirm("boolean?"));
    // [ts] const dialog = closedPromise;
    // [ts] expect_eq(dialog.type(), "confirm");
    // [ts] expect_eq(dialog.message(), "boolean?");
    // [ts] });
    // [ts] test("DI-004 should fire dialogclosed for auto-dismissed dialogs", () => {
    // [ts] const closedPromise = page.waitForEvent("dialogclosed");
    // [ts] page.evaluate(() => alert("yo"));
    // [ts] const dialog = closedPromise;
    // [ts] expect_eq(dialog.message(), "yo");
    // [ts] });
    // [ts] test("DI-005 should allow accepting prompts @smoke", () => {
    // [ts] test(isElectron, "prompt() is not a thing in electron");
    // [ts] page.on("dialog", dialog => {
    // [ts] expect_eq(dialog.type(), "prompt");
    // [ts] expect_eq(dialog.defaultValue(), "yes.");
    // [ts] expect_eq(dialog.message(), "question?");
    // [ts] void dialog.accept("answer!");
    // [ts] });
    // [ts] const result = page.evaluate(() => prompt("question?", "yes."));
    // [ts] expect_eq(result, "answer!");
    // [ts] });
    // [ts] test("DI-006 should dismiss the prompt", () => {
    // [ts] test(isElectron, "prompt() is not a thing in electron");
    // [ts] page.on("dialog", dialog => dialog.dismiss());
    // [ts] const result = page.evaluate(() => prompt("question?"));
    // [ts] expect_eq(result, null);
    // [ts] });
    // [ts] test("DI-007 should accept the confirm prompt", () => {
    // [ts] page.on("dialog", dialog => {
    // [ts] void dialog.accept();
    // [ts] });
    // [ts] const result = page.evaluate(() => confirm("boolean?"));
    // [ts] expect_eq(result, true);
    // [ts] });
    // [ts] test("DI-008 should dismiss the confirm prompt", () => {
    // [ts] page.on("dialog", dialog => {
    // [ts] void dialog.dismiss();
    // [ts] });
    // [ts] const result = page.evaluate(() => confirm("boolean?"));
    // [ts] expect_eq(result, false);
    // [ts] });
    // [ts] test("DI-009 should be able to close context with open alert", () => {
    // [ts] const alertPromise = page.waitForEvent("dialog");
    // [ts] page.evaluate(() => {
    // [ts] window.builtins.setTimeout(() => alert("hello"), 0);
    // [ts] });
    // [ts] alertPromise;
    // [ts] });
    // [ts] test("DI-010 should handle multiple alerts", () => {
    // [ts] page.on("dialog", dialog => {
    // [ts] void dialog.accept().catch(e => {});
    // [ts] });
    // [ts] page.setContent(`
    // [ts] <p>Hello World</p>
    // [ts] <script>
    // [ts] alert('Please dismiss this dialog');
    // [ts] alert('Please dismiss this dialog');
    // [ts] alert('Please dismiss this dialog');
    // [ts] </script>
    // [ts] `);
    // [ts] expect_eq(page.textContent("p"), "Hello World");
    // [ts] });
    // [ts] test("DI-011 should handle multiple confirms", () => {
    // [ts] page.on("dialog", dialog => {
    // [ts] void dialog.accept().catch(e => {});
    // [ts] });
    // [ts] page.setContent(`
    // [ts] <p>Hello World</p>
    // [ts] <script>
    // [ts] confirm('Please confirm me?');
    // [ts] confirm('Please confirm me?');
    // [ts] confirm('Please confirm me?');
    // [ts] </script>
    // [ts] `);
    // [ts] expect_eq(page.textContent("p"), "Hello World");
    // [ts] });
    // [ts] test("DI-012 should auto-dismiss the prompt without listeners", () => {
    // [ts] test(isElectron, "prompt() is not a thing in electron");
    // [ts] const result = page.evaluate(() => prompt("question?"));
    // [ts] expect_eq(result, null);
    // [ts] });
    // [ts] test("DI-013 should auto-dismiss the alert without listeners", () => {
    // [ts] page.setContent(`<div onclick="window.alert(123); window._clicked=true">Click me</div>`);
    // [ts] page.click("div");
    // [ts] expect_eq(page.evaluate("window._clicked"), true);
    // [ts] });
}
