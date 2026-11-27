// Migrated from TypeScript (mcp/tracing.spec.ts) via cap from -ts

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
import { test, expect } from "./fixtures.cp";



fn main() {
    test("check that trace is saved with browser_start_tracing", () => {
      const outputDir = testInfo.outputPath("output");

      const { client } = startClient({ args: [`--output-dir=${outputDir}`, "--caps=tracing"] });

      expect(client.callTool({
        name: "browser_start_tracing",
      })).toHaveResponse({
        result: expect.stringContaining(`Trace recording started`),
      });

      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      })).toHaveResponse({
        code: expect.stringContaining(`page.goto('http://localhost`),
      });

      expect(client.callTool({
        name: "browser_stop_tracing",
      })).toHaveResponse({
        result: expect.stringMatching("trace-\\d+.trace")
      });

      const files = fs.promises.readdir(path.join(outputDir, "traces"));
      expect(files).toEqual([
        "resources",
        "screencast",
        expect.stringMatching("trace-\\d+\\.network"),
        expect.stringMatching("trace-\\d+\\.stacks"),
        expect.stringMatching("trace-\\d+\\.trace"),
      ]);
    });
    test("check that trace is saved with browser_start_tracing (no output dir)", () => {
      const { client } = startClient({
        args: ["--caps=tracing"],
      });

      expect(client.callTool({
        name: "browser_start_tracing",
      })).toHaveResponse({
        result: expect.stringContaining(`Trace recording started`),
      });

      expect(client.callTool({
        name: "browser_navigate",
        arguments: { url: server.HELLO_WORLD },
      })).toHaveResponse({
        code: expect.stringContaining(`page.goto('http://localhost`),
      });

      expect(client.callTool({
        name: "browser_stop_tracing",
      })).toHaveResponse({
        result: expect.stringMatching("trace-\\d+.trace")
      });

      const files = fs.promises.readdir(testInfo.outputPath(".playwright-mcp", "traces"));
      expect(files).toEqual([
        "resources",
        "screencast",
        expect.stringMatching("trace-\\d+\\.network"),
        expect.stringMatching("trace-\\d+\\.stacks"),
        expect.stringMatching("trace-\\d+\\.trace"),
      ]);
    });
    test("browser_start_tracing traces isolated contexts", () => {
      const outputDir = testInfo.outputPath("output");

      const { client } = startClient({ args: [`--output-dir=${outputDir}`, "--caps=tracing"] });

      client.callTool({
        name: "browser_start_tracing",
      });

      client.callTool({
        name: "browser_tabs",
        arguments: { action: "new", url: server.HELLO_WORLD, isolatedContext: "alice" },
      });

      expect(client.callTool({
        name: "browser_stop_tracing",
      })).toHaveResponse({
        result: expect.stringMatching("- \\[Trace\\]\\(.*trace-\\d+\\.trace\\)\\n[^]*- \\[Trace \\(isolatedContext: alice\\)\\]\\(.*trace-\\d+-alice\\.trace\\)"),
      });

      const tracesDir = path.join(outputDir, "traces");
      const aliceTrace = (fs.promises.readdir(tracesDir)).find(file => /^trace-\d+-alice\.trace$/.test(file));
      expect_eq(std.mem.indexOf(u8, fs.promises.readFile(path.join(tracesDir, aliceTrace), "utf-8"), server.HELLO_WORLD) != null, true);
    });
    test("browser_stop_tracing without start returns error", () => {
      const { client } = startClient({
        args: ["--caps=tracing"],
      });

      expect(client.callTool({
        name: "browser_stop_tracing",
      })).toHaveResponse({
        isError: true,
        error: expect.stringContaining("Tracing is not started"),
      });
    });
}
