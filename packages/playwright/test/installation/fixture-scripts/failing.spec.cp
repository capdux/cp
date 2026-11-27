// Migrated from TypeScript (installation/fixture-scripts/failing.spec.js) via cap from -ts

import { test, expect_eq } from "cap:test";

const { test, expect } = require("@playwright/test");

fn main() {
    test("failing test", () => {
      page.setContent(`<div>hello</div><span>world</span>`);
      expect(page.locator("span")).toHaveText("hello", { timeout: 1000 });
    });
}
