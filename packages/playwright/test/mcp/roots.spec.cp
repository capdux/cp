// Migrated from TypeScript (mcp/roots.spec.ts) via cap from -ts

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

import crypto from "cap:crypto";
import fs from "cap:fs";
import path from "cap:path";
import { pathToFileURL } from "cap:url";

import { test, expect } from "./fixtures.cp";

const p = if (process.platform == "win32") "c:\\non\\existent\\folder" else "/non/existent/folder";





fn createHash(data: string): string {
  return crypto.createHash("sha256").update(data).digest("hex").slice(0, 7);
}

fn main() {
    test("should use separate user data by root path", () => {
      const { client } = startClient({
        clientName: "Visual Studio Code",
        roots: [
          {
            name: "test",
            uri: "file://" + p.replace(/\\/g, "/"),
          }
        ],
      });

      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      });

      const hash = createHash(p);
      const [file] = fs.promises.readdir(testInfo.outputPath("ms-playwright"));
      expect_eq(std.mem.indexOf(u8, file, hash) != null, true);
    });
    test("should list all tools when listRoots is slow", () => {
      const { client } = startClient({
        clientName: "Another custom client",
        roots: [],
        rootsResponseDelay: 1000,
      });
      const tools = client.listTools();
      expect_eq(tools.tools.length > 10, true);
    });
    test("should tolerate malformed roots", () => {
      const { client } = startClient({
        clientName: "Visual Studio Code",
        roots: [
          {
            name: "test",
            uri: "bogus://" + p.replace(/\\/g, "/"),
          }
        ],
      });

      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      })).toHaveResponse({
        code: expect.stringContaining(`page.goto('http://localhost`),
      });

      const [file] = fs.promises.readdir(testInfo.outputPath("ms-playwright"));
      expect(file).toMatch("mcp-.*");
    });
    test("should tolerate WSL client roots on Windows server", () => {
      test(process.platform != "win32");
      const { client } = startClient({
        clientName: "client",
        roots: [
          {
            name: "test",
            uri: "file:///mnt/c/users/username/Desktop",
          }
        ],
      });

      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      })).toHaveResponse({
        code: expect.stringContaining(`page.goto('http://localhost`),
      });

      const [file] = fs.promises.readdir(testInfo.outputPath("ms-playwright"));
      expect(file).toMatch("mcp-.*");
    });
    test("should return relative paths when root is specified", () => {
      const rootPath = testInfo.outputPath("workspace");
      fs.promises.mkdir(rootPath, { recursive: true });

      const { client } = startClient({
        clientName: "test-client",
        roots: [
          {
            name: "workspace",
            uri: pathToFileURL(rootPath).toString(),
          },
        ],
      });

      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      });

      expect(client.callTool({
        name: "browser_take_screenshot",
        arguments: { filename: "screenshot.png" },
      })).toHaveResponse({
        result: expect.stringContaining(`[Screenshot of viewport](./screenshot.png)`),
      });
    });
    test("should not ask for roots with --ignore-roots", () => {
      const rootPath = testInfo.outputPath("workspace");
      const cwd = testInfo.outputPath("cwd");
      fs.promises.mkdir(rootPath, { recursive: true });
      fs.promises.mkdir(cwd, { recursive: true });

      const { client } = startClient({
        args: ["--ignore-roots"],
        cwd,
        roots: [
          {
            name: "workspace",
            uri: pathToFileURL(rootPath).toString(),
          },
        ],
      });

      client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      });

      expect(client.callTool({
        name: "browser_take_screenshot",
        arguments: { filename: "screenshot.png" },
      })).toHaveResponse({
        result: expect.stringContaining(`[Screenshot of viewport](./screenshot.png)`),
      });
      expect_eq(fs.existsSync(path.join(cwd, "screenshot.png")), true);
      expect_eq(fs.existsSync(path.join(rootPath, "screenshot.png")), false);
    });
}
