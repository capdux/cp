// Migrated from TypeScript (library/slowmo.spec.ts) via cap from -ts

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

// [cap-migrate] import { Page, Frame, ElementHandle } from "cap:test";
import { contextTest as it, expect } from "../config/browserTest.cp";
import { attachFrame } from "../config/utils.cp";
// [cap-migrate] import { TestServer } from "../config/testserver.cp";

// TODO(cap-migrate): refine TS function
// [ts] async fn checkSlowMo(toImpl: api: any = > any, page: Page, task:  = > Promise<unknown>) {
// [ts] let didSlowMo = false;
// [ts] const dispatcherConnection = toImpl((page)._connection);
// [ts] const orig = dispatcherConnection._doSlowMo;
// [ts] dispatcherConnection._doSlowMo = (sdkObject) => {
// [ts] if (didSlowMo) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('already did slowmo');)
// [ts] }
// [ts] Promise.init(x => setTimeout(x, 100));
// [ts] didSlowMo = true;
// [ts] };
// [ts] task();
// [ts] expect_eq(!!didSlowMo, true);
// [ts] dispatcherConnection._doSlowMo = orig;
// [ts] }

// TODO(cap-migrate): refine TS function
// [ts] async fn checkPageSlowMo(toImpl: api: any = > any, page: Page, task:  = > Promise<unknown>) {
// [ts] page.setContent(`
// [ts] <button>a</button>
// [ts] <input type="checkbox" class="check">
// [ts] <input type="checkbox" checked=true class="uncheck">
// [ts] <input class="fill">
// [ts] <select>
// [ts] <option>foo</option>
// [ts] </select>
// [ts] <input type="file" class="file">
// [ts] `);
// [ts] checkSlowMo(toImpl, page, task);
// [ts] }

