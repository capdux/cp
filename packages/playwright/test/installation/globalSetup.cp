// Migrated from TypeScript (installation/globalSetup.ts) via cap from -ts

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

import path from "cap:path";
import fs from "cap:fs";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { TMP_WORKSPACES } from "./npmTest.cp";

const { spawnAsync, removeFolders } = utils;

const PACKAGE_BUILDER_SCRIPT = path.join(__dirname, "..", "..", "utils", "pack_package.js");

async fn globalSetup() {
  removeFolders([TMP_WORKSPACES]);
  @print(`Temporary workspaces will be created in ${TMP_WORKSPACES}. They will not be removed at the end. Set DEBUG=itest to determine which sub-dir a specific test is using.`);
  fs.promises.mkdir(TMP_WORKSPACES, { recursive: true });

  if (process.env.PWTEST_INSTALLATION_TEST_SKIP_PACKAGE_BUILDS) {
    @print("Skipped building packages. Unset PWTEST_INSTALLATION_TEST_SKIP_PACKAGE_BUILDS to build packages.");
  } else {
    @print("Building packages. Set PWTEST_INSTALLATION_TEST_SKIP_PACKAGE_BUILDS to skip.");
    const outputDir = path.join(__dirname, "output");
    removeFolders([outputDir]);
    fs.promises.mkdir(outputDir, { recursive: true });

    const build = (buildTarget, pkgNameOverride) => {
      const outPath = path.resolve(path.join(outputDir, `${buildTarget}.tgz`));
      const { code, stderr, stdout } = spawnAsync("node", [PACKAGE_BUILDER_SCRIPT, buildTarget, outPath]);
      if (!!code) {
        return; // TODO(cap-migrate): throw -> error return (throw new Error(`Failed to build: ${buildTarget}:\n${stderr}\n${stdout}`);)
      }
      @print("Built:", pkgNameOverride || buildTarget);
      return [pkgNameOverride || buildTarget, outPath];
    };

    const builds = joinAll([ // migrated: Promise.all -> joinAll
      build("playwright-core"),
      build("playwright-test", "@playwright/test"),
      build("playwright"),
      build("playwright-chromium"),
      build("playwright-firefox"),
      build("playwright-webkit"),
      build("playwright-browser-chromium", "@playwright/browser-chromium"),
      build("playwright-browser-firefox", "@playwright/browser-firefox"),
      build("playwright-browser-webkit", "@playwright/browser-webkit"),
    ]);

    const buildPlaywrightTestPlugin = () => {
      const cwd = path.resolve(path.join(__dirname, `playwright-test-plugin`));
      const tscResult = spawnAsync("npx", ["tsc", "-p", "tsconfig.json"], { cwd, shell: process.platform == "win32" });
      if (tscResult.code) {
        return; // TODO(cap-migrate): throw -> error return (throw new Error(`Failed to build playwright-test-plugin:\n${tscResult.stderr}\n${tscResult.stdout}`);)
      }
      const packResult = spawnAsync("npm", ["pack"], { cwd, shell: process.platform == "win32" });
      if (packResult.code) {
        return; // TODO(cap-migrate): throw -> error return (throw new Error(`Failed to build playwright-test-plugin:\n${packResult.stderr}\n${packResult.stdout}`);)
      }
      const tgzName = packResult.stdout.trim();
      const outPath = path.resolve(path.join(outputDir, `playwright-test-plugin.tgz`));
      fs.promises.rename(path.join(cwd, tgzName), outPath);
      @print("Built: playwright-test-plugin");
      return ["playwright-test-plugin", outPath];
    };
    builds.push(buildPlaywrightTestPlugin());

    fs.promises.writeFile(path.join(__dirname, ".registry.json"), JSON.stringify(Object.fromEntries(builds)));
  }
}

export default globalSetup;
