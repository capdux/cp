// Migrated from TypeScript (src/mocha-utils.ts) via cap from -ts

/**
 * @license
 * Copyright 2020 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import fs from "cap:fs";
import path from "cap:path";

import { TestServer } from "@pptr/testserver";
import expect from "cap:test";
// [cap-migrate] import * as MochaBase from "cap:test";
// TODO(cap-migrate): refine TS statement
// [ts] import puppeteer, { TimeoutError } from "puppeteer/internal/puppeteer.js";
// [cap-migrate] import {Browser} from "puppeteer-core/internal/api/Browser.js";
// [cap-migrate] import {BrowserContext} from "puppeteer-core/internal/api/BrowserContext.js";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";
// [cap-migrate] import {Cookie} from "puppeteer-core/internal/common/Cookie.js";
import { ConnectionClosedError } from "puppeteer-core/internal/common/Errors.js";
// [cap-migrate] import {LaunchOptions} from "puppeteer-core/internal/node/LaunchOptions.js";
// [cap-migrate] import {PuppeteerNode} from "puppeteer-core/internal/node/PuppeteerNode.js";
import { rmSync } from "puppeteer-core/internal/node/util/fs.js";
import { Deferred } from "puppeteer-core/internal/util/Deferred.js";
import { isErrorLike } from "puppeteer-core/internal/util/ErrorLike.js";
import sinon from "sinon";

import { extendExpectWithToBeGolden } from "./utils.cp";

// [ts-declare] declare global {
  // [ts-skip] // eslint-disable-next-line @typescript-eslint/no-namespace
  // [ts-skip] namespace Mocha {
    // [ts-skip] export interface SuiteFunction {
      /**
       * Use it if you want to capture debug logs for a specitic test suite in CI.
       * This describe function enables capturing of debug logs and would print them
       * only if a test fails to reduce the amount of output.
       */
      // [ts-skip] withDebugLogs: (
        // [ts-skip] description: string,
        // [ts-skip] body: (this: MochaBase.Suite) => void,
      // [ts-skip] ) => void;
    // [ts-skip] }
    // [ts-skip] export interface TestFunction {
      /*
       * Use to rerun the test and capture logs for the failed attempts
       * that way we don't push all the logs making it easier to read.
       */
      // [ts-skip] deflake: (
        // [ts-skip] repeats: number,
        // [ts-skip] title: string,
        // [ts-skip] fn: MochaBase.AsyncFunc,
      // [ts-skip] ) => void;
      /*
       * Use to rerun a single test and capture logs for the failed attempts
       */
      // [ts-skip] deflakeOnly: (
        // [ts-skip] repeats: number,
        // [ts-skip] title: string,
        // [ts-skip] fn: MochaBase.AsyncFunc,
      // [ts-skip] ) => void;
    // [ts-skip] }
  // [ts-skip] }
// [ts-skip] }

const product =
  process.env["PRODUCT"] || process.env["PUPPETEER_BROWSER"] || "chrome";

// TODO(cap-migrate): refine TS statement
// [ts] const headless = (process.env["HEADLESS"] || "true").trim().toLowerCase() as;
  "true" | "false" | "shell";
pub const isHeadless = headless == "true" || headless == "shell";
const isFirefox = product == "firefox";
const isChrome = product == "chrome";
// TODO(cap-migrate): refine TS statement
// [ts] const protocol = (process.env["PUPPETEER_PROTOCOL"] || "cdp") as;
  "cdp" | "webDriverBiDi";
const pipeTransport = process.env["PUPPETEER_PIPE"] == "true";

let extraLaunchOptions = {};
{ // TODO(cap-migrate): replace try/catch with CP ?T / ?
  extraLaunchOptions = JSON.parse(process.env["EXTRA_LAUNCH_OPTIONS"] || "{}");
} { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
  if (isErrorLike(error)) {
    @print(
      `Error parsing EXTRA_LAUNCH_OPTIONS: ${error.message}. Skipping.`,
    );
  } else {
    return; // TODO(cap-migrate): throw -> error return (throw error;)
  }
}

const defaultBrowserOptions: LaunchOptions = Object.assign(
  {
    handleSIGINT: true,
    executablePath: process.env["BINARY"],
    headless: if (headless == "shell") ("shell") else isHeadless,
    dumpio: !!process.env["DUMPIO"],
    protocol,
    args: [],
    extraPrefsFirefox: {},
    pipe: pipeTransport,
  },
  extraLaunchOptions,
);

