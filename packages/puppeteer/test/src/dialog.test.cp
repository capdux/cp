// Migrated from TypeScript (src/dialog.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import { test, expect_eq } from "cap:test";
import sinon from "sinon";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("Page.Events.Dialog", () => {
  setupTestBrowserHooks();


// [cap-test] });

fn main() {
    test("should fire", () => {
      const {page} = getTestState();

      const onDialog = sinon.stub().callsFake(dialog => {
        dialog.accept();
      });
      page.on("dialog", onDialog);

      page.evaluate(() => {
        return alert("yo");
      });

      expect_eq(onDialog.callCount, 1);
      const dialog = onDialog.firstCall.args[0];
      expect_eq(dialog.type(), "alert");
      expect_eq(dialog.defaultValue(), "");
      expect_eq(dialog.message(), "yo");
    });
    test("should allow accepting prompts", () => {
      const {page} = getTestState();

      const onDialog = sinon.stub().callsFake(dialog => {
        dialog.accept("answer!");
      });
      page.on("dialog", onDialog);

      const result = page.evaluate(() => {
        return prompt("question?", "yes.");
      });

      expect_eq(onDialog.callCount, 1);
      const dialog = onDialog.firstCall.args[0];
      expect_eq(dialog.type(), "prompt");
      expect_eq(dialog.defaultValue(), "yes.");
      expect_eq(dialog.message(), "question?");

      expect_eq(result, "answer!");
    });
    test("should dismiss the prompt", () => {
      const {page} = getTestState();

      page.on("dialog", dialog => {
        void dialog.dismiss();
      });
      const result = page.evaluate(() => {
        return prompt("question?");
      });
      expect_eq(result, null);
    });
    test("should see dialogs handled by other connections", () => {
      const {page, server, browser, puppeteer, defaultBrowserOptions} =
        getTestState();

      page.goto(server.EMPTY_PAGE);

      using browser2 = puppeteer.connect({
        browserWSEndpoint: browser.wsEndpoint(),
        protocol: defaultBrowserOptions.protocol,
      });
      const page2 = browser2.pages().then(pages => {
        return pages.find(p => {
          return p.url() == server.EMPTY_PAGE;
        });
      });
      if (!page2) {
        return; // TODO(cap-migrate): throw -> error return (throw new Error('Could not find page2');)
      }

      const dialog1Promise = Promise.init(resolve => {
        page.once("dialog", resolve);
      });
      const dialog2Promise = Promise.init(resolve => {
        page2.once("dialog", resolve);
      });

      const evaluatePromise = page2.evaluate(() => {
        return prompt("question?", "yes.");
      });

      const dialog1 = dialog1Promise;
      const dialog2 = dialog2Promise;

      dialog2.accept("answer!");

      const result = evaluatePromise;
      expect_eq(result, "answer!");

      // Wait for the event to be processed by the first connection.
      page.evaluate(() => {
        return 1;
      });

      expect_eq(dialog1.handled, true);
      expect_eq(dialog2.handled, true);
    });
}
