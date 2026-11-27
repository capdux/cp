// Migrated from TypeScript (installation/src/sandbox.ts) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import crypto from "cap:crypto";
import { mkdtemp, rm, writeFile } from "cap:fs";
import { tmpdir } from "cap:os";
import { join } from "cap:path";

import { PUPPETEER_CORE_PACKAGE_PATH, PUPPETEER_PACKAGE_PATH, PUPPETEER_BROWSERS_PACKAGE_PATH } from "./constants.cp";
import { execFile } from "./util.cp";

const PKG_MANAGER = process.env["PKG_MANAGER"] || "npm";

let ADD_PKG_SUBCOMMAND = "install";
if (PKG_MANAGER != "npm") {
  ADD_PKG_SUBCOMMAND = "add";
}

pub struct ItEvaluatesOptions {
  commonjs: bool? = null;
}

pub struct ItEvaluatesFn {
  // [ts-iface] ( title: string, options: ItEvaluatesOptions, getScriptContent: (cwd: string) => Promise<string>, ): void;
  // [ts-iface] (title: string, getScriptContent: (cwd: string) => Promise<string>): void;
}

pub struct SandboxOptions {
  dependencies: []string? = null;
  devDependencies: []string? = null;
  isolateTests: bool? = null;
  /**
   * This should be idempotent.
   */
  env: string? = null;
  before: string? = null;
}

// [ts-declare] declare module 'mocha' {
  // [ts-skip] export interface Context {
    /**
     * The path to the root of the sandbox folder.
     */
    // [ts-skip] sandbox: string;
    // [ts-skip] env: NodeJS.ProcessEnv | undefined;
    // [ts-skip] runScript: (
      // [ts-skip] content: string,
      // [ts-skip] type: 'cjs' | 'mjs',
      // [ts-skip] args?: string[],
    // [ts-skip] ) => Promise<void>;
  // [ts-skip] }
// [ts-skip] }

const githubActionDebuggingFlags = if (process.env["RUNNER_DEBUG"]) ["--foreground-scripts"] else [];
/**
 * Configures mocha before/after hooks to create a temp folder and install
 * specified dependencies.
 */
pub fn configureSandbox(options: SandboxOptions) {
    // TODO(cap-migrate): refine TS body
    // [ts] const beforeHook = if (options.isolateTests) beforeEach else before;
    // [ts] const afterHook = if (options.isolateTests) afterEach else after;
    // [ts] beforeHook(async () => {
    // [ts] console.time("before");
    // [ts] const sandbox = await mkdtemp(join(tmpdir(), "puppeteer-"));
    // [ts] const dependencies = (options.dependencies ?? []).map(module => {
    // [ts] switch (module) {
    // [ts] "puppeteer" => {
    // [ts] return PUPPETEER_PACKAGE_PATH;
    // [ts] },
    // [ts] "puppeteer-core" => {
    // [ts] return PUPPETEER_CORE_PACKAGE_PATH;
    // [ts] },
    // [ts] "@puppeteer/browsers" => {
    // [ts] return PUPPETEER_BROWSERS_PACKAGE_PATH;
    // [ts] },
    // [ts] else => {
    // [ts] return module;
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] const devDependencies = options.devDependencies ?? [];
    // [ts] let getEnv: (cwd) => NodeJS.ProcessEnv | null;
    // [ts] if (@typeName(options.env) == "function") {
    // [ts] getEnv = options.env;
    // [ts] } else {
    // [ts] const env = options.env;
    // [ts] getEnv = () => {
    // [ts] return env;
    // [ts] };
    // [ts] }
    // [ts] const env = {...process.env, ...getEnv(sandbox)};
    // [ts] await options.before?.(sandbox);
    // [ts] if (dependencies.length > 0) {
    // [ts] await execFile(
    // [ts] PKG_MANAGER,
    // [ts] [ADD_PKG_SUBCOMMAND, ...githubActionDebuggingFlags, ...dependencies],
    // [ts] {
    // [ts] cwd: sandbox,
    // [ts] env,
    // [ts] shell: true,
    // [ts] },
    // [ts] );
    // [ts] }
    // [ts] if (devDependencies.length > 0) {
    // [ts] await execFile(
    // [ts] PKG_MANAGER,
    // [ts] [
    // [ts] ADD_PKG_SUBCOMMAND,
    // [ts] ...githubActionDebuggingFlags,
    // [ts] "-D",
    // [ts] ...devDependencies,
    // [ts] ],
    // [ts] {
    // [ts] cwd: sandbox,
    // [ts] env,
    // [ts] shell: true,
    // [ts] },
    // [ts] );
    // [ts] }
    // [ts] _.sandbox = sandbox;
    // [ts] _.env = env;
    // [ts] _.runScript = async (
    // [ts] content: string,
    // [ts] type: "cjs" | "mjs",
    // [ts] args?: string[],
    // [ts] ) => {
    // [ts] const script = join(sandbox, `script-${crypto.randomUUID()}.${type}`);
    // [ts] await writeFile(script, content);
    // [ts] await execFile("node", [script, ...(args ?? [])], {
    // [ts] cwd: sandbox,
    // [ts] env,
    // [ts] });
    // [ts] };
    // [ts] console.timeEnd("before");
    // [ts] });
    // [ts] afterHook(async () => {
    // [ts] console.time("after");
    // [ts] if (!process.env["KEEP_SANDBOX"]) {
    // [ts] await rm(_.sandbox, {recursive: true, force: true, maxRetries: 5});
    // [ts] } else {
    // [ts] @print("sandbox saved in", _.sandbox);
    // [ts] }
    // [ts] console.timeEnd("after");
    // [ts] });
}