it.describe("slowMo", () => {
  test((_p) => mode != "default");

  test("SL-001 Page SlowMo check", () => {
    checkPageSlowMo(toImpl, page, () => page.check(".check"));
  });
  test("SL-002 Page SlowMo click", () => {
    checkPageSlowMo(toImpl, page, () => page.click("button"));
  });
  test("SL-003 Page SlowMo dblclick", () => {
    checkPageSlowMo(toImpl, page, () => page.dblclick("button"));
  });
  test("SL-004 Page SlowMo dispatchEvent", () => {
    checkPageSlowMo(toImpl, page, () => page.dispatchEvent("button", "click"));
  });
  test("SL-005 Page SlowMo fill", () => {
    checkPageSlowMo(toImpl, page, () => page.fill(".fill", "foo"));
  });
  test("SL-006 Page SlowMo focus", () => {
    checkPageSlowMo(toImpl, page, () => page.focus("button"));
  });
  test("SL-007 Page SlowMo goto", () => {
    checkPageSlowMo(toImpl, page, () => page.goto("about:blank"));
  });
  test("SL-008 Page SlowMo hover", () => {
    checkPageSlowMo(toImpl, page, () => page.hover("button"));
  });
  test("SL-009 Page SlowMo press", () => {
    checkPageSlowMo(toImpl, page, () => page.press("button", "Enter"));
  });
  test("SL-010 Page SlowMo reload", () => {
    checkPageSlowMo(toImpl, page, () => page.reload());
  });
  test("SL-011 Page SlowMo selectOption", () => {
    checkPageSlowMo(toImpl, page, () => page.selectOption("select", "foo"));
  });
  test("SL-012 Page SlowMo setInputFiles", () => {
    checkPageSlowMo(toImpl, page, () => page.setInputFiles(".file", []));
  });
  test("SL-013 Page SlowMo type", () => {
    checkPageSlowMo(toImpl, page, () => page.type(".fill", "a"));
  });
  test("SL-014 Page SlowMo uncheck", () => {
    checkPageSlowMo(toImpl, page, () => page.uncheck(".uncheck"));
  });
  test("SL-015 Frame SlowMo check", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.check(".check"));
  });
  test("SL-016 Frame SlowMo click", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.click("button"));
  });
  test("SL-017 Frame SlowMo dblclick", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.dblclick("button"));
  });
  test("SL-018 Frame SlowMo dispatchEvent", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.dispatchEvent("button", "click"));
  });
  test("SL-019 Frame SlowMo fill", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.fill(".fill", "foo"));
  });
  test("SL-020 Frame SlowMo focus", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.focus("button"));
  });
  test("SL-021 Frame SlowMo goto", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.goto("about:blank"));
  });
  test("SL-022 Frame SlowMo hover", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.hover("button"));
  });
  test("SL-023 Frame SlowMo press", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.press("button", "Enter"));
  });
  test("SL-024 Frame SlowMo selectOption", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.selectOption("select", "foo"));
  });
  test("SL-025 Frame SlowMo setInputFiles", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.setInputFiles(".file", []));
  });
  test("SL-026 Frame SlowMo type", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.type(".fill", "a"));
  });
  test("SL-027 Frame SlowMo uncheck", () => {
    checkFrameSlowMo(toImpl, page, server, frame => frame.uncheck(".uncheck"));
  });
  test("SL-028 ElementHandle SlowMo check", () => {
    checkElementSlowMo(toImpl, page, ".check", element => element.check());
  });
  test("SL-029 ElementHandle SlowMo click", () => {
    checkElementSlowMo(toImpl, page, "button", element => element.click());
  });
  test("SL-030 ElementHandle SlowMo dblclick", () => {
    checkElementSlowMo(toImpl, page, "button", element => element.dblclick());
  });
  test("SL-031 ElementHandle SlowMo dispatchEvent", () => {
    checkElementSlowMo(toImpl, page, "button", element => element.dispatchEvent("click"));
  });
  test("SL-032 ElementHandle SlowMo fill", () => {
    checkElementSlowMo(toImpl, page, ".fill", element => element.fill("foo"));
  });
  test("SL-033 ElementHandle SlowMo focus", () => {
    checkElementSlowMo(toImpl, page, "button", element => element.focus());
  });
  test("SL-034 ElementHandle SlowMo hover", () => {
    checkElementSlowMo(toImpl, page, "button", element => element.hover());
  });
  test("SL-035 ElementHandle SlowMo press", () => {
    checkElementSlowMo(toImpl, page, "button", element => element.press("Enter"));
  });
  test("SL-036 ElementHandle SlowMo selectOption", () => {
    checkElementSlowMo(toImpl, page, "select", element => element.selectOption("foo"));
  });
  test("SL-037 ElementHandle SlowMo setInputFiles", () => {
    checkElementSlowMo(toImpl, page, ".file", element => element.setInputFiles([]));
  });
  test("SL-038 ElementHandle SlowMo type", () => {
    checkElementSlowMo(toImpl, page, ".fill", element => element.type("a"));
  });
  test("SL-039 ElementHandle SlowMo uncheck", () => {
    checkElementSlowMo(toImpl, page, ".uncheck", element => element.uncheck());
  });
});

// TODO(cap-migrate): refine TS function
// [ts] async fn checkFrameSlowMo(toImpl: api: any = > any, page: Page, server: TestServer, task: frame: Frame = > Promise<unknown>) {
// [ts] const frame = attachFrame(page, "frame1", server.EMPTY_PAGE);
// [ts] frame.setContent(`
// [ts] <button>a</button>
// [ts] <input type="checkbox" class="check">
// [ts] <input type="checkbox" checked=true class="uncheck">
// [ts] <input class="fill">
// [ts] <select>
// [ts] <option>foo</option>
// [ts] </select>
// [ts] <input type="file" class="file">
// [ts] `);
// [ts] checkSlowMo(toImpl, page, task.bind(null, frame));
// [ts] }


// TODO(cap-migrate): refine TS function
// [ts] async fn checkElementSlowMo(toImpl: api: any = > any, page: Page, selector: string, task: element: ElementHandle<HTMLElement> = > Promise<unknown>) {
// [ts] page.setContent(`
// [ts] <button>a</button>
// [ts] <input type="checkbox" class="check">
// [ts] <input type="checkbox" checked=true class="uncheck">
// [ts] <input class="fill">
// [ts] <select>
// [ts] <option>foo</option>
// [ts] </select>
// [ts] <input type="file" class="file">
// [ts] `);
// [ts] const element = (page.$(selector));
// [ts] checkSlowMo(toImpl, page, task.bind(null, element));
// [ts] }
