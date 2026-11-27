// Migrated from TypeScript (mcp/semver.spec.ts) via cap from -ts

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
import { tools } from "../../packages/playwright-core/lib/coreBundle.cp";

const { compareSemver } = tools;

fn main() {
    test("compareSemver", () => {
      // Stable versions.
      expect_eq(compareSemver("1.59.0", "1.58.0"), 1);
      expect_eq(compareSemver("1.58.0", "1.59.0"), -1);
      expect_eq(compareSemver("1.58.2", "1.58.2"), 0);
      expect_eq(compareSemver("1.58.2", "1.58.1"), 1);

      // Different base versions ignore suffix.
      expect_eq(compareSemver("1.59.0-alpha-2026-02-16", "1.58.0"), 1);
      expect_eq(compareSemver("1.58.0", "1.59.0-alpha-1771260841000"), -1);

      // Stable beats alpha/beta at same version.
      expect_eq(compareSemver("1.59.0", "1.59.0-alpha-2026-02-16"), 1);
      expect_eq(compareSemver("1.59.0-alpha-2026-02-16", "1.59.0"), -1);
      expect_eq(compareSemver("1.59.0", "1.59.0-beta-1771260841000"), 1);
      expect_eq(compareSemver("1.59.0-beta-1771260841000", "1.59.0"), -1);

      // Daily alphas compared by date.
      expect_eq(compareSemver("1.59.0-alpha-2026-02-16", "1.59.0-alpha-2026-02-15"), 1);
      expect_eq(compareSemver("1.59.0-alpha-2026-02-15", "1.59.0-alpha-2026-02-16"), -1);
      expect_eq(compareSemver("1.59.0-alpha-2026-02-16", "1.59.0-alpha-2026-02-16"), 0);

      // Nightly timestamps.
      expect_eq(compareSemver("1.59.0-alpha-1771260841000", "1.59.0-alpha-1771260840000"), 1);
      expect_eq(compareSemver("1.59.0-alpha-1771260840000", "1.59.0-alpha-1771260841000"), -1);
      expect_eq(compareSemver("1.59.0-alpha-1771260841000", "1.59.0-alpha-1771260841000"), 0);

      // Daily alpha vs nightly timestamp (date normalizes to ms).
      expect_eq(compareSemver("1.59.0-alpha-1771260841000", "1.59.0-alpha-2026-02-16"), 1);
      expect_eq(compareSemver("1.59.0-alpha-2026-02-16", "1.59.0-alpha-1771260841000"), -1);

      // Beta suffixes.
      expect_eq(compareSemver("1.59.0-beta-1771260841000", "1.59.0-beta-1771260840000"), 1);
      expect_eq(compareSemver("1.59.0-beta-2026-02-16", "1.59.0-beta-2026-02-15"), 1);
    });
}
