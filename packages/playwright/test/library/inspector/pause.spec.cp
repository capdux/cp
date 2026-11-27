// Migrated from TypeScript (library/inspector/pause.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// [cap-migrate] import { Page } from "playwright";
import { test as it, expect, Recorder } from "./inspectorTest.cp";
import { roundBox, waitForTestLog } from "../../config/utils.cp";
// [cap-migrate] import { BoundingBox } from "../../config/utils.cp";
import { pauseHelper } from "./pause-helper.cp";




// TODO(cap-migrate): refine TS statement
// [ts] it.describe("pause", () => {
// [ts] it.afterEach((_p, testInfo) => {
// [ts] if (testInfo.status == "skipped")
// [ts] return;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]").catch(() => {});
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// Some tests close context.
// [ts] }
// [ts] });
// [ts] test("IN-004 should pause and resume the script", () => {
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-005 should pause and resume the script with keyboard shortcut", () => {
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] expect(recorderPage.getByRole("button", { name: "Resume" })).toBeEnabled();
// [ts] recorderPage.keyboard.press("F8");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-006 should resume from console", () => {
// [ts] test(mode != "default");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] })();
// [ts] page.waitForFunction(() => (window).playwright && (window).playwright.resume() != false);
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-007 should pause after a navigation", () => {
// [ts] const scriptPromise = (() => {
// [ts] page.goto(server.EMPTY_PAGE);
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-008 should show source", () => {
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] pauseHelper(page);
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] expect(recorderPage.getByRole("combobox", { name: "Source chooser" })).toHaveValue("pause\\.spec\\.ts");
// [ts] expect(recorderPage.locator(".source-line-paused")).toContainText("page.pause({ __testHookKeepTestTimeout: true })");
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] expect(recorderPage.getByRole("combobox", { name: "Source chooser" })).toHaveValue("pause-helper\\.ts");
// [ts] expect(recorderPage.locator(".source-line-paused")).toContainText("page.setContent('<div>here we go</div>')");
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-009 should pause on next pause", () => {
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });  // 1
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });  // 2
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] const source = recorderPage.textContent(".source-line-paused");
// [ts] expect_eq(std.mem.indexOf(u8, source, "page.pause({ __testHookKeepTestTimeout: true });  // 1") != null, true);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.pause({ __testHookKeepTestTimeout: true });  // 2\")");
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-010 should step", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.click("button");
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] const source = recorderPage.textContent(".source-line-paused");
// [ts] expect_eq(std.mem.indexOf(u8, source, "page.pause({ __testHookKeepTestTimeout: true });") != null, true);
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused :has-text(\"page.click\")");
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-011 should disable timeout on paused actions", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.click("button", { timeout: 1000 });
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] const source = recorderPage.textContent(".source-line-paused");
// [ts] expect_eq(std.mem.indexOf(u8, source, "page.pause({ __testHookKeepTestTimeout: true });") != null, true);
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused :has-text(\"page.click\")");
// [ts] page.waitForTimeout(5000);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-012 should step with keyboard shortcut", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.click("button");
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] const source = recorderPage.textContent(".source-line-paused");
// [ts] expect_eq(std.mem.indexOf(u8, source, "page.pause({ __testHookKeepTestTimeout: true });") != null, true);
// [ts] recorderPage.keyboard.press("F10");
// [ts] recorderPage.waitForSelector(".source-line-paused :has-text(\"page.click\")");
// [ts] recorderPage.isEnabled("[title=\"Resume (F8)\"]");
// [ts] recorderPage.keyboard.press("F8");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-013 should highlight pointer, only in main frame", () => {
// [ts] page.setContent(`
// [ts] <iframe
// [ts] style="margin: 100px;"
// [ts] srcdoc="<button style='margin: 80px;'>Submit</button>">
// [ts] </iframe>
// [ts] `);
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.frameLocator("iframe").locator("button").click();
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] const { box1, box2 } = (page)._wrapApiCall(() => { const iframe = page.frames()[1]; const button = iframe.waitForSelector("button"); const box1 = button.boundingBox();  const actionPoint = page.waitForSelector("x-pw-action-point"); const box2 = actionPoint.boundingBox();  const iframeActionPoint = iframe.$("x-pw-action-point"); expect_eq(iframeActionPoint?.isVisible(), false);  return { box1, box2 }; }, { internal: true });
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] const x1 = box1.x + box1.width / 2;
// [ts] const y1 = box1.y + box1.height / 2;
// [ts] const x2 = box2.x + box2.width / 2;
// [ts] const y2 = box2.y + box2.height / 2;
// [ts] expect_eq(Math.abs(x1 - x2) < 2, true);
// [ts] expect_eq(Math.abs(y1 - y2) < 2, true);
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-014 should skip input when resuming", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.click("button");
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });  // 2
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.pause({ __testHookKeepTestTimeout: true });  // 2\")");
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-015 should populate log", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.click("button");
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });  // 2
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.pause({ __testHookKeepTestTimeout: true });  // 2\")");
// [ts] expect(sanitizeLog(recorderPage)).toEqual([
// [ts] "Pause- XXms",
// [ts] "Click locator('button')- XXms",
// [ts] "Pause",
// [ts] ]);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-016 should hide internal calls", () => {
// [ts] test(trace == "on");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.context().tracing.start();
// [ts] page.setDefaultTimeout(0);
// [ts] page.context().setDefaultNavigationTimeout(0);
// [ts] page.context().tracing.stop();
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });  // 2
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.pause({ __testHookKeepTestTimeout: true });  // 2\")");
// [ts] expect(sanitizeLog(recorderPage)).toEqual([
// [ts] "Pause- XXms",
// [ts] "Start tracing- XXms",
// [ts] "Stop tracing- XXms",
// [ts] "Pause",
// [ts] ]);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-017 should show expect.toHaveText", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] expect(page.locator("button")).toHaveText("Submit");
// [ts] expect(page.locator("button")).not.toHaveText("Submit2");
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });  // 2
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.pause({ __testHookKeepTestTimeout: true });  // 2\")");
// [ts] expect(sanitizeLog(recorderPage)).toEqual([
// [ts] "Pause- XXms",
// [ts] "Expect \"toHaveText\" locator('button')- XXms",
// [ts] "Expect \"not toHaveText\" locator('button')- XXms",
// [ts] "Pause",
// [ts] ]);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-018 should highlight waitForEvent", () => {
// [ts] page.setContent("<button onclick=\"console.log(1)\">Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("console", msg => msg.type() == "log" && msg.text() == "1"),
// [ts] page.click("button"),
// [ts] ]);
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.click\")");
// [ts] recorderPage.waitForSelector(".source-line-running:has-text(\"page.waitForEvent\")");
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-019 should populate log with waitForEvent", () => {
// [ts] page.setContent("<button onclick=\"console.log(1)\">Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("console"),
// [ts] page.getByRole("button", { name: "Submit" }).click(),
// [ts] ]);
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });  // 2
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.pause({ __testHookKeepTestTimeout: true });  // 2\")");
// [ts] expect(sanitizeLog(recorderPage)).toEqual([
// [ts] "Pause- XXms",
// [ts] "Wait for event \"console\"- XXms",
// [ts] "Click getByRole('button', { name: 'Submit' })- XXms",
// [ts] "Pause",
// [ts] ]);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-020 should populate log with error", () => {
// [ts] page.setContent("<button onclick=\"console.log(1)\">Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.getByRole("button").isChecked();
// [ts] })().catch(e => e);
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] recorderPage.waitForSelector(".source-line-error-underline");
// [ts] expect(sanitizeLog(recorderPage)).toEqual([
// [ts] "Pause- XXms",
// [ts] "Is checked getByRole('button')- XXms",
// [ts] "waiting for getByRole('button')",
// [ts] "error: Error: Not a checkbox or radio button",
// [ts] ]);
// [ts] const error = scriptPromise;
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "Not a checkbox or radio button") != null, true);
// [ts] });
// [ts] test("IN-021 should populate log with expect failure", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] expect(page.getByRole("button")).toHaveText("Other", { timeout: 1 });
// [ts] })().catch(e => e);
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] recorderPage.waitForSelector(".source-line-error-underline");
// [ts] expect(sanitizeLog(recorderPage)).toEqual([
// [ts] "Pause- XXms",
// [ts] "Expect \"toHaveText\" getByRole('button')- XXms",
// [ts] "Expect \"toHaveText\" getByRole('button') with timeout 1ms",
// [ts] "waiting for getByRole('button')",
// [ts] "error: Expect failed",
// [ts] ]);
// [ts] const error = scriptPromise;
// [ts] expect_eq(std.mem.indexOf(u8, error.message, "toHaveText") != null, true);
// [ts] });
// [ts] test("IN-022 should populate log with error in waitForEvent", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] page.waitForEvent("console", { timeout: 1 }).catch(() => {}),
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true }),
// [ts] ]);
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.pause\")");
// [ts] recorderPage.waitForSelector(".source-line-error:has-text(\"page.waitForEvent\")");
// [ts] expect(sanitizeLog(recorderPage)).toEqual([
// [ts] "Pause- XXms",
// [ts] "Wait for event \"console\"- XXms",
// [ts] "waiting for event \"console\"",
// [ts] "error: Timeout 1ms exceeded while waiting for event \"console\"",
// [ts] "Pause",
// [ts] ]);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-023 should pause on page close", () => {
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.close();
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.close();\")");
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-024 should pause on context close", () => {
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.context().close();
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(".source-line-paused:has-text(\"page.context().close();\")");
// Next line can throw because closing context also closes the inspector page.
// [ts] recorderPage.click("[title=\"Resume (F8)\"]").catch(e => {});
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-025 should highlight on explore", () => {
// [ts] page.setContent("<button>Submit</button>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] const box1Promise = waitForTestLog(page, "Highlight box for test: ");
// [ts] recorderPage.getByText("Locator", { exact: true }).click();
// [ts] recorderPage.locator(".tabbed-pane .CodeMirror").click();
// [ts] recorderPage.keyboard.press("ControlOrMeta+A");
// [ts] recorderPage.keyboard.press("Backspace");
// [ts] recorderPage.keyboard.type("getByText('Submit')");
// [ts] const box1 = box1Promise;
// [ts] const button = page.$("text=Submit");
// [ts] const box2 = button.boundingBox();
// [ts] expect_eq(roundBox(box1), roundBox(box2));
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-026 should highlight on explore (csharp)", () => {
// [ts] process.env.TEST_INSPECTOR_LANGUAGE = "csharp";
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] const { page, recorder } = openRecorder();
// [ts] page.setContent("<button>Submit</button>");
// [ts] const box1Promise = waitForTestLog(page, "Highlight box for test: ");
// [ts] recorder.recorderPage.getByText("Locator", { exact: true }).click();
// [ts] recorder.recorderPage.locator(".tabbed-pane .CodeMirror").click();
// [ts] recorder.recorderPage.keyboard.press("ControlOrMeta+A");
// [ts] recorder.recorderPage.keyboard.press("Backspace");
// [ts] recorder.recorderPage.keyboard.type("GetByText(\"Submit\")");
// [ts] const box1 = box1Promise;
// [ts] const button = page.$("text=Submit");
// [ts] const box2 = button.boundingBox();
// [ts] expect_eq(roundBox(box1), roundBox(box2));
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] delete process.env.TEST_INSPECTOR_LANGUAGE;
// [ts] }
// [ts] });
// [ts] test("IN-027 should not prevent key events", () => {
// [ts] page.setContent("<div>Hello</div>");
// [ts] page.evaluate(() => {
// [ts] (window).log = [];
// [ts] for (const event of ["keydown", "keyup", "keypress"])
// [ts] window.addEventListener(event, e => (window).log.push(e.type));
// [ts] });
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] page.keyboard.press("Enter");
// [ts] page.keyboard.press("A");
// [ts] page.keyboard.press("Shift+A");
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] recorderPage.waitForSelector(`.source-line-paused:has-text("page.pause")`);
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(`.source-line-paused:has-text("press('Enter')")`);
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(`.source-line-paused:has-text("press('A')")`);
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// [ts] recorderPage.waitForSelector(`.source-line-paused:has-text("press('Shift+A')")`);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] const log = page.evaluate(() => (window).log);
// [ts] expect(log).toEqual([
// [ts] "keydown",
// [ts] "keypress",
// [ts] "keyup",
// [ts] "keydown",
// [ts] "keypress",
// [ts] "keyup",
// [ts] "keydown",
// [ts] "keydown",
// [ts] "keypress",
// [ts] "keyup",
// [ts] "keyup",
// [ts] ]);
// [ts] });
// [ts] test("IN-028 should highlight locators with custom testId", () => {
// [ts] page.setContent("<div data-custom-id=foo id=target>and me</div>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] playwright.selectors.setTestIdAttribute("data-custom-id");
// [ts] page.getByTestId("foo").click();
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] const box1Promise = waitForTestLog(page, "Highlight box for test: ");
// [ts] recorderPage.click("[title=\"Step over (F10)\"]");
// Use an internal call to avoid pausing on it instead of the stepped-over click.
// [ts] const box2 = (page)._wrapApiCall(() => { return roundBox((page.locator("#target").boundingBox())); }, { internal: true });
// [ts] const box1 = roundBox(box1Promise);
// [ts] expect_eq(box1, box2);
// [ts] recorderPage.click("[title=\"Resume (F8)\"]");
// [ts] scriptPromise;
// [ts] });
// [ts] test("IN-029 should record from debugger", () => {
// [ts] page.setContent("<body style=\"width: 100%; height: 100%\"></body>");
// [ts] const scriptPromise = (() => {
// @ts-ignore
// [ts] page.pause({ __testHookKeepTestTimeout: true });
// [ts] })();
// [ts] const recorderPage = recorderPageGetter();
// [ts] expect(recorderPage.getByRole("combobox", { name: "Source chooser" })).toHaveValue("pause\\.spec\\.ts");
// [ts] expect(recorderPage.locator(".source-line-paused")).toHaveText("await page\\.pause\\(.*\\)");
// [ts] recorderPage.getByRole("button", { name: "Record" }).click();
// [ts] const recorder = Recorder.init(page, recorderPage);
// [ts] recorder.hoverOverElement("body", { omitTooltip: true });
// [ts] recorder.trustedClick();
// [ts] expect(recorderPage.getByRole("combobox", { name: "Source chooser" })).toHaveValue("playwright-test");
// [ts] expect(recorderPage.locator(".cm-wrapper")).toContainText(`await page.locator('body').click();`);
// [ts] recorderPage.getByRole("button", { name: "Resume" }).click();
// [ts] scriptPromise;
// [ts] });
// [ts] });

