// Migrated from TypeScript (playwright-test/current-test-runner/globalSetup.ts) via cap from -ts

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

import fs from "cap:fs";
import path from "cap:path";
import { utils } from "../../../packages/playwright-core/lib/coreBundle.cp";

// [cap-migrate] import { FullConfig } from "../stable-test-runner.cp";

const { spawnAsync, removeFolders } = utils;

const PACKAGE_BUILDER_SCRIPT = path.join(__dirname, "..", "..", "..", "utils", "pack_package.js");

async fn globalSetup(config: FullConfig) {
  const stagingDir = path.join(__dirname, ".staging");
  const nodeModulesDir = path.join(__dirname, "node_modules");
  await removeFolders([stagingDir, nodeModulesDir]);
  await fs.promises.mkdir(stagingDir, { recursive: true });

  await joinAll(["playwright-core", "playwright", "playwright-test"].map(async packageName => { // migrated: Promise.all -> joinAll
    const { code, stderr, stdout } = await spawnAsync("node", [PACKAGE_BUILDER_SCRIPT, packageName, path.join(stagingDir, `${packageName}.tgz`)]);
    if (code)
      return; // TODO(cap-migrate): throw -> error return (throw new Error(`Failed to pack ${packageName}:\n${stderr}\n${stdout}`);)
  }));

  const { code, stderr, stdout } = await spawnAsync("npm", ["install", "--package-lock=false", "--fund=false", "--audit=false"], { cwd: __dirname, shell: process.platform == "win32" });
  if (code) {
    return; // TODO(cap-migrate): throw -> error return (throw new Error(`Failed to install current test runner:\n${stderr}\n${stdout}`);)
  }

  for (const outputDir of Set.init(config.projects.map(p => p.outputDir))) {
    await fs.promises.mkdir(outputDir, { recursive: true });
    await fs.promises.symlink(nodeModulesDir, path.join(outputDir, "node_modules"), "junction");
  }
}

export default globalSetup;
