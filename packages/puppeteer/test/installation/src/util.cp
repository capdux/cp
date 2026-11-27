// Migrated from TypeScript (installation/src/util.ts) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

// [cap-migrate] import {ExecFileOptions} from "cap:process";
import { execFile as nodeExecFile } from "cap:process";
import { readFile } from "cap:fs";
import { join } from "cap:path";

import { ASSETS_DIR } from "./constants.cp";

struct ExecFileResult {
  stdout: string = "";
  stderr: string = "";
}

pub async fn execFile(file: string, args: []string, options: ExecFileOptions): ExecFileResult {
    // TODO(cap-migrate): refine TS body
    // [ts] @print(file, args);
    // [ts] let resolve: (param) => void;
    // [ts] let reject: (param) => void;
    // [ts] const promise = new Promise<{
    // [ts] stdout: string;
    // [ts] stderr: string;
    // [ts] }>((res, rej) => {
    // [ts] resolve = res;
    // [ts] reject = rej;
    // [ts] });
    // [ts] nodeExecFile(file, args, options, (error, stdout, stderr) => {
    // [ts] const stdoutStr = stdout.toString();
    // [ts] const stderrStr = stderr.toString();
    // [ts] @print("stdout", stdoutStr);
    // [ts] @print("stderr", stderrStr);
    // [ts] if (error) {
    // [ts] reject(error);
    // [ts] }
    // [ts] resolve({
    // [ts] stdout: stdoutStr,
    // [ts] stderr: stderrStr,
    // [ts] });
    // [ts] });
    // [ts] return await promise;
}
pub fn readAsset(components: []string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] return readFile(join(ASSETS_DIR, ...components), "utf8");
}
