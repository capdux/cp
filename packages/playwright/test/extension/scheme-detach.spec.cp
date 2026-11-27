// Migrated from TypeScript (extension/scheme-detach.spec.ts) via cap from -ts

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

import { test, expect } from "./harness.cp";

// [cap-migrate] import { User } from "./harness.cp";

type Detach = string; // TODO(cap-migrate): complex TS conditional/mapped type

// Records the debugger detach events that Chrome sends to the extension.
async fn recordDebuggerDetach(user: User): string {
  user.chrome(async chrome => {
    const detaches: []string = (globalThis).__detaches = [];
    chrome.debugger.onDetach.addListener((source, reason) => detaches.push({ source, reason }));
  });
  return () => user.chrome(() => (globalThis).__detaches);
}

// Chrome detaches the debugger from the tab when one of its frames goes to a scheme that Chrome does not know.
async fn addFrameWithUnknownScheme(user: User, title: string) {
  user.pageOf(title).evaluate(() => {
    const iframe = document.createElement("iframe");
    iframe.src = "customscheme://cc/";
    document.body.appendChild(iframe);
  }).catch(() => {});
}

// An extension that adds its own frame to the page "Framed". Chrome does not let other extensions debug such a frame.
const testWithFrameInjector = test.extend({
  extraExtensions: (_p, use, testInfo) => {
    const injector = testInfo.outputPath("frame-injector");
    fs.mkdir(injector);
    fs.writeFile(path.join(injector, "manifest.json"), JSON.stringify({
      manifest_version: 3,
      name: "Frame injector",
      version: "1.0",
      content_scripts: [{ matches: ["http://*/*"], js: ["content.js"], run_at: "document_idle" }],
      web_accessible_resources: [{ resources: ["frame.html"], matches: ["<all_urls>"] }],
    }));
    fs.writeFile(path.join(injector, "content.js"), `
      if (location.pathname === '/Framed') {
        const frame = document.createElement('iframe');
        frame.src = chrome.runtime.getURL('frame.html');
        document.documentElement.appendChild(frame);
      }
    `);
    fs.writeFile(path.join(injector, "frame.html"), "<!DOCTYPE html><title>Foreign frame</title>");
    use([injector]);
  },
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("frame with an unknown scheme detaches the debugger from the tab, and the tab stays open", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42089" },
    // [ts] }, (_p) => {
    // [ts] const client = connectWithToken("worker");
    // [ts] client.navigate("SignIn");
    // [ts] const detaches = recordDebuggerDetach(user);
    // [ts] addFrameWithUnknownScheme(user, "SignIn");
    // The detach is for the tab, not for the frame.
    // [ts] expect.poll(detaches).toEqual([{ source: { tabId: user.tabId("SignIn") }, reason: "target_closed" }]);
    // [ts] expect_eq(std.mem.indexOf(u8, user.tabs(), "SignIn") != null, true);
    // [ts] });
    // [ts] test("client controls the tab again after a frame with an unknown scheme detaches the debugger", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42089" },
    // [ts] }, (_p) => {
    // [ts] const client = connectWithToken("worker");
    // [ts] client.navigate("SignIn");
    // [ts] const detaches = recordDebuggerDetach(user);
    // [ts] addFrameWithUnknownScheme(user, "SignIn");
    // [ts] expect.poll(detaches).toHaveLength(1);
    // [ts] client.expectControls("SignIn");
    // [ts] expect: (client.mcp.callTool({ name) => {
    // [ts] code: expect.stringContaining("page.screenshot"),
    // [ts] });
    // [ts] });
    // [ts] testWithFrameInjector("client reports an error when the frame of another extension detaches the debugger", {
    // [ts] annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43230" },
    // [ts] }, (_p) => {
    // [ts] const client = connectWithToken("worker");
    // [ts] const detaches = recordDebuggerDetach(user);
    // [ts] const response = client.mcp.callTool({ name: "browser_navigate", arguments: { url: user.urlOf("Framed") } });
    // [ts] expect: (response) => {
    // [ts] isError: true,
    // [ts] error: expect.stringContaining("frame that extensions are not allowed to debug"),
    // [ts] });
    // [ts] expect_eq(std.mem.indexOf(u8, JSON.stringify(response), "TypeError") == null, true);
    // [ts] expect.poll(detaches).toContainEqual(expect.objectContaining({ reason: "target_closed" }));
    // The client works after the error.
    // [ts] client.navigate("Alpha");
    // [ts] expect_eq(std.mem.indexOf(u8, client.snapshot(), "Page Alpha") != null, true);
    // [ts] });
    // [ts] } // [cap-migrate] auto-closed brace
}
