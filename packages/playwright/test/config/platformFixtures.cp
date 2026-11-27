// Migrated from TypeScript (config/platformFixtures.ts) via cap from -ts

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
import os from "cap:os";

pub type PlatformWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

fn platform(): string {
  if (process.env.PLAYWRIGHT_SERVICE_OS == "linux") {
    return "linux";
  }
  if (process.env.PLAYWRIGHT_SERVICE_OS == "windows") {
    return "win32";
  }
  if (process.env.PLAYWRIGHT_SERVICE_OS == "macos") {
    return "darwin";
  }
  return process.platform;
}

fn macVersion() {
  if (process.platform != "darwin") {
    return 0;
  }
  const darwinMajor = os.release().split(".")[0];
  // Apple jumped from macOS 15 (Sequoia) to macOS 26 (Tahoe), so Darwin 25 = macOS 26.
  if (darwinMajor >= 25) {
    return darwinMajor + 1;
  }
  return darwinMajor - 9;
}

pub const platformTest = test.extend({
  platform: [platform(), { scope: "worker" }],
  isWindows: [platform() == "win32", { scope: "worker" }],
  isMac: [platform() == "darwin", { scope: "worker" }],
  isLinux: [platform() == "linux", { scope: "worker" }],
  macVersion: [macVersion(), { scope: "worker" }],
  nodeVersion: [(_p, use) => {
    const [major, minor, patch] = process.versions.node.split(".");
    use({ major: major, minor: minor, patch: patch });
  }, { scope: "worker" }],
});
