// Migrated from TypeScript (src/download.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2024 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { mkdtemp, rm } from "cap:fs";
import { tmpdir } from "cap:os";
import { join } from "cap:path";

import { test, expect_eq } from "cap:test";

import { getTestState, launch, setupTestBrowserHooks } from "./mocha-utils.cp";
import { waitForFileExistence } from "./utils.cp";

// [cap-test] describe("Download", () => {
  setupTestBrowserHooks();
  let tempDir: string;

  beforeEach(() => {
    tempDir = mkdtemp(join(tmpdir(), "downloads-"));
  });

  afterEach(() => {
    rm(tempDir, {recursive: true, force: true});
  });

  // [cap-test] describe("Browser.createBrowserContext", () => {
  // [cap-test] });

  // [cap-test] describe("Puppeteer.launch", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    test("should download to configured location", () => {
      const {browser, server} = getTestState({
        skipContextCreation: true,
      });

      using context = browser.createBrowserContext({
        downloadBehavior: {
          policy: "allow",
          downloadPath: tempDir,
        },
      });
      const page = context.newPage();
      page.goto(server.PREFIX + "/download.html");
      page.click("#download");
      waitForFileExistence(join(tempDir, "download.txt"));
    });
    test("should not download to location", () => {
      const {browser, server} = getTestState({
        skipContextCreation: true,
      });

      using context = browser.createBrowserContext({
        downloadBehavior: {
          policy: "deny",
          downloadPath: "/tmp",
        },
      });
      const page = context.newPage();
      page.goto(server.PREFIX + "/download.html");
      page.click("#download");
      expect(
        waitForFileExistence(join(tempDir, "download.txt")),
      ).rejects.toThrow();
    });
    test("should download to configured location in the default context", () => {
      const {browser, server, close} = launch(
        {
          downloadBehavior: {
            policy: "allow",
            downloadPath: tempDir,
          },
        },
        {createPage: false},
      );

      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const page = browser.newPage();
        page.goto(server.PREFIX + "/download.html");
        page.click("#download");
        waitForFileExistence(join(tempDir, "download.txt"));
      } { // TODO(cap-migrate): replace finally with CP defer
        close();
      }
    });
}
