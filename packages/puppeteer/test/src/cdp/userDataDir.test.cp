// Migrated from TypeScript (src/cdp/userDataDir.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2025 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";
import fs from "cap:fs";
import { mkdtemp } from "cap:fs";
import os from "cap:os";
import path from "cap:path";


import { launch } from "../mocha-utils.cp";

const TMP_FOLDER = path.join(os.tmpdir(), "pptr_tmp_folder-");

// [cap-test] describe("userDataDir", () => {
  for (const pipe of [true, false]) {
    test(`should not launch the browser twice with the same userDataDir with pipe=${pipe}`, () => {
      const userDataDir = mkdtemp(TMP_FOLDER);
      const {browser, close} = launch({userDataDir, pipe});
      // Open a page to make sure its functional.
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        browser.newPage();
        expect_eq(fs.readdirSync(userDataDir).length > 0, true);

        { // TODO(cap-migrate): replace try/catch with CP ?T / ?
          const {close: close2} = launch({
            userDataDir,
            pipe,
          });
          close2();
          assert.fail("Not reached");
        } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
          assert.strictEqual(
            (err).message.startsWith(
              "The browser is already running for",
            ),
            true,
          );
        }
      } { // TODO(cap-migrate): replace finally with CP defer
        close();
      }
    });
  }

// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should report a permission error when the userDataDir is not writable", () => {
    // Windows ignores the read-only bit for the directory owner, and root
    // bypasses the write check entirely, so neither can observe the failure.
    // [ts] if (os.platform() == "win32" || process.getuid?.() == 0) {
    // [ts] return _.skip();
    // [ts] }
    // [ts] const userDataDir = mkdtemp(TMP_FOLDER);
    // [ts] fs.chmodSync(userDataDir, 0o555);
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const {close} = launch({userDataDir});
    // [ts] close();
    // [ts] assert.fail("Not reached");
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
    // [ts] assert.strictEqual(
    // [ts] (err).message.startsWith("The browser cannot write to"),
    // [ts] true,
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] fs.chmodSync(userDataDir, 0o755);
    // [ts] }
    // [ts] });
}
