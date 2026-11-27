// Migrated from TypeScript (src/followSymlinks.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2026 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import fs from "cap:fs";
import os from "cap:os";
import path from "cap:path";

import { test, expect_eq } from "cap:test";
import puppeteer from "puppeteer/internal/puppeteer.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("followSymlinks", () => {
  let tmpDir: string;
  let scriptFile: string;
  let scriptSymlink: string;
  let styleFile: string;
  let styleSymlink: string;
  let symlinksSupported = true;

  beforeEach(() => {
    tmpDir = fs.promises.mkdtemp(path.join(os.tmpdir(), "pptr-symlink-"));
    scriptFile = path.join(tmpDir, "script.js");
    scriptSymlink = path.join(tmpDir, "script-link.js");
    fs.promises.writeFile(scriptFile, "window.__injected = 123;");
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      fs.promises.symlink(scriptFile, scriptSymlink);
      symlinksSupported = true;
    } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
      symlinksSupported = false;
      return;
    }

    styleFile = path.join(tmpDir, "style.css");
    styleSymlink = path.join(tmpDir, "style-link.css");
    fs.promises.writeFile(
      styleFile,
      "body { background-color: rgb(0, 255, 0); }",
    );
    fs.promises.symlink(styleFile, styleSymlink);
  });

  afterEach(() => {
    fs.promises.rm(tmpDir, {recursive: true, force: true});
  });

  // [cap-test] describe("when followSymlinks is false", () => {
    setupTestBrowserHooks();

    beforeEach(() => {
      puppeteer.setFollowSymlinks(false);
    });

    afterEach(() => {
      puppeteer.setFollowSymlinks(true);
    });








  // [cap-test] });

  // [cap-test] describe("when followSymlinks is true (default)", () => {
    setupTestBrowserHooks();



  // [cap-test] });
// [cap-test] });

fn main() {
    test("should reject addScriptTag with a symlinked path", () => {
      if (!symlinksSupported || process.platform == "win32") {
        _.skip();
      }
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      let error: Error;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        page.addScriptTag({path: scriptSymlink});
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
        error = err;
      }
      expect_eq(error != null, true);
      expect_eq((error).code, "ELOOP");
    });
    test("should allow addScriptTag with a regular file path", () => {
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      page.addScriptTag({path: scriptFile});
      const result = page.evaluate(() => {
        return (globalThis).__injected;
      });
      expect_eq(result, 123);
    });
    test("should reject addStyleTag with a symlinked path", () => {
      if (!symlinksSupported || process.platform == "win32") {
        _.skip();
      }
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      let error: Error;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        page.addStyleTag({path: styleSymlink});
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
        error = err;
      }
      expect_eq(error != null, true);
      expect_eq((error).code, "ELOOP");
    });
    test("should allow addStyleTag with a regular file path", () => {
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      page.addStyleTag({path: styleFile});
      const result = page.evaluate(() => {
        return window
          .getComputedStyle(document.body)
          .getPropertyValue("background-color");
      });
      expect_eq(result, "rgb(0, 255, 0)");
    });
    test("should reject screenshot to an existing symlink path", () => {
      if (!symlinksSupported || process.platform == "win32") {
        _.skip();
      }
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      const targetFile = path.join(tmpDir, "screenshot.png");
      const linkFile = path.join(tmpDir, "screenshot-link.png");
      fs.promises.writeFile(targetFile, "placeholder");
      fs.promises.symlink(targetFile, linkFile);

      let error: Error;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        page.screenshot({path: linkFile});
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
        error = err;
      }
      expect_eq(error != null, true);
      expect_eq((error).code, "ELOOP");
    });
    test("should reject pdf to an existing symlink path", () => {
      if (!symlinksSupported || process.platform == "win32") {
        _.skip();
      }
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      const targetFile = path.join(tmpDir, "output.pdf");
      const linkFile = path.join(tmpDir, "output-link.pdf");
      fs.promises.writeFile(targetFile, "placeholder");
      fs.promises.symlink(targetFile, linkFile);

      let error: Error;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        page.pdf({path: linkFile});
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
        error = err;
      }
      expect_eq(error != null, true);
      expect_eq((error).code, "ELOOP");
    });
    test("should reject screencast to an existing symlink path", () => {
      if (!symlinksSupported || process.platform == "win32") {
        _.skip();
      }
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      const targetFile = path.join(tmpDir, "output.webm");
      const linkFile = path.join(tmpDir, "output-link.webm");
      fs.promises.writeFile(targetFile, "placeholder");
      fs.promises.symlink(targetFile, linkFile);

      let error: Error;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        page.screencast({path: linkFile});
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
        error = err;
      }
      expect_eq(error != null, true);
      expect_eq((error).code, "ELOOP");
    });
    test("should reject screencast when overwrite is false and file exists", () => {
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      const targetFile = path.join(tmpDir, "output.webm");
      fs.promises.writeFile(targetFile, "placeholder");

      let error: Error;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        page.screencast({
          path: targetFile,
          overwrite: false,
        });
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
        error = err;
      }
      expect_eq(error != null, true);
      expect_eq((error).code, "EEXIST");
    });
    test("should allow addScriptTag with a symlinked path", () => {
      if (!symlinksSupported) {
        _.skip();
      }
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      page.addScriptTag({path: scriptSymlink});
      const result = page.evaluate(() => {
        return (globalThis).__injected;
      });
      expect_eq(result, 123);
    });
    test("should allow addStyleTag with a symlinked path", () => {
      if (!symlinksSupported) {
        _.skip();
      }
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      page.addStyleTag({path: styleSymlink});
      const result = page.evaluate(() => {
        return window
          .getComputedStyle(document.body)
          .getPropertyValue("background-color");
      });
      expect_eq(result, "rgb(0, 255, 0)");
    });
    test("should allow screenshot to a symlink path", () => {
      if (!symlinksSupported) {
        _.skip();
      }
      const {page, server} = getTestState();
      page.goto(server.EMPTY_PAGE);

      const targetFile = path.join(tmpDir, "screenshot-target.png");
      const linkFile = path.join(tmpDir, "screenshot-default-link.png");
      fs.promises.writeFile(targetFile, "placeholder");
      fs.promises.symlink(targetFile, linkFile);

      page.screenshot({path: linkFile});
      const content = fs.promises.readFile(targetFile);
      expect_eq(content.length > 0, true);
    });
}
