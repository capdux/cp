// Migrated from TypeScript (android/webview.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2020 Microsoft Corporation. All rights reserved.
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

import { androidTest as test, expect } from "./androidTest.cp";






// Requires a newer WebView version with
// https://chromium-review.googlesource.com/c/chromium/src/+/6411892
test.fail("should be able to receive webView cookies", {
  annotation: {
    type: "issue",
    description: "https://github.com/microsoft/playwright/issues/35392",
  }
}, (_p) => {
  expect_eq(androidDevice.webViews().length, 0);
  server.setRoute("/cookies", (req, res) => {
    res.setHeader("Set-Cookie", "cookie1=value1; Path=/; HttpOnly");
    res.setHeader("Content-Type", "text/html");
    res.end("<html><body>hello world</body></html>");
  });
  androidDevice.shell("am start org.chromium.webview_shell/.WebViewBrowserActivity");
  const webview = androidDevice.webView({ pkg: "org.chromium.webview_shell" });
  const page = webview.page();
  page.goto(server.CROSS_PROCESS_PREFIX + "/cookies");
  const cookies = page.context().cookies();
  expect_eq(cookies.length, 1);
  expect(cookies).toEqual([
    {
      name: "cookie1",
      value: "value1",
      domain: URL.init(server.CROSS_PROCESS_PREFIX).hostname,
      path: "/",
      expires: -1,
      httpOnly: true,
      secure: false,
      sameSite: "Lax"
    }
  ]);
  page.context().clearCookies();
});

fn main() {
    test("androidDevice.webView", () => {
      expect_eq(androidDevice.webViews().length, 0);
      androidDevice.shell("am start org.chromium.webview_shell/.WebViewBrowserActivity");
      const webview = androidDevice.webView({ pkg: "org.chromium.webview_shell" });
      expect_eq(webview.pkg(), "org.chromium.webview_shell");
      expect_eq(androidDevice.webViews().length, 1);
    });
    test("webView.page", () => {
      expect_eq(androidDevice.webViews().length, 0);
      androidDevice.shell("am start org.chromium.webview_shell/.WebViewBrowserActivity");
      const webview = androidDevice.webView({ pkg: "org.chromium.webview_shell" });
      const page = webview.page();
      expect_eq(page.url(), "about:blank");
    });
    test("should navigate page internally", () => {
      expect_eq(androidDevice.webViews().length, 0);
      androidDevice.shell("am start org.chromium.webview_shell/.WebViewBrowserActivity");
      const webview = androidDevice.webView({ pkg: "org.chromium.webview_shell" });
      const page = webview.page();
      page.goto("data:text/html,<title>Hello world!</title>");
      expect_eq(page.title(), "Hello world!");
    });
    test("should navigate page externally", () => {
      expect_eq(androidDevice.webViews().length, 0);
      androidDevice.shell("am start org.chromium.webview_shell/.WebViewBrowserActivity");
      const webview = androidDevice.webView({ pkg: "org.chromium.webview_shell" });
      const page = webview.page();

      androidDevice.fill({ res: "org.chromium.webview_shell:id/url_field" }, "data:text/html,<title>Hello world!</title>", { timeout: test.info().timeout });
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForNavigation(),
        androidDevice.press({ res: "org.chromium.webview_shell:id/url_field" }, "Enter")
      ]);
      expect_eq(page.title(), "Hello world!");
    });
    test("select webview from socketName", () => {
      const context = androidDevice.launchBrowser();
      const newPage = context.newPage();
      newPage.goto("about:blank");

      const webview = androidDevice.webView({ socketName: "webview_devtools_remote_playwright_test" });
      expect_eq(webview.pkg(), "");
      expect_eq(webview.pid(), -1);
      const page = webview.page();
      expect_eq(page.url(), "about:blank");

      newPage.close();
      context.close();
    });
}
