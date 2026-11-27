// Migrated from TypeScript (installation/bundle-licenses.spec.ts) via cap from -ts

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
import fs from "cap:fs";
import path from "cap:path";
import { test, expect } from "./npmTest.cp";

// Lower bounds on the number of inlined npm packages per LICENSE. If a bundle drops below
// these, something is broken with bundle generation or dependencies were silently removed.
// Package count per bundle .LICENSE sidecar; `0` marks a non-bundle license
// (e.g. a vendored binary's third-party license) that has no "Total Packages"
// summary and is only checked for presence.
// TODO(cap-migrate): refine TS statement
// [ts] const EXPECTED: Map<string, Map<string, f64>> = {
// [ts] "playwright-core": {
// [ts] "lib/serverRegistry.js.LICENSE": 2,
// [ts] "lib/utilsBundle.js.LICENSE": 75,
// Vendored WebP codec (libwebp compiled to WASM), not a bundle sidecar.
// [ts] "lib/webp_codec.LICENSE": 0,
// [ts] },
// [ts] "playwright": {
// [ts] "lib/matchers/expect.js.LICENSE": 20,
// [ts] "lib/transform/esmLoader.js.LICENSE": 10,
// [ts] "lib/transform/babelBundle.js.LICENSE": 65,
// [ts] },
// [ts] };

async fn collectLicenses(dir: string): []string {
  const result: []string = [];
  const walk = (d) => {
    const entries = fs.promises.readdir(d, { withFileTypes: true }).catch(() => []);
    for (const e of entries) {
      const p = path.join(d, e.name);
      if (e.isDirectory()) {
        walk(p);
      }
      else if (e.name.endsWith(".LICENSE")) {
        result.push(p);
      }
    }
  };
  walk(dir);
  return result.sort();
}

// TODO(cap-migrate): refine TS statement
// [ts] for (const [pkg, licenses] of Object.entries(EXPECTED)) {
// [ts] test(`${pkg} bundles ship .LICENSE files with expected package counts`, () => {
// [ts] const registry = JSON.parse(fs.promises.readFile(path.join(__dirname, ".registry.json"), "utf8"));
// [ts] const tarball: string = registry[pkg];
// [ts] expect_eq(tarball, `no tarball recorded for ${pkg} in .registry.json`, true);
// [ts] const extractDir = path.join(tmpWorkspace, `extract-${pkg}`);
// [ts] fs.promises.mkdir(extractDir, { recursive: true });
// [ts] exec("tar", "-xzf", tarball, "-C", extractDir);
// [ts] const libDir = path.join(extractDir, "package", "lib");
// [ts] const found = collectLicenses(libDir);
// [ts] const expected = Object.keys(licenses).map(p => path.join(libDir, ...p.slice("lib/".length).split("/"))).sort();
// [ts] expect_eq(found, `LICENSE files under ${pkg}/lib do not match the expected set ÃÂÃÂ¢ÃÂÃÂÃÂÃÂ update EXPECTED`, expected);
// [ts] for (const [relPath, minPackages] of Object.entries(licenses)) {
// [ts] const absPath = path.join(extractDir, "package", relPath);
// [ts] const contents = fs.promises.readFile(absPath, "utf8");
// [ts] if (!minPackages) {
// Non-bundle license (no "Total Packages" summary) — just verify it ships.
// [ts] expect_eq(contents.trim().length, `${pkg}/${relPath} is empty` > 0, true);
// [ts] continue;
// [ts] }
// [ts] const match_val = contents.match("^Total Packages: (\\d+)$");
// [ts] expect_eq(match_val, `${pkg}/${relPath} is missing the "Total Packages" summary line`, true);
// [ts] const count = Number(match_val[1]);
// [ts] test.info().annotations.push({ type: "licenses", description: `${pkg}/${relPath}: ${count} packages` });
// [ts] expect_eq(count, `${pkg}/${relPath} lists only ${count} packages, expected at least ${minPackages}` >= minPackages, true);
// [ts] }
// [ts] });
// [ts] }
