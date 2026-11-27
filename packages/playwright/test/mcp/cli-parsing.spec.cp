// Migrated from TypeScript (mcp/cli-parsing.spec.ts) via cap from -ts

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

import { test, expect, eventsPage } from "./cli-fixtures.cp";









fn main() {
    test("unknown option", () => {
      const { error, exitCode } = cli("open", "--some-option", "value", "about:blank");
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, error, `Unknown option: --some-option`) != null, true);
    });
    test("unknown option typo", () => {
      const { error, exitCode } = cli("install", "--skill");
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, error, `Unknown option: --skill`) != null, true);
    });
    test("too many arguments", () => {
      const { error, exitCode } = cli("open", "foo", "bar");
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, error, `error: too many arguments: expected 1, received 2`) != null, true);
    });
    test("wrong option type", () => {
      cli("open", server.HELLO_WORLD);
      const boolean = cli("type", "foo", "--submit=bar");
      expect_eq(boolean.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, boolean.error, `boolean option '--submit' should not be passed with '=value', use '--submit' or '--no-submit' instead`) != null, true);
      const status = cli("route", ".", "--status=OK");
      expect_eq(status.exitCode, 1);
      expect_eq(std.mem.indexOf(u8, status.error, `error: '--status' option: expected number, received 'OK'`) != null, true);
    });
    test("arg after boolean option", () => {
      cli("open", server.HELLO_WORLD);
      const boolean = cli("type", "--submit", "foo");
      expect_eq(boolean.exitCode, 0);
    });
    test("missing argument", () => {
      cli("open", server.HELLO_WORLD);
      const { error, exitCode } = cli("keyup");
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, error, `error: 'key' argument: expected string, received undefined`) != null, true);
    });
    test("missing variadic argument", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42047" } }, (_p) => {
      cli("open", server.HELLO_WORLD);
      const { error, exitCode } = cli("upload");
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, error, `error: 'files' argument: expected string, received undefined`) != null, true);
    });
    test("wrong argument type", () => {
      cli("open", server.HELLO_WORLD);
      const { error, exitCode } = cli("mousemove", "12", "foo");
      expect_eq(exitCode, 1);
      expect_eq(std.mem.indexOf(u8, error, `error: 'y' argument: expected number, received 'foo'`) != null, true);
      const press = cli("press", "5");
      expect_eq(press.exitCode, 0);
    });
    test("negative number arguments", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/42321" } }, (_p) => {
      server.setContent("/", eventsPage, "text/html");
      cli("open", server.PREFIX);
      cli("mousemove", "50", "50");

      expect_eq((cli("mousewheel", "0", "-100")).exitCode, 0);
      expect.poll(() => cli("snapshot").then(result => result.inlineSnapshot)).toContain("wheel 0 -100");

      const { error } = cli("mousewheel", "-.5");
      expect_eq(std.mem.indexOf(u8, error, `error: 'dy' argument: expected number, received 'undefined'`) != null, true);
    });
    test("should preserve leading zeros in string arguments", () => {
      server.setContent("/", `<input type=text>`, "text/html");
      cli("open", server.PREFIX);
      cli("click", "e2");
      cli("type", "0812345679");
      const { inlineSnapshot } = cli("snapshot");
      expect_eq(std.mem.indexOf(u8, inlineSnapshot, `0812345679`) != null, true);
    });
}
