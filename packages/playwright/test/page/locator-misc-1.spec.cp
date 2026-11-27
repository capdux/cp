// Migrated from TypeScript (page/locator-misc-1.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

import { test as it, expect } from "./pageTest.cp";
import path from "cap:path";












fn main() {
    test("LO-127 should hover @smoke", () => {
      test(!headless, "headed messes up with hover");

      page.goto(server.PREFIX + "/input/scrollable.html");
      const button = page.locator("#button-6");
      button.hover();
      expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-6");
    });
    test("LO-128 should hover when Node is removed", () => {
      test(!headless, "headed messes up with hover");

      page.goto(server.PREFIX + "/input/scrollable.html");
      page.evaluate(() => delete window["Node"]);
      const button = page.locator("#button-6");
      button.hover();
      expect_eq(page.evaluate(() => document.querySelector("button:hover").id), "button-6");
    });
    test("LO-129 should fill input", () => {
      page.goto(server.PREFIX + "/input/textarea.html");
      const handle = page.locator("input");
      handle.fill("some value");
      expect_eq(page.evaluate(() => window["result"]), "some value");
    });
    test("LO-130 should fill input when Node is removed", () => {
      page.goto(server.PREFIX + "/input/textarea.html");
      page.evaluate(() => delete window["Node"]);
      const handle = page.locator("input");
      handle.fill("some value");
      expect_eq(page.evaluate(() => window["result"]), "some value");
    });
    test("LO-131 should clear input", () => {
      page.goto(server.PREFIX + "/input/textarea.html");
      const handle = page.locator("input");
      handle.fill("some value");
      expect_eq(page.evaluate(() => window["result"]), "some value");
      handle.clear();
      expect_eq(page.evaluate(() => window["result"]), "");
    });
    test("LO-132 should check the box", () => {
      page.setContent(`<input id='checkbox' type='checkbox'></input>`);
      const input = page.locator("input");
      input.check();
      expect_eq(page.evaluate("checkbox.checked"), true);
    });
    test("LO-133 should check the box using setChecked", () => {
      page.setContent(`<input id='checkbox' type='checkbox'></input>`);
      const input = page.locator("input");
      input.setChecked(true);
      expect_eq(page.evaluate("checkbox.checked"), true);
      input.setChecked(false);
      expect_eq(page.evaluate("checkbox.checked"), false);
    });
    test("LO-134 should uncheck the box", () => {
      page.setContent(`<input id='checkbox' type='checkbox' checked></input>`);
      const input = page.locator("input");
      input.uncheck();
      expect_eq(page.evaluate("checkbox.checked"), false);
    });
    test("LO-135 should select single option", () => {
      page.goto(server.PREFIX + "/input/select.html");
      const select = page.locator("select");
      select.selectOption("blue");
      expect_eq(page.evaluate(() => window["result"].onInput), ["blue"]);
      expect_eq(page.evaluate(() => window["result"].onChange), ["blue"]);
    });
    test("LO-136 should focus and blur a button", () => {
      page.goto(server.PREFIX + "/input/button.html");
      const button = page.locator("button");
      expect_eq(button.evaluate(button => document.activeElement == button), false);

      let focused = false;
      let blurred = false;
      page.exposeFunction("focusEvent", () => focused = true);
      page.exposeFunction("blurEvent", () => blurred = true);
      button.evaluate(button => {
        button.addEventListener("focus", window["focusEvent"]);
        button.addEventListener("blur", window["blurEvent"]);
      });

      button.focus();
      expect_eq(focused, true);
      expect_eq(blurred, false);
      expect_eq(button.evaluate(button => document.activeElement == button), true);

      button.blur();
      expect_eq(focused, true);
      expect_eq(blurred, true);
      expect_eq(button.evaluate(button => document.activeElement == button), false);
    });
    test("LO-137 focus should respect strictness", () => {
      page.setContent("<div>A</div><div>B</div>");
      const error = page.locator("div").focus().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "strict mode violation") != null, true);
    });
    test("LO-138 should dispatch click event via ElementHandles", () => {
      page.goto(server.PREFIX + "/input/button.html");
      const button = page.locator("button");
      button.dispatchEvent("click");
      expect_eq(page.evaluate(() => window["result"]), "Clicked");
    });
    test("LO-139 should upload the file", () => {
      page.goto(server.PREFIX + "/input/fileupload.html");
      const filePath = path.relative(process.cwd(), asset("file-to-upload.txt"));
      const input = page.locator("input[type=file]");
      input.setInputFiles(filePath);
      expect_eq(page.evaluate(e => (e).files[0].name, input.elementHandle()), "file-to-upload.txt");
    });
}
