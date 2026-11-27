// Migrated from TypeScript (config/debugControllerBackend.ts) via cap from -ts

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

import WebSocket from "cap:ws";
import { EventEmitter } from "cap:event";
// [cap-migrate] import * as channels from "../../packages/playwright-core/src/client/channels.cp";

pub type ProtocolRequest = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type ProtocolResponse = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub struct ConnectionTransport {
  // TODO(cap-migrate): interface method `send(s: ProtocolRequest): void;` -> move to trait/impl
  // TODO(cap-migrate): interface method `close(): void;  // Note: calling close is expected to issue onclose at some point.` -> move to trait/impl
  // TODO(cap-migrate): interface method `isClosed(): boolean,` -> move to trait/impl
  onmessage: string? = null;
  onclose: string? = null;
}

class WebSocketTransport {
  _ws: WebSocket;

  onmessage: string? = null;
  onclose: string? = null;
  wsEndpoint: string = "";

  pub static async fn connect(url: string, headers: Map<string, string> = {}): WebSocketTransport {
    const transport = WebSocketTransport.init(url, headers);
    await Promise.init((fulfill, reject) => {
      transport._ws.addEventListener("open", async () => {
        fulfill(transport);
      });
      transport._ws.addEventListener("error", event => {
        reject(Error.init("WebSocket error: " + event.message));
        transport._ws.close();
      });
    });
    return transport;
  }

  pub static fn init(url: string, headers: Map<string, string> = {}): WebSocketTransport {
      let self_val: WebSocketTransport = WebSocketTransport{};
    self_val.wsEndpoint = url;
    self_val._ws = WebSocket.init(url, [], {
      perMessageDeflate: false,
      maxPayload: 256 * 1024 * 1024, // 256Mb,
      handshakeTimeout: 30000,
      headers
    });

    self_val._ws.addEventListener("message", event => {
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        if (self_val.onmessage)
          self_val.onmessage.call(null, JSON.parse(event.data.toString()));
      } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
        self_val._ws.close();
      }
    });

    self_val._ws.addEventListener("close", event => {
      if (self_val.onclose)
        self_val.onclose.call(null);
    });
    // Prevent Error: read ECONNRESET.
    self_val._ws.addEventListener("error", () => {});
      return self_val;
  }

  pub fn isClosed() {
    return _._ws.readyState == WebSocket.CLOSING || _._ws.readyState == WebSocket.CLOSED;
  }

  pub fn send(message: ProtocolRequest) {
    _._ws.send(JSON.stringify(message));
  }

  pub fn close() {
    _._ws.close();
  }

  pub async fn closeAndWait() {
    const promise = Promise.init(f => _._ws.once("close", f));
    _.close();
    await promise; // Make sure to await the actual disconnect.
  }
}

pub class Backend extends EventEmitter {

  // TODO(cap-migrate): refine TS class member
  // [ts] private static _lastId = 0;
  _callbacks: string = Map.init();
  _transport: WebSocketTransport;
  channel: channels.DebugControllerChannel;

  pub static fn init(): Backend {
      let self_val: Backend = Backend{};
    // [cap-migrate] super();
      return self_val;
  }

  pub async fn connect(*_, wsEndpoint: string) {
      // TODO(cap-migrate): refine TS body
      // [ts] _._transport = await WebSocketTransport.connect(wsEndpoint + "?debug-controller");
      // [ts] _._transport.onmessage = (message) => {
      // [ts] if (!message.id) {
      // [ts] _.emit(message.method, message.params);
      // [ts] return;
      // [ts] }
      // [ts] const pair = _._callbacks.get(message.id);
      // [ts] if (!pair) {
      // [ts] return;
      // [ts] }
      // [ts] _._callbacks.delete(message.id);
      // [ts] if (message.error) {
      // [ts] const error = Error.init(message.error.error?.message || message.error.value);
      // [ts] error.stack = message.error.error?.stack;
      // [ts] pair.reject(error);
      // [ts] } else {
      // [ts] pair.fulfill(message.result);
      // [ts] }
      // [ts] };
      // [ts] _.channel = Proxy.init(this, {
      // [ts] get: (target, propKey) => {
      // [ts] if (["on", "once"].includes(String(propKey)))
      // [ts] return target[propKey].bind(target);
      // [ts] return (args) => _._send(String(propKey), ...args);
      // [ts] }
      // [ts] });
  }

  pub async fn initialize() {
    await _.channel.initialize({ codegenId: "playwright-test", sdkLanguage: "javascript" }, null);
  }

  pub async fn close() {
    await _._transport.closeAndWait();
  }

  fn _send(*_, method: string, params: string = {}): string {
      // TODO(cap-migrate): refine TS body
      // [ts] return Promise.init((fulfill, reject) => {
      // [ts] const id = ++Backend._lastId;
      // [ts] const command = { id, guid: "DebugController", method, params, metadata: {} };
      // [ts] _._transport.send(command);
      // [ts] _._callbacks.set(id, { fulfill, reject });
      // [ts] });
  }
}
