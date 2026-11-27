// Migrated from TypeScript (library/screencast-actions.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { expect, browserTest as test } from "../config/browserTest.cp";


const style = {
  point: "width: 20px; height: 20px; border-radius: 50%; background: red",
  highlight: "outline: 2px solid #333",
};



// TODO(cap-migrate): refine TS statement
// [ts] for (const { position, expected } of [
// [ts] { position: "top-left", expected: { top: "6px", left: "6px" } },
// [ts] { position: "top", expected: { top: "6px", left: "50%" } },
// [ts] { position: "bottom-left", expected: { bottom: "6px", left: "6px" } },
// [ts] { position: "bottom", expected: { bottom: "6px", left: "50%" } },
// [ts] { position: "bottom-right", expected: { bottom: "6px", right: "6px" } },
// [ts] ]) { test(`SC-018 should position title at ${position}`, (_p) => { const context = browser.newContext(); const page = context.newPage(); page.goto(server.PREFIX + "/input/button.html");  page.screencast.showActions({ duration: 5000, position }); page.click("button").catch(() => {});  const title = page.locator("x-pw-title"); expect(title).toBeVisible();  const titleStyle = title.evaluate((el) => ({ top: el.style.top, bottom: el.style.bottom, left: el.style.left, right: el.style.right, }));  for (const [key, value] of Object.entries(expected)) expect_eq(titleStyle[key], value);  context.close(); }); }








fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test((_p) => mode != "default", "Annotations use an open shadow root only in default mode");
    // [ts] test("SC-016 should show annotation on click", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.screencast.showActions({ duration: 5000 });
    // [ts] page.click("button").catch(() => {});
    // [ts] expect(page.locator("x-pw-title")).toBeVisible();
    // [ts] expect(page.locator("x-pw-title")).toHaveText("click");
    // Point and highlight are only shown when styled.
    // [ts] expect(page.locator("x-pw-screencast-highlight")).toHaveCount(0);
    // [ts] expect(page.locator("x-pw-screencast-point")).toHaveCount(0);
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-017 should render annotation styles", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.screencast.showActions({ duration: 5000, fontSize: 32, style: { ...style, title: "top: 100px; background-color: rgb(1, 2, 3)" } });
    // [ts] const buttonBox = (page.locator("button").boundingBox());
    // [ts] page.click("button").catch(() => {});
    // Highlight: styled box that covers the target.
    // [ts] const highlight = page.locator("x-pw-screencast-highlight");
    // [ts] expect(highlight).toBeVisible();
    // [ts] expect_eq(highlight.evaluate((el) => getComputedStyle(el).outlineWidth), "2px");
    // [ts] const box = (highlight.boundingBox());
    // [ts] expect(box.x).toBeCloseTo(buttonBox.x, 0);
    // [ts] expect(box.y).toBeCloseTo(buttonBox.y, 0);
    // [ts] expect(box.width).toBeCloseTo(buttonBox.width, 0);
    // [ts] expect(box.height).toBeCloseTo(buttonBox.height, 0);
    // Point: styled marker centered on the click point, Firefox rounds the click point.
    // [ts] const pointBox = (page.locator("x-pw-screencast-point").boundingBox());
    // [ts] expect_eq(pointBox.width, 20);
    // [ts] expect_eq(Math.abs(pointBox.x + 10 - (buttonBox.x + buttonBox.width / 2)) <= 1, true);
    // [ts] expect_eq(Math.abs(pointBox.y + 10 - (buttonBox.y + buttonBox.height / 2)) <= 1, true);
    // Title: style applies on top of deprecated fontSize, position wins over style.
    // [ts] const title = page.locator("x-pw-title");
    // [ts] expect(title).toBeVisible();
    // [ts] const titleStyle = title.evaluate((el) => {
    // [ts] const cs = getComputedStyle(el);
    // [ts] return {
    // [ts] color: cs.color, borderRadius: cs.borderRadius, padding: cs.padding, backgroundColor: cs.backgroundColor,
    // [ts] top: el.style.top, right: el.style.right, fontSize: el.style.fontSize,
    // [ts] };
    // [ts] });
    // [ts] expect_eq(titleStyle.backgroundColor, "rgb(1, 2, 3)");
    // [ts] expect_eq(titleStyle.color, "rgb(255, 255, 255)");
    // [ts] expect_eq(titleStyle.borderRadius, "6px");
    // [ts] expect_eq(titleStyle.padding, "6px");
    // [ts] expect_eq(titleStyle.top, "6px");
    // [ts] expect_eq(titleStyle.right, "6px");
    // [ts] expect_eq(titleStyle.fontSize, "32px");
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-019 should clear annotation after duration", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.screencast.showActions({ duration: 1000, style });
    // [ts] page.click("button");
    // [ts] expect(page.locator("x-pw-screencast-point")).toHaveCount(0);
    // [ts] expect(page.locator("x-pw-screencast-highlight")).toHaveCount(0);
    // [ts] expect(page.locator("x-pw-title")).toBeHidden();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-020 should annotate fill action", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/textarea.html");
    // [ts] page.screencast.showActions({ duration: 5000 });
    // [ts] page.fill("textarea", "hello").catch(() => {});
    // [ts] const title = page.locator("x-pw-title");
    // [ts] expect(title).toBeVisible();
    // [ts] expect(title).toHaveText("fill");
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-021 should stop showing actions after dispose", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] const actions = page.screencast.showActions({ duration: 1000 });
    // [ts] page.click("button");
    // [ts] actions.dispose();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] expect(page.locator("x-pw-title")).toBeHidden();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-022 should stop showing actions after hideActions", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.screencast.showActions({ duration: 1000 });
    // [ts] page.click("button");
    // [ts] page.screencast.hideActions();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button");
    // [ts] expect(page.locator("x-pw-title")).toBeHidden();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-023 should render an action cursor that animates to the click point", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.screencast.showActions({ duration: 5000 });
    // [ts] page.click("button").catch(() => {});
    // [ts] const cursor = page.locator("x-pw-action-cursor");
    // [ts] expect(cursor).toBeVisible();
    // [ts] const initial = cursor.evaluate((el) => ({
    // [ts] top: el.style.top,
    // [ts] left: el.style.left,
    // [ts] }));
    // [ts] expect(initial.top).toMatch("\\d+px");
    // [ts] expect(initial.left).toMatch("\\d+px");
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-024 cursor moves between two pointer actions", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.setContent(`
    // [ts] <div style="position: fixed; top: 20px; left: 20px; width: 60px; height: 60px;" id="a">A</div>
    // [ts] <div style="position: fixed; bottom: 20px; right: 20px; width: 60px; height: 60px;" id="b">B</div>
    // [ts] `);
    // [ts] page.screencast.showActions({ duration: 5000 });
    // [ts] const cursor = page.locator("x-pw-action-cursor");
    // [ts] page.click("#a", { force: true }).catch(() => {});
    // [ts] expect(cursor).toBeVisible();
    // [ts] const first = cursor.evaluate((el) => ({ top: el.style.top, left: el.style.left }));
    // [ts] page.click("#b", { force: true }).catch(() => {});
    // [ts] expect.poll(() => {
    // [ts] return cursor.evaluate((el) => ({ top: el.style.top, left: el.style.left }));
    // [ts] }).not.toEqual(first);
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-025 cursor: \"none\" suppresses the action cursor decoration", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.screencast.showActions({ duration: 5000, cursor: "none" });
    // [ts] page.click("button").catch(() => {});
    // The title still renders, but the cursor does not.
    // [ts] expect(page.locator("x-pw-title")).toBeVisible();
    // [ts] expect(page.locator("x-pw-action-cursor")).toBeHidden();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-026 cursor stays at the last action point until hideActions", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.screencast.showActions({ duration: 100 });
    // [ts] page.click("button");
    // [ts] expect(page.locator("x-pw-title")).toBeHidden();
    // [ts] const cursor = page.locator("x-pw-action-cursor");
    // [ts] expect(cursor).toBeVisible();
    // [ts] const position = cursor.evaluate((el) => ({ top: el.style.top, left: el.style.left }));
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] expect(cursor).toBeVisible();
    // [ts] expect_eq(cursor.evaluate((el) => ({ top: el.style.top, left: el.style.left })), position);
    // [ts] page.screencast.hideActions();
    // [ts] expect(cursor).toBeHidden();
    // [ts] context.close();
    // [ts] });
    // [ts] test("SC-027 should survive navigation", () => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.screencast.showActions({ duration: 5000 });
    // [ts] page.click("button").catch(() => {});
    // [ts] expect(page.locator("x-pw-title")).toBeVisible();
    // [ts] page.goto(server.PREFIX + "/input/button.html");
    // [ts] page.click("button").catch(() => {});
    // [ts] expect(page.locator("x-pw-title")).toBeVisible();
    // [ts] context.close();
    // [ts] });
}
