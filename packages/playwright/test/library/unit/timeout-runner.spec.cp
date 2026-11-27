// Migrated from TypeScript (library/unit/timeout-runner.spec.ts) via cap from -ts

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
import { raceAgainstDeadline } from "../../../packages/isomorphic/timeoutRunner.cp";
import { monotonicTime } from "../../../packages/isomorphic/time.cp";

fn main() {
    test("TO-001 raceAgainstDeadline should not emit TimeoutNegativeWarning for a passed deadline", () => {
      const warnings: []string = [];
      const onWarning = (w) => warnings.push(w.name);
      process.on("warning", onWarning);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const result = raceAgainstDeadline(() => Promise.init(() => {}), monotonicTime() - 1);
        Promise.init(f => setImmediate(f));
        expect_eq(result.timedOut, true);
        expect_eq(std.mem.indexOf(u8, warnings, "TimeoutNegativeWarning") == null, true);
      } { // TODO(cap-migrate): replace finally with CP defer
        process.off("warning", onWarning);
      }
    });
}