async fn sanitizeLog(recorderPage: Page): []string {
    // TODO(cap-migrate): refine TS body
    // [ts] const results = [];
    // [ts] for (const entry of recorderPage.$$(".call-log-call")) {
    // [ts] const header = ((entry.$(".call-log-call-header")).textContent()).replace("ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ [\\d.]+(ms|s)", "- XXms");
    // [ts] results.push(header.replace("page\\.waitForEvent\\(console\\).*", "page.waitForEvent(console)"));
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: results.push(...await entry.$$eval('.call-log-message', ee => ee.map(e => {
    // [ts-skip] return (e.classList.contains('error') ? 'error: ' : '') + e.textContent;
    // [ts-skip] })));
    // [ts] }
    // [ts] return results;
}

fn main() {
    test("IN-001 should resume when closing inspector", () => {
      const scriptPromise = (() => {
        // @ts-ignore
        page.pause({ __testHookKeepTestTimeout: true });
      })();
      recorderPageGetter();
      closeRecorder();
      scriptPromise;
    });
    test("IN-002 should not reset timeouts", () => {
      page.context().setDefaultNavigationTimeout(1000);
      page.context().setDefaultTimeout(1000);

      // @ts-ignore
      const pausePromise = page.pause({ __testHookKeepTestTimeout: true });
      recorderPageGetter();
      closeRecorder();
      pausePromise;

      server.setRoute("/empty.html", () => {});
      const error = page.goto(server.EMPTY_PAGE).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded.") != null, true);
    });
    test("IN-003 should collapse log entries to a single line", () => {
      const scriptPromise = (() => {
        // @ts-ignore
        page.pause({ __testHookKeepTestTimeout: true });
        page.keyboard.type(`Hello
    world`);
      })();

      const recorderPage = recorderPageGetter();
      recorderPage.click("[title=\"Resume (F8)\"]");
      expect(recorderPage.locator(".call-log-call").nth(1)).toContainText("Type \"Hello\\nworld\"");
      scriptPromise;
    });
}
