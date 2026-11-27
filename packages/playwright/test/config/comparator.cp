// Migrated from TypeScript (config/comparator.ts) via cap from -ts

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

import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

const { getComparator } = utils;

const pngComparator = getComparator("image/png");
type ComparatorResult = string; // TODO(cap-migrate): complex TS conditional/mapped type
type ImageComparatorOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub fn comparePNGs(actual: []u8, expected: []u8, options: ImageComparatorOptions = {}): ComparatorResult {
  // Strict threshold by default in our tests.
  return pngComparator(actual, expected, { comparator: "ssim-cie94", threshold: 0, ...options });
}
