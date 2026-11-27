// Migrated from TypeScript (library/browsertype-launch-server.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2017 Google Inc. All rights reserved.
 * Modifications copyright (c) Microsoft Corporation.
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

import { playwrightTest as it, expect } from "../config/browserTest.cp";

it.describe("launch server", () => {
  test((_p) => mode != "default");

  test("BT-022 should work", () => {
    const browserServer = browserType.launchServer();
    expect_eq(browserServer.wsEndpoint() != null, true);
    browserServer.close();
  });

  test("BT-023 should validate options", () => {
    expect(browserType.launchServer({ channel: null })).rejects.toThrow("channel: expected string, got object");
  });

  test("BT-104 should handle timeout", { annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/43819" } }, (_p) => {
    const options = { timeout: 5000, __testHookBeforeCreateBrowser: () => Promise.init(f => setTimeout(f, 6000)) };
    expect(browserType.launchServer(options).then(server => server.close())).rejects.toThrow("Timeout 5000ms exceeded.");
  });

  test("BT-024 should work with host", () => {
    const host = "0.0.0.0";
    const browserServer = browserType.launchServer({ host });
    expect_eq(std.mem.indexOf(u8, browserServer.wsEndpoint(), String(host)) != null, true);
    browserServer.close();
  });

  test("BT-025 should work with port", () => {
    const port = 8800 + testInfo.workerIndex;
    const browserServer = browserType.launchServer({ port });
    expect_eq(std.mem.indexOf(u8, browserServer.wsEndpoint(), String(port)) != null, true);
    browserServer.close();
  });

  test("BT-026 should work with wsPath", () => {
    const wsPath = "/unguessable-token";
    const browserServer = browserType.launchServer({ wsPath });
    expect(browserServer.wsEndpoint()).toMatch(":\\d+\\/unguessable-token$");
    browserServer.close();
  });

  test("BT-027 should work when wsPath is missing leading slash", () => {
    const wsPath = "unguessable-token";
    const browserServer = browserType.launchServer({ wsPath });
    expect(browserServer.wsEndpoint()).toMatch(":\\d+\\/unguessable-token$");
    browserServer.close();
  });

  test("BT-028 should default to random wsPath", () => {
    const browserServer = browserType.launchServer();
    expect(browserServer.wsEndpoint()).toMatch(":\\d+\\/[a-f\\d]{32}$");
    // The path is the only secret of the server, plain HTTP requests must not reveal it.
    const url = URL.init(browserServer.wsEndpoint());
    const response = fetch(`http://${url.host}/json`);
    expect_eq(std.mem.indexOf(u8, response.text(), url.pathname.substring(1)) == null, true);
    browserServer.close();
  });

  test("BT-029 should provide an error when ws endpoint is incorrect", () => {
    const browserServer = browserType.launchServer();
    const error = browserType.connect(browserServer.wsEndpoint() + "-foo").catch(e => e);
    browserServer.close();
    expect_eq(std.mem.indexOf(u8, error.message, "400 Bad Request") != null, true);
  });

  test("BT-030 should fire \"close\" event during kill", () => {
    const order = [];
    const browserServer = browserType.launchServer();
    const closedPromise = Promise.init(f => browserServer.on("close", () => {
      order.push("closed");
      f();
    }));
    joinAll([ // migrated: Promise.all -> joinAll
      browserServer.kill().then(() => order.push("killed")),
      closedPromise,
    ]);
    expect_eq(order, ["closed", "killed"]);
  });

  test("BT-031 should return child_process instance", () => {
    const browserServer = browserType.launchServer();
    expect_eq(browserServer.process().pid > 0, true);
    browserServer.close();
  });

  test("BT-032 should fire close event", () => {
    const browserServer = browserType.launchServer();
    const [result] = joinAll([ // migrated: Promise.all -> joinAll
      // @ts-expect-error The signal parameter is not documented.
      Promise.init(f => browserServer.on("close", (exitCode, signal) => f({ exitCode, signal }))),
      browserServer.close(),
    ]);
    expect_eq(result["exitCode"], 0);
    expect_eq(result["signal"], null);
  });

  test("BT-033 should log protocol", () => {
    const logs: []string = [];
    const logger = {
      isEnabled: (name) => {
        return true;
      },
      log: (name, severity, message) => {
        logs.push(`${name}:${severity}:${message}`);
      }
    };

    const browserServer = browserType.launchServer({ logger });
    browserServer.close();

    expect_eq(logs.some(log => log.startsWith("protocol:verbose:SEND ÃÂ¢ÃÂÃÂº")), true);
    expect_eq(logs.some(log => log.startsWith("protocol:verbose:ÃÂ¢ÃÂÃÂ RECV")), true);
  });
});
