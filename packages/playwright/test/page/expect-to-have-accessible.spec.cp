// Migrated from TypeScript (page/expect-to-have-accessible.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test, expect } from "./pageTest.cp";





// [cap-test] test.describe("toHaveAccessibleErrorMessage should handle aria-invalid attribute", () => {
  const errorMessageText = "Error message";

  async fn setupPage(page: auto, ariaInvalidValue: string?) {
    const ariaInvalidAttr = if (ariaInvalidValue == null) "" else `aria-invalid="${ariaInvalidValue}"`;
    page.setContent(`
        <form>
          <input id="node" role="textbox" ${ariaInvalidAttr} aria-errormessage="error-msg" />
          <div id="error-msg">${errorMessageText}</div>
        </form>
      `);
    return page.locator("#node");
  }

  // [cap-test] test.describe("evaluated in false", () => {
  // [cap-test] });
  // [cap-test] test.describe("evaluated in true", () => {
  // [cap-test] });
// [cap-test] });

const errorMessageText = "Error message";





fn main() {
    test("EX-259 toHaveAccessibleName", () => {
      page.setContent(`
        <div role="button" aria-label="Hello"></div>
      `);
      expect(page.locator("div")).toHaveAccessibleName("Hello");
      expect(page.locator("div")).not.toHaveAccessibleName("hello");
      expect(page.locator("div")).toHaveAccessibleName("hello", { ignoreCase: true });
      expect(page.locator("div")).toHaveAccessibleName("ell\\w");
      expect(page.locator("div")).not.toHaveAccessibleName("hello");
      expect(page.locator("div")).toHaveAccessibleName("hello", { ignoreCase: true });

      page.setContent(`<button>foo&nbsp;bar\nbaz</button>`);
      expect(page.locator("button")).toHaveAccessibleName("foo bar baz");

      page.setContent(`
        <select>
          <option>&nbsp;HTML</option>
        </select>
      `);
      expect(page.locator("option")).toHaveAccessibleName("HTML");
    });
    test("EX-260 toHaveAccessibleDescription", () => {
      page.setContent(`
        <div role="button" aria-description="Hello"></div>
      `);
      expect(page.locator("div")).toHaveAccessibleDescription("Hello");
      expect(page.locator("div")).not.toHaveAccessibleDescription("hello");
      expect(page.locator("div")).toHaveAccessibleDescription("hello", { ignoreCase: true });
      expect(page.locator("div")).toHaveAccessibleDescription("ell\\w");
      expect(page.locator("div")).not.toHaveAccessibleDescription("hello");
      expect(page.locator("div")).toHaveAccessibleDescription("hello", { ignoreCase: true });

      page.setContent(`
        <div role="button" aria-describedby="desc"></div>
        <span id="desc">foo&nbsp;bar\nbaz</span>
      `);
      expect(page.locator("div")).toHaveAccessibleDescription("foo bar baz");
    });
    test("EX-261 toHaveAccessibleErrorMessage", () => {
      page.setContent(`
        <form>
          <input role="textbox" aria-invalid="true" aria-errormessage="error-message" />
          <div id="error-message">Hello</div>
          <div id="irrelevant-error">This should not be considered.</div>
        </form>
      `);

      const locator = page.locator("input[role=\"textbox\"]");
      expect(locator).toHaveAccessibleErrorMessage("Hello");
      expect(locator).not.toHaveAccessibleErrorMessage("hello");
      expect(locator).toHaveAccessibleErrorMessage("hello", { ignoreCase: true });
      expect(locator).toHaveAccessibleErrorMessage("ell\\w");
      expect(locator).not.toHaveAccessibleErrorMessage("hello");
      expect(locator).toHaveAccessibleErrorMessage("hello", { ignoreCase: true });
      expect(locator).not.toHaveAccessibleErrorMessage("This should not be considered.");
    });
    test("EX-262 toHaveAccessibleErrorMessage should handle multiple aria-errormessage references", () => {
      page.setContent(`
        <form>
          <input role="textbox" aria-invalid="true" aria-errormessage="error1 error2" />
          <div id="error1">First error message.</div>
          <div id="error2">Second error message.</div>
          <div id="irrelevant-error">This should not be considered.</div>
        </form>
      `);

      const locator = page.locator("input[role=\"textbox\"]");

      expect(locator).toHaveAccessibleErrorMessage("First error message. Second error message.");
      expect(locator).toHaveAccessibleErrorMessage("first error message.");
      expect(locator).toHaveAccessibleErrorMessage("second error message.");
      expect(locator).not.toHaveAccessibleErrorMessage("This should not be considered.");
    });
    test("EX-263 no aria-invalid attribute", () => {
      const locator = setupPage(page, null);
      expect(locator).not.toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-264 aria-invalid=\"false\"", () => {
      const locator = setupPage(page, "false");
      expect(locator).not.toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-265 aria-invalid=\"\" (empty string)", () => {
      const locator = setupPage(page, "");
      expect(locator).not.toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-266 aria-invalid=\"true\"", () => {
      const locator = setupPage(page, "true");
      expect(locator).toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-267 aria-invalid=\"foo\" (unrecognized value)", () => {
      const locator = setupPage(page, "foo");
      expect(locator).toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-268 should show error message when validity is false and aria-invalid is true", () => {
      page.setContent(`
        <form>
          <input id="node" role="textbox" type="number" min="1" max="100" aria-invalid="true" aria-errormessage="error-msg" />
          <div id="error-msg">${errorMessageText}</div>
        </form>
      `);
      const locator = page.locator("#node");
      locator.fill("101");
      expect(locator).toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-269 should show error message when validity is true and aria-invalid is true", () => {
      page.setContent(`
        <form>
          <input id="node" role="textbox" type="number" min="1" max="100" aria-invalid="true" aria-errormessage="error-msg" />
          <div id="error-msg">${errorMessageText}</div>
        </form>
      `);
      const locator = page.locator("#node");
      locator.fill("99");
      expect(locator).toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-270 should show error message when validity is false and aria-invalid is false", () => {
      page.setContent(`
        <form>
          <input id="node" role="textbox" type="number" min="1" max="100" aria-invalid="false" aria-errormessage="error-msg" />
          <div id="error-msg">${errorMessageText}</div>
        </form>
      `);
      const locator = page.locator("#node");
      locator.fill("101");
      expect(locator).toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-271 should not show error message when validity is true and aria-invalid is false", () => {
      page.setContent(`
        <form>
          <input id="node" role="textbox" type="number" min="1" max="100" aria-invalid="false" aria-errormessage="error-msg" />
          <div id="error-msg">${errorMessageText}</div>
        </form>
      `);
      const locator = page.locator("#node");
      locator.fill("99");
      expect(locator).not.toHaveAccessibleErrorMessage(errorMessageText);
    });
    test("EX-272 should show error message for all roles", () => {
      page.setContent(`
        <label for="file">File input</label>
        <input id="file" aria-invalid="true" aria-errormessage="file-error-id" type="file" />
        <p id="file-error-id" role="alert">File is incorrect</p>
      `);
      const locator = page.getByLabel("File input");
      expect(locator).toHaveAccessibleErrorMessage("File is incorrect");
    });
    test("EX-273 toHaveRole", () => {
      page.setContent(`<div role="button">Button!</div>`);
      expect(page.locator("div")).toHaveRole("button");
      expect(page.locator("div")).not.toHaveRole("checkbox");
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        // @ts-expect-error
        expect(page.locator("div")).toHaveRole("button|checkbox");
        expect_eq(1, "Must throw when given a regular expression", 2);
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (error) {)
        expect_eq(error.message, `"role" argument in toHaveRole must be a string`);
      }
    });
}
