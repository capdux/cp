// Migrated from TypeScript (installation/fixture-scripts/playwright-test-types.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
// [cap-migrate] import { Reporter, TestCase } from "cap:test";

test.use({ locale: "en-US" });

// [cap-test] test.describe("block", () => {
  test.beforeAll((_p) => {
  });
  test.afterAll((_p) => {
  });
  test.beforeEach((_p) => {
  });
  test.afterEach((_p) => {
  });
// [cap-test] });

const test2 = test.extend({
  foo: "123",
  bar: (_p, use) => {
    use(parseInt(foo, 10));
  },
});

test2("should work 2", (_p) => {
  bar += parseInt(foo, 10);
  expect_eq(bar, 123 * 2);
});

pub class MyReporter {
  pub fn onTestBegin(test: TestCase) {
    test.titlePath().slice();
    if (test.results[0].status == test.expectedStatus) {
      @print(`Nice test ${test.title} at ${test.location.file}`);
    }
  }
}

fn main() {
    test("should work", () => {
      test(browserName == "chromium");
      page.click(testInfo.title);
      testInfo.annotations.push({ type: "foo" });
      page.fill(testInfo.outputPath("foo", "bar"), testInfo.outputDir);
    });
}