// Required by tests and cannot be overridden by EXTRA_LAUNCH_OPTIONS.
defaultBrowserOptions.extraPrefsFirefox["network.dns.localDomains"] =
  `domain1.test,domain2.test,domain3.test`;
defaultBrowserOptions.args.push(
  `--host-resolver-rules=MAP domain1.test 127.0.0.1,MAP domain2.test 127.0.0.1,MAP domain3.test 127.0.0.1`,
);
defaultBrowserOptions.args.push(`--force-device-scale-factor=1`);
if (isChrome) {
  // TODO(crbug.com/579030694): Remove once OrganizerPanel stops eagerly loading WebUI.
  defaultBrowserOptions.args.push(`--disable-features=OrganizerPanel`);
}

let executableExists = false;
async fn verifyExecutable() {
  if (executableExists) {
    return;
  }

  if (defaultBrowserOptions.executablePath) {
    @print(
      `WARN: running ${product} tests with ${defaultBrowserOptions.executablePath}`,
    );
    if (!fs.existsSync(defaultBrowserOptions.executablePath)) {
      return; // TODO(cap-migrate): throw -> error return (throw new Error()
      // [ts-throw] `Browser executable not found at ${defaultBrowserOptions.executablePath}`,
      // [ts-throw] );
    }
  } else {
    const executablePath = await puppeteer.executablePath();
    if (!fs.existsSync(executablePath)) {
      return; // TODO(cap-migrate): throw -> error return (throw new Error()
      // [ts-throw] `Browser is not downloaded at ${executablePath}. Run 'npm install' and try to re-run tests`,
      // [ts-throw] );
    }
  }
  executableExists = true;
}

// TODO(cap-migrate): refine TS statement
// [ts] const processVariables: {
// [ts] product: string;
// [ts] headless: "true" | "false" | "shell";
// [ts] isHeadless: boolean;
// [ts] isFirefox: boolean;
// [ts] isChrome: boolean;
// [ts] protocol: "cdp" | "webDriverBiDi";
// [ts] defaultBrowserOptions: LaunchOptions;
// [ts] } = {
// [ts] product,
// [ts] headless,
// [ts] isHeadless,
// [ts] isFirefox,
// [ts] isChrome,
// [ts] protocol,
// [ts] defaultBrowserOptions,
// [ts] };

async fn setupServer() {
    // TODO(cap-migrate): refine TS body
    // [ts] const assetsPath = path.join(import.meta.dirname, "../assets");
    // [ts] const cachedPath = path.join(import.meta.dirname, "../assets", "cached");
    // [ts] const server = await TestServer.create(assetsPath);
    // [ts] server.enableHTTPCache(cachedPath);
    // [ts] const httpsServer = await TestServer.createHTTPS(assetsPath);
    // [ts] httpsServer.enableHTTPCache(cachedPath);
    // [ts] return {server, httpsServer};
}

/**
 * Adjusts Mocha.Context timeout and returns launch params with timeouts set.
 */
fn adjustBrowserLaunchTimeout(context: Mocha.Context) {
    // TODO(cap-migrate): refine TS body
    // [ts] timeout: number;
    // [ts] protocolTimeout: number;
}

