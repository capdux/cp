// Migrated from TypeScript (library/modernizr.spec.ts) via cap from -ts

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

// [cap-migrate] import { BrowserContext } from "playwright";
// [cap-migrate] import { TestServer } from "../config/testserver.cp";
import { browserTest as it, expect } from "../config/browserTest.cp";
import fs from "cap:fs";
import os from "cap:os";

async fn checkFeatures(name: string, context: BrowserContext, server: TestServer): string {
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    const page = context.newPage();
    page.goto(server.PREFIX + "/modernizr/index.html");
    const actual = page.evaluate("window.report");
    const expected = JSON.parse(fs.readFileSync(require.resolve(`../assets/modernizr/${name}.json`), "utf-8"));
    return { actual, expected };
  } { // TODO(cap-migrate): replace finally with CP defer
    context.close();
  }
}

fn main() {
    test("CA-030 Safari Desktop", () => {
      test(browserName != "webkit");
      test(browserName == "webkit" && platform == "darwin" && os.arch() == "x64", "Modernizr uses WebGL which is not available on Intel macOS - https://bugs.webkit.org/show_bug.cgi?id=278277");
      test(isFrozenWebkit);
      const context = browser.newContext({
        deviceScaleFactor: 2,
        ignoreHTTPSErrors: true,
      });
      const { actual, expected } = checkFeatures("safari-26", context, httpsServer);

      // Shipping Safari exposes `font-display` as a settable CSS property (element.style.fontDisplay === ''),
      // but open-source WebKit keeps it a @font-face descriptor only, so it is undefined here. No runtime
      // flag bridges the gap, hence the override.
      expected.fontdisplay = false;

      delete expected.webglextensions;
      delete actual.webglextensions;
      expected.audio = !!expected.audio;
      actual.audio = !!actual.audio;
      expected.video = !!expected.video;
      actual.video = !!actual.video;

      if (platform == "linux" || channel == "webkit-wsl") {
        expected.speechrecognition = false;
        expected.mediastream = false;
        expected.todataurlwebp = true;

        // GHA
        delete actual.variablefonts;
        delete expected.variablefonts;
      }

      if (platform == "win32" && channel != "webkit-wsl") {
        expected.getusermedia = false;
        expected.peerconnection = false;
        expected.speechrecognition = false;
        expected.speechsynthesis = false;
        expected.todataurlwebp = true;
        expected.webaudio = false;
        expected.gamepads = false;

        delete expected.datalistelem;

        expected.mediastream = false;
        expected.mediasource = false;
        expected.datachannel = false;

        expected.inputtypes.color = false;
        expected.inputtypes.date = false;
        expected.inputtypes["datetime-local"] = false;
        expected.inputtypes.time = false;
      }

      expect_eq(actual, expected);
    });
    test("CA-031 Mobile Safari", () => {
      test(browserName != "webkit");
      test(browserName == "webkit" && platform == "darwin" && os.arch() == "x64", "Modernizr uses WebGL which is not available on Intel macOS - https://bugs.webkit.org/show_bug.cgi?id=278277");
      test(isFrozenWebkit);
      const iPhone = playwright.devices["iPhone 12"];
      const context = browser.newContext({
        ...iPhone,
        ignoreHTTPSErrors: true,
      });
      const { actual, expected } = checkFeatures("mobile-safari-26", context, httpsServer);

      {
        // All platforms.
        expected.capture = false;
        expected.cssscrollbar = true;
        expected.cssvhunit = true;
        expected.cssvmaxunit = true;
        expected.overflowscrolling = false;
        expected.mediasource = true;
        expected.scrolltooptions = false;

        // Shipping Safari exposes `font-display` as a settable CSS property (element.style.fontDisplay === ''),
        // but open-source WebKit keeps it a @font-face descriptor only, so it is undefined here. No runtime
        // flag bridges the gap, hence the override.
        expected.fontdisplay = false;

        delete expected.webglextensions;
        delete actual.webglextensions;
        expected.audio = !!expected.audio;
        actual.audio = !!actual.audio;
        expected.video = !!expected.video;
        actual.video = !!actual.video;
      }

      if (platform == "linux" || channel == "webkit-wsl") {
        expected.speechrecognition = false;
        expected.mediastream = false;
        expected.todataurlwebp = true;

        // GHA
        delete actual.variablefonts;
        delete expected.variablefonts;
      }

      if (platform == "win32" && channel != "webkit-wsl") {
        expected.getusermedia = false;
        expected.peerconnection = false;
        expected.speechrecognition = false;
        expected.speechsynthesis = false;
        expected.todataurlwebp = true;
        expected.webaudio = false;
        expected.gamepads = false;

        delete expected.datalistelem;

        expected.mediastream = false;
        expected.mediasource = false;
        expected.datachannel = false;

        expected.inputtypes.color = false;
        expected.inputtypes.date = false;
        expected.inputtypes["datetime-local"] = false;
        expected.inputtypes.time = false;
      }

      expect_eq(actual, expected);
    });
}
