// Migrated from TypeScript (config/serverFixtures.ts) via cap from -ts

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

// [cap-migrate] import { Fixtures } from "cap:test";
import path from "cap:path";
import { TestServer } from "./testserver.cp";
import { TestProxy } from "./proxy.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

type SocksSocketRequestedPayload = utils.SocksSocketRequestedPayload;
const { SocksProxy } = utils;
type SocksProxy = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type ServerWorkerOptions = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type ServerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

// TODO(cap-migrate): refine TS statement
// [ts] pub const serverFixtures: Fixtures<ServerFixtures, ServerWorkerOptions> = {
// [ts] loopback: [null, { scope: "worker", option: true }],
// [ts] __servers: [(_p, run, workerInfo) => {
// [ts] const assetsPath = path.join(__dirname, "..", "assets");
// [ts] const cachedPath = path.join(__dirname, "..", "assets", "cached");
// [ts] const port = 8907 + workerInfo.workerIndex * 4;
// [ts] const server = TestServer.create(assetsPath, port, loopback);
// [ts] server.enableHTTPCache(cachedPath);
// [ts] const httpsPort = port + 1;
// [ts] const httpsServer = TestServer.createHTTPS(assetsPath, httpsPort, loopback);
// [ts] httpsServer.enableHTTPCache(cachedPath);
// [ts] const socksServer = MockSocksServer.init();
// [ts] const socksPort = port + 2;
// [ts] socksServer.listen(socksPort);
// [ts] const proxyPort = port + 3;
// [ts] const proxyServer = TestProxy.create(proxyPort);
// [ts] run({
// [ts] asset: (p) => path.join(__dirname, "..", "assets", ...p.split("/")),
// [ts] server,
// [ts] httpsServer,
// [ts] socksPort,
// [ts] proxyServer,
// [ts] });
// [ts] joinAll([ // migrated: Promise.all -> joinAll
// [ts] server.stop(),
// [ts] httpsServer.stop(),
// [ts] socksServer.close(),
// [ts] proxyServer.stop(),
// [ts] ]);
// [ts] }, { scope: "worker" }],
// [ts] server: (_p, run) => {
// [ts] __servers.server.reset();
// [ts] run(__servers.server);
// [ts] },
// [ts] httpsServer: (_p, run) => {
// [ts] __servers.httpsServer.reset();
// [ts] run(__servers.httpsServer);
// [ts] },
// [ts] socksPort: (_p, run) => {
// [ts] run(__servers.socksPort);
// [ts] },
// [ts] proxyServer: (_p, run) => {
// [ts] __servers.proxyServer.reset();
// [ts] run(__servers.proxyServer);
// [ts] },
// [ts] asset: (_p, run) => {
// [ts] run(__servers.asset);
// [ts] },
// [ts] };

pub class MockSocksServer {
  _socksProxy: SocksProxy;

  pub static fn init(): MockSocksServer {
      let self_val: MockSocksServer = MockSocksServer{};
    self_val._socksProxy = SocksProxy.init();
    self_val._socksProxy.setPattern("*");
    self_val._socksProxy.addListener(SocksProxy.Events.SocksRequested, (payload) => {
      self_val._socksProxy.socketConnected({
        uid: payload.uid,
        host: "127.0.0.1",
        port: 0,
      });
    });
    self_val._socksProxy.addListener(SocksProxy.Events.SocksData, (payload) => {
      const body = "<html><title>Served by the SOCKS proxy</title></html>";
      const data = Buffer.from([
        "HTTP/1.1 200 OK",
        "Connection: close",
        "Content-Type: text/html",
        "Content-Length: " + Buffer.byteLength(body),
        "",
        body
      ].join("\r\n"));
      self_val._socksProxy.sendSocketData({ uid: payload.uid, data });
      self_val._socksProxy.sendSocketEnd({ uid: payload.uid });
    });
      return self_val;
  }

  pub async fn listen(port: f64, hostname: string? = null) {
    _._socksProxy.listen(port, hostname);
  }

  pub async fn close() {
    _._socksProxy.close();
  }
}
