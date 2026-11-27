// Migrated from TypeScript (library/inspector/cli-codegen-2.spec.ts) via cap from -ts

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

// [cap-test] test.describe("cli codegen", () => {



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("CG-039 should contain open page", () => {
    // [ts] const { recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(``);
    // [ts] const sources = recorder.waitForOutput("JavaScript", `page.goto`);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] const page = context.newPage();`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] Page page = context.newPage();`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page = context.new_page()`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page = context.new_page()`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] var page = context.NewPageAsync();`);
    // [ts] });
    // [ts] test("CG-040 should contain second page", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(``);
    // [ts] page.context().newPage();
    // [ts] const sources = recorder.waitForOutput("JavaScript", "page1");
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] const page1 = context.newPage();`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] Page page1 = context.newPage();`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page1 = context.new_page()`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page1 = context.new_page()`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] var page1 = context.NewPageAsync();`);
    // [ts] });
    // [ts] test("CG-041 should contain close page", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(``);
    // [ts] page.context().newPage();
    // [ts] recorder.page.close();
    // [ts] const sources = recorder.waitForOutput("JavaScript", "page.close();");
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.close();`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.close();`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.close()`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.close()`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.CloseAsync();`);
    // [ts] });
    // [ts] test("CG-042 should not lead to an error if html gets clicked", () => {
    // [ts] test(platform == "darwin" && macVersion < 15, "recorder.page.evaluate hangs on CDP layer for some reason on macOS 14.");
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait("");
    // [ts] page.context().newPage();
    // [ts] const errors: []string = [];
    // [ts] recorder.page.on("pageerror", e => errors.push(e));
    // [ts] recorder.page.evaluate(() => document.querySelector("body").remove());
    // [ts] page.dispatchEvent("html", "mousemove", { detail: 1 });
    // [ts] recorder.page.close();
    // [ts] recorder.waitForOutput("JavaScript", "page.close();");
    // [ts] expect_eq(errors.length, 0);
    // [ts] });
    // [ts] test("CG-043 should upload a single file", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <form>
    // [ts] <input type="file">
    // [ts] </form>
    // [ts] `);
    // [ts] recorder.hoverOverElement("input[type=file]");
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // Simulate the time a real user takes to pick a file from the OS dialog
    // so the merge logic cannot rely on a short delay between click and setInputFiles.
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] chooser.setFiles(asset("file-to-upload.txt"));
    // [ts] const sources = recorder.waitForOutput("JavaScript", "setInputFiles");
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Choose File' }).setInputFiles('file-to-upload.txt');`);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `click()`) == null, true);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Choose File")).setInputFiles(Paths.get("file-to-upload.txt"));`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Choose File").set_input_files(\"file-to-upload.txt\")`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Choose File").set_input_files(\"file-to-upload.txt\")`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Choose File" }).SetInputFilesAsync(new[] { \"file-to-upload.txt\" });`);
    // [ts] });
    // [ts] test("CG-044 should upload multiple files", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <form>
    // [ts] <input type="file" multiple>
    // [ts] </form>
    // [ts] `);
    // [ts] page.focus("input[type=file]");
    // [ts] page.setInputFiles("input[type=file]", [asset("file-to-upload.txt"), asset("file-to-upload-2.txt")]);
    // [ts] page.click("input[type=file]");
    // [ts] const sources = recorder.waitForOutput("JavaScript", "setInputFiles");
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Choose File' }).setInputFiles(['file-to-upload.txt', 'file-to-upload-2.txt']);`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Choose File")).setInputFiles(new Path[] {Paths.get("file-to-upload.txt"), Paths.get("file-to-upload-2.txt")});`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Choose File").set_input_files([\"file-to-upload.txt\", \"file-to-upload-2.txt\"]`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Choose File").set_input_files([\"file-to-upload.txt\", \"file-to-upload-2.txt\"]`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Choose File" }).SetInputFilesAsync(new[] { \"file-to-upload.txt\", \"file-to-upload-2.txt\" });`);
    // [ts] });
    // [ts] test("CG-045 should clear files", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <form>
    // [ts] <input type="file" multiple>
    // [ts] </form>
    // [ts] `);
    // [ts] page.focus("input[type=file]");
    // [ts] page.setInputFiles("input[type=file]", asset("file-to-upload.txt"));
    // [ts] page.setInputFiles("input[type=file]", []);
    // [ts] page.click("input[type=file]");
    // [ts] const sources = recorder.waitForOutput("JavaScript", "setInputFiles");
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.getByRole('button', { name: 'Choose File' }).setInputFiles([]);`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("Choose File")).setInputFiles(new Path[0]);`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="Choose File").set_input_files([])`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="Choose File").set_input_files([])`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "Choose File" }).SetInputFilesAsync(new[] {  });`);
    // [ts] });
    // [ts] test("CG-046 should upload a file via hidden input triggered by button", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button onclick="document.querySelector('input[type=file]').click()">select file</button>
    // [ts] <input type="file" style="display: none">
    // [ts] `);
    // [ts] recorder.hoverOverElement("button");
    // [ts] const [chooser] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("filechooser"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] Promise.init(f => setTimeout(f, 1000));
    // [ts] chooser.setFiles(asset("file-to-upload.txt"));
    // [ts] const sources = recorder.waitForOutput("JavaScript", "setInputFiles");
    // The setInputFiles call must target the actual file input, not the trigger button.
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `setInputFiles('file-to-upload.txt')`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `getByRole('button', { name: 'select file' }).setInputFiles`) == null, true);
    // [ts] });
    // [ts] test("CG-047 should download files", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] server.setRoute("/download", (req, res) => {
    // [ts] const pathName = URL.init(req.url, "http://localhost").pathname;
    // [ts] if (pathName == "/download") {
    // [ts] res.setHeader("Content-Type", "application/octet-stream");
    // [ts] res.setHeader("Content-Disposition", "attachment; filename=file.txt");
    // [ts] res.end(`Hello world`);
    // [ts] } else {
    // [ts] res.setHeader("Content-Type", "text/html; charset=utf-8");
    // [ts] res.end("");
    // [ts] }
    // [ts] });
    // [ts] recorder.setContentAndWait(`
    // [ts] <a href="${server.PREFIX}/download" download>Download</a>
    // [ts] `, server.PREFIX);
    // [ts] recorder.hoverOverElement("a");
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("download"),
    // [ts] page.click("a")
    // [ts] ]);
    // [ts] const sources = recorder.waitForOutput("JavaScript", "await downloadPromise");
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] const downloadPromise = page.waitForEvent('download');
    // [ts] page.getByRole('link', { name: 'Download' }).click();
    // [ts] const download = downloadPromise;`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] Download download = page.waitForDownload(() -> {
    // [ts] page.getByRole(AriaRole.LINK, new Page.GetByRoleOptions().setName("Download")).click();
    // [ts] });`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] with page.expect_download() as download_info:
    // [ts] page.get_by_role("link", name="Download").click()
    // [ts] download = download_info.value`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] async with page.expect_download() as download_info:
    // [ts] page.get_by_role("link", name="Download").click()
    // [ts] download = download_info.value`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] var download = page.RunAndWaitForDownloadAsync(() =>
    // [ts] {
    // [ts] page.GetByRole(AriaRole.Link, new() { Name = "Download" }).ClickAsync();
    // [ts] });`);
    // [ts] });
    // [ts] test("CG-048 should handle dialogs", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button onclick="alert()">click me</button>
    // [ts] `);
    // [ts] recorder.hoverOverElement("button");
    // [ts] page.once("dialog", async dialog => {
    // [ts] dialog.dismiss();
    // [ts] });
    // [ts] page.click("button");
    // [ts] const sources = recorder.waitForOutput("JavaScript", "once");
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`
    // [ts] page.once('dialog', dialog => {
    // [ts] console.log(\`Dialog message: \${dialog.message()}\`);
    // [ts] dialog.dismiss().catch(() => {});
    // [ts] });
    // [ts] page.getByRole('button', { name: 'click me' }).click();`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.onceDialog(dialog -> {
    // [ts] System.out.println(String.format("Dialog message: %s", dialog.message()));
    // [ts] dialog.dismiss();
    // [ts] });
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("click me")).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.once(\"dialog\", lambda dialog: dialog.dismiss())
    // [ts] page.get_by_role("button", name="click me").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.once(\"dialog\", lambda dialog: dialog.dismiss())
    // [ts] page.get_by_role("button", name="click me").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] void page_Dialog_EventHandler(object sender, IDialog dialog)
    // [ts] {
    // [ts] Console.WriteLine($\"Dialog message: {dialog.Message}\");
    // [ts] dialog.DismissAsync();
    // [ts] page.Dialog -= page_Dialog_EventHandler;
    // [ts] }
    // [ts] page.Dialog += page_Dialog_EventHandler;
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "click me" }).ClickAsync();`);
    // [ts] });
    // [ts] test("CG-049 should handle history.postData", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <script>
    // [ts] let seqNum = 0;
    // [ts] function pushState() {
    // [ts] history.pushState({}, 'title', '${server.PREFIX}/#seqNum=' + (++seqNum));
    // [ts] }
    // [ts] </script>`, server.PREFIX);
    // [ts] let i = 1;
    // [ts] while (i < 3) { // [cap-migrate] for-step: ++i
    // [ts] page.evaluate("pushState()");
    // [ts] recorder.waitForOutput("JavaScript", `await page.goto('${server.PREFIX}/#seqNum=${i}');`);
    // [ts] }
    // [ts] });
    // [ts] test("CG-050 should record open in a new tab with url", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<a href="about:blank?foo">link</a>`);
    // [ts] const locator = recorder.hoverOverElement("a");
    // [ts] expect_eq(locator, `getByRole('link', { name: 'link' })`);
    // [ts] page.click("a", { modifiers: ["ControlOrMeta"] });
    // [ts] const sources = recorder.waitForOutput("JavaScript", "page1");
    // [ts] if (browserName != "firefox") {
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] const page1 = context.newPage();
    // [ts] page1.goto('about:blank?foo');`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page1 = context.new_page()
    // [ts] page1.goto("about:blank?foo")`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] var page1 = context.NewPageAsync();
    // [ts] page1.GotoAsync("about:blank?foo");`);
    // [ts] } else {
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] const page1Promise = page.waitForEvent('popup');
    // [ts] page.getByRole('link', { name: 'link' }).click({
    // [ts] modifiers: ['ControlOrMeta']
    // [ts] });
    // [ts] const page1 = page1Promise;`);
    // [ts] }
    // [ts] });
    // [ts] test("CG-051 should not clash pages", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] const [popup1] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("page"),
    // [ts] page.evaluate(`window.open('about:blank')`)
    // [ts] ]);
    // [ts] recorder.setPageContentAndWait(popup1, "<input id=name>");
    // [ts] const [popup2] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.context().waitForEvent("page"),
    // [ts] page.evaluate(`window.open('about:blank')`)
    // [ts] ]);
    // [ts] recorder.setPageContentAndWait(popup2, "<input id=name>");
    // [ts] popup1.type("input", "TextA");
    // [ts] recorder.waitForOutput("JavaScript", "TextA");
    // [ts] popup2.type("input", "TextB");
    // [ts] recorder.waitForOutput("JavaScript", "TextB");
    // [ts] const sources = recorder.sources();
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `await page1.locator('#name').fill('TextA');`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("JavaScript").text, `await page2.locator('#name').fill('TextB');`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("Java").text, `page1.locator("#name").fill("TextA");`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("Java").text, `page2.locator("#name").fill("TextB");`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("Python").text, `page1.locator("#name").fill("TextA")`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("Python").text, `page2.locator("#name").fill("TextB")`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("Python Async").text, `await page1.locator("#name").fill("TextA")`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("Python Async").text, `await page2.locator("#name").fill("TextB")`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("C#").text, `await page1.Locator("#name").FillAsync("TextA");`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, sources.get("C#").text, `await page2.Locator("#name").FillAsync("TextB");`) != null, true);
    // [ts] });
    // [ts] test("CG-052 click should emit events in order", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button id=button>
    // [ts] <script>
    // [ts] button.addEventListener('mousedown', e => console.log(e.type));
    // [ts] button.addEventListener('mouseup', e => console.log(e.type));
    // [ts] button.addEventListener('click', e => console.log(e.type));
    // [ts] </script>
    // [ts] `);
    // [ts] const messages: []string = [];
    // [ts] page.on("console", message => {
    // [ts] if (message.type() != "error")
    // [ts] messages.push(message.text());
    // [ts] });
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.click("button"),
    // [ts] recorder.waitForOutput("JavaScript", ".click(")
    // [ts] ]);
    // [ts] expect.poll(() => messages).toEqual(["mousedown", "mouseup", "click"]);
    // [ts] });
    // [ts] test("CG-053 should reset hover model on action when element detaches", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="checkbox" onclick="document.getElementById('checkbox').remove()">`);
    // [ts] const [models] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForActionPerformed(),
    // [ts] page.click("input")
    // [ts] ]);
    // [ts] expect_eq(models.hovered, null);
    // [ts] });
    // [ts] test("CG-054 should update active model on action", () => {
    // [ts] test(browserName == "webkit", "webkit does not actually focus the input after click");
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="checkbox" type="checkbox" name="accept" onchange="checkbox.name='updated'"></input>`);
    // [ts] const [models] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForActionPerformed(),
    // [ts] page.click("input")
    // [ts] ]);
    // [ts] expect_eq(models.active, "#checkbox");
    // [ts] });
    // [ts] test("CG-055 should check input with chaining id", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<input id="checkbox" type="checkbox" name="accept" onchange="checkbox.name = 'updated'"></input>`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForActionPerformed(),
    // [ts] page.click("input[id=checkbox]")
    // [ts] ]);
    // [ts] });
    // [ts] test("CG-056 should record navigations after identical pushState", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] server.setRoute("/page2.html", (req, res) => {
    // [ts] res.setHeader("Content-Type", "text/html; charset=utf-8");
    // [ts] res.end("Hello world");
    // [ts] });
    // [ts] recorder.setContentAndWait(`
    // [ts] <script>
    // [ts] function pushState() {
    // [ts] history.pushState({}, 'title', '${server.PREFIX}');
    // [ts] }
    // [ts] </script>`, server.PREFIX);
    // [ts] let i = 1;
    // [ts] while (i < 3) // [cap-migrate] for-step: ++i
    // [ts] page.evaluate("pushState()");
    // [ts] page.goto(server.PREFIX + "/page2.html");
    // [ts] recorder.waitForOutput("JavaScript", `await page.goto('${server.PREFIX}/page2.html');`);
    // [ts] });
    // [ts] test("CG-057 should fill tricky characters", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`<textarea spellcheck=false id="textarea" name="name" oninput="console.log(textarea.value)"></textarea>`);
    // [ts] const locator = recorder.focusElement("textarea");
    // [ts] expect_eq(locator, `locator('#textarea')`);
    // [ts] const [message, sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.type() != "error"),
    // [ts] recorder.waitForOutput("JavaScript", "fill"),
    // [ts] page.fill("textarea", "Hello'\"\`\nWorld")
    // [ts] ]);
    // [ts] expect(sources.get("JavaScript").text).toContain(`
    // [ts] page.locator('#textarea').fill('Hello\\'"\`\\nWorld');`);
    // [ts] expect(sources.get("Java").text).toContain(`
    // [ts] page.locator("#textarea").fill("Hello'\\"\`\\nWorld");`);
    // [ts] expect(sources.get("Python").text).toContain(`
    // [ts] page.locator("#textarea").fill(\"Hello'\\"\`\\nWorld\")`);
    // [ts] expect(sources.get("Python Async").text).toContain(`
    // [ts] page.locator("#textarea").fill(\"Hello'\\"\`\\nWorld\")`);
    // [ts] expect(sources.get("C#").text).toContain(`
    // [ts] page.Locator("#textarea").FillAsync(\"Hello'\\"\`\\nWorld\");`);
    // [ts] expect_eq(message.text(), "Hello'\"\`\nWorld");
    // [ts] });
    // [ts] test("CG-058 should --test-id-attribute", () => {
    // [ts] const { page, recorder } = openRecorder({ testIdAttributeName: "my-test-id" });
    // [ts] recorder.setContentAndWait(`<div my-test-id="foo">Hello</div>`);
    // [ts] page.click("[my-test-id=foo]");
    // [ts] const sources = recorder.waitForOutput("JavaScript", `page.getByTestId`);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`await page.getByTestId('foo').click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`page.getByTestId("foo").click()`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`page.get_by_test_id("foo").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`await page.get_by_test_id("foo").click()`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`await page.GetByTestId("foo").ClickAsync();`);
    // [ts] });
    // [ts] test("CG-059 should generate getByTestId for any of the configured testIdAttributes", () => {
    // [ts] const { page, recorder } = openRecorder({ testIdAttributeName: "data-pw,data-ti" });
    // [ts] recorder.setContentAndWait(`
    // [ts] <button data-pw="primary">First</button>
    // [ts] <button data-ti="secondary">Second</button>
    // [ts] `);
    // [ts] page.click("[data-pw=primary]");
    // [ts] let sources = recorder.waitForOutput("JavaScript", `page.getByTestId('primary')`);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`await page.getByTestId('primary').click()`);
    // [ts] page.click("[data-ti=secondary]");
    // [ts] sources = recorder.waitForOutput("JavaScript", `page.getByTestId('secondary')`);
    // [ts] expect.soft(sources.get("JavaScript").text).toContain(`await page.getByTestId('secondary').click()`);
    // [ts] });
    // [ts] test("CG-060 should auto-generate toBeVisible", () => {
    // [ts] const { page, recorder } = openRecorder();
    // [ts] recorder.setContentAndWait(`
    // [ts] <button id=one>one</button>
    // [ts] <div id=insertion></div>
    // [ts] <button id=two>two</button>
    // [ts] <script>
    // [ts] const one = document.getElementById('one');
    // [ts] const insertion = document.getElementById('insertion');
    // [ts] one.addEventListener('click', () => {
    // [ts] insertion.innerHTML = '<h2>new header</h2>';
    // [ts] console.log('clicked one');
    // [ts] });
    // [ts] two.addEventListener('click', () => {
    // [ts] console.log('clicked two');
    // [ts] });
    // [ts] </script>
    // [ts] `);
    // [ts] recorder.recorderPage.getByRole("button", { name: "Settings" }).click();
    // [ts] recorder.recorderPage.getByRole("checkbox", { name: "Generate assertions" }).check();
    // [ts] const locatorOne = recorder.hoverOverElement("#one");
    // [ts] expect_eq(locatorOne, `getByRole('button', { name: 'one' })`);
    // [ts] joinAll([ // migrated: Promise.all -> joinAll
    // [ts] page.waitForEvent("console", msg => msg.text() == "clicked one"),
    // [ts] recorder.waitForOutput("JavaScript", "one"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] const locatorTwo = recorder.hoverOverElement("#two");
    // [ts] expect_eq(locatorTwo, `getByRole('button', { name: 'two' })`);
    // [ts] const [sources] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] recorder.waitForOutput("JavaScript", "two"),
    // [ts] page.waitForEvent("console", msg => msg.text() == "clicked two"),
    // [ts] recorder.trustedClick(),
    // [ts] ]);
    // [ts] expect.soft(sources.get("Playwright Test").text).toContain(`
    // [ts] page.getByRole('button', { name: 'one' }).click();
    // [ts] expect(page.getByRole('heading', { name: 'new header' })).toBeVisible();
    // [ts] page.getByRole('button', { name: 'two' }).click();`);
    // [ts] expect.soft(sources.get("Python").text).toContain(`
    // [ts] page.get_by_role("button", name="one").click()
    // [ts] expect(page.get_by_role("heading", name="new header")).to_be_visible()
    // [ts] page.get_by_role("button", name="two").click()`);
    // [ts] expect.soft(sources.get("Python Async").text).toContain(`
    // [ts] page.get_by_role("button", name="one").click()
    // [ts] expect(page.get_by_role("heading", name="new header")).to_be_visible()
    // [ts] page.get_by_role("button", name="two").click()`);
    // [ts] expect.soft(sources.get("Java").text).toContain(`
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("one")).click();
    // [ts] assertThat(page.getByRole(AriaRole.HEADING, new Page.GetByRoleOptions().setName("new header"))).isVisible();
    // [ts] page.getByRole(AriaRole.BUTTON, new Page.GetByRoleOptions().setName("two")).click();`);
    // [ts] expect.soft(sources.get("C#").text).toContain(`
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "one" }).ClickAsync();
    // [ts] Expect(page.GetByRole(AriaRole.Heading, new() { Name = "new header" })).ToBeVisibleAsync();
    // [ts] page.GetByRole(AriaRole.Button, new() { Name = "two" }).ClickAsync();`);
    // [ts] });
}
// TODO(cap-migrate): refine TS statement
// [ts] }
