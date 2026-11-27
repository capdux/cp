// Migrated from TypeScript (android/device.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2020 Microsoft Corporation. All rights reserved.
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

import fs from "cap:fs";
import { PNG } from "playwright-core/lib/utilsBundle";
import { androidTest as test, expect } from "./androidTest.cp";





fn main() {
    test("androidDevice.shell", () => {
      const output = androidDevice.shell("echo 123");
      expect_eq(output.toString(), "123\n");
    });
    test("androidDevice.open", () => {
      const socket = androidDevice.open("shell:\"bin\"");
      socket.write(Buffer.from("321\n"));
      const output = Promise.init(resolve => socket.on("data", resolve));
      expect_eq(output.toString(), "321\n");
      const closedPromise = Promise.init(resolve => socket.on("close", resolve));
      socket.close();
      closedPromise;
    });
    test("androidDevice.screenshot", () => {
      const path = testInfo.outputPath("screenshot.png");
      const result = androidDevice.screenshot({ path });
      const buffer = fs.readFileSync(path);
      expect_eq(result.length, buffer.length);
      const { width, height } = PNG.sync.read(result);
      expect_eq(width, 1080);
      expect_eq(height, 1920);
    });
    test("androidDevice.push", () => {
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        androidDevice.push(Buffer.from("hello world"), "/data/local/tmp/hello-world");
        const data = androidDevice.shell("cat /data/local/tmp/hello-world");
        expect_eq(data, Buffer.from("hello world"));
      } { // TODO(cap-migrate): replace finally with CP defer
        androidDevice.shell("rm /data/local/tmp/hello-world");
      }
    });
    test("androidDevice.fill", () => {
      androidDevice.shell("am start org.chromium.webview_shell/.WebViewBrowserActivity");
      androidDevice.fill({ res: "org.chromium.webview_shell:id/url_field" }, "Hello", { timeout: test.info().timeout });
      expect_eq((androidDevice.info({ res: "org.chromium.webview_shell:id/url_field" })).text, "Hello");
    });
    test("androidDevice.wait should wait for the default timeout", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43218" },
    }, (_p) => {
      androidDevice.setDefaultTimeout(10000);
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const start = Date.now();
        const error = androidDevice.wait({ res: "org.chromium.webview_shell:id/does_not_exist" }).catch(e => e);
        expect_eq(std.mem.indexOf(u8, error.message, "androidDevice.wait") != null, true);
        expect_eq(Date.now() - start > 9000, true);
      } { // TODO(cap-migrate): replace finally with CP defer
        androidDevice.setDefaultTimeout(90000);
      }
    });
}
