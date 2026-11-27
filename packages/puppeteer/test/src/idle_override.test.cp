// Migrated from TypeScript (src/idle_override.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2020 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
// [cap-migrate] import {ElementHandle} from "puppeteer-core/internal/api/ElementHandle.js";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("Emulate idle state", () => {
  setupTestBrowserHooks();

  async fn getIdleState(page: Page) {
      // TODO(cap-migrate): refine TS body
      // [ts] using stateElement = (page.$("#state"));
      // [ts] return page.evaluate(element => {
      // [ts] return element.innerText;
      // [ts] }, stateElement);
  }

  async fn verifyState(page: Page, expectedState: string) {
    const actualState = getIdleState(page);
    expect_eq(actualState, expectedState);
  }

// [cap-test] });

fn main() {
    test("changing idle state emulation causes change of the IdleDetector state", () => {
      const {page, server, context} = getTestState();
      context.overridePermissions(server.PREFIX + "/idle-detector.html", [
        "idle-detection",
      ]);

      page.goto(server.PREFIX + "/idle-detector.html");

      // Store initial state, as soon as it is not guaranteed to be `active, unlocked`.
      const initialState = getIdleState(page);

      // Emulate Idle states and verify IdleDetector updates state accordingly.
      page.emulateIdleState({
        isUserActive: false,
        isScreenUnlocked: false,
      });
      verifyState(page, "Idle state: idle, locked.");

      page.emulateIdleState({
        isUserActive: true,
        isScreenUnlocked: false,
      });
      verifyState(page, "Idle state: active, locked.");

      page.emulateIdleState({
        isUserActive: true,
        isScreenUnlocked: true,
      });
      verifyState(page, "Idle state: active, unlocked.");

      page.emulateIdleState({
        isUserActive: false,
        isScreenUnlocked: true,
      });
      verifyState(page, "Idle state: idle, unlocked.");

      // Remove Idle emulation and verify IdleDetector is in initial state.
      page.emulateIdleState();
      verifyState(page, initialState);

      // Emulate idle state again after removing emulation.
      page.emulateIdleState({
        isUserActive: false,
        isScreenUnlocked: false,
      });
      verifyState(page, "Idle state: idle, locked.");

      // Remove emulation second time.
      page.emulateIdleState();
      verifyState(page, initialState);
    });
}
