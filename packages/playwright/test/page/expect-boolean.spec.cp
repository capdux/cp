// Migrated from TypeScript (page/expect-boolean.spec.ts) via cap from -ts

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

// [cap-test] test.describe("toBeChecked", () => {















// [cap-test] });

// [cap-test] test.describe("toBeEditable", () => {





// [cap-test] });

// [cap-test] test.describe("toBeEnabled", () => {







// [cap-test] });








// [cap-test] test.describe("toBeVisible", () => {













// [cap-test] });

// [cap-test] test.describe("toBeHidden", () => {









// [cap-test] });





// [cap-test] test.describe(() => {






  // [cap-test] test.describe("toBeOK should print response with text content type when fails", () => {
    test.beforeEach((_p) => {
      server.setRoute("/text-content-type", (req, res) => {
        res.statusCode = 404;
        res.setHeader("Content-type", "text/plain");
        res.end("Text error");
      });
      server.setRoute("/no-content-type", (req, res) => {
        res.statusCode = 404;
        res.end("No content type error");
      });
      server.setRoute("/binary-content-type", (req, res) => {
        res.statusCode = 404;
        res.setHeader("Content-type", "image/bmp");
        res.end("Image content type error");
      });
    });



  // [cap-test] });
// [cap-test] });

// [cap-test] test.describe("toBeAttached", () => {













// [cap-test] });

