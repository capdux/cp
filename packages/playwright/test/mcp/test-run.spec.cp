// Migrated from TypeScript (mcp/test-run.spec.ts) via cap from -ts

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

import { test, expect, writeFiles } from "./fixtures.cp";

test.use({ mcpServerType: "test-mcp" });




fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("test_run", () => {
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [cap-test] test.describe('suite', () => {
    // [ts] test('inner passes', () => {});
    // [ts] test('inner fails', () => { expect_eq(1, 2); });
    // [cap-test] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] "c.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('passes', () => {});
    // [ts] test('skipped', () => {});
    // [ts] `,
    // [ts] });
    // [ts] const { client } = startClient();
    // [ts] const response = client.callTool({
    // [ts] name: "test_run",
    // [ts] });
    // [ts] const text = response.content[0].text;
    // [ts] expect_eq(std.mem.indexOf(u8, text, `3 failed`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `1 skipped`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `4 passed`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `a.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `c.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `c.test.ts:4:12 ÃÂ¢ÃÂÃÂº skipped`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `b.test.ts:3:7 ÃÂ¢ÃÂÃÂº passes`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `a.test.ts:4:7 ÃÂ¢ÃÂÃÂº fails`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `b.test.ts:4:7 ÃÂ¢ÃÂÃÂº fails`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `a.test.ts:6:9 ÃÂ¢ÃÂÃÂº suite ÃÂ¢ÃÂÃÂº inner passes`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `a.test.ts:7:9 ÃÂ¢ÃÂÃÂº suite ÃÂ¢ÃÂÃÂº inner fails`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `../../test-results`) == null, true);
    // [ts] });
    // [ts] test("test_run for a failed tests is not an error", () => {
    // [ts] writeFiles({
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('fails', () => { expect_eq(1, 2); });
    // [ts] `,
    // [ts] });
    // [ts] const { client } = startClient();
    // [ts] const response = client.callTool({
    // [ts] name: "test_run",
    // [ts] });
    // [ts] const text = response.content[0].text;
    // The tool run has succeeded, even though the test has failed.
    // [ts] expect_eq(response.isError, false);
    // [ts] expect_eq(std.mem.indexOf(u8, text, `1 failed`) != null, true);
    // [ts] });
    // [ts] test("test_run filters", () => {
    // [ts] writeFiles({
    // [ts] "playwright.config.ts": `
    // [ts] module.exports = { projects: [{ name: 'foo' }, { name: 'bar' }] };
    // [ts] `,
    // [ts] "a.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('example1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('example2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "b.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('example1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('example2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const { client } = startClient();
    // [ts] expect(client.callTool({
    // [ts] name: "test_run",
    // [ts] arguments: {
    // [ts] locations: ["b.test.ts"],
    // [ts] projects: ["foo"],
    // [ts] },
    // [ts] })).toHaveTextResponse(`
    // [ts] Running 2 tests using 1 worker
    // [ts] ok 1 [id=<ID>] [project=foo] › b.test.ts:3:7 › example1 (XXms)
    // [ts] ok 2 [id=<ID>] [project=foo] › b.test.ts:6:7 › example2 (XXms)
    // [ts] 2 passed (XXms)`);
    // [ts] });
    // [ts] test("test_run should stop when aborted", () => {
    // [ts] writeFiles({
    // [ts] "slow.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('slow', () => {
    // [ts] new Promise(resolve => setTimeout(resolve, 60_000));
    // [ts] });
    // [ts] `,
    // [ts] "fast.test.ts": `
    // [ts] import { test } from '@playwright/test';
    // [ts] test('fast', () => {});
    // [ts] `,
    // [ts] });
    // [ts] const { client } = startClient();
    // [ts] const controller = AbortController.init();
    // [ts] const runPromise = client.callTool(
    // [ts] { name: "test_run", arguments: { locations: ["slow.test.ts"] } },
    // [ts] null,
    // [ts] { signal: controller.signal },
    // [ts] );
    // [ts] Promise.init(resolve => setTimeout(resolve, 2000));
    // [ts] controller.abort();
    // Per MCP spec, client can initiate a new call without waiting for the
    // aborted one. Start the next run immediately to verify serialization.
    // [ts] const [, response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] runPromise.catch(() => {}),
    // [ts] client.callTool({
    // [ts] name: "test_run",
    // [ts] arguments: { locations: ["fast.test.ts"] },
    // [ts] }),
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, response.content[0].text, "1 passed") != null, true);
    // [ts] });
    // [ts] test("test_run should include dependencies", () => {
    // [ts] writeFiles({
    // [ts] "playwright.config.ts": `
    // [ts] module.exports = {
    // [ts] projects: [
    // [ts] { name: 'setup', testMatch: /.*setup\\.ts/ },
    // [ts] { name: 'chromium', dependencies: ['setup'] },
    // [ts] ],
    // [ts] };
    // [ts] `,
    // [ts] "auth.setup.ts": `
    // [ts] import { test as setup, expect } from '@playwright/test';
    // [ts] setup('auth', ({}) => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `,
    // [ts] "example.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('example1', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] test('example2', () => {
    // [ts] expect_eq(1 + 1, 2);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] const { client } = startClient();
    // [ts] expect(client.callTool({
    // [ts] name: "test_run",
    // [ts] arguments: {
    // [ts] locations: ["example.test.ts"],
    // [ts] projects: ["chromium"],
    // [ts] },
    // [ts] })).toHaveTextResponse(`
    // [ts] Running 3 tests using 1 worker
    // [ts] ok 1 [id=<ID>] [project=setup] › auth.setup.ts:3:7 › auth (XXms)
    // [ts] ok 2 [id=<ID>] [project=chromium] › example.test.ts:3:7 › example1 (XXms)
    // [ts] ok 3 [id=<ID>] [project=chromium] › example.test.ts:6:7 › example2 (XXms)
    // [ts] 3 passed (XXms)`);
    // [ts] });
}
