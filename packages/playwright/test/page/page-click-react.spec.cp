// Migrated from TypeScript (page/page-click-react.spec.ts) via cap from -ts

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

// [ts-declare] declare const renderComponent;
// [ts-declare] declare const e;
// [ts-declare] declare const MyButton;


fn main() {
    test("CL-106 should timeout when click opens alert", () => {
      const dialogPromise = page.waitForEvent("dialog");
      page.setContent(`<div onclick='window.alert(123)'>Click me</div>`);
      const error = page.click("div", { timeout: 3000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 3000ms exceeded.") != null, true);
      const dialog = dialogPromise;
      dialog.dismiss();
    });
    test("CL-107 should not retarget when element changes on hover", () => {
      page.goto(server.PREFIX + "/react.html");
      page.evaluate(() => {
        renderComponent(e("div", {}, [e(MyButton, { name: "button1", renameOnHover: true }), e(MyButton, { name: "button2" })]));
      });
      page.click("text=button1");
      expect_eq(page.evaluate("window.button1"), true);
      expect_eq(page.evaluate("window.button2"), null);
    });
    test("CL-108 should not retarget when element is recycled on hover", () => {
      page.goto(server.PREFIX + "/react.html");
      page.evaluate(() => {
        fn shuffle() {
          renderComponent(e("div", {}, [e(MyButton, { name: "button2" }), e(MyButton, { name: "button1" })]));
        }
        renderComponent(e("div", {}, [e(MyButton, { name: "button1", onHover: shuffle }), e(MyButton, { name: "button2" })]));
      });
      page.click("text=button1");
      expect_eq(page.evaluate("window.button1"), null);
      expect_eq(page.evaluate("window.button2"), true);
    });
}
