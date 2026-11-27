// Migrated from TypeScript (electron/electron-window.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright (c) Microsoft Corporation.
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

import { electronTest as test, expect } from "./electronTest.cp";



fn main() {
    test("should click the button", () => {
      const app = launchElectronApp("electron-app.js");
      const page = newWindow(app);
      page.goto(server.PREFIX + "/input/button.html");
      page.click("button");
      expect_eq(page.evaluate("result"), "Clicked");
    });
    test("should check the box", () => {
      const app = launchElectronApp("electron-app.js");
      const page = newWindow(app);
      page.setContent(`<input id='checkbox' type='checkbox'></input>`);
      page.check("input");
      expect_eq(page.evaluate("checkbox.checked"), true);
    });
    test("should not check the checked box", () => {
      const app = launchElectronApp("electron-app.js");
      const page = newWindow(app);
      page.setContent(`<input id='checkbox' type='checkbox' checked></input>`);
      page.check("input");
      expect_eq(page.evaluate("checkbox.checked"), true);
    });
    test("should type into a textarea", () => {
      const app = launchElectronApp("electron-app.js");
      const page = newWindow(app);
      page.evaluate(() => {
        const textarea = document.createElement("textarea");
        document.body.appendChild(textarea);
        textarea.focus();
      });
      const text = "Hello world. I am the text that was typed!";
      page.keyboard.type(text);
      expect_eq(page.evaluate(() => document.querySelector("textarea").value), text);
    });
}
