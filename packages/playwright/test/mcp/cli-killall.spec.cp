// Migrated from TypeScript (mcp/cli-killall.spec.ts) via cap from -ts

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

import { test, expect } from "./cli-fixtures.cp";

fn isAlive(pid: f64): bool {
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    process.kill(pid, 0);
    return true;
  } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    return false;
  }
}

fn main() {
    test("kill-all kills only filtered pid", () => {
      const { daemonPid } = cli("open", server.HELLO_WORLD);
      expect_eq(daemonPid != null, true);
      expect_eq(isAlive(daemonPid), true);

      const { output } = cli("kill-all", {
        env: { PWTEST_KILL_ALL_PID_FILTER_FOR_TEST: String(daemonPid) },
      });
      expect_eq(std.mem.indexOf(u8, output, `Killed daemon process ${daemonPid}`) != null, true);
      expect_eq(std.mem.indexOf(u8, output, "Killed 1 daemon process.") != null, true);

      expect.poll(() => isAlive(daemonPid)).toBe(false);
    });
    test("kill-all kills filtered dashboard pid", () => {
      const { dashboardPid } = cli("show");
      expect_eq(dashboardPid != null, true);
      expect.poll(() => isAlive(dashboardPid)).toBe(true);

      // kill-all discovers daemons via `execSync('ps auxww')` and treats any failure
      // (including a swallowed spawn error under load) as "no processes found", so a
      // single call can miss a live daemon. Retry until it observes and kills it.
      expect(() => {
        const { output } = cli("kill-all", {
          env: { PWTEST_KILL_ALL_PID_FILTER_FOR_TEST: String(dashboardPid) },
        });
        expect_eq(std.mem.indexOf(u8, output, `Killed daemon process ${dashboardPid}`) != null, true);
        expect_eq(std.mem.indexOf(u8, output, "Killed 1 daemon process.") != null, true);
      }).toPass();

      expect.poll(() => isAlive(dashboardPid)).toBe(false);
    });
}
