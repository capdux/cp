// Migrated from TypeScript (mcp/cli-json.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";

const SNAPSHOT_FILE = expect.stringMatching("^\\.playwright-cli[\\\\/]page-[\\dTZ:.-]+\\.yml$");
























fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("output is pretty-printed", () => {
    // [ts] const { output } = cli("--json", "list");
    // [ts] expect: (output) => {
    // [ts] "browsers": []
    // [ts] }`);
    // [ts] });
    // [ts] test('version command returns JSON', () => {
    // [ts] const { output } = cli('--json', '--version');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect_eq(parsed, { version: expect.any(String) });
    // [ts] });
    // [ts] test('unknown command returns JSON error with exit 1', () => {
    // [ts] const { output, exitCode } = cli('--json', 'nope');
    // [ts] expect_eq(JSON.parse(output), { isError: true, error: 'Unknown command: nope' });
    // [ts] expect_eq(exitCode, 1);
    // [ts] });
    // [ts] test('unknown flag returns JSON error with exit 1', () => {
    // [ts] const { output, exitCode } = cli('--json', 'list', '--bogus');
    // [ts] expect_eq(JSON.parse(output), { isError: true, error: 'Unknown option: --bogus' });
    // [ts] expect_eq(exitCode, 1);
    // [ts] });
    // [ts] test('running tool command without open browser returns JSON error', () => {
    // [ts] const { output, exitCode } = cli('--json', 'goto', 'about:blank');
    // [ts] expect(JSON.parse(output)).toEqual({
    // [ts] isError: true,
    // [ts] error: `The browser 'default' is not open, please run open first`,
    // [ts] });
    // [ts] expect_eq(exitCode, 1);
    // [ts] });
    // [ts] test('list returns empty browsers before open', () => {
    // [ts] const { output } = cli('--json', 'list');
    // [ts] expect_eq(JSON.parse(output), { browsers: [] });
    // [ts] });
    // [ts] test('close returns not-open status when nothing is running', () => {
    // [ts] const { output } = cli('--json', 'close');
    // [ts] expect_eq(JSON.parse(output), { session: 'default', status: 'not-open' });
    // [ts] });
    // [ts] test('delete-data returns not-deleted when there is no session', () => {
    // [ts] const { output } = cli('--json', 'delete-data');
    // [ts] expect_eq(JSON.parse(output), { session: 'default', deleted: false });
    // [ts] });
    // [ts] test('kill-all returns zero killed when filter matches nothing', () => {
    // [ts] const { output } = cli('--json', 'kill-all', { env: { PWTEST_KILL_ALL_PID_FILTER_FOR_TEST: '0' } });
    // [ts] expect_eq(JSON.parse(output), { killed: 0, pids: [] });
    // [ts] });
    // [ts] test('open returns envelope with session, pid and snapshot file', () => {
    // [ts] const { output } = cli('--json', 'open', server.HELLO_WORLD);
    // [ts] expect(JSON.parse(output)).toEqual({
    // [ts] session: 'default',
    // [ts] pid: expect.any(Number),
    // [ts] result: { snapshot: { file: SNAPSHOT_FILE } },
    // [ts] });
    // [ts] });
    // [ts] test('list after open returns one browser entry', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] const { output } = cli('--json', 'list');
    // [ts] expect(JSON.parse(output)).toEqual({
    // [ts] browsers: [{
    // [ts] name: 'default',
    // [ts] workspace: expect.any(String),
    // [ts] status: 'open',
    // [ts] browserType: mcpBrowserNormalized,
    // [ts] userDataDir: null,
    // [ts] headed: false,
    // [ts] persistent: false,
    // [ts] attached: false,
    // [ts] compatible: true,
    // [ts] version: expect.any(String),
    // [ts] }],
    // [ts] });
    // [ts] });
    // [ts] test('goto returns snapshot file envelope', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] const { output } = cli('--json', 'goto', server.HELLO_WORLD);
    // [ts] expect_eq(JSON.parse(output), { snapshot: { file: SNAPSHOT_FILE } });
    // [ts] });
    // [ts] test('eval returns result string', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] const { output } = cli('--json', 'eval', '() => 1 + 2');
    // [ts] expect_eq(JSON.parse(output), { result: '3' });
    // [ts] });
    // [ts] test('eval with error surfaces isError JSON', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] const { output } = cli('--json', 'eval', '() => { throw new Error("boom"); }');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect_eq(parsed.isError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, parsed.error, 'boom') != null, true);
    // [ts] });
    // [ts] test('tab-new creates a new tab and returns tab list', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] const { output } = cli('--json', 'tab-new');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect(parsed.result.split('\n')).toEqual([
    // [ts] `- 0: [Title](${server.HELLO_WORLD})`,
    // [ts] '- 1: (current) [](about:blank)',
    // [ts] ]);
    // [ts] });
    // [ts] test('tab-list lists all tabs', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] cli('tab-new');
    // [ts] const { output } = cli('--json', 'tab-list');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect(parsed.result.split('\n')).toEqual([
    // [ts] `- 0: [Title](${server.HELLO_WORLD})`,
    // [ts] '- 1: (current) [](about:blank)',
    // [ts] ]);
    // [ts] });
    // [ts] test('tab-close closes a tab and returns remaining tabs', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] cli('tab-new');
    // [ts] const { output } = cli('--json', 'tab-close');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect(parsed.result.split('\n')).toEqual([
    // [ts] `- 0: (current) [Title](${server.HELLO_WORLD})`,
    // [ts] ]);
    // [ts] });
    // [ts] test('snapshot returns structured inline snapshot', () => {
    // [ts] server.setContent('/', '<h1>Hi</h1>', 'text/html');
    // [ts] cli('open', server.PREFIX);
    // [ts] const { output } = cli('--json', 'snapshot');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect(parsed.snapshot).toEqual([
    // [ts] { role: 'heading', name: 'Hi', level: 1, ref: expect.stringMatching(/^e\d+$/) },
    // [ts] ]);
    // [ts] });
    // [ts] test('snapshot returns structured children and flags', () => {
    // [ts] server.setContent('/', `
    // [ts] <a style="cursor: pointer" href="https://example.com/">Link</a>
    // [ts] <button disabled>Click</button>
    // [ts] `, 'text/html');
    // [ts] cli('open', server.PREFIX);
    // [ts] const { output } = cli('--json', 'snapshot');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect(parsed.snapshot).toEqual([
    // [ts] {
    // [ts] role: 'generic',
    // [ts] active: true,
    // [ts] ref: expect.stringMatching(/^e\d+$/),
    // [ts] children: [
    // [ts] {
    // [ts] role: 'link',
    // [ts] name: 'Link',
    // [ts] url: 'https://example.com/',
    // [ts] ref: expect.stringMatching(/^e\d+$/),
    // [ts] cursor: 'pointer',
    // [ts] },
    // [ts] { role: 'button', name: 'Click', disabled: true, ref: expect.stringMatching(/^e\d+$/) },
    // [ts] ],
    // [ts] },
    // [ts] ]);
    // [ts] });
    // [ts] test('tool error on bad navigation returns JSON error', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] const { output } = cli('--json', 'goto', 'https://not-a-real-domain.example.invalid');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect_eq(parsed.isError, true);
    // [ts] expect_eq(typeof parsed.error, 'string');
    // [ts] });
    // [ts] test('close after open returns closed status', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] const { output } = cli('--json', 'close');
    // [ts] expect_eq(JSON.parse(output), { session: 'default', status: 'closed' });
    // [ts] });
    // [ts] test('close-all after open returns closed sessions', () => {
    // [ts] cli('open', server.HELLO_WORLD);
    // [ts] const { output } = cli('--json', 'close-all');
    // [ts] expect_eq(JSON.parse(output), { closed: ['default'] });
    // [ts] });
    // [ts] test('requests returns numbered list as JSON result', () => {
    // [ts] cli('open', server.PREFIX);
    // [ts] cli('eval', '() => fetch("/hello-world")');
    // [ts] const { output } = cli('--json', 'requests');
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect_eq(typeof parsed.result, 'string');
    // [ts] expect(parsed.result).toMatch(new RegExp(String.raw`^\d+\. \[GET\] [^ ]+/hello-world => \[200\] OK$`, 'm'));
    // [ts] });
    // [ts] test('request and per-part commands return JSON result', () => {
    // [ts] server.setContent('/', `
    // [ts] <button onclick="fetch('/api', { method: 'POST', headers: { 'X-Custom-Header': 'test-value' }, body: JSON.stringify({ key: 'value' }) })">Click me</button>
    // [ts] `, 'text/html');
    // [ts] server.setRoute('/api', (_req, res) => {
    // [ts] res.setHeader('X-Custom-Response', 'response-value');
    // [ts] res.setHeader('Content-Type', 'application/json');
    // [ts] res.end(JSON.stringify({ name: 'John Doe' }));
    // [ts] });
    // [ts] cli('open', server.PREFIX);
    // [ts] cli('click', 'e2');
    // [ts] const { output: list } = cli('requests');
    // [ts] const num = list.match(/^(\d+)\. \[POST\] [^ ]+\/api =>/m)![1];
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.parse((cli('--json', 'request-headers', num)).output).result, 'x-custom-header: test-value') != null, true);
    // [ts] expect_eq(JSON.parse((cli('--json', 'request-body', num)).output), { result: '{"key":"value"}' });
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.parse((cli('--json', 'response-headers', num)).output).result, 'x-custom-response: response-value') != null, true);
    // [ts] expect_eq(JSON.parse((cli('--json', 'response-body', num)).output), { result: '{"name":"John Doe"}' });
    // [ts] const detail = JSON.parse((cli('--json', 'request', num)).output);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, `#${num} [POST] ${server.PREFIX}/api`) != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, detail.result, `Run \`response-body ${num}\` to read the response body.`) != null, true);
    // [ts] });
    // [ts] test("request with out-of-range index returns JSON error", () => {
    // [ts] cli("open", server.PREFIX);
    // [ts] const { output } = cli("--json", "request", "999");
    // [ts] const parsed = JSON.parse(output);
    // [ts] expect_eq(parsed.isError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, parsed.error, "Request #999 not found") != null, true);
    // [ts] });
}