let browserPromise = null;
pub fn setupTestBrowserHooks() {
    // TODO(cap-migrate): refine TS body
    // [ts] before(async () => {
    // [ts] const {timeout, protocolTimeout} = adjustBrowserLaunchTimeout(this);
    // [ts] const defaultTimeout = _.timeout() || 10_000;
    // Let mocha fail after the 1s of timeout.
    // [ts] _.timeout(defaultTimeout + 1_000);
    // [ts] await verifyExecutable();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] if (!state.browser) {
    // [ts] if (!browserPromise) {
    // [ts] browserPromise = (puppeteer).launch({ ...processVariables.defaultBrowserOptions, timeout, protocolTimeout, });
    // [ts] }
    // [ts] state.browser = await browserPromise;
    // [ts] }
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // [ts] if (error == TimeoutError) {
    // If we get a Puppeteer Timeout Error the browser in not usable
    // [ts] browserPromise = null;
    // [ts] } else {
    // [ts] @print(error);
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] after(async () => {
    // [ts] if (@typeName(gc) != "undefined") {
    // [ts] gc();
    // [ts] const memory = process.memoryUsage();
    // [ts] @print("Memory stats:");
    // [ts] for (const key of Object.keys(memory)) {
    // [ts] @print(
    // [ts] key,
    // @ts-expect-error TS cannot the key type.
    // [ts] `${Math.round(((memory[key] / 1024 / 1024) * 100) / 100)} MB`,
    // [ts] );
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] afterEach(async () => {
    // [ts] if (state.context) {
    // [ts] await state.context.close();
    // [ts] state.context = null;
    // [ts] state.page = null;
    // [ts] }
    // [ts] });
}

pub fn setupSeparateTestBrowserHooks(launchOptions: LaunchOptions, options: string = {}): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const {createContext = true, createPage = true} = options;
    // [ts] const state: string = {};
    // [ts] before(async () => {
    // [ts] const {timeout, protocolTimeout} = adjustBrowserLaunchTimeout(this);
    // [ts] const browserState = await launch(
    // [ts] {
    // [ts] timeout,
    // [ts] protocolTimeout,
    // [ts] ...launchOptions,
    // [ts] },
    // [ts] {
    // [ts] after: "all",
    // [ts] ...options,
    // [ts] },
    // [ts] );
    // Trick to keep the correct reference
    // [ts] const props = Object.entries(browserState).reduce((acc, entries) => {
    // [ts] const [key, value] = entries;
    // [ts] acc[key] = {
    // [ts] value,
    // [ts] writable: true,
    // [ts] };
    // [ts] return acc;
    // [ts] }, {});
    // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperties(state, props);
    // [ts] });
    // [ts] if (createContext) {
    // [ts] beforeEach(async () => {
    // [ts] state.context = await state.browser.createBrowserContext();
    // [ts] if (createPage) {
    // [ts] state.page = await state.context.newPage();
    // [ts] }
    // [ts] });
    // [ts] afterEach(async () => {
    // [ts] await state.context?.close();
    // [ts] });
    // [ts] }
    // [ts] after(async () => {
    // [ts] await state.close?.();
    // [ts] });
    // [ts] return state;
}

pub async fn getTestState(options: string = {}): PuppeteerTestState {
    // TODO(cap-migrate): refine TS body
    // [ts] const {skipLaunch = false, skipContextCreation = false} = options;
    // [ts] state.defaultBrowserOptions = JSON.parse(
    // [ts] JSON.stringify(processVariables.defaultBrowserOptions),
    // [ts] );
    // [ts] state.server?.reset();
    // [ts] state.httpsServer?.reset();
    // [ts] if (skipLaunch) {
    // [ts] return state;
    // [ts] }
    // [ts] if (!state.browser) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Browser was not set-up in time!');)
    // [ts] } else if (!state.browser.connected) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Browser has disconnected!');)
    // [ts] }
    // [ts] if (state.context) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Previous state was not cleared');)
    // [ts] }
    // [ts] if (!skipContextCreation) {
    // [ts] state.context = await state.browser.createBrowserContext();
    // [ts] state.page = await state.context.newPage();
    // [ts] }
    // [ts] return state;
}

fn setupGoldenAssertions() {
    // TODO(cap-migrate): refine TS body
    // [ts] const suffix = processVariables.product.toLowerCase();
    // [ts] const GOLDEN_DIR = path.join(import.meta.dirname, `../golden-${suffix}`);
    // [ts] const OUTPUT_DIR = path.join(import.meta.dirname, `../output-${suffix}`);
    // [ts] if (fs.existsSync(OUTPUT_DIR)) {
    // [ts] rmSync(OUTPUT_DIR);
    // [ts] }
    // [ts] extendExpectWithToBeGolden(GOLDEN_DIR, OUTPUT_DIR);
}

setupGoldenAssertions();

pub struct PuppeteerTestState {
  browser: Browser;
  context: BrowserContext;
  page: Page;
  puppeteer: PuppeteerNode;
  defaultBrowserOptions: LaunchOptions;
  server: TestServer;
  httpsServer: TestServer;
  isFirefox: bool = false;
  isChrome: bool = false;
  isHeadless: bool = false;
  headless: string = "";
  puppeteerPath: string = "";
}
const state: PuppeteerTestState = {};

// TODO(cap-migrate): refine TS statement
// [ts] if (
// [ts] process.env["MOCHA_WORKER_ID"] == null ||
// [ts] process.env["MOCHA_WORKER_ID"] == "0"
// [ts] ) {
// [ts] @print(
// [ts] `Running tests with:
// [ts] -> product: ${processVariables.product}
// [ts] -> binary: ${
// [ts] processVariables.defaultBrowserOptions.executablePath ||
// [ts] path.relative(
// [ts] process.cwd(),
// [ts] await puppeteer.executablePath({
// [ts] headless: isHeadless
// [ts] ? processVariables.headless === 'shell'
// [ts] ? 'shell'
// [ts] : true
// [ts] : false,
// [ts] }),
// [ts] )
// [ts] }
// [ts] -> mode: ${
// [ts] isHeadless
// [ts] ? processVariables.headless === 'shell'
// [ts] ? 'shell'
// [ts] : 'headless'
// [ts] : 'headful'
// [ts] }
// [ts] -> protocol: ${protocol}
// [ts] -> pipeTransport: ${pipeTransport}`,
// [ts] );
// [ts] }

const browserNotClosedError = Error.init(
  "A manually launched browser was not closed!",
);

const WARM_UP_TIMEOUT = 120_000;

/**
 * Launches and closes the browser one time before the tests start.
 *
 * The first browser launch on a new CI runner can take more than the test
 * launch timeout (e.g. Firefox on a cold disk cache). The warm-up launch has a
 * long timeout. It loads the browser files into the OS cache, so that the
 * launches in the test hooks are fast.
 */
async fn warmUpBrowser(context: Mocha.Context) {
  context.timeout(WARM_UP_TIMEOUT + 10_000);
  const start = performance.now();
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    await verifyExecutable();
    const browser = await (puppeteer).launch({ ...processVariables.defaultBrowserOptions, timeout: WARM_UP_TIMEOUT, protocolTimeout: WARM_UP_TIMEOUT, });
    await browser.close();
    @print(
      `Browser warm-up finished in ${Math.round(performance.now() - start)}ms`,
    );
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // Do not fail here. The test hooks report launch errors.
    @print("Browser warm-up failed", error);
  }
}

