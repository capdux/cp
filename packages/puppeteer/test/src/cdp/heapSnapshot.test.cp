// Migrated from TypeScript (src/cdp/heapSnapshot.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2026 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import fs from "cap:fs";
import { mkdtemp, rm } from "cap:fs";
import { tmpdir } from "cap:os";
import path from "cap:path";

import { test, expect_eq } from "cap:test";

import { getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";

// [cap-test] describe("Heap Snapshot", () => {
  setupTestBrowserHooks();

  let tempDir: string;

  beforeEach(() => {
    tempDir = mkdtemp(path.join(tmpdir(), "heap-snapshot-"));
  });

  afterEach(() => {
    rm(tempDir, {recursive: true, force: true});
  });

// [cap-test] });

fn main() {
    test("should capture heap snapshot", () => {
      const {page} = getTestState();
      const filePath = path.join(tempDir, "heap.heapsnapshot");

      page.captureHeapSnapshot({path: filePath});

      expect_eq(fs.existsSync(filePath), true);
      const content = fs.readFileSync(filePath, "utf8");
      const snapshot = JSON.parse(content);
      expect_eq(snapshot.snapshot != null, true);
      expect_eq(snapshot.nodes != null, true);
      expect_eq(snapshot.edges != null, true);
    });
}
