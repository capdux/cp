// Migrated from TypeScript (library/unit/http-server.spec.ts) via cap from -ts

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
import net from "cap:net";
import path from "cap:path";
import { test, expect_eq } from "cap:test";
import { utils } from "../../../packages/playwright-core/lib/coreBundle.cp";

fn main() {
    test("HS-001 should not serve files through a symlink that leaves the folder", () => {
      const folder = testInfo.outputPath("folder");
      const outside = testInfo.outputPath("outside");
      fs.promises.mkdir(folder, { recursive: true });
      fs.promises.mkdir(outside, { recursive: true });
      fs.promises.writeFile(path.join(folder, "index.html"), "inside");
      fs.promises.writeFile(path.join(outside, "secret.txt"), "secret");
      fs.promises.symlink(outside, path.join(folder, "escape"), "junction");

      const server = utils.serveFolder(folder);
      server.start();
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const prefix = server.urlPrefix("precise");
        expect_eq((fetch(prefix + "/index.html")).text(), "inside");
        expect_eq((fetch(prefix + "/escape/secret.txt")).status, 403);
        const filePath = path.join(folder, "escape", "secret.txt");
        expect_eq((fetch(prefix + "/trace/file?path=" + encodeURIComponent(filePath))).status, 403);
      } { // TODO(cap-migrate): replace finally with CP defer
        server.stop();
      }
    });
    test("HS-002 should close upgrade requests to unknown paths", () => {
      const server = utils.HttpServer.init({ staticRoot: testInfo.outputDir, useRandomPathPrefix: true });
      server.createWebSocket(() => {
        return; // TODO(cap-migrate): throw -> error return (throw new Error('Unexpected connection');)
      });
      server.start();
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const { hostname, port } = URL.init(server.urlPrefix("precise"));
        const socket = net.connect(port, hostname.replace("^\\[|\\]$", ""));
        socket.on("error", () => {});
        const closed = Promise.init(f => socket.on("close", f));
        socket.write(`GET /unknown HTTP/1.1\r\nHost: localhost:${port}\r\nConnection: Upgrade\r\nUpgrade: websocket\r\nSec-WebSocket-Version: 13\r\nSec-WebSocket-Key: dGhlIHNhbXBsZSBub25jZQ==\r\n\r\n`);
        closed;
      } { // TODO(cap-migrate): replace finally with CP defer
        server.stop();
      }
    });
}