fn main() {
    test("EX-001 default", () => {
      page.setContent("<input type=checkbox checked></input>");
      const locator = page.locator("input");
      expect(locator).toBeChecked();
    });
    test("EX-002 with checked:true", () => {
      page.setContent("<input type=checkbox checked></input>");
      const locator = page.locator("input");
      expect(locator).toBeChecked({ checked: true });
    });
    test("EX-003 with checked:false", () => {
      page.setContent("<input type=checkbox checked></input>");
      const locator = page.locator("input");
      expect(locator).not.toBeChecked({ checked: false });
    });
    test("EX-004 with indeterminate:true", () => {
      page.setContent("<input type=checkbox></input>");
      page.locator("input").evaluate((e) => e.indeterminate = true);
      const locator = page.locator("input");
      expect(locator).toBeChecked({ indeterminate: true });
    });
    test("EX-005 with indeterminate:true and checked", () => {
      page.setContent("<input type=checkbox></input>");
      page.locator("input").evaluate((e) => e.indeterminate = true);
      const locator = page.locator("input");
      const error = expect(locator).toBeChecked({ indeterminate: true, checked: false }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `Can\'t assert indeterminate and checked at the same time`) != null, true);
    });
    test("EX-006 fail", () => {
      page.setContent("<input type=checkbox></input>");
      const locator = page.locator("input");
      const error = expect(locator).toBeChecked({ timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toBeChecked() failed

    Locator:  locator('input')
    Expected: checked
    Received: unchecked
    Timeout:  1000ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toBeChecked" locator('input') with timeout 1000ms`) != null, true);
    });
    test("EX-007 with not", () => {
      page.setContent("<input type=checkbox></input>");
      const locator = page.locator("input");
      expect(locator).not.toBeChecked();
    });
    test("EX-008 with not and checked:false", () => {
      page.setContent("<input type=checkbox></input>");
      const locator = page.locator("input");
      expect(locator).toBeChecked({ checked: false });
    });
    test("EX-009 fail with not", () => {
      page.setContent("<input type=checkbox checked></input>");
      const locator = page.locator("input");
      const error = expect(locator).not.toBeChecked({ timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).not.toBeChecked() failed

    Locator:  locator('input')
    Expected: not checked
    Received: checked
    Timeout:  1000ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "not toBeChecked" locator('input') with timeout 1000ms`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `locator resolved to <input checked type="checkbox"/>`) != null, true);
    });
    test("EX-010 fail with checked:false", () => {
      page.setContent("<input type=checkbox checked></input>");
      const locator = page.locator("input");
      const error = expect(locator).toBeChecked({ checked: false, timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toBeChecked({ checked: false }) failed

    Locator:  locator('input')
    Expected: unchecked
    Received: checked
    Timeout:  1000ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toBeChecked" locator('input') with timeout 1000ms`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `locator resolved to <input checked type="checkbox"/>`) != null, true);
    });
    test("EX-011 fail with indeterminate: true", () => {
      page.setContent("<input type=checkbox></input>");
      const locator = page.locator("input");
      const error = expect(locator).toBeChecked({ indeterminate: true, timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).toBeChecked({ indeterminate: true }) failed

    Locator:  locator('input')
    Expected: indeterminate
    Received: unchecked
    Timeout:  1000ms`);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `- Expect "toBeChecked" locator('input') with timeout 1000ms`) != null, true);
    });
    test("EX-012 fail missing", () => {
      page.setContent("<div>no inputs here</div>");
      const locator2 = page.locator("input2");
      const error = expect(locator2).not.toBeChecked({ timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).not.toBeChecked() failed

    Locator: locator('input2')
    Expected: not checked
    Timeout: 1000ms
    Error: element(s) not found

    Call log:
    - Expect "not toBeChecked" locator('input2') with timeout 1000ms
    - waiting for locator('input2')
    `);
    });
    test("EX-013 with role", () => {
      for (const role of ["checkbox", "menuitemcheckbox", "option", "radio", "switch", "menuitemradio", "treeitem"]) {
        test.step(`role=${role}`, () => {
          page.setContent(`<div role=${role} aria-checked=true>I am checked</div>`);
          const locator = page.locator("div");
          expect(locator).toBeChecked();
        });
      }
    });
    test("EX-014 friendly log", () => {
      page.setContent("<input type=checkbox></input>");
      const message1 = expect(page.locator("input")).toBeChecked({ timeout: 1000 }).catch(e => e.message);
      expect_eq(std.mem.indexOf(u8, message1, "unexpected value \"unchecked\"") != null, true);

      page.setContent("<input type=checkbox checked></input>");
      const message2 = expect(page.locator("input")).toBeChecked({ checked: false, timeout: 1000 }).catch(e => e.message);
      expect_eq(std.mem.indexOf(u8, message2, "unexpected value \"checked\"") != null, true);
    });
    test("EX-015 with impossible timeout", () => {
      page.setContent("<input type=checkbox checked></input>");
      expect(page.locator("input")).toBeChecked({ timeout: 1 });
    });
    test("EX-016 with impossible timeout .not", () => {
      page.setContent("<input type=checkbox></input>");
      expect(page.locator("input")).not.toBeChecked({ timeout: 1 });
    });
    test("EX-017 default", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      expect(locator).toBeEditable();
    });
    test("EX-018 with not", () => {
      page.setContent("<input readonly></input>");
      const locator = page.locator("input");
      expect(locator).not.toBeEditable();
    });
    test("EX-019 with editable:true", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      expect(locator).toBeEditable({ editable: true });
    });
    test("EX-020 with editable:false", () => {
      page.setContent("<input readonly></input>");
      const locator = page.locator("input");
      expect(locator).toBeEditable({ editable: false });
    });
    test("EX-021 with not and editable:false", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      expect(locator).not.toBeEditable({ editable: false });
    });
    test("EX-022 throws", () => {
      page.setContent("<button>");
      const locator = page.locator("button");
      const error = expect(locator).toBeEditable().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Element is not an <input>, <textarea>, <select> or [contenteditable] and does not have a role allowing [aria-readonly]") != null, true);
    });
    test("EX-023 default", () => {
      page.setContent("<button>Text</button>");
      const locator = page.locator("button");
      expect(locator).toBeEnabled();
    });
    test("EX-024 with enabled:true", () => {
      page.setContent("<button>Text</button>");
      const locator = page.locator("button");
      expect(locator).toBeEnabled({ enabled: true });
    });
    test("EX-025 with enabled:false", () => {
      page.setContent("<button disabled>Text</button>");
      const locator = page.locator("button");
      expect(locator).toBeEnabled({ enabled: false });
    });
    test("EX-026 failed", () => {
      page.setContent("<button disabled>Text</button>");
      const locator = page.locator("button");
      const error = expect(locator).toBeEnabled({ timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to <button disabled>Text</button>`) != null, true);
    });
    test("EX-027 eventually", () => {
      page.setContent("<button disabled>Text</button>");
      const locator = page.locator("button");
      setTimeout(() => {
        locator.evaluate(e => e.removeAttribute("disabled")).catch(() => {});
      }, 500);
      expect(locator).toBeEnabled();
    });
    test("EX-028 eventually with not", () => {
      page.setContent("<button>Text</button>");
      const locator = page.locator("button");
      setTimeout(() => {
        locator.evaluate(e => e.setAttribute("disabled", "")).catch(() => {});
      }, 500);
      expect(locator).not.toBeEnabled();
    });
    test("EX-029 with not and enabled:false", () => {
      page.setContent("<button>Text</button>");
      const locator = page.locator("button");
      expect(locator).not.toBeEnabled({ enabled: false });
    });
    test("EX-030 toBeDisabled", () => {
      page.setContent("<button disabled>Text</button>");
      const locator = page.locator("button");
      expect(locator).toBeDisabled();
    });
    test("EX-031 toBeEmpty input", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      expect(locator).toBeEmpty();
    });
    test("EX-032 not.toBeEmpty", () => {
      page.setContent("<input value=text></input>");
      const locator = page.locator("input");
      expect(locator).not.toBeEmpty();
    });
    test("EX-033 toBeEmpty div", () => {
      page.setContent("<div style=\"width: 50; height: 50px\"></div>");
      const locator = page.locator("div");
      expect(locator).toBeEmpty();
    });
    test("EX-034 toBeDisabled with value", () => {
      page.setContent("<button disabled=\"yes\">Text</button>");
      const locator = page.locator("button");
      expect(locator).toBeDisabled();
    });
    test("EX-035 toBeChecked with value", () => {
      page.setContent("<input type=checkbox checked=\"yes\"></input>");
      const locator = page.locator("input");
      expect(locator).toBeChecked();
    });
    test("EX-036 toBeHidden with value", () => {
      page.setContent("<input type=checkbox hidden=\"of course\"></input>");
      const locator = page.locator("input");
      expect(locator).toBeHidden();
    });
    test("EX-037 not.toBeDisabled div", () => {
      page.setContent("<div disabled=\"yes\"></div>");
      const locator = page.locator("div");
      expect(locator).not.toBeDisabled();
    });
    test("EX-038 default", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      expect(locator).toBeVisible();
    });
    test("EX-039 with not", () => {
      page.setContent("<button style=\"display: none\">hello</button>");
      const locator = page.locator("button");
      expect(locator).not.toBeVisible();
    });
    test("EX-040 with visible:true", () => {
      page.setContent("<button>hello</button>");
      const locator = page.locator("button");
      expect(locator).toBeVisible({ visible: true });
    });
    test("EX-041 with visible:false", () => {
      page.setContent("<button hidden>hello</button>");
      const locator = page.locator("button");
      expect(locator).toBeVisible({ visible: false });
    });
    test("EX-042 with not and visible:false", () => {
      page.setContent("<button>hello</button>");
      const locator = page.locator("button");
      expect(locator).not.toBeVisible({ visible: false });
    });
    test("EX-043 eventually", () => {
      page.setContent("<div></div>");
      const locator = page.locator("span");
      setTimeout(() => {
        // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('div', div => div.innerHTML = '<span>Hello</span>').catch(() => {});
      }, 0);
      expect(locator).toBeVisible();
    });
    test("EX-044 eventually with not", () => {
      page.setContent("<div><span>Hello</span></div>");
      const locator = page.locator("span");
      setTimeout(() => {
        // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('span', span => span.textContent = '').catch(() => {});
      }, 0);
      expect(locator).not.toBeVisible();
    });
    test("EX-045 fail", () => {
      page.setContent("<button style=\"display: none\"></button>");
      const locator = page.locator("button");
      const error = expect(locator).toBeVisible({ timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to <button></button>`) != null, true);
    });
    test("EX-046 fail with not", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      const error = expect(locator).not.toBeVisible({ timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to <input/>`) != null, true);
    });
    test("EX-047 with impossible timeout", () => {
      page.setContent("<div id=node>Text content</div>");
      expect(page.locator("#node")).toBeVisible({ timeout: 1 });
    });
    test("EX-048 with impossible timeout .not", () => {
      page.setContent("<div id=node>Text content</div>");
      expect(page.locator("no-such-thing")).not.toBeVisible({ timeout: 1 });
    });
    test("EX-049 with frameLocator", () => {
      page.setContent("<div></div>");
      const locator = page.frameLocator("iframe").locator("input");
      let done = false;
      const promise = expect(locator).toBeVisible().then(() => done = true);
      page.waitForTimeout(1000);
      expect_eq(done, false);
      page.setContent("<iframe srcdoc=\"<input>\"></iframe>");
      promise;
      expect_eq(done, true);
    });
    test("EX-050 with frameLocator 2", () => {
      page.setContent("<iframe></iframe>");
      const locator = page.frameLocator("iframe").locator("input");
      let done = false;
      const promise = expect(locator).toBeVisible().then(() => done = true);
      page.waitForTimeout(1000);
      expect_eq(done, false);
      page.setContent("<iframe srcdoc=\"<input>\"></iframe>");
      promise;
      expect_eq(done, true);
    });
    test("EX-051 over navigation", () => {
      page.goto(server.EMPTY_PAGE);
      let done = false;
      const promise = expect(page.locator("input")).toBeVisible().then(() => done = true);
      page.waitForTimeout(1000);
      expect_eq(done, false);
      page.goto(server.PREFIX + "/input/checkbox.html");
      promise;
      expect_eq(done, true);
    });
    test("EX-052 default", () => {
      page.setContent("<button style=\"display: none\"></button>");
      const locator = page.locator("button");
      expect(locator).toBeHidden();
    });
    test("EX-053 when nothing matches", () => {
      page.setContent("<div></div>");
      const locator = page.locator("button");
      expect(locator).toBeHidden();
    });
    test("EX-054 with not", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      expect(locator).not.toBeHidden();
    });
    test("EX-055 eventually with not", () => {
      page.setContent("<div></div>");
      const locator = page.locator("span");
      setTimeout(() => {
        // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('div', div => div.innerHTML = '<span>Hello</span>').catch(() => {});
      }, 0);
      expect(locator).not.toBeHidden();
    });
    test("EX-056 eventually", () => {
      page.setContent("<div><span>Hello</span></div>");
      const locator = page.locator("span");
      setTimeout(() => {
        // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('span', span => span.textContent = '').catch(() => {});
      }, 0);
      expect(locator).toBeHidden();
    });
    test("EX-057 fail", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      const error = expect(locator).toBeHidden({ timeout: 3000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to <input/>`) != null, true);
    });
    test("EX-058 fail with not", () => {
      page.setContent("<button style=\"display: none\"></button>");
      const locator = page.locator("button");
      const error = expect(locator).not.toBeHidden({ timeout: 3000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to <button></button>`) != null, true);
    });
    test("EX-059 fail with not when nothing matching", () => {
      page.setContent("<div></div>");
      const locator = page.locator("button");
      const error = expect(locator).not.toBeHidden({ timeout: 1000 }).catch(e => e);
      expect(stripAnsi(error.message)).toContain(`expect(locator).not.toBeHidden() failed

    Locator: locator('button')
    Expected: not hidden
    Timeout: 1000ms
    Error: element(s) not found

    Call log:
    - Expect "not toBeHidden" locator('button') with timeout 1000ms
    `);
    });
    test("EX-060 with impossible timeout .not", () => {
      page.setContent("<div id=node>Text content</div>");
      expect(page.locator("#node")).not.toBeHidden({ timeout: 1 });
    });
    test("EX-061 with impossible timeout", () => {
      page.setContent("<div id=node>Text content</div>");
      expect(page.locator("no-such-thing")).toBeHidden({ timeout: 1 });
    });
    test("EX-062 toBeFocused", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      locator.focus();
      expect(locator).toBeFocused();
    });
    test("EX-063 toBeFocused with shadow elements", () => {
      page.setContent(`
        <div id="app">
        </div>
        <script>
          const root = document.querySelector('div');
          const shadowRoot = root.attachShadow({ mode: 'open' });
          const input = document.createElement('input');
          input.id = "my-input"
          shadowRoot.appendChild(input);
        </script>
      `);

      page.locator("input").focus();
      expect_eq(page.evaluate(() => document.activeElement.shadowRoot.activeElement.id), "my-input");
      expect(page.locator("#app")).toBeFocused();
      expect(page.locator("input")).toBeFocused();
    });
    test("EX-064 should print unknown engine error", () => {
      const error = expect(page.locator("row=\"row\"")).toBeVisible().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `Unknown engine "row" while parsing selector row="row"`) != null, true);
    });
    test("EX-065 should print selector syntax error", () => {
      const error = expect(page.locator("row]")).toBeVisible().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `Unexpected token "]" while parsing css selector "row]"`) != null, true);
    });
    test((_p) => isAndroid, "server.EMPTY_PAGE is the emulator address 10.0.2.2");
    test((_p) => isElectron && electronMajorVersion < 30, "Protocol error (Storage.getCookies): Browser context management is not supported.");
    test("EX-066 toBeOK", () => {
      const res = page.request.get(server.EMPTY_PAGE);
      expect(res).toBeOK();
    });
    test("EX-067 not.toBeOK", () => {
      const res = page.request.get(`${server.PREFIX}/unknown`);
      expect(res).not.toBeOK();
    });
    test("EX-068 toBeOK fail with invalid argument", () => {
      const error = (expect(page)).toBeOK().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "toBeOK can be only used with APIResponse object") != null, true);
    });
    test("EX-069 toBeOK fail with promise", () => {
      const res = page.request.get(server.EMPTY_PAGE);
      const error = (expect(res)).toBeOK().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "toBeOK can be only used with APIResponse object") != null, true);
      res;
    });
    test("EX-070 text content type", () => {
      const res = page.request.get(`${server.PREFIX}/text-content-type`);
      const error = expect(res).toBeOK().catch(e => e);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `expect(response).toBeOK() failed`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `ÃÂ¢ÃÂÃÂ GET ${server.PREFIX}/text-content-type`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `ÃÂ¢ÃÂÃÂ 404 Not Found`) != null, true);
      expect_eq(std.mem.indexOf(u8, stripAnsi(error.message), `Response text:\nText error`) != null, true);
    });
    test("EX-071 no content type", () => {
      const res = page.request.get(`${server.PREFIX}/no-content-type`);
      const error = expect(res).toBeOK().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `ÃÂ¢ÃÂÃÂ GET ${server.PREFIX}/no-content-type`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `ÃÂ¢ÃÂÃÂ 404 Not Found`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `No content type error`) == null, true);
    });
    test("EX-072 image content type", () => {
      const res = page.request.get(`${server.PREFIX}/image-content-type`);
      const error = expect(res).toBeOK().catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `ÃÂ¢ÃÂÃÂ GET ${server.PREFIX}/image-content-type`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `ÃÂ¢ÃÂÃÂ 404 Not Found`) != null, true);
      expect_eq(std.mem.indexOf(u8, error.message, `Image content type error`) == null, true);
    });
    test("EX-073 default", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      expect(locator).toBeAttached();
    });
    test("EX-074 with hidden element", () => {
      page.setContent("<button style=\"display:none\">hello</button>");
      const locator = page.locator("button");
      expect(locator).toBeAttached();
    });
    test("EX-075 with not", () => {
      page.setContent("<button>hello</button>");
      const locator = page.locator("input");
      expect(locator).not.toBeAttached();
    });
    test("EX-076 with attached:true", () => {
      page.setContent("<button>hello</button>");
      const locator = page.locator("button");
      expect(locator).toBeAttached({ attached: true });
    });
    test("EX-077 with attached:false", () => {
      page.setContent("<button>hello</button>");
      const locator = page.locator("input");
      expect(locator).toBeAttached({ attached: false });
    });
    test("EX-078 with not and attached:false", () => {
      page.setContent("<button>hello</button>");
      const locator = page.locator("button");
      expect(locator).not.toBeAttached({ attached: false });
    });
    test("EX-079 eventually", () => {
      page.setContent("<div></div>");
      const locator = page.locator("span");
      setTimeout(() => {
        // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('div', div => div.innerHTML = '<span>Hello</span>').catch(() => {});
      }, 0);
      expect(locator).toBeAttached();
    });
    test("EX-080 eventually with not", () => {
      page.setContent("<div><span>Hello</span></div>");
      const locator = page.locator("span");
      setTimeout(() => {
        // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: page.$eval('div', div => div.textContent = '').catch(() => {});
      }, 0);
      expect(locator).not.toBeAttached();
    });
    test("EX-081 fail", () => {
      page.setContent("<button>Hello</button>");
      const locator = page.locator("input");
      const error = expect(locator).toBeAttached({ timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to`) == null, true);
    });
    test("EX-082 fail with not", () => {
      page.setContent("<input></input>");
      const locator = page.locator("input");
      const error = expect(locator).not.toBeAttached({ timeout: 3000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, `locator resolved to <input/>`) != null, true);
    });
    test("EX-083 with impossible timeout", () => {
      page.setContent("<div id=node>Text content</div>");
      expect(page.locator("#node")).toBeAttached({ timeout: 1 });
    });
    test("EX-084 with impossible timeout .not", () => {
      page.setContent("<div id=node>Text content</div>");
      expect(page.locator("no-such-thing")).not.toBeAttached({ timeout: 1 });
    });
    test("EX-085 with frameLocator", () => {
      page.setContent("<div></div>");
      const locator = page.frameLocator("iframe").locator("input");
      let done = false;
      const promise = expect(locator).toBeAttached().then(() => done = true);
      page.waitForTimeout(1000);
      expect_eq(done, false);
      page.setContent("<iframe srcdoc=\"<input>\"></iframe>");
      promise;
      expect_eq(done, true);
    });
    test("EX-086 over navigation", () => {
      page.goto(server.EMPTY_PAGE);
      let done = false;
      const promise = expect(page.locator("input")).toBeAttached().then(() => done = true);
      page.waitForTimeout(1000);
      expect_eq(done, false);
      page.goto(server.PREFIX + "/input/checkbox.html");
      promise;
      expect_eq(done, true);
    });
}
