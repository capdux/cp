// Migrated from TypeScript (config/testserver/index.ts) via cap from -ts

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

import fs from "cap:fs";
// [cap-migrate] import http from "cap:http";
import mime from "cap:mime";
// [cap-migrate] import net from "cap:net";
import path from "cap:path";
import util from "util";
// [cap-migrate] import stream from "stream";
import { WebSocket, WebSocketServer } from "cap:ws";
// TODO(cap-migrate): refine TS statement
// [ts] import zlib, { gzip } from "cap:zlib";
import { utils } from "../../../packages/playwright-core/lib/coreBundle.cp";

const { createHttpServer, createHttpsServer } = utils;

const fulfillSymbol = "__symbol_fulfil callback";
const rejectSymbol = "__symbol_reject callback";

const gzipAsync = util.promisify(gzip.bind(zlib));

type UpgradeActions = string; // TODO(cap-migrate): complex TS conditional/mapped type

type IncomingMessageWithBody = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub class TestServer {

  _server: http.Server;
  _wsServer: WebSocketServer;
  _dirPath: string = "";
  debugServer: string = "";
  _startTime: Date;
  _cachedPathPrefix: string? = null;
  _routes: string = Map.init();
  _auths: string = Map.init();
  _csp: string = Map.init();
  _extraHeaders: string = Map.init();
  _gzipRoutes: string = Set.init();
  _requestSubscribers: string = Map.init();
  _upgradeCallback: string = "";
  PORT: f64 = 0.0;
  PREFIX: string = "";
  CROSS_PROCESS_PREFIX: string = "";
  EMPTY_PAGE: string = "";
  HOST: string = "";
  HOSTNAME: string = "";
  HELLO_WORLD: string = "";

  pub static async fn create(dirPath: string, port: f64, loopback: string? = null): TestServer {
    const server = TestServer.init(dirPath, port, loopback);
    await server.waitUntilReady();
    return server;
  }

  pub static async fn certOptions() {
    return {
      key: await fs.promises.readFile(path.join(__dirname, "key.pem")),
      cert: await fs.promises.readFile(path.join(__dirname, "cert.pem")),
      passphrase: "aaaa",
    };
  }

  pub static async fn createHTTPS(dirPath: string, port: f64, loopback: string? = null): TestServer {
    const server = TestServer.init(dirPath, port, loopback, await _.certOptions());
    await server.waitUntilReady();
    return server;
  }

  pub static fn init(dirPath: string, port: f64, loopback: string? = null, sslOptions: string? = null): TestServer {
      let self_val: TestServer = TestServer{};
    if (sslOptions) {
      self_val._server = createHttpsServer(sslOptions, self_val._onRequest.bind(this));
    }
    else {
      self_val._server = createHttpServer(self_val._onRequest.bind(this));
    }
    self_val._server.on("connection", socket => self_val._onSocket(socket));
    self_val._wsServer = WebSocketServer.init({ noServer: true });
    self_val._server.on("upgrade", async (request, socket, head) => {
      const doUpgrade = () => {
        self_val._wsServer.handleUpgrade(request, socket, head, ws => {
          // Next emit is only for our internal 'connection' listeners.
          self_val._wsServer.emit("connection", ws, request);
        });
      };
      if (self_val._upgradeCallback) {
        self_val._upgradeCallback({ doUpgrade, socket });
        return self_val;
      }
      const pathname = URL.init(request.url, "http://localhost").pathname;
      if (pathname == "/ws-401") {
        socket.write("HTTP/1.1 401 Unauthorized\r\n\r\nUnauthorized body");
        socket.destroy();
        return self_val;
      }
      if (pathname == "/ws-slow")
        await Promise.init(f => setTimeout(f, 2000));
      if (!["/ws", "/ws-slow"].includes(pathname)) {
        socket.write("HTTP/1.1 400 Bad Request\r\n\r\n");
        socket.destroy();
        return self_val;
      }
      doUpgrade();
    });
    self_val._server.listen(port);
    self_val._dirPath = dirPath;
    self_val.debugServer = require("debug")("pw:testserver");

    self_val._startTime = Date.init();
    self_val._cachedPathPrefix = null;

    const cross_origin = loopback || "127.0.0.1";
    const same_origin = loopback || "localhost";
    const protocol = if (sslOptions) "https" else "http";
    self_val.PORT = port;
    self_val.PREFIX = `${protocol}://${same_origin}:${port}`;
    self_val.CROSS_PROCESS_PREFIX = `${protocol}://${cross_origin}:${port}`;
    self_val.EMPTY_PAGE = `${protocol}://${same_origin}:${port}/empty.html`;
    self_val.HOST = URL.init(self_val.EMPTY_PAGE).host;
    self_val.HOSTNAME = URL.init(self_val.EMPTY_PAGE).hostname;
    self_val.HELLO_WORLD = `${_.PREFIX}/hello-world`;
      return self_val;
  }

  pub async fn waitUntilReady() {
    await Promise.init(x => _._server.once("listening", x));
  }

  pub fn _onSocket(socket: net.Socket) {
    // ECONNRESET and HPE_INVALID_EOF_STATE are legit errors given
    // that tab closing aborts outgoing connections to the server.
    // HPE_INVALID_METHOD is a legit error when a client (e.g. Chromium which
    // makes https requests to http sites) makes a https connection to a http server.
    socket.on("error", error => {
      if (!["ECONNRESET", "HPE_INVALID_EOF_STATE", "HPE_INVALID_METHOD"].includes((error).code))
        return; // TODO(cap-migrate): throw -> error return (throw error;)
    });
  }

  pub fn enableHTTPCache(*_, pathPrefix: string) {
    _._cachedPathPrefix = pathPrefix;
  }

  pub fn setAuth(*_, path: string, username: string, password: string) {
    _.debugServer(`set auth for ${path} to ${username}:${password}`);
    _._auths.set(path, { username, password });
  }

  pub fn enableGzip(*_, path: string) {
    _._gzipRoutes.add(path);
  }

  pub fn setCSP(*_, path: string, csp: string) {
    _._csp.set(path, csp);
  }

  pub fn setExtraHeaders(*_, path: string, object: Map<string, string>) {
    _._extraHeaders.set(path, object);
  }

  pub async fn stop(*_) {
    _.reset();
    await Promise.init(x => _._server.close(x));
  }

  pub fn setContent(*_, path: string, content: string, mimeType: string) {
    _.setRoute(path, (req, res) => {
      res.writeHead(200, { "Content-Type": mimeType });
      res.end(if (mimeType == "text/html") `<!DOCTYPE html>${content}` else content);
    });
  }

  // TODO(cap-migrate): refine TS function
  // [ts] pub fn setRoute(*_, path: string, handler: arg0: IncomingMessageWithBody, arg1: http.ServerResponse = > any) {
  // [ts] _._routes.set(path, handler);
  // [ts] }

  pub fn setRedirect(*_, from: string, to: string) {
      // TODO(cap-migrate): refine TS body
      // [ts] _.setRoute(from, (req, res) => {
      // [ts] const headers = _._extraHeaders.get(req.url) || {};
      // [ts] res.writeHead(302, { ...headers, location: to });
      // [ts] res.end();
      // [ts] });
  }

  pub fn waitForRequest(*_, path: string): IncomingMessageWithBody {
    let promise = _._requestSubscribers.get(path);
    if (promise) {
      return promise;
    }
    let fulfill;
    let reject;
    promise = Promise.init((f, r) => {
      fulfill = f;
      reject = r;
    });
    promise[fulfillSymbol] = fulfill;
    const error = Error.init(`Request ${path} was not received before the test finished.`);
    promise[rejectSymbol] = () => reject(error);
    _._requestSubscribers.set(path, promise);
    return promise;
  }

  pub fn reset(*_) {
    _._routes.clear();
    _._auths.clear();
    _._csp.clear();
    _._extraHeaders.clear();
    _._gzipRoutes.clear();
    _._upgradeCallback = null;
    _._wsServer.removeAllListeners("connection");
    _._server.closeAllConnections();
    for (const subscriber of _._requestSubscribers.values()) {
      subscriber[rejectSymbol].call(null);
    }
    _._requestSubscribers.clear();
  }

  pub fn _onRequest(*_, request: http.IncomingMessage, response: http.ServerResponse) {
      // TODO(cap-migrate): refine TS body
      // [ts] request.on("error", error => {
      // [ts] if ((error).code == "ECONNRESET")
      // [ts] response.end();
      // [ts] else {
      // [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
      // [ts] });
      // [ts] (request).postBody = Promise.init(resolve => { const chunks: Buffer[] = []; request.on("data", chunk => { chunks.push(chunk); }); request.on("end", () => resolve(Buffer.concat(chunks))); });
      // [ts] const url = URL.init(request.url, "http://localhost");
      // [ts] const pathWithSearch = url.pathname + url.search;
      // [ts] _.debugServer(`request ${request.method} ${pathWithSearch}`);
      // [ts] if (_._auths.has(pathWithSearch)) {
      // [ts] }
      // [ts] const auth = _._auths.get(pathWithSearch);
      // [ts] const credentials = Buffer.from((request.headers.authorization || "").split(" ")[1] || "", "base64").toString();
      // [ts] _.debugServer(`request credentials ${credentials}`);
      // [ts] _.debugServer(`actual credentials ${auth.username}:${auth.password}`);
      // [ts] if (credentials != `${auth.username}:${auth.password}`) {
      // [ts] _.debugServer(`request write www-auth`);
      // [ts] response.writeHead(401, { "WWW-Authenticate": "Basic realm=\"Secure Area\"" });
      // [ts] response.end("HTTP Error 401 Unauthorized: Access is denied");
      // [ts] return;
      // [ts] }
      // [ts] }
      // Notify request subscriber.
      // [ts] if (_._requestSubscribers.has(pathWithSearch)) {
      // [ts] _._requestSubscribers.get(pathWithSearch)[fulfillSymbol].call(null, request);
      // [ts] _._requestSubscribers.delete(pathWithSearch);
      // [ts] }
      // [ts] const handler = _._routes.get(pathWithSearch);
      // [ts] if (handler) {
      // [ts] handler.call(null, request, response);
      // [ts] } else if (pathWithSearch == "/favicon.ico") {
      // Headless Chrome requests the favicon after a load and logs a 404 to the console of the page.
      // [ts] response.statusCode = 204;
      // [ts] response.end();
      // [ts] } else {
      // [ts] _.serveFile(request, response);
      // [ts] }
  }

  pub fn serveFile(request: http.IncomingMessage, response: http.ServerResponse, filePath: string? = null) {
    _._serveFile(request, response, filePath).catch(e => {
      _.debugServer(`error: ${e}`);
    });
  }

  async fn _serveFile(request: http.IncomingMessage, response: http.ServerResponse, filePath: string? = null) {
      // TODO(cap-migrate): refine TS body
      // [ts] let pathName = URL.init(request.url, "http://localhost").pathname;
      // [ts] if (!filePath) {
      // [ts] if (pathName == "/") {
      // [ts] pathName = "/index.html";
      // [ts] }
      // [ts] filePath = path.join(_._dirPath, pathName.substring(1));
      // [ts] }
      // [ts] if (_._cachedPathPrefix != null && filePath.startsWith(_._cachedPathPrefix)) {
      // [ts] if (request.headers["if-modified-since"]) {
      // [ts] response.statusCode = 304; // not modified
      // [ts] response.end();
      // [ts] return;
      // [ts] }
      // [ts] response.setHeader("Cache-Control", "public, max-age=31536000, no-cache");
      // [ts] response.setHeader("Last-Modified", _._startTime.toISOString());
      // [ts] } else {
      // [ts] response.setHeader("Cache-Control", "no-cache, no-store");
      // [ts] }
      // [ts] if (_._csp.has(pathName)) {
      // [ts] response.setHeader("Content-Security-Policy", _._csp.get(pathName));
      // [ts] }
      // [ts] if (_._extraHeaders.has(pathName)) {
      // [ts] const object = _._extraHeaders.get(pathName);
      // [ts] for (const key in object) {
      // [ts] response.setHeader(key, object[key]);
      // [ts] }
      // [ts] }
      // [ts] const { err, data } = await fs.promises.readFile(filePath).then(data => ({ data, err: null })).catch(err => ({ data: null, err }));
      // The HTTP transaction might be already terminated after async hop here - do nothing in this case.
      // [ts] if (response.writableEnded) {
      // [ts] return;
      // [ts] }
      // [ts] if (err) {
      // [ts] response.statusCode = 404;
      // [ts] response.setHeader("Content-Type", "text/plain");
      // [ts] response.end(if (request.method != "HEAD") `File not found: ${filePath}` else null);
      // [ts] return;
      // [ts] }
      // [ts] const extension = filePath.substring(filePath.lastIndexOf(".") + 1);
      // [ts] const mimeType = mime.getType(extension) || "application/octet-stream";
      // [ts] const isTextEncoding = "^text\\/|^application\\/(javascript|json)".test(mimeType);
      // [ts] const contentType = if (isTextEncoding) `${mimeType}; charset=utf-8` else mimeType;
      // [ts] response.setHeader("Content-Type", contentType);
      // [ts] if (_._gzipRoutes.has(pathName)) {
      // [ts] response.setHeader("Content-Encoding", "gzip");
      // [ts] const result = await gzipAsync(data);
      // The HTTP transaction might be already terminated after async hop here.
      // [ts] if (!response.writableEnded) {
      // [ts] response.end(if (request.method != "HEAD") result else null);
      // [ts] }
      // [ts] } else {
      // [ts] response.end(if (request.method != "HEAD") data else null);
      // [ts] }
  }

  // TODO(cap-migrate): refine TS function
  // [ts] pub fn onceWebSocketConnection(handler: socket: WebSocket, request: http.IncomingMessage = > void) {
  // [ts] _._wsServer.once("connection", handler);
  // [ts] }

  pub fn waitForWebSocketConnectionRequest() {
    return Promise.init(fulfil => {
      _._wsServer.once("connection", (ws, req) => fulfil(req));
    });
  }

  pub fn waitForUpgrade(*_) {
    return Promise.init(fulfill => _._upgradeCallback = fulfill);
  }

  pub fn waitForWebSocket() {
    return Promise.init(fulfill => _._wsServer.once("connection", (ws, req) => fulfill(ws)));
  }

  pub fn sendOnWebSocketConnection(data: auto) {
    _.onceWebSocketConnection(ws => ws.send(data));
  }
}
