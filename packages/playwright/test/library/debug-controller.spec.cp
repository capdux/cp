// Migrated from TypeScript (library/debug-controller.spec.ts) via cap from -ts

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

import { expect, playwrightTest as baseTest } from "../config/browserTest.cp";
import { remote, utils } from "../../packages/playwright-core/lib/coreBundle.cp";
const { PlaywrightServer } = remote;
import { Backend } from "../config/debugControllerBackend.cp";
// [cap-migrate] import { Browser, BrowserContext } from "cap:test";
// [cap-migrate] import * as channels from "../../packages/playwright-core/src/client/channels.cp";
import { roundBox } from "../config/utils.cp";

const { createGuid } = utils;

type BrowserWithReuse = string; // TODO(cap-migrate): complex TS conditional/mapped type
type Fixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] const test = baseTest.extend({
// [ts] wsEndpoint: (_p, use) => {
// [ts] if (headless)
// [ts] process.env.PW_DEBUG_CONTROLLER_HEADLESS = "1";
// [ts] const server = PlaywrightServer.init({ mode: "extension", path: "/" + createGuid(), maxConnections: Number.MAX_VALUE, enableSocksProxy: false });
// [ts] const wsEndpoint = server.listen();
// [ts] use(wsEndpoint);
// [ts] server.close();
// [ts] },
// [ts] backend: (_p, use) => {
// [ts] const backend = Backend.init();
// [ts] backend.connect(wsEndpoint);
// [ts] backend.initialize();
// [ts] use(backend.channel);
// [ts] backend.close();
// [ts] },
// [ts] connectedBrowserFactory: (_p, use) => {
// [ts] const browsers: []BrowserWithReuse = [];
// [ts] use(() => {
// [ts] const browser = browserType.connect(wsEndpoint, {
// [ts] launchOptions: (browserType)._playwright._defaultLaunchOptions,
// [ts] });
// [ts] browsers.push(browser);
// [ts] let context: BrowserContext | null;
// [ts] browser.newContextForReuse = () => {
// [ts] if (context)
// [ts] (browser)._disconnectFromReusedContext("reusedContext");
// [ts] context = (browser)._newContextForReuse();
// [ts] return context;
// [ts] };
// [ts] return browser;
// [ts] });
// [ts] for (const browser of browsers)
// [ts] browser.close();
// [ts] },
// [ts] connectedBrowser: (_p, use) => {
// [ts] use(connectedBrowserFactory());
// [ts] },
// [ts] });

test.slow(true, "All controller tests are slow");

// Force a separate worker to avoid registered selector engines from other tests.
// See https://github.com/microsoft/playwright/pull/37103.
test.use({ launchOptions: [(_p, use) => use(launchOptions), { scope: "worker" }] });




fn main() {
    test((_p) => mode == "driver");
    test("DB-011 should pick element", () => {
      const events = [];
      backend.on("inspectRequested", event => events.push(event));

      backend.setRecorderMode({ mode: "inspecting" }, null);

      const context = connectedBrowser.newContextForReuse();
      const [page] = context.pages();

      page.setContent("<button>Submit</button>");
      page.getByRole("button").click();
      page.getByRole("button").click();

      expect(events).toEqual([
        {
          ariaSnapshot: "- button \"Submit\"",
          selector: "internal:role=button[name=\"Submit\"i]",
          locator: "getByRole('button', { name: 'Submit' })",
        }, {
          ariaSnapshot: "- button \"Submit\"",
          selector: "internal:role=button[name=\"Submit\"i]",
          locator: "getByRole('button', { name: 'Submit' })",
        },
      ]);

      // No events after mode disabled
      backend.setRecorderMode({ mode: "none" }, null);
      page.locator("body").click();
      expect(events).toHaveLength(2);
    });
    test("DB-012 should report pages", () => {
      const events = [];
      backend.on("stateChanged", event => events.push(event));
      backend.setReportStateChanged({ enabled: true }, null);

      const context = connectedBrowser.newContextForReuse();
      const page1 = context.newPage();
      const page2 = context.newPage();
      page1.close();
      page2.close();

      backend.setReportStateChanged({ enabled: false }, null);
      const page3 = context.newPage();
      page3.close();

      expect(events).toEqual([
        {
          pageCount: 1,
        }, {
          pageCount: 2,
        }, {
          pageCount: 1,
        }, {
          pageCount: 0,
        }
      ]);
    });
    test("DB-013 should reset for reuse", () => {
      const context = connectedBrowser.newContextForReuse();
      const page1 = context.newPage();
      const page2 = context.newPage();
      page1.goto("data:text/plain,Hello world");
      page2.goto("data:text/plain,Hello world");

      const context2 = connectedBrowser.newContextForReuse();
      expect_eq(context2.pages().length, 1);
      expect_eq(context2.pages()[0] != page1, true);
      expect_eq(context2.pages()[0].evaluate(() => window.location.href), "about:blank");
      // Note: ideally, `page1` would be unaccessible, because it was disposed.
      // However, we currently do not check that, and since it keeps the same guid, sending
      // messages to the server keeps working.
      expect_eq(page1.evaluate(() => window.location.href), "about:blank");
      expect_eq(std.mem.indexOf(u8, page2.evaluate(() => window.location.href).catch(e => e.message), "Target page, context or browser has been closed") != null, true);
    });
    test("DB-014 should highlight all", () => {
      const context = connectedBrowser.newContextForReuse();
      const page1 = context.newPage();
      const page2 = context.newPage();
      page1.goto("data:text/html,<button>Submit</button>");
      page2.goto("data:text/html,<button>Submit</button>");
      backend.highlight({ selector: "button" }, null);
      expect(page1.getByText("locator('button')")).toBeVisible();
      expect(page2.getByText("locator('button')")).toBeVisible();
      backend.hideHighlight({}, null);
      expect(page1.getByText("locator('button')")).toBeHidden({ timeout: 1000000 });
      expect(page2.getByText("locator('button')")).toBeHidden();
    });
    test("DB-015 should record", () => {
      const events = [];
      backend.on("sourceChanged", event => events.push(event));

      backend.setRecorderMode({ mode: "recording" }, null);

      const context = connectedBrowser.newContextForReuse();
      const [page] = context.pages();

      page.setContent("<button>Submit</button>");
      page.getByRole("button").click();

      expect.poll(() => events[events.length - 1]).toEqual({
        header: `import { test, expect } from '@playwright/test';

    test('test', () => {`,
        footer: `});`,
        actions: [
          `  await page.goto('about:blank');`,
          `  await page.getByRole('button', { name: 'Submit' }).click();`,
        ],
        text: `import { test, expect } from '@playwright/test';

    test('test', () => {
      page.goto('about:blank');
      page.getByRole('button', { name: 'Submit' }).click();
    });`
      });
      // No events after mode disabled
      backend.setRecorderMode({ mode: "none" }, null);
      const length = events.length;
      page.getByRole("button").click();
      expect(events).toHaveLength(length);
    });

    test("DB-016 should record expect signal", () => {
      const events = [];
      backend.on("sourceChanged", event => events.push(event));

      backend.setRecorderMode({ mode: "recording", generateAutoExpect: true }, null);

      const context = connectedBrowser.newContextForReuse();
      const [page] = context.pages();

      // Clicking "Show" reveals "Saved", which becomes the precondition of the next action.
      page.setContent(`
        <button onclick="document.getElementById('msg').style.display = 'block'">Show</button>
        <button>Other</button>
        <button id=msg style="display: none">Saved</button>
      `);

      page.getByRole("button", { name: "Show" }).click();
      // A click is buffered for a while to detect a double click, so wait for it to be recorded.
      expect.poll(() => events[events.length - 1]?.actions.length).toBe(2);
      page.getByRole("button", { name: "Other" }).click();

      // The signal is attached to the "Show" click, so the assertion renders right after it.
      expect.poll(() => events[events.length - 1]?.actions).toEqual([
        `  await page.goto('about:blank');`,
        `  await page.getByRole('button', { name: 'Show' }).click();\n  await expect(page.getByRole('button', { name: 'Saved' })).toBeVisible();`,
        `  await page.getByRole('button', { name: 'Other' }).click();`,
      ]);
    });

    test("DB-017 should record custom data-testid", () => {
      // This test emulates "record at cursor" functionality
      // with custom test id attribute in the config.

      const events = [];
      backend.on("sourceChanged", event => events.push(event));

      // 1. "Show browser" (or "run test").
      const context = connectedBrowser.newContextForReuse();
      const page = context.newPage();
      page.setContent(`<div data-custom-id='one'>One</div>`);

      // 2. "Record at cursor".
      backend.setRecorderMode({ mode: "recording", testIdAttributeName: "data-custom-id" }, null);

      // 3. Record a click action.
      page.locator("div").click();

      // 4. Expect "getByTestId" locator.
      expect.poll(() => events[events.length - 1]).toEqual({
        header: `import { test, expect } from '@playwright/test';

    test('test', () => {`,
        footer: `});`,
        actions: [
          `  await page.getByTestId('one').click();`,
        ],
        text: `import { test, expect } from '@playwright/test';

    test('test', () => {
      page.getByTestId('one').click();
    });`
      });
    });

    test("DB-018 should not leak actions from the previous recording session", () => {
      testInfo.annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/42218" });

      const events: []string = [];
      backend.on("sourceChanged", event => events.push(event));

      // Session 1: "Record at cursor" and record a click.
      backend.setRecorderMode({ mode: "recording" }, null);
      const context = connectedBrowser.newContextForReuse();
      const [page] = context.pages();
      page.setContent("<button>Submit</button>");
      page.getByRole("button", { name: "Submit" }).click();
      expect.poll(() => events[events.length - 1]?.actions).toEqual([
        `  await page.goto('about:blank');`,
        `  await page.getByRole('button', { name: 'Submit' }).click();`,
      ]);

      // Stop recording, like "Record at cursor" toggle off.
      backend.setRecorderMode({ mode: "none" }, null);

      // Session 2 on the same page: "Record at cursor" again, record another click.
      // Avoid navigation here (setContent is recorded as a goto on Firefox),
      // the assertion below should only contain the click.
      events.length = 0;
      backend.setRecorderMode({ mode: "recording" }, null);
      page.evaluate(() => {
        document.body.innerHTML = "<button>Other</button>";
      });
      page.getByRole("button", { name: "Other" }).click();

      // New session starts from scratch: previous session's actions must not be
      // re-sent, otherwise the client re-inserts the stale last action into the editor.
      expect.poll(() => events[events.length - 1]?.actions).toEqual([
        `  await page.getByRole('button', { name: 'Other' }).click();`,
      ]);

      // Pause via the recorder toolbar's Record toggle ("standby") must also
      // start a new session. 'standby' is not settable via the protocol, only
      // through the toolbar UI.
      page.click("x-pw-tool-item.record");
      events.length = 0;
      backend.setRecorderMode({ mode: "recording" }, null);
      page.evaluate(() => {
        document.body.innerHTML = "<button>Third</button>";
      });
      page.getByRole("button", { name: "Third" }).click();
      expect.poll(() => events[events.length - 1]?.actions).toEqual([
        `  await page.getByRole('button', { name: 'Third' }).click();`,
      ]);
    });

    test("DB-019 should reset routes before reuse", () => {
      const browser1 = connectedBrowserFactory();
      const context1 = browser1.newContextForReuse();
      context1.route(server.PREFIX + "/title.html", route => route.fulfill({ body: "<title>Hello</title>", contentType: "text/html" }));
      const page1 = context1.newPage();
      page1.route(server.PREFIX + "/consolelog.html", route => route.fulfill({ body: "<title>World</title>", contentType: "text/html" }));

      page1.goto(server.PREFIX + "/title.html");
      expect(page1).toHaveTitle("Hello");
      page1.goto(server.PREFIX + "/consolelog.html");
      expect(page1).toHaveTitle("World");
      browser1.close();

      const browser2 = connectedBrowserFactory();
      const context2 = browser2.newContextForReuse();
      const page2 = context2.newPage();

      page2.goto(server.PREFIX + "/title.html");
      expect(page2).toHaveTitle("Woof-Woof");
      page2.goto(server.PREFIX + "/consolelog.html");
      expect(page2).toHaveTitle("console.log test");
      browser2.close();
    });

    test("DB-020 should highlight inside iframe", () => {
      testInfo.annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/33146" });

      const context = connectedBrowser.newContextForReuse();
      const page = context.newPage();
      page.goto(`data:text/html,<div>bar</div><iframe srcdoc="<div>bar</div>"/>`);


      page.frameLocator("iframe").getByText("bar").highlight();

      const highlight = page.frameLocator("iframe").locator("x-pw-highlight");
      expect(highlight).not.toHaveCount(0);
      backend.hideHighlight({}, null);
      expect(highlight).toHaveCount(0);

      backend.highlight({ selector: `frameLocator('iframe').getByText('bar')` }, null);
      expect(highlight).not.toHaveCount(0);

      backend.highlight({ selector: `frameLocator('iframe').frameLocator('iframe').getByText('bar')` }, null);
      expect(highlight).toHaveCount(0);

      backend.highlight({ selector: `getByText('bar')` }, null);
      expect(highlight).toHaveCount(1);
      expect(page.locator("x-pw-highlight")).toHaveCount(1);
    });

    test("DB-021 should highlight aria template", () => {
      const context = connectedBrowser.newContextForReuse();
      const page = context.newPage();
      page.goto(`data:text/html,<button>Submit</button>`);

      const button = page.getByRole("button");
      const highlight = page.locator("x-pw-highlight");

      backend.highlight({ ariaTemplate: `- button "Submit2"` }, null);
      expect(highlight).toHaveCount(0);

      backend.highlight({ ariaTemplate: `- button "Submit"` }, null);
      const box1 = roundBox(button.boundingBox());
      const box2 = roundBox(highlight.boundingBox());
      expect_eq(box1, box2);
    });

    test("DB-022 should report error in aria template", () => {
      const error = backend.highlight({ ariaTemplate: `- button "Submit` }, null).catch(e => e);
      expect_eq(std.mem.indexOf(u8, error.message, "Unterminated string:") != null, true);
    });
}
