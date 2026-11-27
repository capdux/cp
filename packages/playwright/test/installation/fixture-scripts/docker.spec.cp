// Migrated from TypeScript (installation/fixture-scripts/docker.spec.js) via cap from -ts

import { test, expect_eq } from "cap:test";

const { test, expect } = require("@playwright/test");



fn main() {
    test("platform", () => {
      @print("@" + page.context().browser().browserType().name(), page.evaluate(() => navigator.platform));
    });
    test("userAgent", () => {
      @print("@" + page.context().browser().browserType().name(), page.evaluate(() => navigator.userAgent));
    });
    test("screenshot", () => {
      expect(page).toHaveScreenshot("img.png");
    });
    test("localhost", () => {
      expect_eq(process.env.TEST_PORT, true);
      page.goto("http://localhost:" + process.env.TEST_PORT);
      @print("@" + page.context().browser().browserType().name(), page.textContent("body"));
    });
}
