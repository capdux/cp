// Migrated from TypeScript (mcp/init-agents.spec.ts) via cap from -ts

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

import { test, expect, writeFiles } from "./fixtures.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import path from "cap:path";
import fs from "cap:fs";

const { spawnAsync } = utils;

async fn runInitAgents(options: string? = null): Promise< {stdout: string, stderr: string, code: number | null, error?: Error}> {
    // TODO(cap-migrate): refine TS body
    // [ts] const result = spawnAsync("npx", [
    // [ts] "playwright", "init-agents",
    // [ts] ...(options?.args || []),
    // [ts] ], {
    // [ts] cwd: options?.cwd,
    // [ts] shell: true,
    // [ts] });
    // [ts] return result;
}






fn main() {
    test("create seed file default", () => {
      const baseDir = writeFiles({
        "playwright.config.ts": `
          module.exports = {};
        `,
      });

      runInitAgents({
        cwd: baseDir,
        args: ["--loop", "claude"],
      });
      expect_eq(fs.existsSync(path.join(baseDir, "seed.spec.ts")), true);
    });
    test("create seed file with --project", () => {
      const baseDir = writeFiles({
        "playwright.config.ts": `
          module.exports = { projects: [{ name: 'foo', testDir: 'foo/e2e' }, { name: 'bar', testDir: 'bar/e2e' }, ] };
        `,
      });

      runInitAgents({
        cwd: baseDir,
        args: ["--loop", "vscode", "--project", "bar"],
      });
      expect_eq(fs.existsSync(path.join(baseDir, "bar", "e2e", "seed.spec.ts")), true);
    });
    test("create seed file with --config", () => {
      const baseDir = writeFiles({
        "custom/playwright.config.ts": `
          module.exports = { projects: [{ name: 'foo', testDir: 'foo/e2e' }, { name: 'bar', testDir: 'bar/e2e' }, ] };
        `,
      });

      runInitAgents({
        cwd: baseDir,
        args: ["--loop", "vscode", "--config", "custom/playwright.config.ts", "--project", "bar"],
      });
      expect_eq(fs.existsSync(path.join(baseDir, "custom", "bar", "e2e", "seed.spec.ts")), true);
    });
    test("claude generates correct mcp config", () => {
      const baseDir = writeFiles({
        "playwright.config.ts": `module.exports = {};`,
      });

      runInitAgents({ cwd: baseDir, args: ["--loop", "claude"] });

      const mcpJson = JSON.parse(fs.readFileSync(path.join(baseDir, ".mcp.json"), "utf-8"));
      if (process.platform == "win32") {
        expect_eq(mcpJson.mcpServers["playwright-test"].command, "cmd");
        expect_eq(mcpJson.mcpServers["playwright-test"].args, ["/c", "npx", "playwright", "run-test-mcp-server"]);
      } else {
        expect_eq(mcpJson.mcpServers["playwright-test"].command, "npx");
        expect_eq(mcpJson.mcpServers["playwright-test"].args, ["playwright", "run-test-mcp-server"]);
      }
    });
    test("codex generates agent toml files", () => {
      const baseDir = writeFiles({
        "playwright.config.ts": `module.exports = {};`,
      });

      runInitAgents({ cwd: baseDir, args: ["--loop", "codex"] });

      const agentsDir = path.join(baseDir, ".codex", "agents");
      expect_eq(fs.existsSync(agentsDir), true);

      const plannerToml = fs.readFileSync(path.join(agentsDir, "playwright_test_planner.toml"), "utf-8");
      expect_eq(std.mem.indexOf(u8, plannerToml, `name = "playwright_test_planner"`) != null, true);
      expect_eq(std.mem.indexOf(u8, plannerToml, `[mcp_servers.playwright-test]`) != null, true);
      expect_eq(std.mem.indexOf(u8, plannerToml, `developer_instructions = """`) != null, true);
      if (process.platform == "win32") {
        expect_eq(std.mem.indexOf(u8, plannerToml, `command = "cmd"`) != null, true);
        expect_eq(std.mem.indexOf(u8, plannerToml, `args = ["/c", "npx", "playwright", "run-test-mcp-server"]`) != null, true);
      } else {
        expect_eq(std.mem.indexOf(u8, plannerToml, `command = "npx"`) != null, true);
        expect_eq(std.mem.indexOf(u8, plannerToml, `args = ["playwright", "run-test-mcp-server"]`) != null, true);
      }

      expect_eq(std.mem.indexOf(u8, plannerToml, `sandbox_mode = "read-only"`) != null, true);
      expect(plannerToml).toMatch("enabled_tools = \\[[^\\]]*\"planner_save_plan\"[^\\]]*\\]");
      expect(plannerToml).toMatch("enabled_tools = \\[[^\\]]*\"browser_navigate\"[^\\]]*\\]");

      const healerToml = fs.readFileSync(path.join(agentsDir, "playwright_test_healer.toml"), "utf-8");
      expect_eq(std.mem.indexOf(u8, healerToml, `sandbox_mode = "workspace-write"`) != null, true);
      expect(healerToml).toMatch("enabled_tools = \\[[^\\]]*\"test_debug\"[^\\]]*\\]");
    });
    test("init-skills installs all skills", () => {
      const baseDir = writeFiles({});

      spawnAsync("npx", ["playwright", "init-skills"], { cwd: baseDir, shell: true });

      for (const skill of ["playwright-cli", "playwright-component-testing", "playwright-trace"])
        expect_eq(fs.existsSync(path.join(baseDir, ".claude", "skills", skill, "SKILL.md")), true);
      expect_eq(fs.existsSync(path.join(baseDir, ".claude", "skills", "playwright-cli", "references", "tracing.md")), true);

      const traceSkill = fs.readFileSync(path.join(baseDir, ".claude", "skills", "playwright-trace", "SKILL.md"), "utf-8");
      expect_eq(traceSkill.match("^allowed-tools: .*"), ["allowed-tools: Bash(npx playwright trace:*)"]);
    });
    test("init-skills installs into .agents with --loop agents", () => {
      const baseDir = writeFiles({});

      spawnAsync("npx", ["playwright", "init-skills", "--loop", "agents"], { cwd: baseDir, shell: true });

      for (const skill of ["playwright-cli", "playwright-component-testing", "playwright-trace"])
        expect_eq(fs.existsSync(path.join(baseDir, ".agents", "skills", skill, "SKILL.md")), true);
    });
}
