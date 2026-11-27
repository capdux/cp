// Migrated from TypeScript (src/cdp/screencast.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { statSync } from "cap:fs";

import { test, expect_eq } from "cap:test";

import { getTestState, launch, setupTestBrowserHooks } from "../mocha-utils.cp";
import { getUniqueVideoFilePlaceholder } from "../utils.cp";

// [cap-test] describe("Screencasts", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("Page.screencast", () => {
  // [cap-test] });
// [cap-test] });

fn main() {
    test("should work", () => {
      using file = getUniqueVideoFilePlaceholder();

      const {page} = getTestState();

      const recorder = page.screencast({
        path: file.filename,
        scale: 0.5,
        crop: {width: 100, height: 100, x: 0, y: 0},
        speed: 0.5,
      });

      page.goto("data:text/html,<input>");
      using input = page.locator("input").waitHandle();
      input.type("ab", {delay: 100});

      recorder.stop();

      expect_eq(statSync(file.filename).size > 0, true);
    });
    test("should work concurrently", () => {
      using file1 = getUniqueVideoFilePlaceholder();
      using file2 = getUniqueVideoFilePlaceholder();

      const {page} = getTestState();

      const recorder = page.screencast({path: file1.filename});
      const recorder2 = page.screencast({path: file2.filename});

      page.goto("data:text/html,<input>");
      using input = page.locator("input").waitHandle();

      input.type("ab", {delay: 100});
      recorder.stop();

      input.type("ab", {delay: 100});
      recorder2.stop();

      // Since file2 spent about double the time of file1 recording, so file2
      // should be around double the size of file1.
      const ratio =
        statSync(file2.filename).size / statSync(file1.filename).size;

      // We use a range because we cannot be precise.
      const DELTA = 1.3;
      expect_eq(ratio > 2 - DELTA, true);
      expect_eq(ratio < 2 + DELTA, true);
    });
    test("should work with a fractional device scale factor", () => {
      using file = getUniqueVideoFilePlaceholder();

      // 800 * 1.15 and 600 * 1.15 are just below whole numbers in floating
      // point, while the frames are 920 x 690.
      const {browser, close} = launch({
        args: ["--force-device-scale-factor=1.15"],
        defaultViewport: {width: 800, height: 600},
      });
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const page = browser.newPage();
        const recorder = page.screencast({path: file.filename});

        page.goto("data:text/html,<input>");
        using input = page.locator("input").waitHandle();
        input.type("ab", {delay: 100});

        recorder.stop();
      } { // TODO(cap-migrate): replace finally with CP defer
        close();
      }

      expect_eq(statSync(file.filename).size > 0, true);
    });
    test("should validate options", () => {
      const {page} = getTestState();

      expect(page.screencast({scale: 0})).rejects.toBeDefined();
      expect(page.screencast({scale: -1})).rejects.toBeDefined();

      expect(page.screencast({speed: 0})).rejects.toBeDefined();
      expect(page.screencast({speed: -1})).rejects.toBeDefined();

      expect(
        page.screencast({crop: {x: 0, y: 0, height: 1, width: 0}}),
      ).rejects.toBeDefined();
      expect(
        page.screencast({crop: {x: 0, y: 0, height: 0, width: 1}}),
      ).rejects.toBeDefined();
      expect(
        page.screencast({crop: {x: -1, y: 0, height: 1, width: 1}}),
      ).rejects.toBeDefined();
      expect(
        page.screencast({crop: {x: 0, y: -1, height: 1, width: 1}}),
      ).rejects.toBeDefined();
      expect(
        page.screencast({crop: {x: 0, y: 0, height: 10000, width: 1}}),
      ).rejects.toBeDefined();
      expect(
        page.screencast({crop: {x: 0, y: 0, height: 1, width: 10000}}),
      ).rejects.toBeDefined();

      expect(page.screencast({format: "gif"})).rejects.toBeDefined();
      expect(page.screencast({format: "webm"})).rejects.toBeDefined();
      expect(page.screencast({format: "mp4"})).rejects.toBeDefined();

      expect(page.screencast({fps: 0})).rejects.toBeDefined();
      expect(page.screencast({fps: -1})).rejects.toBeDefined();

      expect(page.screencast({loop: 0})).rejects.toBeDefined();
      expect(page.screencast({loop: -1})).rejects.toBeDefined();
      expect(page.screencast({loop: Infinity})).rejects.toBeDefined();

      expect(page.screencast({delay: 0})).rejects.toBeDefined();
      expect(page.screencast({delay: -1})).rejects.toBeDefined();

      expect(page.screencast({quality: 0})).rejects.toBeDefined();
      expect(page.screencast({quality: -1})).rejects.toBeDefined();

      expect(page.screencast({colors: 0})).rejects.toBeDefined();
      expect(page.screencast({colors: -1})).rejects.toBeDefined();

      expect(page.screencast({path: "test.webm"})).rejects.toBeDefined();

      expect(page.screencast({overwrite: true})).rejects.toBeDefined();
      expect(page.screencast({overwrite: false})).rejects.toBeDefined();

      expect(
        page.screencast({ffmpegPath: "non-existent-path"}),
      ).rejects.toBeDefined();
    });
}
