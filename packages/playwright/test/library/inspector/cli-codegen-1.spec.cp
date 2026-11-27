// Migrated from TypeScript (library/inspector/cli-codegen-1.spec.ts) via cap from -ts

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

import { test, expect } from "./inspectorTest.cp";
// [cap-migrate] import { ConsoleMessage } from "playwright";

// [cap-test] test.describe("cli codegen", () => {





























    expect.soft(sources.get("Java").text).toContain(`
      page.navigate(\"about:blank\");
      page.getByRole(AriaRole.TEXTBOX).click();
      page.getByRole(AriaRole.TEXTBOX).fill(\"h\");
    }`);

    expect.soft(sources.get("Python").text).toContain(`
    page.goto("about:blank")
    page.get_by_role("textbox").click()
    page.get_by_role("textbox").fill("h")

    # ---------------------
    context.close()`);

    expect.soft(sources.get("Python Async").text).toContain(`
    page.goto("about:blank")
    page.get_by_role("textbox").click()
    page.get_by_role("textbox").fill("h")

    # ---------------------
    context.close()`);

    expect.soft(sources.get("Pytest").text).toContain(`
    page.goto("about:blank")
    page.get_by_role("textbox").click()
    page.get_by_role("textbox").fill("h")`);

    expect.soft(sources.get("C#").text).toContain(`
page.GotoAsync("about:blank");
page.GetByRole(AriaRole.Textbox).ClickAsync();
page.GetByRole(AriaRole.Textbox).FillAsync("h");`);

    expect_eq(std.mem.indexOf(u8, page.url(), "about:blank#foo") != null, true);
  // [cap-test] });








