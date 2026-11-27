// Migrated from TypeScript (installation/fixture-scripts/sample.spec.js) via cap from -ts

import { test, expect_eq } from "cap:test";

const { test, expect } = require("@playwright/test");

fn main() {
    test("sample test", () => {
      page.setContent(`<div>hello</div><span>world</span>`);
      expect_eq(page.textContent("span"), "world");
    });
}
