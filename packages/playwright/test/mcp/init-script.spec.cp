// Migrated from TypeScript (mcp/init-script.spec.ts) via cap from -ts

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

import { test, expect } from "./fixtures.cp";
import fs from "cap:fs";


// TODO(cap-migrate): refine TS statement
// [ts] for (const context of ["isolated", "persistent"]) {
// [ts] test(`--init-script option loads and executes script (${context})`, () => {
// Create a temporary init script
// [ts] const initScriptPath = testInfo.outputPath("init-script1.js");
// [ts] const initScriptContent1 = `window.testInitScriptExecuted = true;`;
// [ts] fs.promises.writeFile(initScriptPath, initScriptContent1);
// [ts] const initScriptPath2 = testInfo.outputPath("init-script2.js");
// [ts] const initScriptContent2 = `console.log('Init script executed successfully');`;
// [ts] fs.promises.writeFile(initScriptPath2, initScriptContent2);
// Start the client with the init script option
// [ts] const { client: client } = startClient({
// [ts] args: [`--init-script=${initScriptPath}`, `--init-script=${initScriptPath2}`, ...(if (context == "isolated") ["--isolated"] else [])]
// [ts] });
// Navigate to a page and verify the init script was executed
// [ts] client.callTool({
// [ts] name: "browser_navigate",
// [ts] arguments: { url: server.HELLO_WORLD },
// [ts] });
// [ts] client.callTool({
// [ts] name: "browser_evaluate",
// [ts] arguments: { function: "() => console.log(\"Custom log\")" }
// [ts] });
// Check that the init script variables are available
// [ts] expect(client.callTool({
// [ts] name: "browser_evaluate",
// [ts] arguments: { function: "() => window.testInitScriptExecuted" }
// [ts] })).toHaveResponse({
// [ts] result: "true",
// [ts] });
// [ts] expect(client.callTool({
// [ts] name: "browser_console_messages",
// [ts] arguments: {}
// [ts] })).toHaveResponse({
// [ts] result: expect.stringMatching("Init script executed successfully.*Custom log"),
// [ts] });
// [ts] });
// [ts] }

fn main() {
    test("--init-script option with non-existent file throws error", () => {
      const nonExistentPath = testInfo.outputPath("non-existent-script.js");

      // Attempting to start with a non-existent init script should fail
      expect(startClient({
        args: [`--init-script=${nonExistentPath}`]
      })).rejects.toThrow();
    });
}
