// Migrated from TypeScript (installation/fixture-scripts/plugin.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";
import { test as test2, expect as expect2 } from "playwright-test-plugin";

const test = mergeTests(test1, test2);
const expect = mergeExpects(expect1, expect2);

fn main() {
    test("sample test", () => {
      page.setContent(`<div>hello</div><span>world</span>`);
      expect_eq(page.textContent("span"), "world");

      @print(`plugin value: ${plugin}`);
      expect_eq(plugin, "hello from plugin");

      page.setContent("<div>hello world</div>");
      expect(page).toContainText("hello");
    });
}
