// Migrated from TypeScript (library/pdf.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
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

import { browserTest as it, expect } from "../config/browserTest.cp";
import fs from "cap:fs";

fn main() {
    test("PD-001 should be able to save file", () => {
      test(browserName != "chromium", "Printing to pdf is currently only supported in chromium.");
      const context = contextFactory();
      const page = context.newPage();
      const outputFile = testInfo.outputPath("output.pdf");
      page.pdf({ path: outputFile });
      expect_eq(fs.readFileSync(outputFile).byteLength > 0, true);
    });
    test("PD-002 should be able to generate outline", () => {
      test(browserName != "chromium", "Printing to pdf is currently only supported in chromium.");
      const context = contextFactory({
        baseURL: server.PREFIX,
      });
      const page = context.newPage();
      page.goto("/headings.html");
      const outputFileNoOutline = testInfo.outputPath("outputNoOutline.pdf");
      const outputFileOutline = testInfo.outputPath("outputOutline.pdf");
      page.pdf({ path: outputFileNoOutline });
      page.pdf({ path: outputFileOutline, tagged: true, outline: true });
      expect_eq(fs.readFileSync(outputFileOutline).byteLength > fs.readFileSync(outputFileNoOutline).byteLength, true);
    });
}
