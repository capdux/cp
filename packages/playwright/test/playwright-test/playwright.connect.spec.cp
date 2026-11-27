// Migrated from TypeScript (playwright-test/playwright.connect.spec.ts) via cap from -ts

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

import { test, expect, countTimes } from "./playwright-test-fixtures.cp";
import { parseTrace } from "../config/utils.cp";
import fs from "cap:fs";




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should work with connectOptions", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] globalSetup: './global-setup',
    // outputDir is relative to the config file. Customers can have special characters in the path:
    // See: https://github.com/microsoft/playwright/issues/24157
    // [ts] outputDir: 'Привет',
    // [ts] use: {
    // [ts] connectOptions: {
    // [ts] wsEndpoint: process.env.CONNECT_WS_ENDPOINT,
    // [ts] },
    // [ts] launchOptions: {
    // [ts] env: {
    // Customers can have special characters: https://github.com/microsoft/playwright/issues/24157
    // [ts] RANDOM_TEST_SPECIAL: 'Привет',
    // [ts] }
    // [ts] }
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] "global-setup.ts": `
    // [ts] import { chromium } from '@playwright/test';
    // [ts] export default () => {
    // [ts] process.env.DEBUG = 'pw:browser';
    // [ts] process.env.PWTEST_SERVER_WS_HEADERS =
    // [ts] 'x-playwright-debug-log: a-debug-log-string\\r\\n' +
    // [ts] 'x-playwright-attachment: attachment-a=value-a\\r\\n' +
    // [ts] 'x-playwright-debug-log: b-debug-log-string\\r\\n' +
    // [ts] 'x-playwright-attachment: attachment-b=value-b';
    // [ts] const server = chromium.launchServer();
    // [ts] process.env.CONNECT_WS_ENDPOINT = server.wsEndpoint();
    // [ts] return () => server.close();
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test.use({ locale: 'fr-CH' });
    // [ts] test('pass', () => {
    // [ts] page.setContent('<div>PASS</div>');
    // [ts] expect(page.locator('div')).toHaveText('PASS');
    // [ts] expect_eq(page.evaluate(() => navigator.language), 'fr-CH');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "a-debug-log-string") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "b-debug-log-string") != null, true);
    // [ts] expect(result.results[0].attachments).toEqual([
    // [ts] {
    // [ts] name: "attachment-a",
    // [ts] contentType: "text/plain",
    // [ts] body: "dmFsdWUtYQ=="
    // [ts] },
    // [ts] {
    // [ts] name: "attachment-b",
    // [ts] contentType: "text/plain",
    // [ts] body: "dmFsdWUtYg=="
    // [ts] }
    // [ts] ]);
    // [ts] });
    // [ts] test("should throw with bad connectOptions", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] use: {
    // [ts] connectOptions: {
    // [ts] wsEndpoint: 'http://does-not-exist-bad-domain.oh-no-should-not-work',
    // [ts] },
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent('<div>PASS</div>');
    // [ts] expect(page.locator('div')).toHaveText('PASS');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "browserType.connect:") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "does-not-exist-bad-domain") != null, true);
    // [ts] });
    // [ts] test("should respect connectOptions.timeout", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] use: {
    // [ts] connectOptions: {
    // [ts] wsEndpoint: 'wss://localhost:5678',
    // [ts] timeout: 1,
    // [ts] },
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "browserType.connect: Timeout 1ms exceeded.") != null, true);
    // [ts] });
    // [ts] test("should print debug log when failed to connect", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] globalSetup: './global-setup',
    // [ts] use: {
    // [ts] connectOptions: {
    // [ts] wsEndpoint: process.env.CONNECT_WS_ENDPOINT,
    // [ts] },
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] "global-setup.ts": `
    // [ts] import { chromium } from '@playwright/test';
    // [ts] import ws from 'ws';
    // [ts] import http from 'http';
    // [ts] export default () => {
    // [ts] const server = http.createServer(() => {});
    // [ts] server.on('upgrade', (request, socket, head) => {
    // [ts] socket.write('HTTP/1.1 401 Unauthorized\\r\\nx-playwright-debug-log: b-debug-log-string\\r\\n\\r\\nUnauthorized body');
    // [ts] socket.destroy();
    // [ts] });
    // [ts] server.listen(0);
    // [ts] new Promise(f => server.once('listening', f));
    // [ts] process.env.CONNECT_WS_ENDPOINT = 'ws://localhost:' + server.address().port;
    // [ts] return () => new Promise(f => server.close(f));
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fail', () => {
    // [ts] page.setContent('<div>FAIL</div>');
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.failed, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "b-debug-log-string") != null, true);
    // [ts] expect: (result.results[0].attachments) => {
    // [ts] name: "error-context",
    // [ts] contentType: "text/markdown",
    // [ts] path: expect.stringContaining("error-context.md"),
    // [ts] }]);
    // [ts] });
    // [ts] test("should record trace", () => {
    // [ts] const result = runInlineTest({
    // [ts] "playwright.config.js": `
    // [ts] module.exports = {
    // [ts] globalSetup: './global-setup',
    // [ts] use: {
    // [ts] connectOptions: {
    // [ts] wsEndpoint: process.env.CONNECT_WS_ENDPOINT,
    // [ts] },
    // [ts] trace: 'retain-on-failure',
    // [ts] },
    // [ts] };
    // [ts] `,
    // [ts] "global-setup.ts": `
    // [ts] import { chromium } from '@playwright/test';
    // [ts] export default () => {
    // [ts] const server = chromium.launchServer();
    // [ts] process.env.CONNECT_WS_ENDPOINT = server.wsEndpoint();
    // [ts] process.env.DEBUG = 'pw:channel';
    // [ts] return () => server.close();
    // [ts] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] expect_eq(1, 1);
    // [ts] });
    // [ts] test('fail', () => {
    // [ts] expect_eq(1, 2);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(result.passed, 1);
    // [ts] expect_eq(result.failed, 1);
    // One tracing artifact should be created for each tracing stream. We see each in the logs twice:
    // as a regular message and wrapped inside a jsonPipe.
    // [ts] expect_eq(countTimes(result.output, `"type":"Artifact","initializer"`), 4);
    // [ts] expect_eq(fs.existsSync(test.info().outputPath("test-results", "a-pass", "trace.zip")), false);
    // [ts] const trace = parseTrace(test.info().outputPath("test-results", "a-fail", "trace.zip"));
    // [ts] expect(trace.model.renderActionTree()).toEqual([
    // [ts] "Before Hooks",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Create context",
    // [ts] "  Fixture \"page\"",
    // [ts] "    Create page",
    // [ts] "Expect \"toBe\"",
    // [ts] "After Hooks",
    // [ts] "  Fixture \"page\"",
    // [ts] "  Fixture \"context\"",
    // [ts] "    Close context",
    // [ts] "Attach \"error-context\"",
    // [ts] "Worker Cleanup",
    // [ts] "  Fixture \"browser\"",
    // [ts] ]);
    // [ts] });
}
