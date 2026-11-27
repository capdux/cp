// Migrated from TypeScript (mcp/test-debug.spec.ts) via cap from -ts

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

import { test, expect, writeFiles, prepareDebugTest } from "./fixtures.cp";

test.use({ mcpServerType: "test-mcp" });














fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("test_debug (passed)", () => {
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const { client } = startClient();
    // [ts] const listResult = client.callTool({
    // [ts] name: "test_list",
    // [ts] });
    // [ts] const [, id] = listResult.content[0].text.match("\\[id=([^\\]]+)\\]");
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "pass" },
    // [ts] },
    // [ts] })).toHaveTextResponse(`
    // [ts] Running 1 test using 1 worker
    // [ts] ok 1 [id=<ID>] a.test.ts:3:7 › pass (XXms)
    // [ts] 1 passed (XXms)`);
    // [ts] });
    // [ts] test("test_debug (pause/resume)", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient);
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toHaveTextResponse(`
    // [ts] Running 1 test using 1 worker
    // [ts] ### Paused on error:
    // [ts] Error: expect(locator).toBeVisible() failed
    // [ts] Locator: getByRole('button', { name: 'Missing' })
    // [ts] Expected: visible
    // [ts] Timeout: 1000ms
    // [ts] Error: element(s) not found
    // [ts] Call log:
    // [ts] - Expect "toBeVisible" getByRole('button', { name: 'Missing' }) with timeout 1000ms
    // [ts] - waiting for getByRole('button', { name: 'Missing' })
    // [ts] ### Page state
    // [ts] - Page URL: about:blank
    // [ts] - Page Title:
    // [ts] - Page Snapshot:
    // [ts] \`\`\`yaml
    // [ts] - button "Submit" [ref=e2]
    // [ts] \`\`\`
    // [ts] ### Task
    // [ts] Try recovering from the error prior to continuing`);
    // [ts] expect(client.callTool({
    // [ts] name: "test_run",
    // [ts] arguments: {
    // [ts] locations: ["a.test.ts"],
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining(`1) [id=<ID>] a.test.ts:3:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº fail`));
    // [ts] });
    // [ts] test("test_debug (browser_snapshot/network/console)", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient, `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.goto(${JSON.stringify(server.HELLO_WORLD)});
    // [ts] page.evaluate(() => {
    // [ts] console.log('hello from console');
    // [ts] fetch('/missing');
    // [ts] setTimeout(() => { throw new Error('error from page'); }, 0);
    // [ts] });
    // [ts] expect(page.getByRole('button', { name: 'Missing' })).toBeVisible({ timeout: 1000 });
    // [ts] });
    // [ts] `);
    // [ts] client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] });
    // [ts] expect.poll(() => client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`[GET] ${server.PREFIX}/missing => [404]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringMatching("\\[LOG\\] hello from console.*\\n.*404.*\\nError: error from page"),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`generic [active] [ref=e1]: Hello, world!`),
    // [ts] });
    // [ts] });
    // [ts] test("test_debug (multiple pages and custom error)", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient, `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] const page2 = page.context().newPage();
    // [ts] page.goto(${JSON.stringify(server.EMPTY_PAGE)});
    // [ts] page2.goto(${JSON.stringify(server.PREFIX + '/wrappedlink.html')});
    // [ts] throw new Error('non-api error');
    // [ts] });
    // [ts] `);
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toHaveTextResponse(`
    // [ts] Running 1 test using 1 worker
    // [ts] ### Paused on error:
    // [ts] Error: non-api error
    // [ts] ### Page 1 of 2
    // [ts] - Page URL: ${server.EMPTY_PAGE}
    // [ts] - Page Title:
    // [ts] - Page Snapshot:
    // [ts] \`\`\`yaml
    // [ts] \`\`\`
    // [ts] ### Page 2 of 2
    // [ts] - Page URL: ${server.PREFIX + '/wrappedlink.html'}
    // [ts] - Page Title:
    // [ts] - Page Snapshot:
    // [ts] \`\`\`yaml
    // [ts] - link "123321" [ref=e3] [cursor=pointer]:
    // [ts] - /url: "#clicked"
    // [ts] \`\`\`
    // [ts] ### Task
    // [ts] Try recovering from the error prior to continuing`);
    // [ts] });
    // [ts] test("test_debug (pause/snapshot/resume)", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient);
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toHaveTextResponse(`
    // [ts] Running 1 test using 1 worker
    // [ts] ### Paused on error:
    // [ts] Error: expect(locator).toBeVisible() failed
    // [ts] Locator: getByRole('button', { name: 'Missing' })
    // [ts] Expected: visible
    // [ts] Timeout: 1000ms
    // [ts] Error: element(s) not found
    // [ts] Call log:
    // [ts] - Expect "toBeVisible" getByRole('button', { name: 'Missing' }) with timeout 1000ms
    // [ts] - waiting for getByRole('button', { name: 'Missing' })
    // [ts] ### Page state
    // [ts] - Page URL: about:blank
    // [ts] - Page Title:
    // [ts] - Page Snapshot:
    // [ts] \`\`\`yaml
    // [ts] - button "Submit" [ref=e2]
    // [ts] \`\`\`
    // [ts] ### Task
    // [ts] Try recovering from the error prior to continuing`);
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- button \"Submit\" [ref=e2]`),
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "test_run",
    // [ts] arguments: {
    // [ts] locations: ["a.test.ts"],
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining(`1) [id=<ID>] a.test.ts:3:7 ÃÂÃÂ¢ÃÂÃÂÃÂÃÂº fail`));
    // [ts] });
    // [ts] test("test_debug / evaluate", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient);
    // [ts] client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "() => 21+21",
    // [ts] intent: "Calculate 21+21",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `42`,
    // [ts] });
    // [ts] });
    // [ts] test("test_debug / evaluate x 2", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient);
    // [ts] client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "() => 21+21",
    // [ts] intent: "Calculate 21+21",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `42`,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toEqual({
    // [ts] content: [
    // [ts] { type: "text", text: expect.stringContaining(`Paused on error`) },
    // [ts] ],
    // [ts] isError: false,
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "() => 21+23",
    // [ts] intent: "Calculate 21+23",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `44`,
    // [ts] });
    // [ts] });
    // [ts] test("test_debug / evaluate (with element)", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient);
    // [ts] client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_evaluate",
    // [ts] arguments: {
    // [ts] function: "element => element.textContent",
    // [ts] element: "button",
    // [ts] target: "e2",
    // [ts] intent: "Get button text",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `"Submit"`,
    // [ts] });
    // [ts] });
    // [ts] test("test_debug / generate_locator", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient);
    // [ts] client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] });
    // [ts] expect(client.callTool({
    // [ts] name: "browser_generate_locator",
    // [ts] arguments: {
    // [ts] element: "button",
    // [ts] target: "e2",
    // [ts] },
    // [ts] })).toHaveResponse({
    // [ts] result: `getByRole('button', { name: 'Submit' })`,
    // [ts] });
    // [ts] });
    // [ts] test("test_debug w/ console.log in test", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient, `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] console.log('console.log');
    // [ts] console.error('console.error');
    // [ts] expect(page.getByRole('button', { name: 'Missing' })).toBeVisible({ timeout: 1000 });
    // [ts] });
    // [ts] `);
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining(`
    // [ts] Running 1 test using 1 worker
    // [ts] console.log
    // [ts] [err] console.error
    // [ts] ### Paused on error:
    // [ts] Error: expect(locator).toBeVisible() failed`));
    // [ts] });
    // [ts] test("test_debug w/ console_messages", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient, `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.evaluate(() => {
    // [ts] console.error('console.error');
    // [ts] console.log('console.log');  // Log should be ignored in the initial message.
    // [ts] setTimeout(() => {
    // [ts] const error = new Error('Error from page');
    // [ts] error.stack = '';
    // [ts] throw error;
    // [ts] }, 0);
    // [ts] return new Promise(f => setTimeout(f, 10));
    // [ts] });
    // [ts] throw new Error('failure');
    // [ts] });
    // [ts] `);
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining(`
    // [ts] Running 1 test using 1 worker
    // [ts] ### Paused on error:
    // [ts] Error: failure
    // [ts] ### Page state
    // [ts] - Page URL: about:blank
    // [ts] - Page Title:
    // [ts] - Console Messages:
    // [ts] - [ERROR] console.error @ :2
    // [ts] - Error from page
    // [ts] - Page Snapshot:
    // [ts] `));
    // [ts] expect(client.callTool({
    // [ts] name: "browser_console_messages",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringMatching("\\[ERROR\\] console.error.*\\n\\[LOG] console.log"),
    // [ts] });
    // [ts] });
    // [ts] test("test_debug w/ network_requests", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient, `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.goto(${JSON.stringify(server.HELLO_WORLD)});
    // [ts] page.evaluate(() => {
    // [ts] fetch('missing');
    // [ts] });
    // [ts] expect(page.getByRole('button', { name: 'Missing' })).toBeVisible({ timeout: 1000 });
    // [ts] });
    // [ts] `);
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining(`
    // [ts] Running 1 test using 1 worker
    // [ts] ### Paused on error:
    // [ts] Error: expect(locator).toBeVisible() failed`));
    // [ts] expect(client.callTool({
    // [ts] name: "browser_network_requests",
    // [ts] })).toHaveResponse({
    // [ts] result: expect.stringContaining(`[GET] ${server.PREFIX}/missing => [404] Not Found`),
    // [ts] });
    // [ts] });
    // [ts] test("test_debug w/ route", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient, `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] let counter = 0;
    // [ts] page.route('**', route => route.fulfill({ body: '<title>Title' + (++counter) + '</title><div>mocked</div>', contentType: 'text/html' }));
    // [ts] page.goto(${JSON.stringify(server.EMPTY_PAGE)});
    // [ts] expect(page.getByRole('button', { name: 'Missing' })).toBeVisible({ timeout: 1000 });
    // [ts] });
    // [ts] `);
    // [ts] const response = client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] });
    // [ts] expect(response).toHaveTextResponse(expect.stringContaining(`
    // [ts] Running 1 test using 1 worker
    // [ts] ### Paused on error:
    // [ts] Error: expect(locator).toBeVisible() failed`));
    // [ts] expect(response).toHaveTextResponse(expect.stringContaining(`- Page URL: ${server.EMPTY_PAGE}\n- Page Title: Title1`));
    // [ts] expect(client.callTool({
    // [ts] name: "browser_navigate",
    // [ts] arguments: { url: server.HELLO_WORLD, intent: "Go to hello world" },
    // [ts] })).toHaveResponse({
    // [ts] page: expect.stringContaining(`- Page URL: ${server.HELLO_WORLD}\n- Page Title: Title2`),
    // [ts] });
    // [ts] });
    // [ts] test("test_debug (no default page fixture)", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient, `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] throw new Error('failure');
    // [ts] });
    // [ts] `);
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining(`Only tests that use default Playwright context or page fixture support test_debug`));
    // [ts] });
    // [ts] test("test_debug (custom fixture with browser.newContext)", () => {
    // [ts] const { client, id } = prepareDebugTest(startClient, `
    // [ts] import { test as base, expect } from '@playwright/test';
    // [ts] const test = base.extend<{ customPage: import('@playwright/test').Page }>({
    // [ts] customPage: ({ browser }, use) => {
    // [ts] const context = browser.newContext();
    // [ts] const page = context.newPage();
    // [ts] use(page);
    // [ts] context.close();
    // [ts] },
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] customPage.setContent('<button>Submit</button>');
    // [ts] expect(customPage.getByRole('button', { name: 'Missing' })).toBeVisible({ timeout: 1000 });
    // [ts] });
    // [ts] `);
    // [ts] expect(client.callTool({
    // [ts] name: "test_debug",
    // [ts] arguments: {
    // [ts] test: { id, title: "fail" },
    // [ts] },
    // [ts] })).toHaveTextResponse(expect.stringContaining(`### Paused on error:`));
    // [ts] expect(client.callTool({
    // [ts] name: "browser_snapshot",
    // [ts] })).toHaveResponse({
    // [ts] inlineSnapshot: expect.stringContaining(`- button "Submit" [ref=e2]`),
    // [ts] });
    // [ts] });
}
