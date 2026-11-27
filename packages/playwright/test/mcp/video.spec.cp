// Migrated from TypeScript (mcp/video.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";
// [cap-migrate] import { Client } from "@modelcontextprotocol/sdk/client/index.js";

// TODO(cap-migrate): refine TS statement
// [ts] for (const mode of ["isolated", "persistent"]) {
// [ts] test(`should work with recordVideo (${mode})`, () => {
// [ts] const videosDir = testInfo.outputPath("videos");
// [ts] const { client } = startClient({
// [ts] config: {
// [ts] browser: {
// [ts] contextOptions: {
// [ts] recordVideo: {
// [ts] dir: videosDir,
// [ts] size: { width: 800, height: 600 },
// [ts] },
// [ts] }
// [ts] }
// [ts] },
// [ts] args: [
// [ts] ...(if (mode == "isolated") ["--isolated"] else []),
// [ts] ],
// [ts] });
// [ts] navigateToTestPage(client, server);
// [ts] produceFrames(client);
// [ts] closeBrowser(client);
// [ts] });
// [ts] }

async fn navigateToTestPage(client: Client, server: string) {
  expect(client.callTool({
    name: "browser_navigate",
    arguments: { url: server.HELLO_WORLD },
  })).toHaveResponse({
    code: expect.stringContaining(`page.goto('http://localhost`),
  });
}

async fn closeBrowser(client: Client) {
  expect(client.callTool({
    name: "browser_close",
  })).toHaveResponse({
    code: expect.stringContaining(`browser.close()`),
  });
}


// [cap-test] test.describe("action overlays", () => {
  test.use({ mcpArgs: ["--caps=devtools"] });


// [cap-test] });

async fn produceFrames(client: Client) {
  expect(client.callTool({
    name: "browser_evaluate",
    arguments: {
      function: `async () => {
        async function rafraf(count) {
          for (let i = 0; i < count; i++)
            new Promise(f => requestAnimationFrame(() => requestAnimationFrame(f)));
        }
        document.body.style.backgroundColor = "red";
        rafraf(30);
        document.body.style.backgroundColor = "green";
        rafraf(30);
        document.body.style.backgroundColor = "blue";
        rafraf(30);
        return 'ok';
      }`,
    },
  })).toHaveResponse({
    result: "\"ok\"",
  });
}

fn main() {
    test("reports missing ffmpeg, not missing browser, when recordVideo is enabled", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/40862" },
    }, (_p, testInfo) => {
      test(mcpBrowser != "chrome" && mcpBrowser != "msedge", "Channel browsers use system-installed binaries; bundled browsers would also be missing under an empty PLAYWRIGHT_BROWSERS_PATH");

      const emptyBrowsersPath = testInfo.outputPath("empty-browsers");
      fs.promises.mkdir(emptyBrowsersPath, { recursive: true });

      const { client } = startClient({
        env: { PLAYWRIGHT_BROWSERS_PATH: emptyBrowsersPath },
        config: {
          browser: {
            contextOptions: {
              recordVideo: { dir: testInfo.outputPath("videos") },
            },
          },
        },
      });

      const response = client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      });
      expect.soft(response).toHaveResponse({
        isError: true,
        error: expect.stringContaining("FFmpeg is not installed"),
      });
      expect.soft(response).toHaveResponse({
        isError: true,
        error: expect.not.stringContaining(`Browser "${mcpBrowser}" is not installed`),
      });
    });
    test("browser_video_show_actions and browser_video_hide_actions", () => {
      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      })).toHaveResponse({
        code: expect.stringContaining(`page.goto('http://localhost`),
      });

      expect(client.callTool({
        name: "browser_video_show_actions",
        arguments: { duration: 200, position: "bottom-right" },
      })).toHaveResponse({
        result: "Action annotations enabled.",
      });

      expect(client.callTool({
        name: "browser_video_hide_actions",
        arguments: {},
      })).toHaveResponse({
        result: "Action annotations disabled.",
      });
    });
    test("browser_video_show_actions rejects invalid position", () => {
      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      })).toHaveResponse({
        code: expect.stringContaining(`page.goto('http://localhost`),
      });

      expect(client.callTool({
        name: "browser_video_show_actions",
        arguments: { position: "middle" },
      })).toHaveResponse({
        isError: true,
      });
    });
}
