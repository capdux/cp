// Migrated from TypeScript (src/launcher.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";
import fs from "cap:fs";
import { mkdtemp, readFile, writeFile } from "cap:fs";
import os from "cap:os";
import path from "cap:path";
// [cap-migrate] import {TLSSocket} from "node:tls";

import { TimeoutError } from "puppeteer";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";
// [cap-migrate] import {BidiBrowser} from "puppeteer-core/internal/bidi/Browser.js";
import { rmSync } from "puppeteer-core/internal/node/util/fs.js";
import sinon from "sinon";

import { getTestState, launch } from "./mocha-utils.cp";
import { dumpFrames, waitEvent } from "./utils.cp";

const TMP_FOLDER = path.join(os.tmpdir(), "pptr_tmp_folder-");
const FIREFOX_TIMEOUT = 30_000;

// [cap-test] describe("Launcher specs", () => {
  _.timeout(FIREFOX_TIMEOUT);

  // [cap-test] describe("Puppeteer", () => {
    // [cap-test] describe("Browser.disconnect", () => {
    // [cap-test] });
    // [cap-test] describe("Browser.close", () => {

    // [cap-test] });
    // [cap-test] describe("Puppeteer.launch", () => {



    // [cap-test] });

    // [cap-test] describe("Puppeteer.connect", () => {


      // @see https://github.com/puppeteer/puppeteer/issues/4197
      // @see https://github.com/puppeteer/puppeteer/issues/4197#issuecomment-481793410
    // [cap-test] });
    // [cap-test] describe("Puppeteer.executablePath", () => {
      // [cap-test] describe("when executable path is configured", () => {
        const sandbox = sinon.createSandbox();

        beforeEach(() => {
          const {puppeteer} = getTestState({
            skipLaunch: true,
          });
          sandbox
            .stub(puppeteer, "configuration")
            .resolves({executablePath: "SOME_CUSTOM_EXECUTABLE"});
        });

        afterEach(() => {
          sandbox.restore();
        });

      // [cap-test] });
    // [cap-test] });
  // [cap-test] });

  // [cap-test] describe("Browser target events", () => {
  // [cap-test] });

  // [cap-test] describe("Browser.Events.disconnected", () => {
    _.timeout(60_000);

  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should reject navigation when browser closes", () => {
    // [ts] const {browser, close, puppeteer, server} = launch({});
    // [ts] server.setRoute("/one-style.css", () => {});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] using remote = puppeteer.connect({
    // [ts] browserWSEndpoint: browser.wsEndpoint(),
    // [ts] protocol: browser.protocol,
    // [ts] });
    // [ts] const page = remote.newPage();
    // [ts] const navigationPromise = page
    // [ts] .goto(server.PREFIX + "/one-style.html", {timeout: 60_000})
    // [ts] .catch(error_ => {
    // [ts] return error_;
    // [ts] });
    // [ts] server.waitForRequest("/one-style.css");
    // [ts] remote.disconnect();
    // [ts] const error = navigationPromise;
    // [ts] expect(
    // [ts] [
    // [ts] "Navigating frame was detached",
    // [ts] "Protocol error (Page.navigate): Target closed.",
    // [ts] "Protocol error (browsingContext.navigate): Target closed",
    // [ts] "Frame detached",
    // [ts] ].some(message => {
    // [ts] return error.message.startsWith(message);
    // [ts] }),
    // [ts] ).toBeTruthy();
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should reject waitForSelector when browser closes", () => {
    // [ts] const {browser, close, server, puppeteer} = launch({});
    // [ts] server.setRoute("/empty.html", () => {});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] using remote = puppeteer.connect({
    // [ts] browserWSEndpoint: browser.wsEndpoint(),
    // [ts] protocol: browser.protocol,
    // [ts] });
    // [ts] const page = remote.newPage();
    // [ts] const watchdog = page
    // [ts] .waitForSelector("div", {timeout: 60_000})
    // [ts] .catch(error_ => {
    // [ts] return error_;
    // [ts] });
    // [ts] remote.disconnect();
    // [ts] const error = watchdog;
    // [ts] expect_eq(error.message, "Waiting for selector `div` failed");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should terminate network waiters", () => {
    // [ts] const {browser, close, server, puppeteer} = launch({});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] using remote = puppeteer.connect({
    // [ts] browserWSEndpoint: browser.wsEndpoint(),
    // [ts] protocol: browser.protocol,
    // [ts] });
    // [ts] const newPage = remote.newPage();
    // [ts] const results = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] newPage.waitForRequest(server.EMPTY_PAGE).catch(error => {
    // [ts] return error;
    // [ts] }),
    // [ts] newPage.waitForResponse(server.EMPTY_PAGE).catch(error => {
    // [ts] return error;
    // [ts] }),
    // [ts] browser.close(),
    // [ts] ]);
    // [ts] let i = 0;
    // [ts] while (i < 2) { // [cap-migrate] for-step: i++
    // [ts] const message = results[i].message;
    // [ts] expect(message).atLeastOneToContain([
    // [ts] "Target closed",
    // [ts] "Page closed!",
    // [ts] "Browser already closed",
    // [ts] ]);
    // [ts] expect_eq(std.mem.indexOf(u8, message, "Timeout") == null, true);
    // [ts] }
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should stop the browser process when the close command fails", () => {
    // [ts] const {defaultBrowserOptions} = getTestState({skipLaunch: true});
    // [ts] if (defaultBrowserOptions.protocol != "webDriverBiDi") {
    // [ts] _.skip();
    // [ts] }
    // [ts] const {browser, close} = launch({});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const browserProcess = browser.process();
    // [ts] const connection = (browser).connection;
    // [ts] const send = connection.send.bind(connection);
    // [ts] sinon
    // [ts] .stub(connection, "send")
    // [ts] .callsFake((method, params, timeout) => {
    // [ts] if (method == "browser.close") {
    // [ts] return Promise.reject(Error.init("browser.close failed"));
    // [ts] }
    // [ts] return send(method, params, timeout);
    // [ts] });
    // [ts] const exited = Promise.init(resolve => {
    // [ts] return browserProcess.once("exit", resolve);
    // [ts] });
    // [ts] browser.close();
    // [ts] exited;
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("can launch and close the browser", () => {
    // [ts] const {close} = launch({});
    // [ts] close();
    // [ts] });
    // [ts] test("can launch multiple instances without node warnings", () => {
    // [ts] const instances = [];
    // [ts] let warning: Error | null;
    // [ts] const warningHandler: NodeJS.WarningListener = w => {
    // [ts] warning = w;
    // [ts] };
    // [ts] process.on("warning", warningHandler);
    // [ts] process.setMaxListeners(1);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] let i = 0;
    // [ts] while (i < 2) { // [cap-migrate] for-step: i++
    // [ts] instances.push(launch({}));
    // [ts] }
    // [ts] const launchedPromises = Promise.allSettled(instances);
    // [ts] const launched: Array<Awaited<ReturnType<@typeName(launch)>> = [];
    // [ts] for (const browserPromise of launchedPromises) {
    // [ts] if (browserPromise.status == "rejected") {
    // [ts] warning = Error.init("Browser failed to launch");
    // [ts] } else {
    // [ts] launched.push(browserPromise.value);
    // [ts] }
    // [ts] }
    // [ts] joinAll( // migrated: Promise.all -> joinAll
    // [ts] launched.map(instance => {
    // [ts] return instance.close();
    // [ts] }),
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] process.setMaxListeners(10);
    // [ts] }
    // [ts] process.off("warning", warningHandler);
    // [ts] expect_eq(warning?.stack, null);
    // [ts] });
    // [ts] test("should have default url when launching browser", () => {
    // [ts] const {browser, close} = launch({});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const pages = (browser.pages()).map(
    // [ts] (page) => {
    // [ts] return page.url();
    // [ts] },
    // [ts] );
    // [ts] expect_eq(pages, ["about:blank"]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should close browser with beforeunload page", () => {
    // [ts] const {browser, server, close} = launch({});
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = browser.newPage();
    // [ts] page.goto(server.PREFIX + "/beforeunload.html");
    // We have to interact with a page so that 'beforeunload' handlers
    // fire.
    // [ts] page.click("body");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should reject all promises when browser is closed", () => {
    // [ts] const {browser, close} = launch({});
    // [ts] let error: Error;
    // [ts] const page = browser.newPage();
    // [ts] const neverResolves = page
    // [ts] .evaluate(() => {
    // [ts] return Promise.init(() => {});
    // [ts] })
    // [ts] .catch(error_ => {
    // [ts] return (error = error_);
    // [ts] });
    // [ts] close();
    // [ts] neverResolves;
    // [ts] expect_eq(std.mem.indexOf(u8, error.message, "Protocol error") != null, true);
    // [ts] });
    // [ts] test("should reject if executable path is invalid", () => {
    // [ts] let waitError: Error;
    // [ts] launch({
    // [ts] executablePath: "random-invalid-path",
    // [ts] }).catch(error => {
    // [ts] return (waitError = error);
    // [ts] });
    // [ts] expect(waitError.message).toBe(
    // [ts] "Browser was not found at the configured executablePath (random-invalid-path)",
    // [ts] );
    // [ts] });
    // [ts] test("userDataDir option", () => {
    // [ts] const userDataDir = mkdtemp(TMP_FOLDER);
    // [ts] const {browser, close} = launch({userDataDir});
    // Open a page to make sure its functional.
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] browser.newPage();
    // [ts] expect_eq(fs.readdirSync(userDataDir).length > 0, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] expect_eq(fs.readdirSync(userDataDir).length > 0, true);
    // This might throw. See https://github.com/puppeteer/puppeteer/issues/2778
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] rmSync(userDataDir);
    // [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {})
}
