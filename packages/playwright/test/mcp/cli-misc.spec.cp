// Migrated from TypeScript (mcp/cli-misc.spec.ts) via cap from -ts

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

import fs from "cap:fs";
import path from "cap:path";
import { test, expect } from "./cli-fixtures.cp";












fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("daemon shuts down on browser launch failure", () => {
    // [ts] const first = cli("open", server.PREFIX, { env: { PLAYWRIGHT_MCP_EXECUTABLE_PATH: "/nonexistent/browser/path" } });
    // [ts] expect_eq(std.mem.indexOf(u8, first.error, `executable doesn't exist`) != null, true);
    // [ts] const second = cli("open", server.PREFIX);
    // [ts] expect_eq(second.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, second.output, "Page URL") != null, true);
    // [ts] });
    // [ts] test("install-browser", () => {
    // [ts] test(mcpBrowser != "chromium", "Test only chromium");
    // [ts] const { output } = cli("install-browser", "--list");
    // [ts] expect(output).toMatch("chromium-(tot-)?\\d+");
    // [ts] });
    // [ts] test("install-browser --no-shell", () => {
    // [ts] const withShell = cli("install-browser", "chromium", "--dry-run");
    // [ts] expect_eq(withShell.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, withShell.output, "chromium-headless-shell") != null, true);
    // [ts] const noShell = cli("install-browser", "chromium", "--no-shell", "--dry-run");
    // [ts] expect_eq(noShell.exitCode, 0);
    // [ts] expect(noShell.output).toMatch("chromium v(tot-)?\\d+");
    // [ts] expect_eq(std.mem.indexOf(u8, noShell.output, "chromium-headless-shell") == null, true);
    // [ts] });
    // [ts] test("install workspace", () => {
    // [ts] const { output } = cli("install");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Workspace initialized at`) != null, true);
    // [ts] const playwrightDir = testInfo.outputPath(".playwright");
    // [ts] expect_eq(fs.existsSync(playwrightDir), true);
    // [ts] });
    // [ts] test("install adds .playwright-cli/ to .gitignore", () => {
    // [ts] const outsideGitRepo = cli("install");
    // [ts] expect_eq(std.mem.indexOf(u8, outsideGitRepo.output, ".gitignore") == null, true);
    // [ts] expect_eq(fs.existsSync(testInfo.outputPath(".gitignore")), false);
    // [ts] fs.promises.mkdir(testInfo.outputPath(".git"), { recursive: true });
    // [ts] fs.promises.writeFile(testInfo.outputPath(".gitignore"), "node_modules/");
    // [ts] const insideGitRepo = cli("install");
    // [ts] expect_eq(std.mem.indexOf(u8, insideGitRepo.output, "Added `.playwright-cli/` to `.gitignore`.") != null, true);
    // [ts] const expectedContent = "node_modules/\n# Playwright CLI output (may contain credentials)\n.playwright-cli/\n";
    // [ts] expect_eq(fs.promises.readFile(testInfo.outputPath(".gitignore"), "utf8"), expectedContent);
    // [ts] const secondRun = cli("install");
    // [ts] expect_eq(std.mem.indexOf(u8, secondRun.output, ".gitignore") == null, true);
    // [ts] expect_eq(fs.promises.readFile(testInfo.outputPath(".gitignore"), "utf8"), expectedContent);
    // [ts] });
    // [ts] test("install workspace w/skills", () => {
    // [ts] const { output } = cli("install", "--skills");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Skill installed to \`.claude${path.sep}skills${path.sep}playwright-cli\`.`) != null, true);
    // [ts] const skillFile = testInfo.outputPath(".claude", "skills", "playwright-cli", "SKILL.md");
    // [ts] expect_eq(fs.existsSync(skillFile), true);
    // [ts] const skillContents = fs.readFileSync(skillFile, "utf-8");
    // [ts] expect(skillContents.match("^allowed-tools: .*")).toEqual([
    // [ts] "allowed-tools: Bash(playwright-cli:*) Bash(npx playwright:*) Bash(npx --no-install playwright:*)",
    // [ts] ]);
    // [ts] const referencesDir = testInfo.outputPath(".claude", "skills", "playwright-cli", "references");
    // [ts] const references = fs.promises.readdir(referencesDir);
    // [ts] expect_eq(references.length > 0, true);
    // [ts] });
    // [ts] test("install workspace w/--skills=agents", () => {
    // [ts] const { output } = cli("install", "--skills=agents");
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Skill installed to \`.agents${path.sep}skills${path.sep}playwright-cli\`.`) != null, true);
    // [ts] const skillFile = testInfo.outputPath(".agents", "skills", "playwright-cli", "SKILL.md");
    // [ts] expect_eq(fs.existsSync(skillFile), true);
    // [ts] });
    // [ts] test("install w/--skills -g installs into the home directory", () => {
    // [ts] const fakeHome = testInfo.outputPath("fake-home");
    // [ts] fs.promises.mkdir(fakeHome, { recursive: true });
    // [ts] const { output } = cli("install", "--skills", "-g", { env: { HOME: fakeHome, USERPROFILE: fakeHome } });
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Skill installed to") != null, true);
    // [ts] expect_eq(std.mem.indexOf(u8, output, "Workspace initialized") == null, true);
    // [ts] const skillFile = path.join(fakeHome, ".claude", "skills", "playwright-cli", "SKILL.md");
    // [ts] expect_eq(fs.existsSync(skillFile), true);
    // [ts] });
    // [ts] test("install w/--skills=agents --global installs into the home directory", () => {
    // [ts] const fakeHome = testInfo.outputPath("fake-home");
    // [ts] fs.promises.mkdir(fakeHome, { recursive: true });
    // [ts] cli("install", "--skills=agents", "--global", { env: { HOME: fakeHome, USERPROFILE: fakeHome } });
    // [ts] const skillFile = path.join(fakeHome, ".agents", "skills", "playwright-cli", "SKILL.md");
    // [ts] expect_eq(fs.existsSync(skillFile), true);
    // [ts] });
    // [ts] test("install -g without --skills errors", () => {
    // [ts] const result = cli("install", "-g");
    // [ts] expect_eq(result.exitCode, 1);
    // [ts] expect_eq(std.mem.indexOf(u8, result.error, "--global requires --skills") != null, true);
    // [ts] });
    // [ts] test("install handles browser detection", () => {
    // [ts] const { output } = cli("install");
    // Verify that one of the browser detection outcomes occurred
    // [ts] const foundMatch = output.match("Found ((?:chrome|msedge)[\\w-]*), will use it as the default browser\\.");
    // [ts] if (foundMatch?.[1] != "chrome")
    // [ts] expect_eq(std.mem.indexOf(u8, output, `Created default config for ${foundMatch?.[1] ?? "chromium"}.`) != null, true);
    // [ts] });
    // [ts] test("open with very long session name (issue 40878)", () => {
    // Long session names push the unix socket path past sun_path's 104-byte limit on macOS.
    // [ts] const longSessionName = "awesome-coding-agent-orchestrators-with-an-overlong-suffix-for-testing";
    // [ts] const result = cli(`-s=${longSessionName}`, "open", server.PREFIX);
    // [ts] expect_eq(result.error, "");
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Page URL") != null, true);
    // [ts] });
    // [ts] test("open with long multi-byte session name (issue 42153)", () => {
    // [ts] const result = cli("-s=ÃÂ£ÃÂÃÂ»ÃÂ£ÃÂÃÂÃÂ£ÃÂÃÂ·ÃÂ£ÃÂÃÂ§ÃÂ£ÃÂÃÂ³ÃÂ¥ÃÂÃÂÃÂ£ÃÂÃÂÃÂ£ÃÂÃÂ¨ÃÂ£ÃÂÃÂ¦ÃÂ£ÃÂÃÂÃÂ©ÃÂÃÂ·ÃÂ£ÃÂÃÂÃÂ¥ÃÂ ÃÂ´ÃÂ¥ÃÂÃÂÃÂ£ÃÂÃÂ®ÃÂ¥ÃÂÃÂÃÂ¤ÃÂ½ÃÂÃÂ£ÃÂÃÂÃÂ§ÃÂ¢ÃÂºÃÂ¨ÃÂªÃÂÃÂ£ÃÂÃÂÃÂ£ÃÂÃÂÃÂ£ÃÂÃÂÃÂ£ÃÂÃÂÃÂ£ÃÂÃÂ®ÃÂ£ÃÂÃÂÃÂ£ÃÂÃÂ¹ÃÂ£ÃÂÃÂ", "open", server.PREFIX);
    // [ts] expect_eq(result.error, "");
    // [ts] expect_eq(result.exitCode, 0);
    // [ts] expect_eq(std.mem.indexOf(u8, result.output, "Page URL") != null, true);
    // [ts] });
}
