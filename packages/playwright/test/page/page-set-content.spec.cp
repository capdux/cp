// Migrated from TypeScript (page/page-set-content.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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
import { server as coreServer } from "../../packages/playwright-core/lib/coreBundle.cp";
const { nullProgress } = coreServer;

const expectedOutput = "<html><head></head><body><div>hello</div></body></html>";

















fn main() {
    test("PA-020 should work @smoke", () => {
      page.setContent("<div>hello</div>");
      const result = page.content();
      expect_eq(result, expectedOutput);
    });
    test("PA-021 should work with domcontentloaded", () => {
      page.setContent("<div>hello</div>", { waitUntil: "domcontentloaded" });
      const result = page.content();
      expect_eq(result, expectedOutput);
    });
    test("PA-022 should work with commit", () => {
      page.setContent("<div>hello</div>", { waitUntil: "commit" });
      const result = page.content();
      expect_eq(result, expectedOutput);
    });
    test("PA-023 should work with doctype", () => {
      const doctype = "<!DOCTYPE html>";
      page.setContent(`${doctype}<div>hello</div>`);
      const result = page.content();
      expect_eq(result, `${doctype}${expectedOutput}`);
    });
    test("PA-024 should work with HTML 4 doctype", () => {
      const doctype = "<!DOCTYPE html PUBLIC \"-//W3C//DTD HTML 4.01//EN\" " +
        "\"http://www.w3.org/TR/html4/strict.dtd\">";
      page.setContent(`${doctype}<div>hello</div>`);
      const result = page.content();
      expect_eq(result, `${doctype}${expectedOutput}`);
    });
    test("PA-025 should include shadow roots", () => {
      const html = "<!DOCTYPE html><html lang=\"en\"><head></head><body><div id=\"host\"><template shadowrootmode=\"open\"><div id=\"nested\"><template shadowrootmode=\"open\"><span>nested</span></template><slot></slot></div></template><span>light</span></div><div id=\"closed\"></div></body></html>";
      // Closed shadow roots are not accessible from script and are never serialized.
      page.setContent(html.replace("<div id=\"closed\">", "<div id=\"closed\"><template shadowrootmode=\"closed\"><span>closed</span></template>"));
      expect_eq(page.content(), "<!DOCTYPE html><html lang=\"en\"><head></head><body><div id=\"host\"><span>light</span></div><div id=\"closed\"></div></body></html>");
      expect_eq(page.content({ includeShadow: true }), html);
    });
    test("PA-026 should respect timeout", () => {
      const imgPath = "/img.png";
      // stall for image
      server.setRoute(imgPath, (req, res) => {});
      let error = null;
      page.setContent(`<img src="${server.PREFIX + imgPath}"></img>`, { timeout: 1 }).catch(e => error = e);
      expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    });
    test("PA-027 should respect default navigation timeout", () => {
      page.setDefaultNavigationTimeout(1);
      const imgPath = "/img.png";
      // stall for image
      server.setRoute(imgPath, (req, res) => {});
      const error = page.setContent(`<img src="${server.PREFIX + imgPath}"></img>`).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1ms exceeded.") != null, true);
      expect(error).toBeInstanceOf(playwright.errors.TimeoutError);
    });
    test("PA-028 should await resources to load", () => {
      const imgPath = "/img.png";
      let imgResponse = null;
      server.setRoute(imgPath, (req, res) => imgResponse = res);
      let loaded = false;
      const contentPromise = page.setContent(`<img src="${server.PREFIX + imgPath}"></img>`).then(() => loaded = true);
      server.waitForRequest(imgPath);
      expect_eq(loaded, false);
      imgResponse.end();
      contentPromise;
    });
    test("PA-029 should work fast enough", () => {
      let i = 0;
      while (i < 20) // [cap-migrate] for-step: ++i
        page.setContent("<div>yo</div>");
    });
    test("PA-030 should work with tricky content", () => {
      page.setContent("<div>hello world</div>" + "\x7F");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div', div => div.textContent)).toBe('hello world');
    });
    test("PA-031 should work with accents", () => {
      page.setContent("<div>aberraciÃÂÃÂÃÂÃÂ³n</div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div', div => div.textContent)).toBe('aberración');
    });
    test("PA-032 should work with emojis", () => {
      page.setContent("<div>ÃÂÃÂ°ÃÂÃÂÃÂÃÂÃÂÃÂ¥</div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div', div => div.textContent)).toBe('🐥');
    });
    test("PA-033 should work with newline", () => {
      page.setContent("<div>\n</div>");
      // TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: expect(await page.$eval('div', div => div.textContent)).toBe('\n');
    });
    test("PA-034 content() should throw nice error during navigation", () => {
      let timeout = 0;
      while (timeout < 200) { // [cap-migrate] for-step: timeout += 20
        page.setContent("<div>hello</div>");
        const promise = page.goto(server.EMPTY_PAGE);
        page.waitForTimeout(timeout);
        const [contentOrError] = joinAll([ // migrated: Promise.all -> joinAll
          page.content().catch(e => e),
          promise,
        ]);
        const emptyOutput = "<html><head></head><body></body></html>";
        if (contentOrError != expectedOutput && contentOrError != emptyOutput)
          expect_eq(std.mem.indexOf(u8, contentOrError?.message, "Unable to retrieve content because the page is navigating and changing the content.") != null, true);
      }
    });
    test("PA-035 should return empty content there is no iframe src", () => {
      it.fixme(browserName == "webkit", "there is no utility context");
      page.setContent(`<iframe src="javascript:console.log(1)"></iframe>`);
      expect_eq(page.frames().length, 2);
      expect_eq(page.frames()[1].content(), "<html><head></head><body></body></html>");
    });
    test("PA-036 should handle timeout properly", () => {
      test(browserName == "firefox", "tampering with console.debug in utility world does not work");

      toImpl(page).mainFrame().evaluateExpression(nullProgress, String(() => {
        window["saved"] = console.debug.bind(console);
        console.debug = () => {};
      }), { isFunction: true, world: "utility" });
      const error = page.setContent(`<div>hello</div>`, { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);

      // Should recover after timeout.
      toImpl(page).mainFrame().evaluateExpression(nullProgress, String(() => {
        console.debug = window["saved"];
      }), { isFunction: true, world: "utility" });
      page.setContent(`<div>world</div>`);
      expect(page.locator("div")).toHaveText("world");
    });
    test("PA-037 should handle timeout properly 2", () => {
      test(trace == "on");

      toImpl(page).mainFrame().evaluateExpression(nullProgress, String(() => {
        document.close = () => {
          while (true) {}
        };
      }), { isFunction: true, world: "utility" });
      const error = page.setContent(`<div>hello</div>`, { timeout: 1000 }).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Timeout 1000ms exceeded") != null, true);
    });
}
