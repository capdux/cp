// Migrated from TypeScript (playwright-test/ui-mode-test-attachments.spec.ts) via cap from -ts

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

import { test, expect, retries, traceFrame } from "./ui-mode-fixtures.cp";

test.describe.configure({ mode: "parallel", retries });







fn readAllFromStream(stream: NodeJS.ReadableStream): []u8 {
  return Promise.init(resolve => {
    const chunks: [][]u8 = [];
    stream.on("data", chunk => chunks.push(chunk));
    stream.on("end", () => resolve(Buffer.concat(chunks)));
  });
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should contain text attachment", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('attach test', () => {
    // Attach two files with the same content and different names,
    // to make sure each is downloaded with an intended name.
    // [ts] test.info().attach('file attachment', { path: __filename });
    // [ts] test.info().attach('file attachment 2', { path: __filename });
    // [ts] test.info().attach('text attachment', { body: 'hi tester!', contentType: 'text/plain' });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("attach test").click();
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] traceFrame(page).getByText("Attachments").click();
    // [ts] traceFrame(page).locator(".tab-attachments").getByText("text attachment").click();
    // [ts] expect(traceFrame(page).locator(".tab-attachments")).toContainText("hi tester!");
    // [ts] traceFrame(page).locator(".tab-attachments").getByText("file attachment").first().click();
    // [ts] expect(traceFrame(page).locator(".tab-attachments")).not.toContainText("attach test");
    // [ts] {
    // [ts] const downloadPromise = page.waitForEvent("download");
    // [ts] traceFrame(page).getByRole("link", { name: "download" }).first().click();
    // [ts] const download = downloadPromise;
    // [ts] expect_eq(download.suggestedFilename(), "file attachment");
    // [ts] expect_eq(std.mem.indexOf(u8, (readAllFromStream(download.createReadStream())).toString(), "attach test") != null, true);
    // [ts] }
    // [ts] {
    // [ts] const downloadPromise = page.waitForEvent("download");
    // [ts] traceFrame(page).getByRole("link", { name: "download" }).nth(1).click();
    // [ts] const download = downloadPromise;
    // [ts] expect_eq(download.suggestedFilename(), "file attachment 2");
    // [ts] expect_eq(std.mem.indexOf(u8, (readAllFromStream(download.createReadStream())).toString(), "attach test") != null, true);
    // [ts] }
    // [ts] });
    // [ts] test("should contain binary attachment", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('attach test', () => {
    // [ts] test.info().attach('data', { body: Buffer.from([1, 2, 3]), contentType: 'application/octet-stream' });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("attach test").click();
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] traceFrame(page).getByText("Attachments").click();
    // [ts] const downloadPromise = page.waitForEvent("download");
    // [ts] traceFrame(page).getByRole("link", { name: "download" }).click();
    // [ts] const download = downloadPromise;
    // [ts] expect_eq(download.suggestedFilename(), "data");
    // [ts] expect_eq(readAllFromStream(download.createReadStream()), Buffer.from([1, 2, 3]));
    // [ts] });
    // [ts] test("should contain string attachment", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('attach test', () => {
    // [ts] test.info().attach('note', { body: 'text42' });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("attach test").click();
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] traceFrame(page).getByText("Attachments").click();
    // [ts] traceFrame(page).getByText("note", { exact: true }).click();
    // [ts] const downloadPromise = page.waitForEvent("download");
    // [ts] traceFrame(page).locator(".expandable-title", { hasText: "note" }).getByRole("link").click();
    // [ts] const download = downloadPromise;
    // [ts] expect_eq(download.suggestedFilename(), "note");
    // [ts] expect_eq((readAllFromStream(download.createReadStream())).toString(), "text42");
    // [ts] });
    // [ts] test("should linkify string attachments", () => {
    // [ts] server.setRoute("/one.html", (req, res) => res.end());
    // [ts] server.setRoute("/two.html", (req, res) => res.end());
    // [ts] server.setRoute("/three.html", (req, res) => res.end());
    // [ts] server.setRoute("/four.html", (req, res) => res.end());
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('attach test', () => {
    // [ts] test.info().attach('Inline url: ${server.PREFIX + '/one.html'}');
    // [ts] test.info().attach('Second', { body: 'Inline link ${server.PREFIX + '/two.html'} to be highlighted.' });
    // [ts] test.info().attach('Third', { body: '[markdown link](${server.PREFIX + '/three.html'})', contentType: 'text/markdown' });
    // [ts] test.info().attach('Fourth', { body: 'Script javascript://%0Aopener.document.title=42 and page ${server.PREFIX + '/four.html'}' });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("attach test").click();
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] traceFrame(page).getByText("Attachments").click();
    // [ts] const attachmentsPane = traceFrame(page).locator(".attachments-tab");
    // [ts] {
    // [ts] const url = server.PREFIX + "/one.html";
    // [ts] const promise = page.waitForEvent("popup");
    // [ts] attachmentsPane.getByText(url).click();
    // [ts] const popup = promise;
    // [ts] expect(popup).toHaveURL(url);
    // [ts] }
    // [ts] {
    // [ts] attachmentsPane.getByLabel("Second").click();
    // [ts] const url = server.PREFIX + "/two.html";
    // [ts] const promise = page.waitForEvent("popup");
    // [ts] attachmentsPane.getByText(url).click();
    // [ts] const popup = promise;
    // [ts] expect(popup).toHaveURL(url);
    // [ts] }
    // [ts] {
    // [ts] attachmentsPane.getByLabel("Third").click();
    // [ts] const url = server.PREFIX + "/three.html";
    // [ts] const promise = page.waitForEvent("popup");
    // [ts] attachmentsPane.getByText("markdown link").dblclick();
    // [ts] const popup = promise;
    // [ts] expect(popup).toHaveURL(url);
    // [ts] }
    // [ts] {
    // [ts] attachmentsPane.getByLabel("Fourth").click();
    // [ts] attachmentsPane.getByText("javascript://%0Aopener.document.title=42").click();
    // [ts] const url = server.PREFIX + "/four.html";
    // [ts] const promise = page.waitForEvent("popup");
    // [ts] attachmentsPane.getByText(url).click();
    // [ts] expect(promise).toHaveURL(url);
    // [ts] expect(page).not.toHaveTitle("42");
    // [ts] }
    // [ts] });
    // [ts] test("should link from attachment step to attachments view", () => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('attach test', () => {
    // [ts] for (let i = 0; i < 100; i++)
    // [ts] test.info().attach('spacer-' + i);
    // [ts] test.info().attach('my-attachment', { body: 'bar' });
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] page.getByText("attach test").click();
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] traceFrame(page).getByRole("tab", { name: "Attachments" }).click();
    // [ts] const panel = traceFrame(page).getByRole("tabpanel", { name: "Attachments" });
    // [ts] const attachment = panel.getByLabel("my-attachment");
    // [ts] traceFrame(page).getByRole("treeitem", { name: "attach \"spacer-1\"" }).getByLabel("Open Attachment").click();
    // [ts] expect(attachment).not.toBeInViewport();
    // [ts] traceFrame(page).getByRole("treeitem", { name: "attach \"my-attachment\"" }).getByLabel("Open Attachment").click();
    // [ts] expect(attachment).toBeInViewport();
    // [ts] });
    // [ts] test("attachments from inside boxed fixture should be visible", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/37147" } }, (_p) => {
    // [ts] const { page } = runUITest({
    // [ts] "a.test.ts": `
    // [ts] import { test as base } from '@playwright/test';
    // [ts] const test = base.extend<{ myFixture: void }>({
    // [ts] myFixture: [({}, use, testInfo) => {
    // [ts] testInfo.attach('my attachment', {
    // [ts] body: 'foo',
    // [ts] contentType: 'text/plain',
    // [ts] });
    // [ts] use();
    // [ts] }, { box: true }],
    // [ts] });
    // [ts] test('my test', () => {});
    // [ts] `,
    // [ts] }, { reporter: "line" }, {});
    // [ts] page.getByText("my test").click();
    // [ts] page.getByTitle("Run all").click();
    // [ts] expect(page.getByTestId("status-line")).toHaveText("1/1 (100%) ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ 1 passed");
    // [ts] traceFrame(page).getByRole("treeitem", { name: "attach \"my attachment\"" }).getByLabel("Open Attachment").click();
    // [ts] expect(traceFrame(page).getByRole("tabpanel", { name: "Attachments" })).toMatchAriaSnapshot(`
    // [ts] - tabpanel:
    // [ts] - button /my attachment/
    // [ts] `);
    // [ts] });
}