// TODO(cap-migrate): refine TS statement
// [ts] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-001 should click", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button onclick="console.log('click')">Submit</button>`);
    // [ts] const locator = recorder.hoverOverElement("button");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' })`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Submit")).click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Submit" }).ClickAsync();`);
    // [ts] expect_eq(message.text(), "click");
    // [ts] });
    // [ts] test("CG-002 should double click", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button onclick="console.log('click ' + event.detail)" ondblclick="console.log('dblclick ' + event.detail)">Submit</button>`);
    // [ts] const locator = recorder.hoverOverElement("button");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' })`);
    // [ts] const messages: []string = [];
    // [ts] page.on("console", message => {
    // [ts] if (message.text().includes("click"))
    // [ts] messages.push(message.text());
    // [ts] });
    // [ts] const [, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error" && msg.text() == "dblclick 2"),
    // [ts] recorder.waitForOutput("JavaScript", "dblclick"),
    // [ts] recorder.trustedDblclick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).dblclick();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").dblclick()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").dblclick()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Submit")).dblclick()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Submit" }).DblClickAsync();`);
    // [ts] expect(messages).toEqual([
    // [ts] "click 1",
    // [ts] "click 2",
    // [ts] "dblclick 2",
    // [ts] ]);
    // [ts] });
    // [ts] test("CG-003 should click twice", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button onclick="console.log('click')">Submit</button>`);
    // [ts] const locator = recorder.hoverOverElement("button");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' })`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] recorder.waitForActionPerformed(),
    // [ts] ]);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", `click();\n  await`),
    // [ts] recorder.trustedClick(),
    // [ts] recorder.waitForActionPerformed(),
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).click();
    // [ts] page.getByRole('button', { name: 'Submit' }).click();`);
    // [ts] });
    // [ts] test("CG-004 should type after clicking twice", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input type="text" value="foo"/>`);
    // [ts] const locator = recorder.hoverOverElement("input");
    // [ts] expect_eq(locator, `getByRole('textbox')`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] recorder.waitForActionPerformed(),
    // [ts] ]);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", `click();\n  await`),
    // [ts] recorder.trustedClick(),
    // [ts] recorder.waitForActionPerformed(),
    // [ts] ]);
    // [ts] page.keyboard.type("bar");
    // [ts] const sources = recorder.waitForOutput("JavaScript", "bar");
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('textbox').click();
    // [ts] page.getByRole('textbox').click();
    // [ts] page.getByRole('textbox').fill('foobar');`);
    // [ts] });
    // [ts] test("CG-005 should ignore programmatic events", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button onclick="console.log('click')">Submit</button>`);
    // [ts] const locator = recorder.hoverOverElement("button");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' })`);
    // [ts] page.dispatchEvent("button", "click", { detail: 1 });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick()
    // [ts] ]);
    // [ts] const clicks = recorder.sources().get("Playwright Test").actions.filter(l => l.includes("Submit"));
    // [ts] expect_eq(clicks.length, 1);
    // [ts] });
    // [ts] test("CG-006 should click after same-document navigation", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] server.setRoute("/foo.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html; charset=utf-8");
    // [ts] res.end("");
    // [ts] });
    // [ts] recorder.setContentAndWait(`<button onclick="console.log('click')">Submit</button>`, server.PREFIX + "/foo.html");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNavigation(),
    // [ts] page.evaluate(() => history.pushState({}, "", "/url.html")),
    // [ts] ]);
    // This is the only way to give recorder a chance to install
    // the second unnecessary copy of the recorder script.
    // [ts] page.waitForTimeout(1000);
    // [ts] const locator = recorder.hoverOverElement("button");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' })`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).click();`);
    // [ts] expect_eq(message.text(), "click");
    // [ts] });
    // [ts] test("CG-007 should make a positioned click on a canvas", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <canvas width="500" height="500" style="margin: 42px"/>
    // [ts] <script>
    // [ts] document.querySelector("canvas").addEventListener("click", event => {
    // [ts] const rect = event.target.getBoundingClientRect();
    // [ts] console.log("click", event.clientX - rect.left, event.clientY - rect.top);
    // [ts] })
    // [ts] </script>
    // [ts] `);
    // [ts] const locator = recorder.hoverOverElement("canvas", {
    // [ts] position: { x: 250, y: 250 },
    // [ts] });
    // [ts] expect_eq(locator, `locator('canvas')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('canvas').click({
    // [ts] position: {
    // [ts] x: 250,
    // [ts] y: 250
    // [ts] }
    // [ts] });`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.locator("canvas").click(position={"x":250,"y":250})`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("canvas").click(position={"x":250,"y":250})`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.locator("canvas").click(new Locator.ClickOptions()
    // [ts] .setPosition(250, 250));`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.Locator("canvas").ClickAsync(new()
    // [ts] {
    // [ts] Position = new()
    // [ts] {
    // [ts] X = 250,
    // [ts] Y = 250,
    // [ts] },
    // [ts] });`);
    // [ts] expect_eq(message.text(), "click 250 250");
    // [ts] });
    // [ts] test("CG-008 should work with TrustedTypes", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <head>
    // [ts] <meta http-equiv="Content-Security-Policy" content="trusted-types unsafe escape; require-trusted-types-for 'script'">
    // [ts] </head>
    // [ts] <body>
    // [ts] <button onclick="console.log('click')">Submit</button>
    // [ts] </body>`);
    // [ts] const locator = recorder.hoverOverElement("button");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' })`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Submit")).click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Submit" }).ClickAsync();`);
    // [ts] expect_eq(message.text(), "click");
    // [ts] });
    // [ts] test("CG-009 should not target selector preview by text regexp", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<span>dummy</span>`);
    // Force highlight.
    // [ts] recorder.hoverOverElement("span");
    // Append text after highlight.
    // [ts] page.evaluate(() => {
    // [ts] const div = document.createElement("div");
    // [ts] div.setAttribute("onclick", "console.log('click')");
    // [ts] div.textContent = " Some long text here ";
    // [ts] document.body.appendChild(div);
    // [ts] });
    // [ts] const locator = recorder.hoverOverElement("div");
    // [ts] expect_eq(locator, `getByText('Some long text here')`);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: const divContents = await page.$eval('div', div => div.outerHTML);
    // [ts] expect_eq(divContents, `<div onclick="console.log('click')"> Some long text here </div>`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedMove("div").then(() => recorder.trustedClick()),
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByText('Some long text here').click();`);
    // [ts] expect_eq(message.text(), "click");
    // [ts] });
    // [ts] test("CG-010 should fill", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="input" name="name" oninput="console.log(input.value)"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `locator('#input')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "fill"),
    // [ts] page.fill("input", "John")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#input').fill('John');`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.locator("#input").fill("John");`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.locator("#input").fill(\"John\")`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#input").fill(\"John\")`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#input").FillAsync(\"John\");`);
    // [ts] expect_eq(message.text(), "John");
    // [ts] });
    // [ts] test("CG-011 should fill japanese text", () => {
    // [ts] const { page, recorder } = openRecorder();
    // In Japanese, "てすと" or "テスト" means "test".
    // [ts] recorder.setContentAndWait(`<input id="input" name="name" oninput="input.value === 'ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ£ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¦ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ£ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ£ÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂÃÂ¨' && console.log(input.value)"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `locator('#input')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "fill"),
    // [ts] (() => {
    // [ts] recorder.page.dispatchEvent("input", "keydown", { key: "Process" });
    // [ts] recorder.page.keyboard.insertText("ÃÂ£ÃÂÃÂ¦ÃÂ£ÃÂÃÂÃÂ£ÃÂÃÂ¨");
    // [ts] recorder.page.dispatchEvent("input", "keyup", { key: "Process" });
    // [ts] })()
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#input').fill('ã¦ãã¨');`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.locator("#input").fill("てすと");`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.locator("#input").fill(\"てすと\")`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#input").fill(\"ã¦ãã¨\")`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#input").FillAsync(\"ã¦ãã¨\");`);
    // [ts] expect_eq(message.text(), "Ã£ÂÂ¦Ã£ÂÂÃ£ÂÂ¨");
    // [ts] });
    // [ts] test("CG-012 should fill textarea", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<textarea id="textarea" name="name" oninput="console.log(textarea.value)"></textarea>`);
    // [ts] const locator = recorder.focusElement("textarea");
    // [ts] expect_eq(locator, `locator('#textarea')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "fill"),
    // [ts] page.fill("textarea", "John")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#textarea').fill('John');`);
    // [ts] expect_eq(message.text(), "John");
    // [ts] });
    // [ts] test("CG-013 should fill textarea with new lines at the end", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/23774" });
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<textarea id="textarea"></textarea>`);
    // [ts] const textarea = page.locator("textarea");
    // [ts] textarea.evaluate(e => e.addEventListener("input", () => (window).lastInputValue = e.value));
    // [ts] const waitForOutputPromise = recorder.waitForOutput("JavaScript", "Hello\\n");
    // [ts] textarea.type("Hello\n");
    // Issue was that the input event was not fired for the last newline, so we check for that.
    // [ts] page.waitForFunction(() => (window).lastInputValue == "Hello\n");
    // [ts] const sources = waitForOutputPromise;
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `await page.locator('#textarea').fill('Hello\\n');`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `Enter`) == null, true);
    // [ts] });
    // [ts] test("CG-014 should fill [contentEditable]", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<div id="content" contenteditable="" oninput="console.log(content.innerText)"/>`);
    // [ts] const locator = recorder.focusElement("div");
    // [ts] expect_eq(locator, `locator('#content')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "fill"),
    // [ts] page.fill("div", "John Doe")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#content').fill('John Doe');`);
    // [ts] expect_eq(message.text(), "John Doe");
    // [ts] });
    // [ts] test("CG-015 should fill [contentEditable] with initial text", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <section aria-label="First"><h1 contenteditable="" oninput="console.log(event.target.innerText)">Initial text</h1></section>
    // [ts] <section aria-label="Second"><h1 contenteditable="">More text</h1></section>
    // [ts] `);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "fill"),
    // [ts] page.fill("section[aria-label=First] h1", "John Doe")
    // [ts] ]);
    // The selector should not be derived from the text that is being changed by the fill,
    // but can use the accessible name of the parent element.
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('region', { name: 'First' }).getByRole('heading').fill('John Doe');`);
    // [ts] expect_eq(message.text(), "John Doe");
    // [ts] });
    // [ts] test("CG-016 should press", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input name="name" onkeypress="console.log('press')"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `getByRole('textbox')`);
    // [ts] const messages: []string = [];
    // [ts] page.on("console", message => messages.push(message));
    // [ts] const [, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForActionPerformed(),
    // [ts] recorder.waitForOutput("JavaScript", "press"),
    // [ts] page.press("input", "Shift+Enter")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('textbox').press('Shift+Enter');`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.TEXTBOX).press("Shift+Enter");`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("textbox").press("Shift+Enter")`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("textbox").press("Shift+Enter")`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Textbox).PressAsync("Shift+Enter");`);
    // [ts] expect_eq(std.mem.indexOf(u8, messages.map(m => m.text()), "press") != null, true);
    // [ts] });
    // [ts] test("CG-017 should update selected element after pressing Tab", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <input name="one"></input>
    // [ts] <input name="two"></input>
    // [ts] `);
    // [ts] const input1 = page.locator("input[name=\"one\"]");
    // [ts] const input2 = page.locator("input[name=\"two\"]");
    // [ts] {
    // [ts] input1.click();
    // [ts] recorder.waitForOutput("JavaScript", "click");
    // [ts] expect(input1).toBeFocused();
    // [ts] }
    // [ts] {
    // [ts] page.keyboard.type("foobar123");
    // [ts] recorder.waitForOutput("JavaScript", "foobar123");
    // [ts] expect(input1).toHaveValue("foobar123");
    // [ts] }
    // [ts] {
    // [ts] page.keyboard.press("Tab");
    // [ts] recorder.waitForOutput("JavaScript", "Tab");
    // [ts] expect(input2).toBeFocused();
    // [ts] }
    // [ts] {
    // [ts] page.keyboard.type("barfoo321");
    // [ts] recorder.waitForOutput("JavaScript", "barfoo321");
    // [ts] expect(input2).toHaveValue("barfoo321");
    // [ts] }
    // [ts] const text = recorder.sources().get("JavaScript").text;
    // [ts] expect(text).toContain(`
    // [ts] page.locator('input[name="one"]').fill('foobar123');`);
    // [ts] expect(text).toContain(`
    // [ts] page.locator('input[name="one"]').press('Tab');`);
    // [ts] expect(text).toContain(`
    // [ts] page.locator('input[name="two"]').fill('barfoo321');`);
    // [ts] });
    // [ts] test("CG-018 should record ArrowDown", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input name="name" onkeydown="console.log('press:' + event.key)"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `getByRole('textbox')`);
    // [ts] const messages: []string = [];
    // [ts] page.on("console", message => {
    // [ts] messages.push(message);
    // [ts] });
    // [ts] const [, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForActionPerformed(),
    // [ts] recorder.waitForOutput("JavaScript", "press"),
    // [ts] page.press("input", "ArrowDown")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('textbox').press('ArrowDown');`);
    // [ts] expect_eq(std.mem.indexOf(u8, messages.map(m => m.text()), "press:ArrowDown") != null, true);
    // [ts] });
    // [ts] test("CG-019 should emit single keyup on ArrowDown", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input name="name" onkeydown="console.log('down:' + event.key)" onkeyup="console.log('up:' + event.key)"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `getByRole('textbox')`);
    // [ts] const messages: []string = [];
    // [ts] page.on("console", message => {
    // [ts] if (message.type() != "error")
    // [ts] messages.push(message);
    // [ts] });
    // [ts] const [, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForActionPerformed(),
    // [ts] recorder.waitForOutput("JavaScript", "press"),
    // [ts] page.press("input", "ArrowDown")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('textbox').press('ArrowDown');`);
    // [ts] expect_eq(messages.length, 2);
    // [ts] expect_eq(messages[0].text(), "down:ArrowDown");
    // [ts] expect_eq(messages[1].text(), "up:ArrowDown");
    // [ts] });
    // [ts] test("CG-020 should not record a click on Enter press", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button onclick="console.log('clicked')">Submit</button>`);
    // [ts] const locator = recorder.focusElement("button");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' })`);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", `press('Enter')`),
    // [ts] page.keyboard.press("Enter"),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "clicked");
    // Wait for the next action to be recorded, to make sure the keyboard-activated
    // click event that follows the Enter press did not produce a click action.
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", `press('Tab')`),
    // [ts] page.keyboard.press("Tab"),
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).press('Enter');`);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `click()`) == null, true);
    // [ts] });
    // [ts] test("CG-021 should check", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="checkbox" type="checkbox" name="accept" onchange="console.log(checkbox.checked)"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `locator('#checkbox')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "check"),
    // [ts] page.click("input")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#checkbox').check();`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.locator("#checkbox").check();`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.locator("#checkbox").check()`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#checkbox").check()`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#checkbox").CheckAsync();`);
    // [ts] expect_eq(message.text(), "true");
    // [ts] });
    // [ts] test("CG-022 should check a radio button", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="checkbox" type="radio" name="accept" onchange="console.log(checkbox.checked)"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `locator('#checkbox')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "check"),
    // [ts] page.click("input")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#checkbox').check();`);
    // [ts] expect_eq(message.text(), "true");
    // [ts] });
    // [ts] test("CG-023 should check with keyboard", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="checkbox" type="checkbox" name="accept" onchange="console.log(checkbox.checked)"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `locator('#checkbox')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "check"),
    // [ts] page.keyboard.press("Space")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#checkbox').check();`);
    // [ts] expect_eq(message.text(), "true");
    // [ts] });
    // [ts] test("CG-024 should check with keyboard after hover", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="checkbox" type="checkbox" name="accept" onchange="console.log(checkbox.checked)"></input>`);
    // [ts] recorder.hoverOverElement("input");
    // [ts] page.focus("input");
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "check"),
    // [ts] page.keyboard.press("Space")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#checkbox').check();`);
    // [ts] const sources2 = recorder.waitForOutput("JavaScript", "check");
    // [ts] expect(sources2.get("JavaScript").text).not.toContain(`
    // [ts] page.locator('#checkbox').check();
    // [ts] page.locator('#checkbox').check();`);
    // [ts] });
    // [ts] test("CG-025 should uncheck", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="checkbox" type="checkbox" checked name="accept" onchange="console.log(checkbox.checked)"></input>`);
    // [ts] const locator = recorder.focusElement("input");
    // [ts] expect_eq(locator, `locator('#checkbox')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "uncheck"),
    // [ts] page.click("input")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#checkbox').uncheck();`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.locator("#checkbox").uncheck();`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.locator("#checkbox").uncheck()`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#checkbox").uncheck()`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#checkbox").UncheckAsync();`);
    // [ts] expect_eq(message.text(), "false");
    // [ts] });
    // [ts] test("CG-026 should select", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait("<select id=\"age\" onchange=\"console.log(age.selectedOptions[0].value)\"><option value=\"1\"><option value=\"2\"></select>");
    // [ts] const locator = recorder.hoverOverElement("select");
    // [ts] expect_eq(locator, `locator('#age')`);
    // [ts] page.locator("select").click();
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "select"),
    // [ts] page.selectOption("select", "2")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#age').selectOption('2');`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.locator("#age").selectOption("2");`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.locator("#age").select_option("2")`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#age").select_option("2")`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#age").SelectOptionAsync(new[] { "2" });`);
    // [ts] expect_eq(message.text(), "2");
    // [ts] });
    // [ts] test("CG-027 should select with size attribute", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <style>
    // [ts] body {
    // [ts] margin: 0;
    // [ts] }
    // [ts] </style>
    // [ts] <select id="age" size="2" onchange="console.log(age.selectedOptions[0].value)">
    // [ts] <option value="1">v1</option>
    // [ts] <option value="2">v2</option>
    // [ts] </select>
    // [ts] `);
    // [ts] const locator = recorder.hoverOverElement("select");
    // [ts] expect_eq(locator, `locator('#age')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "select"),
    // [ts] page.mouse.click(10, 25)
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#age').selectOption('2');`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.locator("#age").selectOption("2");`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.locator(\"#age\").select_option(\"2\")`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.locator(\"#age\").select_option(\"2\")`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.Locator(\"#age\").SelectOptionAsync(new[] { \"2\" });`);
    // [ts] expect_eq(message.text(), "2");
    // [ts] });
    // [ts] test("CG-028 should await popup", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/36461" });
    // [ts] const { page, recorder } = openRecorder();
    // [ts] server.setRoute("/popup", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html; charset=utf-8");
    // [ts] res.end(`<button>Click me</button>`);
    // [ts] });
    // [ts] recorder.setContentAndWait(`<a target=_blank rel=noopener href="${server.PREFIX}/popup">link</a>`);
    // [ts] const locator = recorder.hoverOverElement("a");
    // [ts] expect_eq(locator, `getByRole('link', { name: 'link' })`);
    // [ts] const [popup] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("page"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] recorder.page = popup;
    // [ts] recorder.hoverOverElement("button");
    // [ts] recorder.trustedClick();
    // [ts] const sources = recorder.waitForOutput("JavaScript", "page1.");
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] const page1Promise = page.waitForEvent('popup');
    // [ts] page.getByRole('link', { name: 'link' }).click();
    // [ts] const page1 = page1Promise;
    // [ts] page1.getByRole('button', { name: 'Click me' }).click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] Page page1 = page.waitForPopup(() -> {
    // [ts] page.getByRole(AriaRole.LINK, new Page.GetByRoleOptions().setName("link")).click();
    // [ts] });`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] with page.expect_popup() as page1_info:
    // [ts] page.get_by_role("link", name="link").click()
    // [ts] page1 = page1_info.value`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] async with page.expect_popup() as page1_info:
    // [ts] page.get_by_role("link", name="link").click()
    // [ts] page1 = page1_info.value`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] var page1 = page.RunAndWaitForPopupAsync(() =>
    // [ts] {
    // [ts] page.GetByRole(AriaRole.Link, new() { Name = "link" }).ClickAsync();
    // [ts] });
    // [ts] page1.GetByRole(AriaRole.Button, new() { Name = \"Click me\" }).ClickAsync();`);
    // [ts] expect.soft(sources.get("C# NUnit").text).toContain(`
    // [ts] var page1 = Page.RunAndWaitForPopupAsync(() =>
    // [ts] {
    // [ts] Page.GetByRole(AriaRole.Link, new() { Name = "link" }).ClickAsync();
    // [ts] });
    // [ts] page1.GetByRole(AriaRole.Button, new() { Name = \"Click me\" }).ClickAsync();`);
    // [ts] });
    // [ts] test("CG-029 should attribute navigation to click", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<a onclick="window.location.href='about:blank#foo'">link</a>`);
    // [ts] const locator = recorder.hoverOverElement("a");
    // [ts] expect_eq(locator, `getByText('link')`);
    // [ts] const [, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNavigation(),
    // [ts] recorder.waitForOutput("JavaScript", ".click()"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.goto('about:blank');
    // [ts] page.getByText('link').click();
    // ---------------------
    // [ts] context.close();`);
    // [ts] expect.soft(sources.get("Playwright Test").text).toContain(`
    // [ts] page.goto('about:blank');
    // [ts] page.getByText('link').click();
    // [ts] });`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.navigate(\"about:blank\");
    // [ts] page.getByText(\"link\").click();
    // [ts] }`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.goto("about:blank")
    // [ts] page.get_by_text("link").click()
    // [ts] # ---------------------
    // [ts] context.close()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.goto("about:blank")
    // [ts] page.get_by_text("link").click()
    // [ts] # ---------------------
    // [ts] context.close()`);
    // [ts] expect.soft(sources.get("Pytest").text).toContain(`
    // [ts] page.goto("about:blank")
    // [ts] page.get_by_text("link").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GotoAsync("about:blank");
    // [ts] page.GetByText("link").ClickAsync();`);
    // [ts] expect_eq(std.mem.indexOf(u8, page.url(), "about:blank#foo") != null, true);
    // [ts] });
    // [ts] test("CG-030 should attribute navigation to press/fill", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input /><script>document.querySelector('input').addEventListener('input', () => window.location.href = 'about:blank#foo');</script>`);
    // [ts] const locator = recorder.hoverOverElement("input");
    // [ts] expect_eq(locator, `getByRole('textbox')`);
    // [ts] recorder.trustedClick();
    // [ts] expect.poll(() => page.locator("input").evaluate(e => e == document.activeElement)).toBeTruthy();
    // [ts] const [, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForNavigation(),
    // [ts] recorder.waitForOutput("JavaScript", ".fill"),
    // [ts] recorder.trustedPress("h"),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.goto('about:blank');
    // [ts] page.getByRole('textbox').click();
    // [ts] page.getByRole('textbox').fill('h');
    // ---------------------
    // [ts] context.close();`);
    // [ts] expect.soft(sources.get("Playwright Test").text).toContain(`
    // [ts] page.goto('about:blank');
    // [ts] page.getByRole('textbox').click();
    // [ts] page.getByRole('textbox').fill('h');
    // [ts] });`);
    // [ts] test("CG-031 should ignore AltGraph", () => {
    // [ts] test(browserName == "firefox", "The TextInputProcessor in Firefox does not work with AltGraph.");
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input></input>`);
    // [ts] recorder.page.type("input", "playwright");
    // [ts] recorder.page.keyboard.press("AltGraph");
    // [ts] recorder.page.keyboard.insertText("@");
    // [ts] recorder.page.keyboard.type("example.com");
    // [ts] recorder.waitForOutput("JavaScript", "example.com");
    // [ts] expect_eq(std.mem.indexOf(u8, recorder.sources().get("JavaScript").text, `await page.getByRole('textbox').press('AltGraph');`) == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, recorder.sources().get("JavaScript").text, `await page.getByRole('textbox').fill('playwright@example.com');`) != null, true);
    // [ts] });
    // [ts] test("CG-032 should middle click", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<a href${JSON.stringify(server.EMPTY_PAGE)}>Click me</a>`);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] page.click("a", { button: "middle" }),
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByText('Click me').click({
    // [ts] button: 'middle'
    // [ts] });`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.get_by_text("Click me").click(button="middle")`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_text("Click me").click(button="middle")`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.getByText("Click me").click(new Locator.ClickOptions()
    // [ts] .setButton(MouseButton.MIDDLE));`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.GetByText("Click me").ClickAsync(new()
    // [ts] {
    // [ts] Button = MouseButton.Middle,
    // [ts] });`);
    // [ts] });
    // [ts] test("CG-033 should record slider", () => {
    // [ts] test.fixme(browserName == "chromium" && isLinux && headless, "https://github.com/microsoft/playwright/issues/38568");
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input type="range" min="0" max="10" value="5">`);
    // [ts] const dragSlider = () => {
    // [ts] const { x, y, width, height } = page.locator("input").boundingBox();
    // [ts] page.mouse.move(x + width / 2, y + height / 2);
    // [ts] page.mouse.down();
    // Dragging to the exact edge is not registered as slider change, so drag close to the end.
    // [ts] page.mouse.move(x + width - 3, y + height / 2);
    // [ts] page.mouse.up();
    // [ts] };
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "fill"),
    // [ts] dragSlider(),
    // [ts] ]);
    // [ts] expect(page.locator("input")).toHaveValue("10");
    // [ts] expect(sources.get("JavaScript").text).not.toContain(`
    // [ts] page.getByRole('slider').click();`);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('slider').fill('10');`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("slider").fill("10")`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("slider").fill("10")`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.SLIDER).fill("10")`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Slider).FillAsync("10");`);
    // [ts] });
    // [ts] test("CG-034 should click button with nested div", () => {
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/29067" });
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button><div role="none">Submit</div></button>`);
    // we hover the nested div, but it must record the button
    // [ts] const locator = recorder.hoverOverElement("div");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' })`);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Submit"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Submit")).click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Submit" }).ClickAsync();`);
    // [ts] });
    // [ts] test("CG-035 should record omnibox navigations after performAction", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button>Submit</button>`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] page.locator("button").click(),
    // [ts] recorder.waitForActionPerformed(),
    // [ts] ]);
    // [ts] page.waitForTimeout(600);  // See "signalThreshold" is signal processor.
    // [ts] page.goto(server.PREFIX + `/empty.html`);
    // [ts] recorder.waitForOutput("JavaScript", `await page.goto('${server.PREFIX}/empty.html');`);
    // [ts] });
    // [ts] test("CG-036 should record omnibox navigations after recordAction", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<textarea></textarea>`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "fill"),
    // [ts] page.locator("textarea").fill("Hello world"),
    // [ts] recorder.waitForActionPerformed(),
    // [ts] ]);
    // [ts] page.waitForTimeout(600);  // See "signalThreshold" is signal processor.
    // [ts] page.goto(server.PREFIX + `/empty.html`);
    // [ts] recorder.waitForOutput("JavaScript", `await page.goto('${server.PREFIX}/empty.html');`);
    // [ts] });
    // [ts] test("CG-037 should not throw csp directive violation errors", () => {
    // [ts] server.setCSP("/empty.html", `default-src 'self'`);
    // [ts] const { page } = openRecorder();
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] const predicate = (msg) => msg.type() == "error" && "Content[\\- ]Security[\\- ]Policy".test(msg.text());
    // [ts] expect(page.waitForEvent("console", { predicate, timeout: 1000 })).rejects.toThrow();
    // [ts] });
    // [ts] test("CG-038 should clear when recording is disabled", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/33802" } }, (_p) => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button id="foo" onclick="console.log('click')">Foo</button>
    // [ts] <button id="bar" onclick="console.log('click')">Bar</button>
    // [ts] `);
    // [ts] recorder.hoverOverElement("#foo");
    // [ts] let [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `getByRole('button', { name: 'Foo' }).click()`) != null, true);
    // [ts] recorder.recorderPage.getByRole("button", { name: "Record" }).click();
    // [ts] recorder.recorderPage.getByRole("button", { name: "Clear" }).click();
    // [ts] recorder.recorderPage.getByRole("button", { name: "Record" }).click();
    // [ts] recorder.hoverOverElement("#bar");
    // [ts] [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `getByRole('button', { name: 'Bar' }).click()`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `getByRole('button', { name: 'Foo' })`) == null, true);
    // [ts] });
}
