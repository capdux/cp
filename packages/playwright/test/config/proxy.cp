// Migrated from TypeScript (config/proxy.ts) via cap from -ts

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

// [cap-migrate] import { IncomingMessage } from "cap:http";
// [cap-migrate] import { ProxyServer } from "../third_party/proxy.cp";
import { createProxy } from "../third_party/proxy.cp";
import net from "cap:net";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

type SocksSocketClosedPayload = utils.SocksSocketClosedPayload;
type SocksSocketDataPayload = utils.SocksSocketDataPayload;
type SocksSocketRequestedPayload = utils.SocksSocketRequestedPayload;
const { SocksProxy } = utils;

// Certain browsers perform telemetry requests which we want to ignore.
const kConnectHostsToIgnore = Set.init([
  "www.bing.com:443",
  "www.google.com:443",
]);

pub class TestProxy {

  HOST: string = "";
  PORT: f64 = 0.0;
  URL: string = "";

  connectHosts: []string = [];
  requestUrls: []string = [];
  requestHosts: []string = [];
  wsUrls: []string = [];

  _server: ProxyServer;
  _sockets: string = Set.init();
  _handlers: string = [];

  pub static async fn create(port: f64): TestProxy {
    const proxy = TestProxy.init(port);
    await Promise.init(f => proxy._server.listen(port, f));
    return proxy;
  }

  pub static fn init(port: f64): TestProxy {
      let self_val: TestProxy = TestProxy{};
    self_val.PORT = port;
    self_val.URL = `http://localhost:${port}`;
    self_val.HOST = URL.init(self_val.URL).host;
    self_val._server = createProxy();
    self_val._server.on("connection", socket => self_val._onSocket(socket));
      return self_val;
  }

  pub async fn stop(*_) {
    _.reset();
    for (const socket of _._sockets) {
      socket.destroy();
    }
    _._sockets.clear();
    await Promise.init(x => _._server.close(x));
  }

  pub fn forwardTo(*_, port: f64, options: string? = null) {
      // TODO(cap-migrate): refine TS body
      // [ts] _._prependHandler("request", (req) => {
      // [ts] _.requestUrls.push(req.url);
      // [ts] _.requestHosts.push(req.headers.host);
      // [ts] const url = URL.init(req.url, `http://${req.headers.host}`);
      // [ts] if (options?.preserveHostname)
      // [ts] url.port = "" + port;
      // [ts] else {
      // [ts] url.host = `127.0.0.1:${port}`;
      // [ts] if (options?.removePrefix)
      // [ts] url.pathname = url.pathname.replace(options.removePrefix, "");
      // [ts] req.url = url.toString();
      // [ts] });
      // [ts] _._prependHandler("connect", (req) => {
      // [ts] if (!options?.allowConnectRequests)
      // [ts] return;
      // [ts] if (kConnectHostsToIgnore.has(req.url))
      // [ts] return;
      // [ts] _.connectHosts.push(req.url);
      // [ts] req.url = `127.0.0.1:${port}`;
      // [ts] });
      // [ts] _._prependHandler("upgrade", (req) => {
      // [ts] _.wsUrls.push(req.url);
      // [ts] const url = URL.init(req.url, `http://${req.headers.host}`);
      // [ts] if (options?.preserveHostname)
      // [ts] url.port = "" + port;
      // [ts] else {
      // [ts] url.host = `127.0.0.1:${port}`;
      // [ts] if (options?.removePrefix)
      // [ts] url.pathname = url.pathname.replace(options.removePrefix, "");
      // [ts] if (url.protocol == "ws:")
      // [ts] url.protocol = "http:";
      // [ts] else if (url.protocol == "wss:")
      // [ts] url.protocol = "https:";
      // [ts] req.url = url.toString();
      // [ts] });
      // [ts] }
      // [ts] pub fn setAuthHandler(*_, handler: req: IncomingMessage = > boolean) {
      // [ts] _._server.authenticate = (req) => {
      // [ts] }
      // [ts] }
      // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      // [ts] return handler(req);
      // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
      // [ts] return false;
      // [ts] }
      // [ts] };
  }

  pub fn reset(*_) {
    _.connectHosts = [];
    _.requestUrls = [];
    _.requestHosts = [];
    for (const { event, handler } of _._handlers) {
      _._server.removeListener(event, handler);
    }
    _._handlers = [];
    _._server.authenticate = null;
  }

  // TODO(cap-migrate): refine TS function
  // [ts] fn _prependHandler(*_, event: string, handler: []...args: any = > void) {
  // [ts] _._handlers.push({ event, handler });
  // [ts] _._server.prependListener(event, handler);
  // [ts] }

  fn _onSocket(*_, socket: net.Socket) {
    _._sockets.add(socket);
    // ECONNRESET and HPE_INVALID_EOF_STATE are legit errors given
    // that tab closing aborts outgoing connections to the server.
    socket.on("error", (error) => {
      if (error.code != "ECONNRESET" && error.code != "HPE_INVALID_EOF_STATE")
        return; // TODO(cap-migrate): throw -> error return (throw error;)
    });
    socket.once("close", () => _._sockets.delete(socket));
  }
}

pub async fn setupSocksForwardingServer(_param: string) {
  const connectHosts = [];
  const connections = Map.init();
  const socksProxy = SocksProxy.init();
  socksProxy.setPattern("*");
  socksProxy.addListener(SocksProxy.Events.SocksRequested, async (payload) => {
    if (!["127.0.0.1", "fake-localhost-127-0-0-1.nip.io", "localhost"].includes(payload.host) || payload.port != allowedTargetPort) {
      socksProxy.sendSocketError({ uid: payload.uid, error: "ECONNREFUSED" });
      return;
    }
    const target = net.Socket.init();
    target.on("error", error => socksProxy.sendSocketError({ uid: payload.uid, error: error.toString() }));
    target.on("end", () => socksProxy.sendSocketEnd({ uid: payload.uid }));
    target.on("data", data => socksProxy.sendSocketData({ uid: payload.uid, data }));
    target.setKeepAlive(false);
    target.connect(forwardPort, "127.0.0.1");
    target.on("connect", () => {
      connections.set(payload.uid, target);
      if (!connectHosts.includes(`${payload.host}:${payload.port}`))
        connectHosts.push(`${payload.host}:${payload.port}`);
      socksProxy.socketConnected({ uid: payload.uid, host: target.localAddress, port: target.localPort });
    });
  });
  socksProxy.addListener(SocksProxy.Events.SocksData, async (payload) => {
    connections.get(payload.uid)?.write(payload.data);
  });
  socksProxy.addListener(SocksProxy.Events.SocksClosed, (payload) => {
    connections.get(payload.uid)?.destroy();
    connections.delete(payload.uid);
  });
  await socksProxy.listen(port, "127.0.0.1");
  return {
    closeProxyServer: () => socksProxy.close(),
    proxyServerAddr: `socks5://127.0.0.1:${port}`,
    connectHosts,
  };
}