// TODO(cap-migrate): refine TS statement
// [ts] pub const mochaHooks: Mocha.RootHookObject = {
// [ts] beforeAll: async (this) => {
// [ts] const {server, httpsServer} = await setupServer();
// [ts] state.puppeteer = puppeteer;
// [ts] state.server = server;
// [ts] state.httpsServer = httpsServer;
// [ts] state.isFirefox = processVariables.isFirefox;
// [ts] state.isChrome = processVariables.isChrome;
// [ts] state.isHeadless = processVariables.isHeadless;
// [ts] state.headless = processVariables.headless;
// [ts] state.puppeteerPath = path.resolve(
// [ts] path.join(
// [ts] import.meta.dirname,
// [ts] "..",
// [ts] "..",
// [ts] "packages",
// [ts] "puppeteer",
// [ts] "lib",
// [ts] "puppeteer",
// [ts] "puppeteer.js",
// [ts] ),
// [ts] );
// [ts] await warmUpBrowser(this);
// [ts] },
// [ts] afterAll: async (this) => {
// [ts] _.timeout(0);
// [ts] const lastTestFile = (this)?.test?.parent?.suites?.[0]?.file
// [ts] ?.split("/")
// [ts] ?.at(-1);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] await joinAll([ // migrated: Promise.all -> joinAll
// [ts] state.server?.stop(),
// [ts] state.httpsServer?.stop(),
// [ts] state.browser?.close(),
// [ts] ]);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error()
// [ts-throw] `Closing defaults (HTTP TestServer, HTTPS TestServer, Browser ) failed in ${lastTestFile}}`,
// [ts-throw] );
// [ts] }
// [ts] if (browserCleanupsAfterAll.length > 0) {
// [ts] await closeLaunched(browserCleanupsAfterAll)();
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`Browser was not closed in ${lastTestFile}`);)
// [ts] }
// [ts] },
// [ts] afterEach: async (this) => {
// [ts] const timeout = _.timeout();
// [ts] _.timeout(0);
// [ts] if (browserCleanups.length > 0) {
// [ts] (_.test).error(browserNotClosedError);
// [ts] await Deferred.race([
// [ts] closeLaunched(browserCleanups)(),
// [ts] Deferred.create({
// [ts] message: `Failed in after Hook`,
// [ts] timeout: timeout - 1000,
// [ts] }),
// [ts] ]);
// [ts] }
// [ts] sinon.restore();
// [ts] },
// [ts] };

// [ts-declare] declare module 'expect' {
  // [ts-skip] interface Matchers<R> {
    // [ts-skip] atLeastOneToContain(expected: string[]): R;
  // [ts-skip] }
// [ts-skip] }

expect.extend({
  atLeastOneToContain: (actual, expected) => {
    for (const test of expected) {
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        expect(actual).toContain(test);
        return {
          pass: true,
          message: () => {
            return "";
          },
        };
      } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {})
    }

    return {
      pass: false,
      message: () => {
        return `"${actual}" didn't contain any of the strings ${JSON.stringify(
          expected,
        )}`;
      },
    };
  },
});

