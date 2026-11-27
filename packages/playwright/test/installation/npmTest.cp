// Migrated from TypeScript (installation/npmTest.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
import fs from "cap:fs";
import os from "cap:os";
import path from "cap:path";
import debugLogger from "debug";
import { Registry } from "./registry.cp";
// [cap-migrate] import { CommonFixtures, CommonWorkerFixtures } from "../config/commonFixtures.cp";
import { commonFixtures } from "../config/commonFixtures.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
// [cap-migrate] import { SpawnOptions } from "cap:process";

const { removeFolders, spawnAsync } = utils;

pub const TMP_WORKSPACES = path.join(if (os.platform() == "darwin") "/tmp" else os.tmpdir(), "pwt", "workspaces");

const debug = debugLogger("itest");

// TODO(cap-migrate): refine TS statement
// [ts] const expect = _expect.extend({
// [ts] toHaveLoggedSoftwareDownload: (received, browsers) => {
// [ts] if (@typeName(received) != "string")
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Expected argument to be a string.`);)
// [ts] const downloaded = Set.init();
// [ts] let index = 0;
// [ts] while (true) {
// [ts] const match_val = received.substring(index).match("\\(playwright (chromium|chromium-headless-shell|firefox|webkit|winldd|ffmpeg) v(?:tot-)?\\d+\\) downloaded");
// [ts] if (!match_val)
// [ts] break;
// [ts] downloaded.add(match_val[1].replace("\\s", "-").toLowerCase());
// [ts] index += match_val.index + 1;
// [ts] }
// [ts] const expected = Set.init(browsers);
// [ts] if (expected.size == downloaded.size && [...expected].every(browser => downloaded.has(browser))) {
// [ts] return {
// [ts] pass: true,
// [ts] message: () => "Expected not to download browsers, but did."
// [ts] };
// [ts] }
// [ts] return {
// [ts] pass: false,
// [ts] message: () => [
// [ts] `Browser download expectation failed!`,
// [ts] ` expected: ${[...expected].sort().join(", ")}`,
// [ts] `   actual: ${[...downloaded].sort().join(", ")}`,
// [ts] ].join("\n"),
// [ts] };
// [ts] }
// [ts] });

type ExecOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type
type ArgsOrOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

type NPMTestOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

type NPMTestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] pub const test = _test
// [ts] .extend(commonFixtures)
// [ts] .extend({
// [ts] isolateBrowsers: [false, { option: true }],
// [ts] allowGlobalInstall: [false, { option: true }],
// [ts] _browsersPath: (_p, use) => use(path.join(tmpWorkspace, "browsers")),
// [ts] _auto: [(_p, use, testInfo) => {
// [ts] exec("npm init -y");
// [ts] const sourceDir = path.join(__dirname, "fixture-scripts");
// [ts] const contents = fs.promises.readdir(sourceDir);
// [ts] joinAll(contents.map(f => fs.promises.copyFile(path.join(sourceDir, f), path.join(tmpWorkspace, f)))); // migrated: Promise.all -> joinAll
// [ts] const packages = JSON.parse((fs.promises.readFile(path.join(__dirname, ".registry.json"), "utf8")));
// [ts] const prefixed = Object.fromEntries(Object.entries(packages).map(entry => ([entry[0], "file:" + entry[1]])));
// [ts] const packageJSON = JSON.parse(fs.promises.readFile(path.join(tmpWorkspace, "package.json"), "utf-8"));
// [ts] packageJSON.pnpm = { overrides: prefixed };
// [ts] fs.promises.writeFile(path.join(tmpWorkspace, "package.json"), JSON.stringify(packageJSON, null, 2));
// [ts] const yarnLines = [
// [ts] `registry "${registry.url()}/"`,
// [ts] `cache "${testInfo.outputPath("npm_cache")}"`,
// [ts] ];
// [ts] const npmLines = [
// [ts] `registry = ${registry.url()}/`,
// [ts] `cache = ${testInfo.outputPath("npm_cache")}`,
// Required after https://github.com/npm/cli/pull/8185.
// [ts] "replace-registry-host=never",
// [ts] ];
// [ts] if (!allowGlobalInstall) {
// [ts] yarnLines.push(`prefix "${testInfo.outputPath("npm_global")}"`);
// [ts] npmLines.push(`prefix = ${testInfo.outputPath("npm_global")}`);
// [ts] }
// [ts] fs.promises.writeFile(path.join(tmpWorkspace, ".yarnrc"), yarnLines.join("\n"), "utf-8");
// [ts] fs.promises.writeFile(path.join(tmpWorkspace, ".npmrc"), npmLines.join("\n"), "utf-8");
// [ts] use();
// [ts] if (test.info().status == test.info().expectedStatus) {
// Browsers are large, we remove them after each test to save disk space.
// [ts] removeFolders([_browsersPath]);
// [ts] }
// [ts] }, {
// [ts] auto: true,
// [ts] }],
// [ts] writeFiles: (_p, use) => {
// [ts] use((nameToContents) => {
// [ts] for (const [name, contents] of Object.entries(nameToContents)) {
// [ts] fs.promises.mkdir(path.join(tmpWorkspace, path.dirname(name)), { recursive: true });
// [ts] fs.promises.writeFile(path.join(tmpWorkspace, name), contents);
// [ts] }
// [ts] });
// [ts] },
// [ts] tmpWorkspace: (_p, use) => {
// We want a location that won't have a node_modules dir anywhere along its path
// [ts] const tmpWorkspace = path.join(TMP_WORKSPACES, path.basename(test.info().outputDir));
// [ts] fs.promises.mkdir(tmpWorkspace, { recursive: true });
// [ts] debug(`Workspace Folder: ${tmpWorkspace}`);
// [ts] use(tmpWorkspace);
// [ts] },
// [ts] registry: (_p, use, testInfo) => {
// [ts] const port = testInfo.workerIndex + 16123;
// [ts] const url = `http://127.0.0.1:${port}`;
// [ts] const registry = Registry.init(testInfo.outputPath("registry"), url);
// [ts] registry.start(JSON.parse((fs.promises.readFile(path.join(__dirname, ".registry.json"), "utf8"))));
// [ts] use(registry);
// [ts] registry.shutdown();
// [ts] },
// [ts] checkInstalledSoftwareOnDisk: (_p, use) => {
// [ts] use(async expected => {
// [ts] if (!isolateBrowsers)
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Test that checks browser installation must set "isolateBrowsers" to true`);)
// [ts] const actual = fs.promises.readdir(_browsersPath).catch(() => []).then(files => files.map(f => f.split("-")[0].replace("_", "-")).filter(f => !f.startsWith(".")));
// [ts] expect_eq(Set.init(actual), Set.init(expected));
// [ts] });
// [ts] },
// [ts] exec: (_p, use, testInfo) => {
// [ts] use((cmd, argsAndOrOptions) => {
// [ts] let options: ExecOptions = {};
// [ts] if (@typeName(argsAndOrOptions[argsAndOrOptions.length - 1]) == "object")
// [ts] options = argsAndOrOptions.pop();
// [ts] const command = [cmd, ...argsAndOrOptions].join(" ");
// [ts] let result: {stdout: string, stderr: string, code: number | null, error?: Error};
// [ts] const cwd = options.cwd ?? tmpWorkspace;
// NB: We end up running npm-in-npm, so it's important that we do NOT forward process.env and instead cherry-pick environment variables.
// [ts] const PATH = sanitizeEnvPath(process.env.PATH || "");
// [ts] const env = {
// [ts] "PATH": PATH,
// [ts] "DISPLAY": process.env.DISPLAY,
// [ts] "XAUTHORITY": process.env.XAUTHORITY,
// [ts] ...(if (isolateBrowsers) { PLAYWRIGHT_BROWSERS_PATH: _browsersPath } else {}),
// [ts] ...options.env,
// [ts] };
// [ts] test.step(`exec: ${command}`, () => {
// [ts] result = spawnAsync(command, { shell: true, cwd, env });
// [ts] });
// [ts] const stdio = result.stdout + result.stderr;
// [ts] const commandEnv = Object.entries(env).map(e => `${e[0]}=${e[1]}`).join(" ");
// [ts] const fullCommand = `cd ${cwd} && ${commandEnv} ${command}`;
// [ts] testInfo.attach(command, { body: `COMMAND: ${fullCommand}\n\nEXIT CODE: ${result.code}\n\n====== STDOUT + STDERR ======\n\n${stdio}` });
// This means something is really off with spawn
// [ts] if (result.error)
// [ts] return; // TODO(cap-migrate): throw -> error return (throw result.error;)
// [ts] const error: []string = [];
// [ts] if (options.expectToExitWithError && result.code == 0)
// [ts] error.push(`Expected the command to exit with an error, but exited cleanly.`);
// [ts] else if (!options.expectToExitWithError && result.code != 0)
// [ts] error.push(`Expected the command to exit cleanly (0 status code), but exited with ${result.code}.`);
// [ts] if (!error.length)
// [ts] return stdio;
// [ts] if (options.message)
// [ts] error.push(`Message: ${options.message}`);
// [ts] error.push(`Command: ${command}`);
// [ts] error.push(`EXIT CODE: ${result.code}`);
// [ts] error.push(`====== STDIO ======\n${stdio}`);
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(error.join('\n'));)
// [ts] });
// [ts] },
// [ts] tsc: (_p, use) => {
// [ts] exec("npm i typescript@5.2.2 @types/node@18");
// [ts] use((args) => exec("npx", "tsc", args, { shell: process.platform == "win32" }));
// [ts] },
// [ts] });

fn sanitizeEnvPath(value: string) {
  if (process.platform == "win32") {
    return value.split(";").filter(path => !path.endsWith("node_modules\\.bin")).join(";");
  }
  return value.split(":").filter(path => !path.endsWith("node_modules/.bin")).join(":");
}

export { expect };
