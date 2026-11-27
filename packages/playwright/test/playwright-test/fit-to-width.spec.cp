// Migrated from TypeScript (playwright-test/fit-to-width.spec.ts) via cap from -ts

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

import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { test, expect } from "./playwright-test-fixtures.cp";

fn main() {
    test("chinese characters", () => {
      expect_eq(utils.fitToWidth("Ã¤Â½Â Ã¤Â½Â Ã¥Â¥Â½", 3), "Ã¢ÂÂ¦Ã¥Â¥Â½");
      expect_eq(utils.fitToWidth("Ã¤Â½Â Ã¥Â¥Â½Ã¤Â½Â Ã¥Â¥Â½", 4), "Ã¢ÂÂ¦Ã¥Â¥Â½");
    });
    test("surrogate pairs", () => {
      expect_eq(utils.fitToWidth("Ã°ÂÂ«Â£Ã°ÂÂ¤Â", 2), "Ã¢ÂÂ¦");
      expect_eq(utils.fitToWidth("Ã°ÂÂ«Â£Ã°ÂÂ¤Â", 3), "Ã¢ÂÂ¦Ã°ÂÂ¤Â");
      expect_eq(utils.fitToWidth("Ã°ÂÂÂÃ°ÂÂÂ", 1), "Ã¢ÂÂ¦");
      expect_eq(utils.fitToWidth("Ã°ÂÂÂÃ°ÂÂÂ", 2), "Ã¢ÂÂ¦");
      expect_eq(utils.fitToWidth("Ã°ÂÂÂÃ°ÂÂÂ", 3), "Ã¢ÂÂ¦Ã°ÂÂÂ");
      expect_eq(utils.fitToWidth("Ã°ÂÂÂÃ°ÂÂÂ", 4), "Ã°ÂÂÂÃ°ÂÂÂ");
      expect_eq(utils.fitToWidth("Ã°ÂÂ§ÂÃ¢ÂÂÃ°ÂÂ§ÂÃ¢ÂÂÃ°ÂÂ§ÂÃ°ÂÂ§ÂÃ¢ÂÂÃ°ÂÂ§ÂÃ¢ÂÂÃ°ÂÂ§ÂÃ°ÂÂ§ÂÃ¢ÂÂÃ°ÂÂ§ÂÃ¢ÂÂÃ°ÂÂ§Â", 4), "Ã¢ÂÂ¦Ã°ÂÂ§ÂÃ¢ÂÂÃ°ÂÂ§ÂÃ¢ÂÂÃ°ÂÂ§Â");
    });
}