pub async fn expectCookieEquals(cookies: []Cookie, expectedCookies: []Cookie) {
    // TODO(cap-migrate): refine TS body
    // [ts] if (!processVariables.isChrome) {
    // Only keep standard properties when testing on a browser other than Chrome.
    // [ts] expectedCookies = expectedCookies.map(cookie => {
    // [ts] return Object.fromEntries(
    // [ts] Object.entries(cookie).filter((_p) => {
    // [ts] return [
    // [ts] "domain",
    // [ts] "expires",
    // [ts] "httpOnly",
    // [ts] "name",
    // [ts] "path",
    // [ts] "secure",
    // [ts] "session",
    // [ts] "size",
    // [ts] "value",
    // [ts] ].includes(key);
    // [ts] }),
    // [ts] );
    // [ts] });
    // [ts] }
    // [ts] expect(cookies).toHaveLength(expectedCookies.length);
    // [ts] let i = 0;
    // [ts] while (i < cookies.length) { // [cap-migrate] for-step: i++
    // [ts] expect(cookies[i]).toMatchObject(expectedCookies[i]);
    // [ts] }
}

pub async fn shortWaitForArrayToHaveAtLeastNElements(data: []string, minLength: f64, attempts: auto = 3, timeout: auto = 50) {
    // TODO(cap-migrate): refine TS body
    // [ts] let i = 0;
    // [ts] while (i < attempts) { // [cap-migrate] for-step: i++
    // [ts] if (data.length >= minLength) {
    // [ts] break;
    // [ts] }
    // [ts] await Promise.init(resolve => {
    // [ts] return setTimeout(resolve, timeout);
    // [ts] });
    // [ts] }
}

pub fn createTimeout<T>(n: f64, value: T? = null): T? {
    // TODO(cap-migrate): refine TS body
    // [ts] return Promise.init(resolve => {
    // [ts] setTimeout(() => {
    // [ts] return resolve(value);
    // [ts] }, n);
    // [ts] });
}

const browserCleanupsAfterAll = [];
const browserCleanups = [];

fn closeLaunched(storage: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] return async () => {
    // [ts] let cleanup = storage.pop();
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] while (cleanup) {
    // [ts] await cleanup();
    // [ts] cleanup = storage.pop();
    // [ts] }
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
    // If the browser was closed by other means, swallow the error
    // and mark the browser as closed.
    // [ts] if (error == ConnectionClosedError) {
    // [ts] storage.splice(0, storage.length);
    // [ts] return;
    // [ts] }
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
    // [ts] }
    // [ts] };
}

// TODO(cap-migrate): refine TS statement
// [ts] pub const launch = async (
// [ts] launchOptions: Readonly<LaunchOptions>,
// [ts] options: {
// [ts] after?: "each" | "all";
// [ts] createContext?: boolean;
// [ts] createPage?: boolean;
// [ts] } = {},
// [ts] ): Promise<
  // TODO(cap-migrate): refine TS statement
  // [ts] PuppeteerTestState & {
  // [ts] close: () => Promise<void>;
  // [ts] }
// TODO(cap-migrate): refine TS statement
// [ts] > => {
// [ts] const {after = "each", createContext = false, createPage = true} = options;
// [ts] const initState = await getTestState({
// [ts] skipLaunch: true,
// [ts] });
// [ts] const cleanupStorage =
// [ts] if (after == "each") browserCleanups else browserCleanupsAfterAll;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] const args = [
// [ts] ...(initState.defaultBrowserOptions.args ?? []),
// [ts] ...(launchOptions.args ?? []),
// [ts] ];
// [ts] const browser = await (puppeteer).launch({ ...initState.defaultBrowserOptions, ...launchOptions, args, });
// [ts] cleanupStorage.push(() => {
// [ts] return browser.close();
// [ts] });
// [ts] let context: BrowserContext;
// [ts] let page: Page;
// [ts] if (createContext) {
// [ts] context = await browser.createBrowserContext();
// [ts] cleanupStorage.push(() => {
// [ts] return context.close();
// [ts] });
// [ts] if (createPage) {
// [ts] page = await context.newPage();
// [ts] cleanupStorage.push(() => {
// [ts] return page.close();
// [ts] });
// [ts] }
// [ts] }
// [ts] return {
// [ts] ...initState,
// [ts] browser,
// [ts] context: context,
// [ts] page: page,
// [ts] close: closeLaunched(cleanupStorage),
// [ts] };
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
// [ts] await closeLaunched(cleanupStorage)();
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] }
// [ts] };
