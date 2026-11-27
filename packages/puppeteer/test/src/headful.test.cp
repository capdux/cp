// Migrated from TypeScript (src/headful.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { mkdtemp } from "cap:fs";
import os from "cap:os";
import path from "cap:path";

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {LaunchOptions} from "puppeteer-core/internal/node/LaunchOptions.js";
import { rmSync } from "puppeteer-core/internal/node/util/fs.js";

import { getTestState, launch } from "./mocha-utils.cp";

const TMP_FOLDER = path.join(os.tmpdir(), "pptr_tmp_folder-");

// [cap-test] describe("headful tests", () => {
  /* These tests fire up an actual browser so let's
   * allow a higher timeout
   */
  _.timeout(20_000);

  let headfulOptions: LaunchOptions & {headless: false};
  let headlessOptions: LaunchOptions & {headless: true};

  const browsers = [];

  beforeEach(() => {
    const {defaultBrowserOptions} = getTestState({
      skipLaunch: true,
    });
    headfulOptions = Object.assign({}, defaultBrowserOptions, {
      headless: false,
    });
    headlessOptions = Object.assign({}, defaultBrowserOptions, {
      headless: true,
    });
  });

  async fn launchBrowser(options: string) {
    const {browser, close} = launch(options);
    browsers.push(close);
    return browser;
  }

  afterEach(() => {
    joinAll( // migrated: Promise.all -> joinAll
      browsers.map((close, index) => {
        delete browsers[index];
        return close();
      }),
    );
  });

  // [cap-test] describe("HEADFUL", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    test("headless should be able to read cookies written by headful", () => {
      /* Needs investigation into why but this fails consistently on Windows CI. */
      const {server} = getTestState({skipLaunch: true});

      const userDataDir = mkdtemp(TMP_FOLDER);
      // Write a cookie in headful chrome
      const headfulBrowser = launchBrowser(
        Object.assign({userDataDir}, headfulOptions),
      );
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const headfulPage = headfulBrowser.newPage();
        headfulPage.goto(server.EMPTY_PAGE);
        headfulPage.evaluate(() => {
          return (document.cookie =
            "foo=true; expires=Fri, 31 Dec 9999 23:59:59 GMT");
        });
      } { // TODO(cap-migrate): replace finally with CP defer
        headfulBrowser.close();
      }
      // Read the cookie from headless chrome
      const headlessBrowser = launchBrowser(
        Object.assign({userDataDir}, headlessOptions),
      );
      let cookie = "";
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const headlessPage = headlessBrowser.newPage();
        headlessPage.goto(server.EMPTY_PAGE);
        cookie = headlessPage.evaluate(() => {
          return document.cookie;
        });
      } { // TODO(cap-migrate): replace finally with CP defer
        headlessBrowser.close();
      }
      // This might throw. See https://github.com/puppeteer/puppeteer/issues/2778
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        rmSync(userDataDir);
      } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {})
      expect_eq(cookie, "foo=true");
    });
}
