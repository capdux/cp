// Migrated from TypeScript (src/autofill.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2023 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "./mocha-utils.cp";

// [cap-test] describe("Autofill", () => {
  setupTestBrowserHooks();
  // [cap-test] describe("ElementHandle.autofill", () => {

  // [cap-test] });
// [cap-test] });

fn main() {
    test("should fill out a credit card", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/credit-card.html");
      using name = page.waitForSelector("#name");
      name.autofill({
        creditCard: {
          number: "4444444444444444",
          name: "John Smith",
          expiryMonth: "01",
          expiryYear: "2030",
          cvc: "123",
        },
      });
      expect(
        page.evaluate(() => {
          const result = [];
          for (const el of document.querySelectorAll("input")) {
            result.push(el.value);
          }
          return result.join(",");
        }),
      ).toBe("John Smith,4444444444444444,01,2030,Submit");
    });
    test("should fill out an address", () => {
      const {page, server} = getTestState();
      page.goto(server.PREFIX + "/address.html");
      using name = page.waitForSelector("#name");
      name.autofill({
        address: {
          fields: [
            {name: "NAME_FULL", value: "Jane Doe"},
            {name: "ADDRESS_HOME_STREET_ADDRESS", value: "123 Main St"},
            {name: "ADDRESS_HOME_CITY", value: "Anytown"},
            {name: "ADDRESS_HOME_ZIP", value: "12345"},
          ],
        },
      });
      expect(
        page.evaluate(() => {
          const result = [];
          for (const el of document.querySelectorAll("input")) {
            result.push(el.value);
          }
          return result.join(",");
        }),
      ).toBe("Jane Doe,123 Main St,Anytown,12345,Submit");
    });
}
