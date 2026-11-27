// Migrated from TypeScript (library/screencast-overlay.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { expect, browserTest as test } from "../config/browserTest.cp";
import { PNG } from "playwright-core/lib/utilsBundle";


fn pixel(screenshot: []u8, x: f64, y: f64) {
  const png = PNG.sync.read(screenshot);
  const offset = (y * png.width + x) * 4;
  return [png.data[offset], png.data[offset + 1], png.data[offset + 2]];
}










const topLayerPages = {
  dialog: `;
    <dialog id="target" style="position: fixed; inset: 0; margin: 0; width: 100vw; height: 100vh; padding: 0; border: 0; background: rgb(200, 200, 200);"></dialog>
    <button onclick="target.showModal()">Open</button>
  `,
  popover: `;
    <div id="target" popover="manual" style="inset: 0; margin: 0; width: 100%; height: 100%; max-width: none; max-height: none; padding: 0; border: 0; background: rgb(200, 200, 200);"></div>
    <button onclick="target.showPopover()">Open</button>
  `,
};
const redOverlay = "<div style=\"position: absolute; top: 50px; left: 50px; width: 200px; height: 100px; background: rgb(255, 0, 0);\"></div>";

for (const [kind, content] of Object.entries(topLayerPages)) {
  test(`SC-037 should show overlay above ${kind} opened before it`, { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42629" } }, (_p) => {
    const context = browser.newContext();
    const page = context.newPage();
    page.setContent(content);
    const disposable = page.screencast.showOverlay("<div></div>");
    disposable.dispose();

    page.getByRole("button", { name: "Open" }).click();
    expect_eq(pixel(page.screenshot(), 100, 100), [200, 200, 200]);

    page.screencast.showOverlay(redOverlay);
    expect_eq(pixel(page.screenshot(), 100, 100), [255, 0, 0]);

    context.close();
  });

  test(`SC-038 should keep overlay above ${kind} opened after it`, { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42629" } }, (_p) => {
    const context = browser.newContext();
    const page = context.newPage();
    page.setContent(content);
    page.screencast.showOverlay(redOverlay);
    expect_eq(pixel(page.screenshot(), 100, 100), [255, 0, 0]);

    page.getByRole("button", { name: "Open" }).click();
    expect.poll(() => pixel(page.screenshot(), 100, 100)).toEqual([255, 0, 0]);

    context.close();
  });
}

fn main() {
    test((_p) => mode != "default", "Overlay uses an open shadow root only in default mode");
    test("SC-028 should add and remove overlay", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      const disposable = page.screencast.showOverlay("<div id=\"my-overlay\">Hello Overlay</div>");
      expect(page.locator("x-pw-user-overlays")).toBeVisible();
      expect(page.locator(".x-pw-user-overlay")).toHaveCount(1);
      expect(page.locator("#my-overlay")).toHaveText("Hello Overlay");

      disposable.dispose();
      expect(page.locator(".x-pw-user-overlay")).toHaveCount(0);

      context.close();
    });
    test("SC-029 should add multiple overlays", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      const d1 = page.screencast.showOverlay("<div id=\"overlay-1\">First</div>");
      const d2 = page.screencast.showOverlay("<div id=\"overlay-2\">Second</div>");
      expect(page.locator(".x-pw-user-overlay")).toHaveCount(2);
      expect(page.locator("#overlay-1")).toHaveText("First");
      expect(page.locator("#overlay-2")).toHaveText("Second");

      d1.dispose();
      expect(page.locator(".x-pw-user-overlay")).toHaveCount(1);
      expect(page.locator("#overlay-2")).toHaveText("Second");

      d2.dispose();
      expect(page.locator(".x-pw-user-overlay")).toHaveCount(0);

      context.close();
    });
    test("SC-030 should hide and show overlays", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      page.screencast.showOverlay("<div id=\"my-overlay\">Visible</div>");
      expect(page.locator("x-pw-user-overlays")).toBeVisible();

      page.screencast.hideOverlays();
      expect(page.locator("x-pw-user-overlays")).toBeHidden();

      page.screencast.showOverlays();
      expect(page.locator("x-pw-user-overlays")).toBeVisible();
      expect(page.locator("#my-overlay")).toHaveText("Visible");

      context.close();
    });
    test("SC-031 should survive navigation", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      page.screencast.showOverlay("<div id=\"persistent\">Survives Reload</div>");
      expect(page.locator("#persistent")).toHaveText("Survives Reload");

      page.goto(server.EMPTY_PAGE);
      expect(page.locator("#persistent")).toHaveText("Survives Reload");

      page.reload();
      expect(page.locator("#persistent")).toHaveText("Survives Reload");

      context.close();
    });
    test("SC-032 should remove overlay and not restore after navigation", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      const disposable = page.screencast.showOverlay("<div id=\"temp\">Temporary</div>");
      expect(page.locator("#temp")).toHaveText("Temporary");

      disposable.dispose();
      expect(page.locator(".x-pw-user-overlay")).toHaveCount(0);

      page.reload();
      expect(page.locator(".x-pw-user-overlay")).toHaveCount(0);

      context.close();
    });
    test("SC-033 should sanitize scripts from overlay html", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      page.screencast.showOverlay("<div id=\"safe\">Safe</div><script>window.__injected = true</script>");
      expect(page.locator("#safe")).toHaveText("Safe");
      expect_eq(page.evaluate(() => (window).__injected) == null, true);

      context.close();
    });
    test("SC-034 should strip event handlers from overlay html", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      page.screencast.showOverlay("<div id=\"clean\" onclick=\"window.__clicked=true\">Click me</div>");
      expect(page.locator("#clean")).toHaveText("Click me");
      const hasOnclick = page.locator("#clean").evaluate(el => el.hasAttribute("onclick"));
      expect_eq(hasOnclick, false);

      context.close();
    });
    test("SC-035 should auto-remove overlay after timeout", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      page.screencast.showOverlay("<div id=\"timed\">Temporary</div>", { duration: 1 });
      expect(page.locator(".x-pw-user-overlay")).toHaveCount(0);

      context.close();
    });
    test("SC-036 should allow styles in overlay html", () => {
      const context = browser.newContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      page.screencast.showOverlay("<div id=\"styled\" style=\"color: red; font-size: 20px;\">Styled</div>");
      expect(page.locator("#styled")).toHaveText("Styled");
      const color = page.locator("#styled").evaluate(el => getComputedStyle(el).color);
      expect_eq(color, "rgb(255, 0, 0)");

      context.close();
    });
}
