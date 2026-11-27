// Migrated from TypeScript (playwright-test/update-aria-snapshot.spec.ts) via cap from -ts

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

import * as fs from "cap:fs";
import { test, expect, stripAnsi } from "./playwright-test-fixtures.cp";
import { execSync } from "cap:process";

test.describe.configure({ mode: "parallel" });

fn trimPatch(patch: string) {
  return patch.split("\n").map(line => line.trimEnd()).join("\n");
}






// TODO(cap-migrate): refine TS statement
// [ts] \\ No newline at end of file
// [ts] `);
// [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
// [ts] const result2 = runInlineTest({});
// [ts] expect_eq(result2.exitCode, 0);
// [ts] });
// [ts] \\ No newline at end of file
// [ts] `);

  execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
  const result2 = runInlineTest({});
  expect_eq(result2.exitCode, 0);
// TODO(cap-migrate): refine TS statement
// [ts] });



fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should update snapshot with the update-snapshots flag with multiple projects", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "playwright.config.ts": `
    // [ts] export default { projects: [{ name: 'p1' }, { name: 'p2' }] };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1><h2>bye</h2>\`);
    // [ts] expect(page).toMatchAriaSnapshot(\`
    // [ts] - heading "world"
    // [ts] \`);
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -3,7 +3,8 @@
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1><h2>bye</h2>\`);
    // [ts] expect(page).toMatchAriaSnapshot(\`
    // [ts] -          - heading "world"
    // [ts] +          - heading "hello" [level=1]
    // [ts] +          - heading "bye" [level=2]
    // [ts] \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] `);
    // [ts] expect(stripAnsi(result.output).replace("\\\\", "/")).toContain(`New baselines created for:
    // [ts] a.spec.ts
    // [ts] git apply test-results/rebaselines.patch
    // [ts] `);
    // [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [ts] test("should update missing snapshots", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: A snapshot is not provided, generating new baseline.") != null, true);
    // [ts] expect(stripAnsi(result.output).replace("\\\\", "/")).toContain(`New baselines created for:
    // [ts] a.spec.ts
    // [ts] git apply test-results/rebaselines.patch
    // [ts] `);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -2,6 +2,8 @@
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] -        expect(page).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page).toMatchAriaSnapshot(\`
    // [ts] +          - heading "hello" [level=1]
    // [ts] +        \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] `);
    // [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [ts] test("should pass while updating missing snapshots with update-snapshots=missing", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "missing" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(result.passed, 1);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -2,6 +2,8 @@
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] -        expect(page).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page).toMatchAriaSnapshot(\`
    // [ts] +          - heading "hello" [level=1]
    // [ts] +        \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] `);
    // [ts] });
    // [ts] test("should update multiple missing snapshots", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page).toMatchAriaSnapshot(\`\`);
    // [ts] expect(page).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect(stripAnsi(result.output).replace("\\\\", "/")).toContain(`New baselines created for:
    // [ts] a.spec.ts
    // [ts] git apply test-results/rebaselines.patch
    // [ts] `);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -2,7 +2,11 @@
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] -        expect(page).toMatchAriaSnapshot(\`\`);
    // [ts] -        expect(page).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page).toMatchAriaSnapshot(\`
    // [ts] +          - heading "hello" [level=1]
    // [ts] +        \`);
    // [ts] +        expect(page).toMatchAriaSnapshot(\`
    // [ts] +          - heading "hello" [level=1]
    // [ts] +        \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] `);
    // [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [ts] test("should generate baseline with regex", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<ul>
    // [ts] <li>Item 1</li>
    // [ts] <li>Item 2</li>
    // [ts] <li>Time 15:30</li>
    // [ts] <li>Year 2022</li>
    // [ts] <li>Duration 12ms</li>
    // [ts] <li>22,333</li>
    // [ts] <li>2,333.79</li>
    // [ts] <li>Total 22</li>
    // [ts] <li>/Regex 1/</li>
    // [ts] <li>/Regex 22ms/</li>
    // [ts] </ul>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -13,6 +13,18 @@
    // [ts] <li>/Regex 1/</li>
    // [ts] <li>/Regex 22ms/</li>
    // [ts] </ul>\`);
    // [ts] -        expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] +          - list:
    // [ts] +            - listitem: Item 1
    // [ts] +            - listitem: Item 2
    // [ts] +            - listitem: /Time \\\\d+:\\\\d+/
    // [ts] +            - listitem: /Year \\\\d+/
    // [ts] +            - listitem: /Duration \\\\d+[hmsp]+/
    // [ts] +            - listitem: /\\\\d+,\\\\d+/
    // [ts] +            - listitem: /\\\\d+,\\\\d+\\\\.\\\\d+/
    // [ts] +            - listitem: /Total \\\\d+/
    // [ts] +            - listitem: /Regex 1/
    // [ts] +            - listitem: /\\\\/Regex \\\\d+[hmsp]+\\\\//
    // [ts] +        \`);
    // [ts] });
    // [ts] test("should generate baseline with special characters", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<ul>
    // [ts] <details>
    // [ts] <summary>one: <a href="#">link1</a> "two <a href="#">link2</a> 'three <a href="#">link3</a> \\\`four</summary>
    // [ts] </details>
    // [ts] <h1>heading "name" [level=1]</h1>
    // [ts] <button>Click: me</button>
    // [ts] <button>Click: 123</button>
    // [ts] <button>Click ' me</button>
    // [ts] <button>Click: ' me</button>
    // [ts] <button>Click " me</button>
    // [ts] <button>Click " me 123</button>
    // [ts] <button>Click \\\\ me</button>
    // [ts] <button>Click \\\\ me 123</button>
    // [ts] <li>Item: 1</li>
    // [ts] <li>Item {a: b}</li>
    // [ts] </ul>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -17,6 +17,30 @@
    // [ts] <li>Item: 1</li>
    // [ts] <li>Item {a: b}</li>
    // [ts] </ul>\`);
    // [ts] -        expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] +          - list:
    // [ts] +            - group:
    // [ts] +              - text: "one:"
    // [ts] +              - link "link1":
    // [ts] +                - /url: "#"
    // [ts] +              - text: "\\\\\"two"
    // [ts] +              - link "link2":
    // [ts] +                - /url: "#"
    // [ts] +              - text: "'three"
    // [ts] +              - link "link3":
    // [ts] +                - /url: "#"
    // [ts] +              - text: "\\\`four"
    // [ts] +            - heading "heading \\\\"name\\\\" [level=1]" [level=1]
    // [ts] +            - 'button "Click: me"'
    // [ts] +            - 'button /Click: \\\\d+/'
    // [ts] +            - button "Click ' me"
    // [ts] +            - 'button "Click: '' me"'
    // [ts] +            - button "Click \\\\" me"
    // [ts] +            - button /Click " me \\\\d+/
    // [ts] +            - button "Click \\\\\\\\ me"
    // [ts] +            - button /Click \\\\\\\\ me \\\\d+/
    // [ts] +            - listitem: \"Item: 1\"
    // [ts] +            - listitem: \"Item {a: b}\"
    // [ts] +        \`);
    // [ts] });
    // [ts] test("should update missing snapshots in tsx", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "src/button.test.tsx": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass', () => {
    // [ts] page.setContent(\`<button>Button</button>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/src/button.test.tsx b/src/button.test.tsx
    // [ts] --- a/src/button.test.tsx
    // [ts] +++ b/src/button.test.tsx
    // [ts] @@ -3,6 +3,8 @@
    // [ts] test('pass', () => {
    // [ts] page.setContent(\`<button>Button</button>\`);
    // [ts] -        expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] +          - button \"Button\"
    // [ts] +        \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] `);
    // [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [ts] test("should update multiple files", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "src/button-1.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass 1', () => {
    // [ts] page.setContent(\`<button>Button</button>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `,
    // [ts] "src/button-2.test.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('pass 2', () => {
    // [ts] page.setContent(\`<button>Button</button>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `,
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect(stripAnsi(result.output).replace("\\\\", "/")).toContain(`New baselines created for:
    // [ts] src/button-1.test.ts
    // [ts] src/button-2.test.ts
    // [ts] git apply test-results/rebaselines.patch
    // [ts] `);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/src/button-1.test.ts b/src/button-1.test.ts
    // [ts] --- a/src/button-1.test.ts
    // [ts] +++ b/src/button-1.test.ts
    // [ts] @@ -3,6 +3,8 @@
    // [ts] test('pass 1', () => {
    // [ts] page.setContent(\`<button>Button</button>\`);
    // [ts] -        expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] +          - button \"Button\"
    // [ts] +        \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] diff --git a/src/button-2.test.ts b/src/button-2.test.ts
    // [ts] --- a/src/button-2.test.ts
    // [ts] +++ b/src/button-2.test.ts
    // [ts] @@ -3,6 +3,8 @@
    // [ts] test('pass 2', () => {
    // [ts] page.setContent(\`<button>Button</button>\`);
    // [ts] -        expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] +          - button \"Button\"
    // [ts] +        \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] `);
    // [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [ts] test("should generate baseline for input values", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<input value="hello world">\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -2,6 +2,8 @@
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<input value="hello world">\`);
    // [ts] -        expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] +        expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] +          - textbox: hello world
    // [ts] +        \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] `);
    // [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [ts] test("should update when options are specified", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<input value="hello world">\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`\`, { timeout: 2500 });
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot('',
    // [ts] {
    // [ts] timeout: 2500
    // [ts] });
    // [ts] });
    // [ts] `
    // [ts] });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -2,8 +2,12 @@
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<input value="hello world">\`);
    // [ts] -        expect(page.locator('body')).toMatchAriaSnapshot(\`\`, { timeout: 2500 });
    // [ts] -        expect(page.locator('body')).toMatchAriaSnapshot('',
    // [ts] +        expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] +          - textbox: hello world
    // [ts] +        \`, { timeout: 2500 });
    // [ts] +        expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] +          - textbox: hello world
    // [ts] +        \`,
    // [ts] {
    // [ts] timeout: 2500
    // [ts] });
    // [ts] `);
    // [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [ts] test("should not update snapshots when locator did not match", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent('<h1>hello</h1>');
    // [ts] expect(page.locator('div')).toMatchAriaSnapshot('- heading', { timeout: 3000 });
    // [ts] });
    // [ts] `,
    // [ts] }, { "update-snapshots": true });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] expect_eq(fs.existsSync(patchPath), false);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "New baselines created") == null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Expected: \"- heading\"") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Error: element(s) not found") != null, true);
    // [ts] });
    // [cap-test] test.describe("update-snapshots none", () => {
    // [ts] test("should create new baseline for matching snapshot", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1><h1>world</h1>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`\`);
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "none" });
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] expect_eq(fs.existsSync(patchPath), false);
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe("update-snapshots all", () => {
    // [ts] test("should create new baseline for matching snapshot", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1><h1>world</h1>\`);
    // [ts] expect(page).toMatchAriaSnapshot(\`
    // [ts] - heading "hello"
    // [ts] \`);
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "all" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] const data = fs.readFileSync(patchPath, "utf-8");
    // [ts] expect(trimPatch(data)).toBe(`diff --git a/a.spec.ts b/a.spec.ts
    // [ts] --- a/a.spec.ts
    // [ts] +++ b/a.spec.ts
    // [ts] @@ -3,7 +3,8 @@
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1><h1>world</h1>\`);
    // [ts] expect(page).toMatchAriaSnapshot(\`
    // [ts] -            - heading "hello"
    // [ts] +            - heading "hello" [level=1]
    // [ts] +            - heading "world" [level=1]
    // [ts] \`);
    // [ts] });
    // [ts] \\ No newline at end of file
    // [ts] `);
    // [ts] expect(stripAnsi(result.output).replace("\\\\", "/")).toContain(`New baselines created for:
    // [ts] a.spec.ts
    // [ts] git apply test-results/rebaselines.patch
    // [ts] `);
    // [ts] execSync(`patch -p1 < ${patchPath}`, { cwd: testInfo.outputPath() });
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [cap-test] });
    // [cap-test] test.describe("update-source-method", () => {
    // [ts] test("should overwrite source", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] - heading "world"
    // [ts] \`);
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "all", "update-source-method": "overwrite" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] expect_eq(fs.existsSync(patchPath), false);
    // [ts] const data = fs.readFileSync(testInfo.outputPath("a.spec.ts"), "utf-8");
    // [ts] expect(data).toBe(`
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] - heading "hello" [level=1]
    // [ts] \`);
    // [ts] });
    // [ts] `);
    // [ts] expect(stripAnsi(result.output).replace("\\\\", "/")).toContain(`New baselines created for:
    // [ts] a.spec.ts
    // [ts] `);
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [ts] test("should 3way source", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] - heading "world"
    // [ts] \`);
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "all", "update-source-method": "3way" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] expect_eq(fs.existsSync(patchPath), false);
    // [ts] const data = fs.readFileSync(testInfo.outputPath("a.spec.ts"), "utf-8");
    // [ts] expect(data).toBe(`
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] \<<<<<<< HEAD
    // [ts] - heading "world"
    // [ts] =======
    // [ts] - heading "hello" [level=1]
    // [ts] >>>>>>> SNAPSHOT
    // [ts] \`);
    // [ts] });
    // [ts] `);
    // [ts] expect(stripAnsi(result.output).replace("\\\\", "/")).toContain(`New baselines created for:
    // [ts] a.spec.ts
    // [ts] `);
    // [ts] });
    // [ts] test("should overwrite source when specified in the config", () => {
    // [ts] const result = runInlineTest({
    // [ts] ".git/marker": "",
    // [ts] "playwright.config.ts": `
    // [ts] export default { updateSourceMethod: 'overwrite' };
    // [ts] `,
    // [ts] "a.spec.ts": `
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] - heading "world"
    // [ts] \`);
    // [ts] });
    // [ts] `
    // [ts] }, { "update-snapshots": "all" });
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] const patchPath = testInfo.outputPath("test-results/rebaselines.patch");
    // [ts] expect_eq(fs.existsSync(patchPath), false);
    // [ts] const data = fs.readFileSync(testInfo.outputPath("a.spec.ts"), "utf-8");
    // [ts] expect(data).toBe(`
    // [ts] import { test, expect } from '@playwright/test';
    // [ts] test('test', () => {
    // [ts] page.setContent(\`<h1>hello</h1>\`);
    // [ts] expect(page.locator('body')).toMatchAriaSnapshot(\`
    // [ts] - heading "hello" [level=1]
    // [ts] \`);
    // [ts] });
    // [ts] `);
    // [ts] expect(stripAnsi(result.output).replace("\\\\", "/")).toContain(`New baselines created for:
    // [ts] a.spec.ts
    // [ts] `);
    // [ts] const result2 = runInlineTest({});
    // [ts] expect_eq(result2.exitCode, 0);
    // [ts] });
    // [cap-test] });
}
