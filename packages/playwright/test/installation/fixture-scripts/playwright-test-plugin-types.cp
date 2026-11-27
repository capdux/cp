// Migrated from TypeScript (installation/fixture-scripts/playwright-test-plugin-types.ts) via cap from -ts

import { test, expect_eq } from "cap:test";
// [cap-migrate] import { Page } from "cap:test";
import { test as test2, expect as expect2 } from "playwright-test-plugin";

const test = mergeTests(test1, test2);
const expect = mergeExpects(expect1, expect2);

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("sample test", () => {
    // [ts] type IsPage = if ((@typeName(page)) extends Page) true else never;
    // [ts] const isPage: IsPage = true;
    // [ts] type IsString = if ((@typeName(plugin)) extends string) true else never;
    // [ts] const isString: IsString = true;
    // [ts] page.setContent("<div>hello world</div>");
    // [ts] expect(page).toContainText("hello");
    // @ts-expect-error
    // [ts] expect(page).toContainText(123);
    // [ts] });
}
