// Migrated from TypeScript (library/inspector/cli-codegen-3.spec.ts) via cap from -ts

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

// [cap-migrate] import { TestServer } from "../../config/testserver.cp";
// [cap-migrate] import { Recorder } from "./inspectorTest.cp";
import { test, expect } from "./inspectorTest.cp";
// [cap-migrate] import { Page } from "cap:test";

// [cap-test] test.describe("cli codegen", () => {

















// [cap-test] test.describe("cli codegen with frameLocator()", () => {

// [cap-test] });

async fn createFrameHierarchy(page: Page, recorder: Recorder, server: TestServer) {
  /*
    iframe
      div Hello1
      iframe
        div Hello2
        iframe[name=one]
          div HelloNameOne
        iframe
          dev HelloAnonymous
  */
  recorder.setContentAndWait(`
    <iframe id=frame1 srcdoc="<div>Hello1</div><iframe srcdoc='<div>Hello2</div><iframe name=one></iframe><iframe name=two></iframe><iframe></iframe>'>">
  `, server.EMPTY_PAGE, 6);
  const frameHello1 = page.mainFrame().childFrames()[0];
  const frameHello2 = frameHello1.childFrames()[0];
  const frameTwo = page.frame({ name: "two" });
  frameTwo.setContent(`<div>HelloNameTwo</div>`);
  const frameAnonymous = frameHello2.childFrames().find(f => !f.name());
  frameAnonymous.setContent(`<div>HelloNameAnonymous</div>`);
  return {
    frameHello1,
    frameHello2,
    frameTwo,
    frameAnonymous,
  };
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-061 should click locator.first", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button onclick="console.log('click1')">Submit</button>
    // [ts] <button onclick="console.log('click2')">Submit</button>
    // [ts] `);
    // [ts] const locator = recorder.hoverOverElement("button");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' }).first()`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick()
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).first().click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").first.click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").first.click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Submit")).first().click();`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Submit" }).First.ClickAsync();`);
    // [ts] const clickAction = sources.get("JSON").actions.map(l => JSON.parse(l)).find(a => a.name == "click");
    // [ts] expect.soft(clickAction).toEqual({
    // [ts] name: "click",
    // [ts] selector: "internal:role=button[name=\"Submit\"i] >> nth=0",
    // [ts] button: "left",
    // [ts] clickCount: 1,
    // [ts] locator: { body: "button", kind: "role", options: { exact: false, attrs: [], name: "Submit" }, next: { body: "", kind: "first", options: {} } },
    // [ts] modifiers: 0,
    // [ts] signals: [],
    // [ts] pageGuid: expect.any(String),
    // [ts] });
    // [ts] expect_eq(message.text(), "click1");
    // [ts] });
    // [ts] test("CG-062 should click locator.nth", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button onclick="console.log('click1')">Submit</button>
    // [ts] <button onclick="console.log('click2')">Submit</button>
    // [ts] `);
    // [ts] const locator = recorder.hoverOverElement("button >> nth=1");
    // [ts] expect_eq(locator, `getByRole('button', { name: 'Submit' }).nth(1)`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick()
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).nth(1).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").nth(1).click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Submit").nth(1).click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Submit")).nth(1).click();`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Submit" }).Nth(1).ClickAsync();`);
    // [ts] expect_eq(message.text(), "click2");
    // [ts] });
    // [ts] test("CG-063 should generate frame locators (1)", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] const { frameHello1 } = createFrameHierarchy(page, recorder, server);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Hello1"),
    // [ts] frameHello1.click("text=Hello1"),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#frame1').contentFrame().getByText('Hello1').click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.locator("#frame1").contentFrame().getByText("Hello1").click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.locator("#frame1").content_frame.get_by_text("Hello1").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#frame1").content_frame.get_by_text("Hello1").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#frame1").ContentFrame.GetByText("Hello1").ClickAsync();`);
    // [ts] const clickAction = sources.get("JSON").actions.map(l => JSON.parse(l)).find(a => a.name == "click");
    // [ts] expect.soft(clickAction).toEqual({
    // [ts] name: "click",
    // [ts] selector: "#frame1 >> internal:control=enter-frame >> internal:text=\"Hello1\"i",
    // [ts] button: "left",
    // [ts] clickCount: 1,
    // [ts] locator: {
    // [ts] body: "#frame1", kind: "default", options: {},
    // [ts] next: {
    // [ts] body: "", kind: "frame", options: {},
    // [ts] next: { body: "Hello1", kind: "text", options: { exact: false } },
    // [ts] },
    // [ts] },
    // [ts] modifiers: 0,
    // [ts] signals: [],
    // [ts] pageGuid: expect.any(String),
    // [ts] });
    // [ts] });
    // [ts] test("CG-064 should generate frame locators (2)", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] const { frameHello2 } = createFrameHierarchy(page, recorder, server);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Hello2"),
    // [ts] frameHello2.click("text=Hello2"),
    // [ts] ]);
    // "Hello2" is unique across frames, so a bare frameLocator() replaces two frameLocator(selector) calls.
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.frameLocator().getByText('Hello2').click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.frameLocator().getByText("Hello2").click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.frame_locator().get_by_text("Hello2").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.frame_locator().get_by_text("Hello2").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.FrameLocator().GetByText("Hello2").ClickAsync();`);
    // [ts] const clickAction = sources.get("JSON").actions.map(l => JSON.parse(l)).find(a => a.name == "click");
    // [ts] expect.soft(clickAction).toEqual({
    // [ts] name: "click",
    // [ts] selector: "internal:control=any-frame >> internal:text=\"Hello2\"i",
    // [ts] button: "left",
    // [ts] clickCount: 1,
    // [ts] locator: {
    // [ts] body: "", kind: "any-frame", options: {},
    // [ts] next: { body: "Hello2", kind: "text", options: { exact: false } },
    // [ts] },
    // [ts] modifiers: 0,
    // [ts] signals: [],
    // [ts] pageGuid: expect.any(String),
    // [ts] });
    // [ts] });
    // [ts] test("CG-065 should generate frame locators (3)", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] const { frameAnonymous } = createFrameHierarchy(page, recorder, server);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "HelloNameAnonymous"),
    // [ts] frameAnonymous.click("text=HelloNameAnonymous"),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.frameLocator().getByText('HelloNameAnonymous').click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.frameLocator().getByText("HelloNameAnonymous").click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.frame_locator().get_by_text("HelloNameAnonymous").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.frame_locator().get_by_text("HelloNameAnonymous").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.FrameLocator().GetByText("HelloNameAnonymous").ClickAsync();`);
    // [ts] const clickAction = sources.get("JSON").actions.map(l => JSON.parse(l)).find(a => a.name == "click");
    // [ts] expect.soft(clickAction).toEqual({
    // [ts] name: "click",
    // [ts] selector: "internal:control=any-frame >> internal:text=\"HelloNameAnonymous\"i",
    // [ts] button: "left",
    // [ts] clickCount: 1,
    // [ts] locator: {
    // [ts] body: "", kind: "any-frame", options: {},
    // [ts] next: { body: "HelloNameAnonymous", kind: "text", options: { exact: false } },
    // [ts] },
    // [ts] modifiers: 0,
    // [ts] signals: [],
    // [ts] pageGuid: expect.any(String),
    // [ts] });
    // [ts] });
    // [ts] test("CG-066 should generate frame locators (4)", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] /*
    // [ts] iframe
    // [ts] div Hello1
    // [ts] iframe
    // [ts] div Hello2
    // [ts] iframe[name=one]
    // [ts] div HelloNameOne
    // [ts] iframe
    // [ts] dev HelloAnonymous
    // [ts] */
    // [ts] recorder.setContentAndWait(`
    // [ts] <iframe id=frame1 srcdoc="<div>Hello1</div><iframe srcdoc='<div>Hello2</div><iframe name=one></iframe><iframe name=two></iframe><iframe></iframe>'>">
    // [ts] `, server.EMPTY_PAGE, 6);
    // [ts] const frameHello1 = page.mainFrame().childFrames()[0];
    // [ts] const frameHello2 = frameHello1.childFrames()[0];
    // [ts] const frameTwo = page.frame({ name: "two" });
    // [ts] frameTwo.setContent(`<div>HelloNameTwo</div>`);
    // [ts] const frameAnonymous = frameHello2.childFrames().find(f => !f.name());
    // [ts] frameAnonymous.setContent(`<div>HelloNameAnonymous</div>`);
    // [ts] let [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Hello1"),
    // [ts] frameHello1.click("text=Hello1"),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#frame1').contentFrame().getByText('Hello1').click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.locator("#frame1").contentFrame().getByText("Hello1").click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.locator("#frame1").content_frame.get_by_text("Hello1").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#frame1").content_frame.get_by_text("Hello1").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#frame1").ContentFrame.GetByText("Hello1").ClickAsync();`);
    // [ts] [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Hello2"),
    // [ts] frameHello2.click("text=Hello2"),
    // [ts] ]);
    // "Hello2" is unique across frames, so a bare frameLocator() replaces two frameLocator(selector) calls.
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.frameLocator().getByText('Hello2').click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.frameLocator().getByText("Hello2").click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.frame_locator().get_by_text("Hello2").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.frame_locator().get_by_text("Hello2").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.FrameLocator().GetByText("Hello2").ClickAsync();`);
    // [ts] [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "HelloNameAnonymous"),
    // [ts] frameAnonymous.click("text=HelloNameAnonymous"),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.frameLocator().getByText('HelloNameAnonymous').click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.frameLocator().getByText("HelloNameAnonymous").click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.frame_locator().get_by_text("HelloNameAnonymous").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.frame_locator().get_by_text("HelloNameAnonymous").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.FrameLocator().GetByText("HelloNameAnonymous").ClickAsync();`);
    // [ts] });
    // [ts] test("CG-067 should generate frame locators with special characters in name attribute", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <iframe srcdoc="<button>Click me</button>">
    // [ts] `, server.EMPTY_PAGE, 2);
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: await page.$eval('iframe', (frame: HTMLIFrameElement) => {
    // [ts-skip] frame.name = 'foo<bar\'"`>';
    // [ts-skip] });
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Click me"),
    // [ts] page.locator("iframe[name=\"foo<bar'\\\"`>\"]").contentFrame().getByRole("button", { name: "Click me" }).click(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('iframe[name="foo<bar\\'\\\\\"\`>"]').contentFrame().getByRole('button', { name: 'Click me' }).click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.locator("iframe[name=\\"foo<bar'\\\\\\"\`>\\"]").contentFrame().getByRole(AriaRole.BUTTON, new FrameLocator.GetByRoleOptions().setName("Click me")).click()`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.locator("iframe[name=\\"foo<bar'\\\\\\"\`>\\"]").content_frame.get_by_role("button", name="Click me").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("iframe[name=\\"foo<bar'\\\\\\"\`>\\"]").content_frame.get_by_role("button", name="Click me").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.Locator("iframe[name=\\"foo<bar'\\\\\\"\`>\\"]").ContentFrame.GetByRole(AriaRole.Button, new() { Name = "Click me" }).ClickAsync()`);
    // [ts] });
    // [ts] test("CG-068 should generate frame locators with title attribute", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <iframe title="hello world" srcdoc="<button>Click me</button>"></iframe>
    // [ts] `, server.EMPTY_PAGE, 1);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Click me"),
    // [ts] page.locator("[title=\"hello world\"]").contentFrame().getByRole("button", { name: "Click me" }).click(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(
    // [ts] `await page.locator('iframe[title="hello world"]').contentFrame().getByRole('button', { name: 'Click me' }).click();`
    // [ts] );
    // [ts] expect.soft(sources.get("Java").text).toContain(
    // [ts] `page.locator(\"iframe[title=\\\"hello world\\\"]\").contentFrame().getByRole(AriaRole.BUTTON, new FrameLocator.GetByRoleOptions().setName(\"Click me\")).click();`
    // [ts] );
    // [ts] expect.soft(sources.get("Python").text).toContain(
    // [ts] `page.locator(\"iframe[title=\\\"hello world\\\"]\").content_frame.get_by_role(\"button\", name=\"Click me\").click()`
    // [ts] );
    // [ts] expect.soft(sources.get("Python Async").text).toContain(
    // [ts] `await page.locator("iframe[title=\\\"hello world\\\"]").content_frame.get_by_role("button", name="Click me").click()`
    // [ts] );
    // [ts] expect.soft(sources.get("C#").text).toContain(
    // [ts] `await page.Locator("iframe[title=\\\"hello world\\\"]").ContentFrame.GetByRole(AriaRole.Button, new() { Name = "Click me" }).ClickAsync();`
    // [ts] );
    // [ts] });
    // [ts] test("CG-069 should generate frame locators with name attribute", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <iframe name="hello world" srcdoc="<button>Click me</button>"></iframe>
    // [ts] `, server.EMPTY_PAGE, 1);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Click me"),
    // [ts] page.locator("[name=\"hello world\"]").contentFrame().getByRole("button", { name: "Click me" }).click(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(
    // [ts] `await page.locator('iframe[name="hello world"]').contentFrame().getByRole('button', { name: 'Click me' }).click();`
    // [ts] );
    // [ts] expect.soft(sources.get("Java").text).toContain(
    // [ts] `page.locator(\"iframe[name=\\\"hello world\\\"]\").contentFrame().getByRole(AriaRole.BUTTON, new FrameLocator.GetByRoleOptions().setName(\"Click me\")).click();`
    // [ts] );
    // [ts] expect.soft(sources.get("Python").text).toContain(
    // [ts] `page.locator(\"iframe[name=\\\"hello world\\\"]\").content_frame.get_by_role(\"button\", name=\"Click me\").click()`
    // [ts] );
    // [ts] expect.soft(sources.get("Python Async").text).toContain(
    // [ts] `await page.locator("iframe[name=\\\"hello world\\\"]").content_frame.get_by_role("button", name="Click me").click()`
    // [ts] );
    // [ts] expect.soft(sources.get("C#").text).toContain(
    // [ts] `await page.Locator("iframe[name=\\\"hello world\\\"]").ContentFrame.GetByRole(AriaRole.Button, new() { Name = "Click me" }).ClickAsync();`
    // [ts] );
    // [ts] });
    // [ts] test("CG-070 should generate frame locators with id attribute", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <iframe id="hello-world" srcdoc="<button>Click me</button>"></iframe>
    // [ts] `, server.EMPTY_PAGE, 1);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Click me"),
    // [ts] page.locator("[id=\"hello-world\"]").contentFrame().getByRole("button", { name: "Click me" }).click(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(
    // [ts] `await page.locator('#hello-world').contentFrame().getByRole('button', { name: 'Click me' }).click();`
    // [ts] );
    // [ts] expect.soft(sources.get("Java").text).toContain(
    // [ts] `page.locator(\"#hello-world\").contentFrame().getByRole(AriaRole.BUTTON, new FrameLocator.GetByRoleOptions().setName(\"Click me\")).click();`
    // [ts] );
    // [ts] expect.soft(sources.get("Python").text).toContain(
    // [ts] `page.locator(\"#hello-world\").content_frame.get_by_role(\"button\", name=\"Click me\").click()`
    // [ts] );
    // [ts] expect.soft(sources.get("Python Async").text).toContain(
    // [ts] `await page.locator("#hello-world").content_frame.get_by_role("button", name="Click me").click()`
    // [ts] );
    // [ts] expect.soft(sources.get("C#").text).toContain(
    // [ts] `await page.Locator("#hello-world").ContentFrame.GetByRole(AriaRole.Button, new() { Name = "Click me" }).ClickAsync();`
    // [ts] );
    // [ts] });
    // [ts] test("CG-071 should generate frame locators with testId", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <iframe data-testid="my-testid" srcdoc="<button>Click me</button>"></iframe>
    // [ts] `, server.EMPTY_PAGE, 1);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "my-testid"),
    // [ts] page.locator("iframe[data-testid=\"my-testid\"]").contentFrame().getByRole("button", { name: "Click me" }).click(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(
    // [ts] `await page.locator('[data-testid="my-testid"]').contentFrame().getByRole('button', { name: 'Click me' }).click();`
    // [ts] );
    // [ts] expect.soft(sources.get("Java").text).toContain(
    // [ts] `page.locator(\"[data-testid=\\\"my-testid\\\"]\").contentFrame().getByRole(AriaRole.BUTTON, new FrameLocator.GetByRoleOptions().setName(\"Click me\")).click();`
    // [ts] );
    // [ts] expect.soft(sources.get("Python").text).toContain(
    // [ts] `page.locator(\"[data-testid=\\\"my-testid\\\"]\").content_frame.get_by_role(\"button\", name=\"Click me\").click()`
    // [ts] );
    // [ts] expect.soft(sources.get("Python Async").text).toContain(
    // [ts] `await page.locator("[data-testid=\\\"my-testid\\\"]").content_frame.get_by_role("button", name="Click me").click()`
    // [ts] );
    // [ts] expect.soft(sources.get("C#").text).toContain(
    // [ts] `await page.Locator("[data-testid=\\\"my-testid\\\"]").ContentFrame.GetByRole(AriaRole.Button, new() { Name = "Click me" }).ClickAsync();`
    // [ts] );
    // [ts] });
    // [ts] test("CG-072 should generate role locators undef frame locators", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<iframe id=frame1 srcdoc="<button>Submit</button>">`, server.EMPTY_PAGE, 2);
    // [ts] const frame = page.mainFrame().childFrames()[0];
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] frame.click("button"),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#frame1').contentFrame().getByRole('button', { name: 'Submit' }).click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.locator("#frame1").contentFrame().getByRole(AriaRole.BUTTON, new FrameLocator.GetByRoleOptions().setName("Submit")).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.locator("#frame1").content_frame.get_by_role("button", name="Submit").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#frame1").content_frame.get_by_role("button", name="Submit").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#frame1").ContentFrame.GetByRole(AriaRole.Button, new() { Name = "Submit" }).ClickAsync();`);
    // [ts] });
    // [ts] test("CG-073 should generate getByTestId", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<div data-testid=testid onclick="console.log('click')">Submit</div>`);
    // [ts] const locator = recorder.hoverOverElement("div");
    // [ts] expect_eq(locator, `getByTestId('testid')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByTestId('testid').click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_test_id("testid").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_test_id("testid").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByTestId("testid").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByTestId("testid").ClickAsync();`);
    // [ts] expect_eq(message.text(), "click");
    // [ts] });
    // [ts] test("CG-074 should generate getByPlaceholder", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input placeholder="Country"></input>`);
    // [ts] const locator = recorder.hoverOverElement("input");
    // [ts] expect_eq(locator, `getByRole('textbox', { name: 'Country' })`);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('textbox', { name: 'Country' }).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("textbox", name="Country").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("textbox", name="Country").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.TEXTBOX, new Page.GetByRoleOptions().setName("Country")).click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Textbox, new() { Name = "Country" }).ClickAsync();`);
    // [ts] });
    // [ts] test("CG-075 should generate getByAltText", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input alt="Country"></input>`);
    // [ts] const locator = recorder.hoverOverElement("input");
    // [ts] expect_eq(locator, `getByAltText('Country')`);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByAltText('Country').click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_alt_text("Country").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_alt_text("Country").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByAltText("Country").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByAltText("Country").ClickAsync();`);
    // [ts] });
    // [ts] test("CG-076 should generate getByLabel", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<label for=target>Country</label><input id=target>`);
    // [ts] const locator = recorder.hoverOverElement("input");
    // [ts] expect_eq(locator, `getByRole('textbox', { name: 'Country' })`);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('textbox', { name: 'Country' }).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("textbox", name="Country").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("textbox", name="Country").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.TEXTBOX, new Page.GetByRoleOptions().setName("Country")).click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Textbox, new() { Name = "Country" }).ClickAsync();`);
    // [ts] });
    // [ts] test("CG-077 should generate getByLabel without regex", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<label for=target>Coun"try</label><input id=target>`);
    // [ts] const locator = recorder.hoverOverElement("input");
    // [ts] expect_eq(locator, `getByRole('textbox', { name: 'Coun"try' })`);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('textbox', { name: 'Coun\"try' }).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("textbox", name="Coun\\"try").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("textbox", name="Coun\\"try").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.TEXTBOX, new Page.GetByRoleOptions().setName(\"Coun\\\"try\")).click();`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Textbox, new() { Name = \"Coun\\\"try\" }).ClickAsync();`);
    // [ts] });
    // [ts] test("CG-078 should pass through pointer events", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button onclick="console.log('clicked')">Submit</button>
    // [ts] <script>
    // [ts] const button = document.querySelector('button');
    // [ts] const log = [];
    // [ts] for (const eventName of ['mousedown', 'mousemove', 'mouseup', 'pointerdown', 'pointermove', 'pointerup', 'click'])
    // [ts] button.addEventListener(eventName, e => log.push(e.type));
    // [ts] </script>
    // [ts] `);
    // [ts] recorder.hoverOverElement("button");
    // [ts] expect_eq(page.evaluate("log"), ["pointermove", "mousemove"]);
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "clicked");
    // [ts] expect(page.evaluate("log")).toEqual([
    // [ts] "pointermove", "mousemove",
    // [ts] "pointerdown", "mousedown",
    // [ts] "pointerup", "mouseup",
    // [ts] "click",
    // [ts] ]);
    // [ts] });
    // [ts] test("CG-079 should consume contextmenu events, despite a custom context menu", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button>Right click me.</button>
    // [ts] <div id="menu" style="display: none; position: absolute;">
    // [ts] <button>Menu option 1</button>
    // [ts] <button>Menu option 2</button>
    // [ts] </div>
    // [ts] <script>
    // [ts] const button = document.querySelector('button');
    // [ts] button.addEventListener('contextmenu', e => {
    // [ts] e.preventDefault();
    // [ts] console.log('right-clicked');
    // show custom context menu
    // [ts] const menu = document.getElementById("menu");
    // [ts] menu.style.display = "block";
    // [ts] menu.style.left = e.pageX + "px";
    // [ts] menu.style.top = e.pageY + "px";
    // [ts] });
    // [ts] const log = [];
    // [ts] for (const eventName of ['auxclick', 'click', 'contextmenu']) {
    // [ts] button.addEventListener(eventName, e => log.push('button: ' + e.type));
    // [ts] menu.addEventListener(eventName, e => log.push('menu: ' + e.type));
    // [ts] }
    // [ts] </script>
    // [ts] `);
    // [ts] recorder.hoverOverElement("button");
    // [ts] expect_eq(page.evaluate("log"), []);
    // [ts] const action = () => {
    // [ts] recorder.trustedClick({ button: "right" });
    // [ts] recorder.page.getByRole("listitem", { name: "Right click" }).click();
    // [ts] };
    // [ts] const [message] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", `button: 'right'`),
    // [ts] action(),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "right-clicked");
    // [ts] expect_eq(page.evaluate("log"), if ((isWindows && browserName == "chromium")) ["button: auxclick", "button: contextmenu"] else ["button: contextmenu"]);
    // [ts] });
    // [ts] test("CG-080 should generate click action from dialog", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button onclick="console.log('clicked')">Submit</button>`);
    // [ts] recorder.hoverOverElement("button");
    // [ts] const action = () => {
    // [ts] recorder.trustedClick({ button: "right" });
    // [ts] recorder.page.getByRole("listitem", { name: "Click", exact: true }).click();
    // [ts] };
    // The dialog kicks off a click; the page reacts and the click is recorded naturally.
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "click"),
    // [ts] action(),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "clicked");
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).click();`);
    // [ts] });
    // [ts] test("CG-081 should generate double click action from dialog", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<button ondblclick="console.log('dblclicked')">Submit</button>`);
    // [ts] recorder.hoverOverElement("button");
    // [ts] const action = () => {
    // [ts] recorder.trustedClick({ button: "right" });
    // [ts] recorder.page.getByRole("listitem", { name: "Double click" }).click();
    // [ts] };
    // The dialog kicks off a double click; it is recorded as a single dblclick action.
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error" && msg.text() == "dblclicked"),
    // [ts] recorder.waitForOutput("JavaScript", "dblclick"),
    // [ts] action(),
    // [ts] ]);
    // [ts] expect_eq(message.text(), "dblclicked");
    // [ts] const text = sources.get("JavaScript").text;
    // [ts] expect(text).toContain(`
    // [ts] page.getByRole('button', { name: 'Submit' }).dblclick();`);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `.click();`) == null, true);
    // [ts] });
    // [ts] test("CG-082 should generate hover action", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<div data-testid=testid>Hover me</div>`);
    // [ts] recorder.hoverOverElement("div");
    // [ts] const action = () => {
    // [ts] recorder.trustedClick({ button: "right" });
    // [ts] expect(recorder.page.getByRole("list", { name: "Choose action" })).toMatchAriaSnapshot(`
    // [ts] - list "Choose action":
    // [ts] - listitem "Click"
    // [ts] - listitem "Right click"
    // [ts] - listitem "Double click"
    // [ts] - listitem "Hover"
    // [ts] - listitem "Pick locator"
    // [ts] `);
    // [ts] recorder.page.getByRole("listitem", { name: "Hover" }).click();
    // [ts] };
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", `hover`),
    // [ts] action(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByTestId('testid').hover();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_test_id("testid").hover()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_test_id("testid").hover()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByTestId("testid").hover()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByTestId("testid").HoverAsync();`);
    // [ts] });
    // [ts] test("CG-083 should assert value", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <input id=first value=foo>
    // [ts] <input id=second disabled value=bar>
    // [ts] <input id=third>
    // [ts] <input id=fourth type=checkbox checked>
    // [ts] `);
    // [ts] recorder.page.click("x-pw-tool-item.value");
    // [ts] recorder.hoverOverElement("#first");
    // [ts] const [sources1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "#first"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources1.get("JavaScript").text).toContain(`await expect(page.locator('#first')).toHaveValue('foo')`);
    // [ts] expect.soft(sources1.get("Python").text).toContain(`expect(page.locator("#first")).to_have_value("foo")`);
    // [ts] expect.soft(sources1.get("Python Async").text).toContain(`await expect(page.locator("#first")).to_have_value("foo")`);
    // [ts] expect.soft(sources1.get("Java").text).toContain(`assertThat(page.locator("#first")).hasValue("foo")`);
    // [ts] expect.soft(sources1.get("C#").text).toContain(`await Expect(page.Locator("#first")).ToHaveValueAsync("foo")`);
    // [ts] recorder.page.click("x-pw-tool-item.value");
    // [ts] recorder.hoverOverElement("#third");
    // [ts] const [sources3] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "#third"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources3.get("JavaScript").text).toContain(`await expect(page.locator('#third')).toBeEmpty()`);
    // [ts] expect.soft(sources3.get("Python").text).toContain(`expect(page.locator("#third")).to_be_empty()`);
    // [ts] expect.soft(sources3.get("Python Async").text).toContain(`await expect(page.locator("#third")).to_be_empty()`);
    // [ts] expect.soft(sources3.get("Java").text).toContain(`assertThat(page.locator("#third")).isEmpty()`);
    // [ts] expect.soft(sources3.get("C#").text).toContain(`await Expect(page.Locator("#third")).ToBeEmptyAsync()`);
    // [ts] recorder.page.click("x-pw-tool-item.value");
    // [ts] recorder.hoverOverElement("#fourth");
    // [ts] const [sources4] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "#fourth"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources4.get("JavaScript").text).toContain(`await expect(page.locator('#fourth')).toBeChecked()`);
    // [ts] expect.soft(sources4.get("Python").text).toContain(`expect(page.locator("#fourth")).to_be_checked()`);
    // [ts] expect.soft(sources4.get("Python Async").text).toContain(`await expect(page.locator("#fourth")).to_be_checked()`);
    // [ts] expect.soft(sources4.get("Java").text).toContain(`assertThat(page.locator("#fourth")).isChecked()`);
    // [ts] expect.soft(sources4.get("C#").text).toContain(`await Expect(page.Locator("#fourth")).ToBeCheckedAsync()`);
    // [ts] });
    // [ts] test("CG-084 should assert value on disabled input", () => {
    // [ts] test(browserName == "firefox", "pointerup event is not dispatched on a disabled input");
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <input id=first value=foo>
    // [ts] <input id=second disabled value=bar>
    // [ts] <input id=third>
    // [ts] <input id=fourth type=checkbox checked>
    // [ts] `);
    // [ts] recorder.page.click("x-pw-tool-item.value");
    // [ts] recorder.hoverOverElement("#second");
    // [ts] const [sources2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "#second"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources2.get("JavaScript").text).toContain(`await expect(page.locator('#second')).toHaveValue('bar')`);
    // [ts] expect.soft(sources2.get("Python").text).toContain(`expect(page.locator("#second")).to_have_value("bar")`);
    // [ts] expect.soft(sources2.get("Python Async").text).toContain(`await expect(page.locator("#second")).to_have_value("bar")`);
    // [ts] expect.soft(sources2.get("Java").text).toContain(`assertThat(page.locator("#second")).hasValue("bar")`);
    // [ts] expect.soft(sources2.get("C#").text).toContain(`await Expect(page.Locator("#second")).ToHaveValueAsync("bar")`);
    // [ts] });
    // [ts] test("CG-085 should assert value on disabled select", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <select id=first><option value=foo1>Foo1</option><option value=bar1>Bar1</option></select>
    // [ts] <select id=second disabled><option value=foo2>Foo2</option><option value=bar2 selected>Bar2</option></select>
    // [ts] `);
    // [ts] recorder.page.click("x-pw-tool-item.value");
    // [ts] recorder.hoverOverElement("#second");
    // [ts] const [sources2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "#second"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources2.get("JavaScript").text).toContain(`await expect(page.locator('#second')).toHaveValue('bar2')`);
    // [ts] expect.soft(sources2.get("Python").text).toContain(`expect(page.locator("#second")).to_have_value("bar2")`);
    // [ts] expect.soft(sources2.get("Python Async").text).toContain(`await expect(page.locator("#second")).to_have_value("bar2")`);
    // [ts] expect.soft(sources2.get("Java").text).toContain(`assertThat(page.locator("#second")).hasValue("bar2")`);
    // [ts] expect.soft(sources2.get("C#").text).toContain(`await Expect(page.Locator("#second")).ToHaveValueAsync("bar2")`);
    // [ts] });
    // [ts] test("CG-086 should assert visibility", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input>`);
    // [ts] recorder.page.click("x-pw-tool-item.visibility");
    // [ts] recorder.hoverOverElement("input");
    // [ts] const [sources1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "textbox"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources1.get("JavaScript").text).toContain(`await expect(page.getByRole('textbox')).toBeVisible()`);
    // [ts] expect.soft(sources1.get("Python").text).toContain(`expect(page.get_by_role("textbox")).to_be_visible()`);
    // [ts] expect.soft(sources1.get("Python Async").text).toContain(`await expect(page.get_by_role("textbox")).to_be_visible()`);
    // [ts] expect.soft(sources1.get("Java").text).toContain(`assertThat(page.getByRole(AriaRole.TEXTBOX)).isVisible()`);
    // [ts] expect.soft(sources1.get("C#").text).toContain(`await Expect(page.GetByRole(AriaRole.Textbox)).ToBeVisibleAsync()`);
    // [ts] });
    // [ts] test("CG-087 should keep toolbar visible even if webpage erases content in hydration", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] const hydrate = () => {
    // [ts] window.builtins.setTimeout(() => {
    // [ts] document.documentElement.innerHTML = "<p>Post-Hydration Content</p>";
    // [ts] }, 500);
    // [ts] };
    // [ts] recorder.setContentAndWait(`
    // [ts] <p>Pre-Hydration Content</p>
    // [ts] <script>(${hydrate})()</script>
    // [ts] `);
    // [ts] expect(recorder.page.getByText("Post-Hydration Content")).toBeVisible();
    // [ts] expect(recorder.page.locator("x-pw-glass")).toBeVisible();
    // [ts] });
    // [ts] test("CG-088 should display inline svg icons on text assertion dialog inside iframe", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.page.click("x-pw-tool-item.text");
    // [ts] const { frameHello1 } = createFrameHierarchy(page, recorder, server);
    // [ts] recorder.trustedMove(frameHello1.locator("div"));
    // [ts] recorder.trustedClick();
    // [ts] const glassPane = frameHello1.locator("x-pw-glass");
    // [ts] expect(glassPane.locator("> x-pw-dialog .accept > x-div").evaluate(elem => getComputedStyle(elem).clipPath)).resolves.toBe("url(\"#icon-check\")");
    // [ts] expect(glassPane.locator("> svg > defs > clipPath#icon-check")).toBeAttached();
    // [ts] });
    // [ts] test("CG-089 should keep dialog open when clicking inside", () => {
    // [ts] const { page } = openRecorder();
    // [ts] page.setContent("<div>hello</div>");
    // [ts] const assertTextButton = page.getByTitle("Assert text");
    // [ts] const helloContent = page.getByText("hello");
    // [ts] assertTextButton.click();
    // [ts] helloContent.click();
    // [ts] const dialog = page.locator("x-pw-dialog");
    // [ts] expect(dialog).toBeVisible();
    // [ts] page.locator("x-pw-dialog x-pw-tools-list").click();
    // [ts] expect(dialog).toBeVisible();
    // [ts] page.locator("x-pw-dialog textarea").click();
    // [ts] expect(dialog).toBeVisible();
    // [ts] page.locator("x-pw-dialog .accept").click();
    // [ts] expect(dialog).toBeHidden();
    // [ts] assertTextButton.click();
    // [ts] helloContent.click();
    // [ts] expect(dialog).toBeVisible();
    // Like the user clicking outside the dialog
    // [ts] page.locator("x-pw-glass").click();
    // [ts] expect(dialog).toBeHidden();
    // [ts] });
    // [ts] test("CG-090 should record when top layer popover is open", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39095" } }, (_p) => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <div
    // [ts] popover="manual"
    // [ts] id="mypopover"
    // [ts] style="inset: 0; width: 100%; height: 100%; max-width: none; max-height: none; margin: 0; padding: 20px;"
    // [ts] >
    // [ts] <button>Close</button>
    // [ts] </div>
    // [ts] <button popovertarget="mypopover">Show Popover</button>
    // [ts] `);
    // [ts] page.getByRole("button", { name: "Show Popover" }).click();
    // [ts] expect(page.getByRole("button", { name: "Close" })).toBeVisible();
    // [ts] page.getByTitle("Assert text").click();
    // [ts] });
    // [ts] test("CG-091 should record when top layer popover inside shadow DOM is open", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39095" } }, (_p) => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <my-component></my-component>
    // [ts] <script>
    // [ts] class MyComponent extends HTMLElement {
    // [ts] constructor() {
    // [ts] super();
    // [ts] const shadow = this.attachShadow({ mode: 'open' });
    // [ts] shadow.innerHTML = \`
    // [ts] <div
    // [ts] popover="manual"
    // [ts] id="mypopover"
    // [ts] style="inset: 0; width: 100%; height: 100%; max-width: none; max-height: none; margin: 0; padding: 20px;"
    // [ts] >
    // [ts] <button>Close</button>
    // [ts] </div>
    // [ts] <button popovertarget="mypopover">Show Popover</button>
    // [ts] \`;
    // [ts] }
    // [ts] }
    // [ts] customElements.define('my-component', MyComponent);
    // [ts] </script>
    // [ts] `);
    // [ts] page.getByRole("button", { name: "Show Popover" }).click();
    // [ts] expect(page.getByRole("button", { name: "Close" })).toBeVisible();
    // [ts] page.getByTitle("Assert text").click();
    // [ts] });
    // [ts] test("CG-092 should record when manual popover with fullscreen backdrop is open", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39095" } }, (_p) => {
    // [ts] const { page, recorder } = openRecorder();
    // Mirrors the dialog pattern from Angular Material: https://material.angular.dev/components/dialog/examples
    // A manual popover with a fullscreen backdrop and a dialog content area.
    // Difference: the dialog is not centered, because Playwright recorder has trouble clicking on backdrops with centered occlusion.
    // [ts] recorder.setContentAndWait(`
    // [ts] <style>
    // [ts] .cdk-overlay-backdrop {
    // [ts] position: fixed;
    // [ts] top: 0;
    // [ts] bottom: 0;
    // [ts] left: 0;
    // [ts] right: 0;
    // [ts] pointer-events: auto;
    // [ts] background: rgba(0, 0, 0, 0.32);
    // [ts] opacity: 1;
    // [ts] }
    // [ts] .dialog-content {
    // [ts] position: fixed;
    // [ts] bottom: 20px;
    // [ts] right: 20px;
    // [ts] background: white;
    // [ts] padding: 20px;
    // [ts] z-index: 1001;
    // [ts] }
    // [ts] </style>
    // [ts] <div
    // [ts] popover="manual"
    // [ts] id="mypopover"
    // [ts] >
    // [ts] <div class="cdk-overlay-backdrop" id="backdrop"></div>
    // [ts] <div class="dialog-content">
    // [ts] <button id="closeBtn">Close Dialog</button>
    // [ts] </div>
    // [ts] </div>
    // [ts] <button id="openBtn">Open Dialog</button>
    // [ts] <script>
    // [ts] document.getElementById('openBtn').onclick = () => {
    // [ts] document.getElementById('mypopover').showPopover();
    // [ts] };
    // [ts] document.getElementById('backdrop').onclick = () => {
    // [ts] document.getElementById('mypopover').hidePopover();
    // [ts] };
    // [ts] document.getElementById('closeBtn').onclick = () => {
    // [ts] document.getElementById('mypopover').hidePopover();
    // [ts] };
    // [ts] </script>
    // [ts] `);
    // [ts] page.getByRole("button", { name: "Open Dialog" }).click();
    // [ts] expect(page.getByRole("button", { name: "Close Dialog" })).toBeVisible();
    // [ts] page.mouse.click(10, 10);
    // [ts] expect(page.getByRole("button", { name: "Close Dialog" })).toBeHidden();
    // [ts] });
    // [ts] test("CG-093 should record when fullscreen element is open", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/39095" } }, (_p) => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <div id="fullscreen-container">
    // [ts] <button id="closeBtn">Close Fullscreen</button>
    // [ts] </div>
    // [ts] <button id="openBtn">Go Fullscreen</button>
    // [ts] <script>
    // [ts] document.getElementById('openBtn').onclick = () => {
    // [ts] document.getElementById('fullscreen-container').requestFullscreen();
    // [ts] };
    // [ts] document.getElementById('closeBtn').onclick = () => {
    // [ts] document.exitFullscreen();
    // [ts] };
    // [ts] </script>
    // [ts] `);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Go Fullscreen"),
    // [ts] page.getByRole("button", { name: "Go Fullscreen" }).click(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `getByRole('button', { name: 'Go Fullscreen' }).click()`) != null, true);
    // [ts] page.waitForFunction(() => !!document.fullscreenElement);
    // Actions inside the fullscreen element are recorded.
    // [ts] const [sources2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "Close Fullscreen"),
    // [ts] page.getByRole("button", { name: "Close Fullscreen" }).click(),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, sources2.get("JavaScript").text, `getByRole('button', { name: 'Close Fullscreen' }).click()`) != null, true);
    // [ts] page.waitForFunction(() => !document.fullscreenElement);
    // After exiting fullscreen, the toolbar is clickable again.
    // [ts] page.getByTitle("Assert text").click();
    // [ts] });
}
    test("CG-094 should prepend the frame selector to disambiguate", () => {
      const { page, recorder } = openRecorder();
      recorder.setContentAndWait(`
        <iframe id=frame1 srcdoc="<iframe srcdoc='<button>Hello</button>'></iframe>"></iframe>
        <iframe id=frame2 srcdoc="<iframe srcdoc='<button>Hello</button>'></iframe>"></iframe>
      `, server.EMPTY_PAGE, 5);

      // The bare selector is ambiguous across frames, so a bare frameLocator() cannot pinpoint
      // the target frame and the full frame chain is used instead.
      const frame = page.mainFrame().childFrames()[0].childFrames()[0];
      const [sources] = joinAll([ // migrated: Promise.all -> joinAll
        recorder.waitForOutput("JavaScript", "click"),
        frame.click("text=Hello"),
      ]);

      expect.soft(sources.get("JavaScript").text).toContain(`
    page.locator('#frame1').contentFrame().locator('iframe').contentFrame().getByRole('button', { name: 'Hello' }).click();`);

      const clickAction = sources.get("JSON").actions.map(l => JSON.parse(l)).find(a => a.name == "click");
      expect.soft(clickAction.selector).toBe("#frame1 >> internal:control=enter-frame >> iframe >> internal:control=enter-frame >> internal:role=button[name=\"Hello\"i]");
    });
    test("CG-095 should not use frameLocator() when it saves a single frameLocator(selector)", () => {
      const { page, recorder } = openRecorder();
      recorder.setContentAndWait(`
        <iframe id=frame1 srcdoc="<button>Hello</button>"></iframe>
      `, server.EMPTY_PAGE, 2);

      // "Hello" is unique across frames, but a bare frameLocator() would only replace a single
      // frameLocator(selector) call, so the plain frame chain is not any longer.
      const frame = page.mainFrame().childFrames()[0];
      const [sources] = joinAll([ // migrated: Promise.all -> joinAll
        recorder.waitForOutput("JavaScript", "click"),
        frame.click("text=Hello"),
      ]);

      expect.soft(sources.get("JavaScript").text).toContain(`
    page.locator('#frame1').contentFrame().getByRole('button', { name: 'Hello' }).click();`);
    });
// TODO(cap-migrate): refine TS statement
// [ts] }
