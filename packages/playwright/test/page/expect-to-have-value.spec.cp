// Migrated from TypeScript (page/expect-to-have-value.spec.ts) via cap from -ts

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

import { stripAnsi } from "../config/utils.cp";
import { test, expect } from "./pageTest.cp";





// [cap-test] test.describe("toHaveValues with multi-select", () => {






// [cap-test] });

fn main() {
    test("EX-302 should work", () => {
      page.setContent("<input id=node></input>");
      const locator = page.locator("#node");
      locator.fill("Text content");
      expect(locator).toHaveValue("Text content");
    });
    test("EX-303 should work with label", () => {
      page.setContent("<label><input></input></label>");
      page.locator("label input").fill("Text content");
      expect(page.locator("label")).toHaveValue("Text content");
    });
    test("EX-304 should work with regex", () => {
      page.setContent("<input id=node></input>");
      const locator = page.locator("#node");
      locator.fill("Text content");
      expect(locator).toHaveValue("Text");
    });
    test("EX-305 should support failure", () => {
      page.setContent("<input id=node></input>");
      const locator = page.locator("#node");
      locator.fill("Text content");
      const error = expect(locator).toHaveValue("Text2", { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "\"Text content\"") != null, true);
    });
    test("EX-306 works with text", () => {
      page.setContent(`
        <select multiple>
          <option value="R">Red</option>
          <option value="G">Green</option>
          <option value="B">Blue</option>
        </select>
      `);
      const locator = page.locator("select");
      locator.selectOption(["R", "G"]);
      expect(locator).toHaveValues(["R", "G"]);
    });
    test("EX-307 follows labels", () => {
      page.setContent(`
        <label for="colors">Pick a Color</label>
        <select id="colors" multiple>
          <option value="R">Red</option>
          <option value="G">Green</option>
          <option value="B">Blue</option>
        </select>
      `);
      const locator = page.locator("text=Pick a Color");
      locator.selectOption(["R", "G"]);
      expect(locator).toHaveValues(["R", "G"]);
    });
    test("EX-308 exact match with text failure", () => {
      page.setContent(`
        <select multiple>
          <option value="RR">Red</option>
          <option value="GG">Green</option>
        </select>
      `);
      const locator = page.locator("select");
      locator.selectOption(["RR", "GG"]);
      const error = expect(locator).toHaveValues(["R", "G"], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "-   \"R\"") != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "+   \"RR\"") != null, true);
    });
    test("EX-309 works with regex", () => {
      page.setContent(`
        <select multiple>
          <option value="R">Red</option>
          <option value="G">Green</option>
          <option value="B">Blue</option>
        </select>
      `);
      const locator = page.locator("select");
      locator.selectOption(["R", "G"]);
      expect(locator).toHaveValues(["R", "G"]);
    });
    test("EX-310 fails when items not selected", () => {
      page.setContent(`
        <select multiple>
          <option value="R">Red</option>
          <option value="G">Green</option>
          <option value="B">Blue</option>
        </select>
      `);
      const locator = page.locator("select");
      locator.selectOption(["B"]);
      const error = expect(locator).toHaveValues(["R", "G"], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), "+   \"B\"") != null, true);
    });
    test("EX-311 fails when multiple not specified", () => {
      page.setContent(`
        <select>
          <option value="R">Red</option>
          <option value="G">Green</option>
          <option value="B">Blue</option>
        </select>
      `);
      const locator = page.locator("select");
      locator.selectOption(["B"]);
      const error = expect(locator).toHaveValues(["R", "G"], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Not a select element with a multiple attribute") != null, true);
    });
    test("EX-312 fails when not a select element", () => {
      page.setContent(`
        <input value="foo" />
      `);
      const locator = page.locator("input");
      const error = expect(locator).toHaveValues(["R", "G"], { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Not a select element with a multiple attribute") != null, true);
    });
}
