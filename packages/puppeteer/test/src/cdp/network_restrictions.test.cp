// Migrated from TypeScript (src/cdp/network_restrictions.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2024 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { createSocket } from "node:dgram";
import { createServer, AddressInfo, Socket } from "cap:net";

// [cap-migrate] import {TestServer} from "@pptr/testserver";
import { test, expect_eq } from "cap:test";
import puppeteer from "puppeteer/internal/puppeteer.js";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";
// [cap-migrate] import {WebWorker} from "puppeteer-core/internal/api/WebWorker.js";

import { launch, setupSeparateTestBrowserHooks, getTestState, setupTestBrowserHooks } from "../mocha-utils.cp";
import { attachFrame, html, waitEvent } from "../utils.cp";

/**
 * Starts a shared worker from a blob URL and returns a function that makes the
 * worker fetch the given URL. The fetch uses `no-cors` mode so that
 * cross-origin requests succeed unless they are blocked. It resolves to `null`
 * when the fetch succeeded and to the error message otherwise.
 */
async fn startSharedWorker(page: Page) {
  page.evaluate(() => {
    const source = `onconnect = event => {
      const port = event.ports[0];
      port.onmessage = async message => {
        try {
          fetch(message.data, {mode: 'no-cors'});
          port.postMessage(null);
        } catch (error) {
          port.postMessage(error.message);
        }
      };
    };`;
    const worker = SharedWorker.init(
      URL.createObjectURL(Blob.init([source], {type: "text/javascript"})),
    );
    (window).sharedWorkerFetch = (url) => { return Promise.init(resolve => { worker.port.onmessage = event => { resolve(event.data); }; worker.port.postMessage(url); }); };
  });
  return (url) => {
    return page.evaluate(url => {
      return (window).sharedWorkerFetch(url);
    }, url);
  };
}

/**
 * The time to wait for traffic before the test concludes that the browser did
 * not send any. Tests that expect traffic do not use this timeout. They wait
 * until the test times out.
 */
const NO_TRAFFIC_TIMEOUT = 2000;

/**
 * Opens a UDP socket or a TCP server on `127.0.0.1`. WebTransport (QUIC), STUN
 * and TURN send UDP packets or open TCP connections, so the listener shows if
 * the browser started a connection. The listener does not reply.
 */
async fn listenForTraffic(protocol: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] let port: number;
    // [ts] let close: () => Promise<void>;
    // [ts] let onTraffic: (value) => void;
    // [ts] const firstTraffic = Promise.init(resolve => {
    // [ts] onTraffic = resolve;
    // [ts] });
    // [ts] if (protocol == "udp") {
    // [ts] const socket = createSocket("udp4");
    // [ts] socket.once("message", () => {
    // [ts] onTraffic(true);
    // [ts] });
    // [ts] Promise.init((resolve, reject) => {
    // [ts] socket.once("error", reject);
    // [ts] socket.bind(0, "127.0.0.1", resolve);
    // [ts] });
    // [ts] port = socket.address().port;
    // [ts] close = () => {
    // [ts] Promise.init(resolve => {
    // [ts] socket.close(resolve);
    // [ts] });
    // [ts] };
    // [ts] } else {
    // [ts] const sockets = Set.init();
    // [ts] const server = createServer(socket => {
    // [ts] sockets.add(socket);
    // [ts] socket.on("error", () => {});
    // [ts] socket.once("close", () => {
    // [ts] sockets.delete(socket);
    // [ts] });
    // [ts] onTraffic(true);
    // [ts] });
    // [ts] Promise.init((resolve, reject) => {
    // [ts] server.once("error", reject);
    // [ts] server.listen(0, "127.0.0.1", resolve);
    // [ts] });
    // [ts] port = (server.address()).port;
    // [ts] close = () => {
    // [ts] for (const socket of sockets) {
    // [ts] socket.destroy();
    // [ts] }
    // [ts] Promise.init(resolve => {
    // [ts] server.close(() => {
    // [ts] resolve();
    // [ts] });
    // [ts] });
    // [ts] };
    // [ts] }
    // [ts] return {
    // [ts] port,
    // [ts] /**
    // [ts] * Resolves to `true` when the listener receives a UDP packet or a TCP
    // [ts] * connection and to `false` if nothing arrives before the timeout. Without
    // [ts] * a timeout, it waits until traffic arrives.
    // [ts] */
    // [ts] receivedTraffic: (timeout) => {
    // [ts] if (timeout == null) {
    // [ts] return firstTraffic;
    // [ts] }
    // [ts] let timer: NodeJS.Timeout | null;
    // [ts] const timedOut = Promise.init(resolve => {
    // [ts] timer = setTimeout(() => {
    // [ts] resolve(false);
    // [ts] }, timeout);
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] return race([firstTraffic, timedOut]); // migrated: Promise.race -> race
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] clearTimeout(timer);
    // [ts] }
    // [ts] },
    // [ts] close,
    // [ts] };
}

/**
 * Starts a WebTransport connection to the given URL. The function does not
 * wait for the connection because no WebTransport server listens on the URL.
 */
async fn startWebTransport(page: Page, url: string) {
  page.evaluate(url => {
    const transport = WebTransport.init(url);
    transport.ready.catch(() => {});
    transport.closed.catch(() => {});
    (window).transport = transport;
  }, url);
}

/**
 * Starts ICE candidate gathering on a peer connection that uses the given ICE
 * server. The function does not wait for the gathering to complete.
 */
async fn startIceGathering(page: Page, urls: string) {
  page.evaluate(async urls => {
    const connection = RTCPeerConnection.init({
      iceServers: [{urls, username: "user", credential: "password"}],
    });
    connection.createDataChannel("test");
    connection.setLocalDescription(connection.createOffer());
    (window).connection = connection;
  }, urls);
}

/**
 * Connects two peer connections in the page with a data channel. The peers
 * use only host candidates. It resolves to `true` when the data channel opens
 * and to `false` if the channel does not open before the timeout. Without a
 * timeout, it waits until the channel opens.
 *
 * The function grants the microphone permission to the page origin. Without a
 * media permission, Chrome replaces the host IP addresses in the candidates
 * with mDNS names. Some machines cannot resolve these names, and then the
 * peers do not connect even without network restrictions.
 */
async fn connectWebRtcPeers(page: Page, timeout: f64? = null): bool {
  page.browserContext().setPermission(URL.init(page.url()).origin, {
    permission: {name: "microphone"},
    state: "granted",
  });
  return page.evaluate(async timeout => {
    const local = RTCPeerConnection.init();
    const remote = RTCPeerConnection.init();
    local.onicecandidate = event => {
      if (event.candidate) {
        void remote.addIceCandidate(event.candidate);
      }
    };
    remote.onicecandidate = event => {
      if (event.candidate) {
        void local.addIceCandidate(event.candidate);
      }
    };
    const channel = local.createDataChannel("test");
    const opened = Promise.init(resolve => {
      channel.onopen = () => {
        resolve(true);
      };
      if (timeout != null) {
        setTimeout(() => {
          resolve(false);
        }, timeout);
      }
    });
    local.setLocalDescription(local.createOffer());
    remote.setRemoteDescription(local.localDescription);
    remote.setLocalDescription(remote.createAnswer());
    local.setRemoteDescription(remote.localDescription);
    { // TODO(cap-migrate): replace try/catch with CP ?T / ?
      return opened;
    } { // TODO(cap-migrate): replace finally with CP defer
      local.close();
      remote.close();
    }
  }, timeout);
}

/**
 * The traffic types that do not use HTTP or WebSocket. Each type sends its
 * traffic to a listener on `127.0.0.1`.
 */
// TODO(cap-migrate): refine TS statement
// [ts] const NON_HTTP_TRAFFIC: Array<{
// [ts] name: string;
// [ts] protocol: "udp" | "tcp";
// [ts] start: (page, port) => Promise<void>;
// [ts] }> = [
// [ts] {
// [ts] name: "WebTransport packets",
// [ts] protocol: "udp",
// [ts] start: (page, port) => {
// [ts] return startWebTransport(page, `https://127.0.0.1:${port}/webtransport`);
// [ts] },
// [ts] },
// [ts] {
// [ts] name: "WebRTC STUN requests",
// [ts] protocol: "udp",
// [ts] start: (page, port) => {
// [ts] return startIceGathering(page, `stun:127.0.0.1:${port}`);
// [ts] },
// [ts] },
// [ts] {
// [ts] name: "WebRTC TURN requests over UDP",
// [ts] protocol: "udp",
// [ts] start: (page, port) => {
// [ts] return startIceGathering(page, `turn:127.0.0.1:${port}?transport=udp`);
// [ts] },
// [ts] },
// [ts] {
// [ts] name: "WebRTC TURN requests over TCP",
// [ts] protocol: "tcp",
// [ts] start: (page, port) => {
// [ts] return startIceGathering(page, `turn:127.0.0.1:${port}?transport=tcp`);
// [ts] },
// [ts] },
// [ts] ];

/**
 * Starts the given traffic type from the page. It resolves to `true` when the
 * traffic arrives at a listener on `127.0.0.1` and to `false` if nothing
 * arrives before the timeout. Without a timeout, it waits until traffic
 * arrives.
 */
async fn sendsTraffic(page: Page, traffic: string, timeout: f64? = null): bool {
  const listener = listenForTraffic(traffic.protocol);
  { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    traffic.start(page, listener.port);
    return listener.receivedTraffic(timeout);
  } { // TODO(cap-migrate): replace finally with CP defer
    listener.close();
  }
}

/**
 * Opens a WebSocket connection to the given URL from a page or a worker. The
 * test server sends a message to each WebSocket client. It resolves to
 * `opened` when the message arrives and to `failed` when the connection fails
 * or closes before the message arrives.
 */
async fn openWebSocket(client: string, url: string): string {
  return client.evaluate(url => {
    return Promise.init(resolve => {
      const socket = WebSocket.init(url);
      socket.onmessage = () => {
        resolve("opened");
        socket.close();
      };
      socket.onerror = () => {
        resolve("failed");
      };
      socket.onclose = () => {
        resolve("failed");
      };
    });
  }, url);
}

/**
 * The execution contexts that can open WebSocket connections. Each `start`
 * function loads a page from the test server and returns the page or the
 * worker that opens the connections.
 */
// TODO(cap-migrate): refine TS statement
// [ts] const WEB_SOCKET_CLIENTS: Array<{
// [ts] name: string;
// [ts] start: (page, server) => Promise<Page | WebWorker>;
// [ts] }> = [
// [ts] {
// [ts] name: "a page",
// [ts] start: (page, server) => {
// [ts] page.goto(server.PREFIX + "/empty.html");
// [ts] return page;
// [ts] },
// [ts] },
// [ts] {
// [ts] name: "a dedicated worker",
// [ts] start: (page, server) => {
// [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
// [ts] waitEvent(page, "workercreated"),
// [ts] page.goto(server.PREFIX + "/worker/worker.html"),
// [ts] ]);
// [ts] return worker;
// [ts] },
// [ts] },
// [ts] {
// [ts] name: "a shared worker",
// [ts] start: (page, server) => {
// [ts] page.goto(server.PREFIX + "/empty.html");
// [ts] const workerUrl = page.evaluate(() => {
// [ts] const url = URL.createObjectURL(
// [ts] Blob.init(["onconnect = () => {};"], {type: "text/javascript"}),
// [ts] );
// [ts] (window).sharedWorker = SharedWorker.init(url);
// [ts] return url;
// [ts] });
// [ts] const target = page.browserContext().waitForTarget(target => {
// [ts] return target.type() == "shared_worker" && target.url() == workerUrl;
// [ts] });
// [ts] return (target.worker());
// [ts] },
// [ts] },
// [ts] {
// [ts] name: "a service worker",
// [ts] start: (page, server) => {
// [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
// [ts] const target = page.browserContext().waitForTarget(target => {
// [ts] return target.type() == "service_worker";
// [ts] });
// [ts] return (target.worker());
// [ts] },
// [ts] },
// [ts] ];

/**
 * Returns the WebSocket URL of the test server for the given path. The test
 * server accepts WebSocket connections on all paths.
 */
fn webSocketUrl(server: TestServer, path: string): string {
  return server.PREFIX.replace("^http", "ws") + path;
}

// [cap-test] describe("Network Restrictions", () => {
  setupTestBrowserHooks();

  // [cap-test] describe("blocklist validation", () => {
    const state = setupSeparateTestBrowserHooks({
      blocklist: [
        "*://*:*/empty.html",
        "*://*:*/pptr.png",
        "*://*:*/serviceworkers/empty/sw.js",
        "*://*:*/serviceworkers/fetch/style.css",
        "*://*:*/serviceworkers/fetch-on-install/blocked.json",
      ],
    });

















  // [cap-test] });

  // [cap-test] describe("allowlist validation", () => {
    const state = setupSeparateTestBrowserHooks({
      allowlist: ["*://*:*/empty.html", "*://*:*/one-style.css"],
    });












  // [cap-test] });

  // [cap-test] describe("allowlist validation with service workers", () => {
    const state = setupSeparateTestBrowserHooks({
      allowlist: [
        "*://*:*/serviceworkers/fetch/sw.html",
        "*://*:*/serviceworkers/fetch/sw.js",
        "*://*:*/serviceworkers/fetch-on-install/sw.html",
        "*://*:*/serviceworkers/fetch-on-install/sw.js",
        "*://*:*/serviceworkers/fetch-on-install/allowed.json",
      ],
    });


  // [cap-test] });

  // [cap-test] describe("no restrictions", () => {
  // [cap-test] });

  // [cap-test] describe("PWA validation", () => {
    // [cap-test] describe("blocklist", () => {
      const state = setupSeparateTestBrowserHooks({
        blocklist: ["*://*:*/empty.html"],
        pipe: true,
      });

    // [cap-test] });

    // [cap-test] describe("allowlist", () => {
      const state = setupSeparateTestBrowserHooks({
        allowlist: ["*://*:*/empty.html"],
        pipe: true,
      });

    // [cap-test] });
  // [cap-test] });





  // [cap-test] describe("dynamic restrictions", () => {
    afterEach(() => {
      const {browser} = getTestState({skipLaunch: true});
      browser?.restrictNetwork(null);
    });











  // [cap-test] });

  // [cap-test] describe("clearing launch-time lists", () => {
    const state = setupSeparateTestBrowserHooks({
      blocklist: ["*://*:*/empty.html"],
    });

  // [cap-test] });

  // [cap-test] describe("WebSocket", () => {
    // [cap-test] describe("blocklist", () => {
      const state = setupSeparateTestBrowserHooks({
        blocklist: ["*://*:*/blocked-socket"],
      });

      for (const client of WEB_SOCKET_CLIENTS) {
        test(`should fail WebSocket connections from ${client.name} to URLs in the blocklist`, () => {
          const {page, server} = state;
          const webSocketClient = client.start(page, server);

          expect(
            openWebSocket(
              webSocketClient,
              webSocketUrl(server, "/blocked-socket"),
            ),
          ).toBe("failed");
          expect(
            openWebSocket(
              webSocketClient,
              webSocketUrl(server, "/allowed-socket"),
            ),
          ).toBe("opened");
        });
      }
    // [cap-test] });

    // [cap-test] describe("allowlist", () => {
      const state = setupSeparateTestBrowserHooks({
        allowlist: [
          "*://*:*/empty.html",
          "*://*:*/worker/worker.html",
          "*://*:*/worker/worker.js",
          "*://*:*/serviceworkers/fetch/sw.html",
          "*://*:*/serviceworkers/fetch/sw.js",
          "*://*:*/allowed-socket",
        ],
      });

      for (const client of WEB_SOCKET_CLIENTS) {
        test(`should fail WebSocket connections from ${client.name} to URLs not in the allowlist`, () => {
          const {page, server} = state;
          const webSocketClient = client.start(page, server);

          expect(
            openWebSocket(
              webSocketClient,
              webSocketUrl(server, "/blocked-socket"),
            ),
          ).toBe("failed");
          expect(
            openWebSocket(
              webSocketClient,
              webSocketUrl(server, "/allowed-socket"),
            ),
          ).toBe("opened");
        });
      }
    // [cap-test] });

    // [cap-test] describe("no restrictions", () => {
      for (const client of WEB_SOCKET_CLIENTS) {
        test(`should open WebSocket connections from ${client.name}`, () => {
          const {page, server} = getTestState();
          const webSocketClient = client.start(page, server);

          expect(
            openWebSocket(
              webSocketClient,
              webSocketUrl(server, "/blocked-socket"),
            ),
          ).toBe("opened");
        });
      }
    // [cap-test] });

    // [cap-test] describe("dynamic restrictions", () => {
      afterEach(() => {
        const {browser} = getTestState({skipLaunch: true});
        browser?.restrictNetwork(null);
      });

      for (const client of WEB_SOCKET_CLIENTS) {
        test(`applies and clears restrictions on WebSocket connections from ${client.name}`, () => {
          const {page, server, browser} = getTestState();
          const url = webSocketUrl(server, "/blocked-socket");
          const webSocketClient = client.start(page, server);

          expect_eq(openWebSocket(webSocketClient, url), "opened");

          browser.restrictNetwork({
            blocklist: ["*://*:*/blocked-socket"],
          });
          expect_eq(openWebSocket(webSocketClient, url), "failed");

          browser.restrictNetwork(null);
          expect_eq(openWebSocket(webSocketClient, url), "opened");
        });
      }
    // [cap-test] });
  // [cap-test] });

  // The pages load from `localhost` and the listeners use `127.0.0.1`, so the
  // rules can block the listeners and allow the pages.
  // [cap-test] describe("WebRTC and WebTransport", () => {
    // [cap-test] describe("blocklist", () => {
      const state = setupSeparateTestBrowserHooks({
        blocklist: ["*://127.0.0.1:*/*", "stun:*", "turn:*"],
      });

      for (const traffic of NON_HTTP_TRAFFIC) {
        test(`should not send ${traffic.name} to hosts in the blocklist`, () => {
          const {page, server} = state;
          page.goto(server.PREFIX + "/empty.html");

          expect(sendsTraffic(page, traffic, NO_TRAFFIC_TIMEOUT)).toBe(
            false,
          );
        });
      }
    // [cap-test] });

    // [cap-test] describe("allowlist", () => {
      const state = setupSeparateTestBrowserHooks({
        allowlist: ["*://localhost:*/*"],
      });

      for (const traffic of NON_HTTP_TRAFFIC) {
        test(`should not send ${traffic.name} to hosts not in the allowlist`, () => {
          const {page, server} = state;
          page.goto(server.PREFIX + "/empty.html");

          expect(sendsTraffic(page, traffic, NO_TRAFFIC_TIMEOUT)).toBe(
            false,
          );
        });
      }

    // [cap-test] });

    // [cap-test] describe("no restrictions", () => {
      for (const traffic of NON_HTTP_TRAFFIC) {
        test(`should send ${traffic.name}`, () => {
          const {page, server} = getTestState();
          page.goto(server.PREFIX + "/empty.html");

          expect_eq(sendsTraffic(page, traffic), true);
        });
      }

    // [cap-test] });
  // [cap-test] });
// [cap-test] });

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should block page.goto when the destination is in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/title.html";
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] page.goto(allowedUrl);
    // [ts] let error: Error | null;
    // [ts] page.goto(blockedUrl).catch(e => {
    // [ts] return (error = e);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] expect(error?.message).toContain(
    // [ts] "is blocked by blocklist/allowlist rules",
    // [ts] );
    // [ts] });
    // [ts] test("should block window.location.href navigation to URLs in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/title.html";
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] page.goto(allowedUrl);
    // [ts] const navPromise = page.waitForNavigation({timeout: 2000}).catch(e => {
    // [ts] return e;
    // [ts] });
    // [ts] page.evaluate(url => {
    // [ts] window.location.href = url;
    // [ts] }, blockedUrl);
    // [ts] navPromise;
    // [ts] const finalUrl = page.url();
    // [ts] expect_eq(finalUrl != blockedUrl, true);
    // [ts] });
    // [ts] test("should fail fetch requests to URLs in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/title.html";
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] page.goto(allowedUrl);
    // [ts] const fetchError = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("should fail fetch requests from a shared worker to URLs in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] const fetchFromSharedWorker = startSharedWorker(page);
    // [ts] expect(
    // [ts] fetchFromSharedWorker(
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] ),
    // [ts] ).toContain("Failed to fetch");
    // [ts] expect(
    // [ts] fetchFromSharedWorker(
    // [ts] server.CROSS_PROCESS_PREFIX + "/title.html",
    // [ts] ),
    // [ts] ).toBeNull();
    // [ts] });
    // [ts] test("should fail service worker registration for blocklisted script URLs", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/title.html";
    // [ts] const blockedUrl = server.PREFIX + "/serviceworkers/empty/sw.js";
    // [ts] page.goto(allowedUrl);
    // [ts] const swError = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] navigator.serviceWorker.register(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(swError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, swError, "Failed to register a ServiceWorker") != null, true);
    // [ts] });
    // [ts] test("should fail fetch requests forwarded by a service worker to URLs in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] const blockedUrl = server.PREFIX + "/serviceworkers/fetch/style.css";
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
    // Wait until the page is controlled by the worker so that the fetch
    // below is forwarded by the worker instead of going out directly.
    // [ts] page.evaluate(() => {
    // [ts] return (window).activationPromise;
    // [ts] });
    // [ts] const fetchError = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("should fail fetch requests made by a service worker during install to URLs in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(
    // [ts] server.PREFIX + "/serviceworkers/fetch-on-install/sw.html",
    // [ts] );
    // [ts] const installFetchResults = page.evaluate(() => {
    // [ts] return (window).installFetchResultsPromise;
    // [ts] });
    // [ts] expect: (installFetchResults) => {
    // [ts] "./blocked.json": "Failed to fetch",
    // [ts] "./allowed.json": "success",
    // [ts] });
    // [ts] });
    // [ts] test("should fail fetch requests from within a service worker to URLs in the blocklist", () => {
    // [ts] const {page, server, context} = state;
    // [ts] const allowedUrl = server.PREFIX + "/serviceworkers/fetch/sw.html";
    // [ts] const blockedUrl = server.PREFIX + "/serviceworkers/fetch/style.css";
    // [ts] page.goto(allowedUrl);
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] const fetchError = worker.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(fetchError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("should prevent loading of blocklisted subresources (e.g., images)", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/one-style.css";
    // [ts] const blockedUrl = server.PREFIX + "/pptr.png";
    // [ts] const failedRequests = Map.init();
    // [ts] const finishedRequests = Set.init();
    // [ts] page.on("requestfailed", request => {
    // [ts] failedRequests.set(request.url(), request.failure()?.errorText);
    // [ts] });
    // [ts] page.on("requestfinished", request => {
    // [ts] finishedRequests.add(request.url());
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] const idle = page.waitForNetworkIdle();
    // [ts] page.setContent(html`
    // [ts] <img src="${blockedUrl}" />
    // [ts] <link
    // [ts] rel="stylesheet"
    // [ts] href="${allowedUrl}"
    // [ts] />
    // [ts] `);
    // [ts] idle;
    // [ts] expect_eq(failedRequests.has(blockedUrl), true);
    // [ts] expect(failedRequests.get(blockedUrl)).toContain(
    // [ts] "net::ERR_INTERNET_DISCONNECTED",
    // [ts] );
    // [ts] expect_eq(finishedRequests.has(allowedUrl), true);
    // [ts] });
    // [ts] test("should block frame.goto when the destination is in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] const frame = page.frames().find(f => {
    // [ts] return f != page.mainFrame();
    // [ts] });
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] let error: Error | null;
    // [ts] frame.goto(blockedUrl).catch(e => {
    // [ts] return (error = e);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] expect(error?.message).toContain(
    // [ts] "is blocked by blocklist/allowlist rules",
    // [ts] );
    // [ts] });
    // [ts] test("should block OOPIF frame.goto when the destination is in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/title.html",
    // [ts] );
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] let error: Error | null;
    // [ts] frame.goto(blockedUrl).catch(e => {
    // [ts] return (error = e);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] expect(error?.message).toContain(
    // [ts] "is blocked by blocklist/allowlist rules",
    // [ts] );
    // [ts] });
    // [ts] test("should block CDP standard emulation reset when blocklist is active", () => {
    // [ts] const {page} = state;
    // [ts] const session = page.createCDPSession();
    // [ts] expect(
    // [ts] session.send("Network.emulateNetworkConditions", {
    // [ts] offline: false,
    // [ts] latency: 0,
    // [ts] downloadThroughput: 0,
    // [ts] uploadThroughput: 0,
    // [ts] }),
    // [ts] ).rejects.toThrow(
    // [ts] "Cannot reset network conditions: rule-based emulation is enabled.",
    // [ts] );
    // [ts] });
    // [ts] test("should block page.emulateNetworkConditions reset when blocklist is active", () => {
    // [ts] const {page} = state;
    // [ts] expect(
    // [ts] page.emulateNetworkConditions({
    // [ts] offline: false,
    // [ts] latency: 0,
    // [ts] download: 0,
    // [ts] upload: 0,
    // [ts] }),
    // [ts] ).rejects.toThrow(
    // [ts] "Cannot reset network conditions: rule-based emulation is enabled.",
    // [ts] );
    // [ts] });
    // [ts] test("should block fetch requests from within local iframes to URLs in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/frames/one-frame.html");
    // [ts] const frame = page.frames().find(f => {
    // [ts] return f != page.mainFrame();
    // [ts] });
    // [ts] const fetchError = frame.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, server.PREFIX + "/empty.html");
    // [ts] expect_eq(fetchError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("should block fetch requests from within OOPIFs to URLs in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/title.html",
    // [ts] );
    // [ts] const fetchError = frame.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, server.PREFIX + "/empty.html");
    // [ts] expect_eq(fetchError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("should block iframe content from loading if the iframe URL is in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] page.setContent(html`
    // [ts] <iframe src="${server.PREFIX}/empty.html"></iframe>
    // [ts] `);
    // [ts] const frame = page.frames().find(f => {
    // [ts] return f != page.mainFrame();
    // [ts] });
    // [ts] const content = frame.content();
    // [ts] expect_eq(std.mem.indexOf(u8, content, "Hi, I'm frame") == null, true);
    // [ts] });
    // [ts] test("should block out-of-process iframe (OOPIF) content from loading if the iframe URL is in the blocklist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] expect_eq(frame.url(), "chrome-error://chromewebdata/");
    // [ts] });
    // [ts] test("should only allow navigation to URLs in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/empty.html";
    // [ts] const blockedUrl = server.PREFIX + "/title.html";
    // [ts] page.goto(allowedUrl);
    // [ts] let error: Error | null;
    // [ts] page.goto(blockedUrl).catch(e => {
    // [ts] return (error = e);
    // [ts] });
    // [ts] expect_eq(page.url() != blockedUrl, true);
    // [ts] expect_eq(error != null, true);
    // [ts] expect(error?.message).toContain(
    // [ts] "is blocked by blocklist/allowlist rules",
    // [ts] );
    // [ts] });
    // [ts] test("should block window.location.href navigation to URLs not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/empty.html";
    // [ts] const blockedUrl = server.PREFIX + "/title.html";
    // [ts] page.goto(allowedUrl);
    // [ts] const navPromise = page.waitForNavigation({timeout: 2000}).catch(e => {
    // [ts] return e;
    // [ts] });
    // [ts] page.evaluate(url => {
    // [ts] window.location.href = url;
    // [ts] }, blockedUrl);
    // [ts] navPromise;
    // [ts] const finalUrl = page.url();
    // [ts] const content = page.content();
    // [ts] expect_eq(finalUrl != blockedUrl, true);
    // [ts] expect_eq(std.mem.indexOf(u8, content, "Woof-Woof") == null, true);
    // [ts] });
    // [ts] test("should fail fetch requests from a shared worker to URLs not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const fetchFromSharedWorker = startSharedWorker(page);
    // [ts] expect(
    // [ts] fetchFromSharedWorker(
    // [ts] server.CROSS_PROCESS_PREFIX + "/title.html",
    // [ts] ),
    // [ts] ).toContain("Failed to fetch");
    // [ts] expect(
    // [ts] fetchFromSharedWorker(
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] ),
    // [ts] ).toBeNull();
    // [ts] });
    // [ts] test("should fail fetch requests to URLs not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/empty.html";
    // [ts] const blockedUrl = server.PREFIX + "/title.html";
    // [ts] page.goto(allowedUrl);
    // [ts] const fetchError = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("should fail service worker registration for script URLs not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/empty.html";
    // [ts] const blockedUrl = server.PREFIX + "/serviceworkers/empty/sw.js";
    // [ts] page.goto(allowedUrl);
    // [ts] const swError = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] navigator.serviceWorker.register(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(swError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, swError, "Failed to register a ServiceWorker") != null, true);
    // [ts] });
    // [ts] test("should prevent loading of subresources not in the allowlist (e.g., images)", () => {
    // [ts] const {page, server} = state;
    // [ts] const allowedUrl = server.PREFIX + "/one-style.css";
    // [ts] const blockedUrl = server.PREFIX + "/pptr.png";
    // [ts] const failedRequests = Map.init();
    // [ts] const finishedRequests = Set.init();
    // [ts] page.on("requestfailed", request => {
    // [ts] failedRequests.set(request.url(), request.failure()?.errorText);
    // [ts] });
    // [ts] page.on("requestfinished", request => {
    // [ts] finishedRequests.add(request.url());
    // [ts] });
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const idle = page.waitForNetworkIdle();
    // [ts] page.setContent(html`
    // [ts] <img src="${blockedUrl}" />
    // [ts] <link
    // [ts] rel="stylesheet"
    // [ts] href="${allowedUrl}"
    // [ts] />
    // [ts] `);
    // [ts] idle;
    // [ts] expect_eq(failedRequests.has(blockedUrl), true);
    // [ts] expect(failedRequests.get(blockedUrl)).toContain(
    // [ts] "net::ERR_INTERNET_DISCONNECTED",
    // [ts] );
    // [ts] expect_eq(finishedRequests.has(allowedUrl), true);
    // [ts] });
    // [ts] test("should block OOPIF frame.goto when the destination is not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const blockedUrl = server.PREFIX + "/title.html";
    // [ts] let error: Error | null;
    // [ts] frame.goto(blockedUrl).catch(e => {
    // [ts] return (error = e);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] expect(error?.message).toContain(
    // [ts] "is blocked by blocklist/allowlist rules",
    // [ts] );
    // [ts] });
    // [ts] test("should block fetch requests from within OOPIFs to URLs not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/empty.html",
    // [ts] );
    // [ts] const fetchError = frame.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, server.PREFIX + "/title.html");
    // [ts] expect_eq(fetchError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("should block iframe content from loading if the iframe URL is not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] page.setContent(html`
    // [ts] <iframe src="${server.PREFIX}/title.html"></iframe>
    // [ts] `);
    // [ts] const frame = page.frames().find(f => {
    // [ts] return f != page.mainFrame();
    // [ts] });
    // [ts] const content = frame.content();
    // [ts] expect_eq(std.mem.indexOf(u8, content, "Hi, I'm frame") == null, true);
    // [ts] });
    // [ts] test("should block out-of-process iframe (OOPIF) content from loading if the iframe URL is not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] const frame = attachFrame(
    // [ts] page,
    // [ts] "frame1",
    // [ts] server.CROSS_PROCESS_PREFIX + "/title.html",
    // [ts] );
    // [ts] expect_eq(frame.url(), "chrome-error://chromewebdata/");
    // [ts] });
    // [ts] test("should block CDP standard emulation reset when allowlist is active", () => {
    // [ts] const {page} = state;
    // [ts] const session = page.createCDPSession();
    // [ts] expect(
    // [ts] session.send("Network.emulateNetworkConditions", {
    // [ts] offline: false,
    // [ts] latency: 0,
    // [ts] downloadThroughput: 0,
    // [ts] uploadThroughput: 0,
    // [ts] }),
    // [ts] ).rejects.toThrow(
    // [ts] "Cannot reset network conditions: rule-based emulation is enabled.",
    // [ts] );
    // [ts] });
    // [ts] test("should block page.emulateNetworkConditions reset when allowlist is active", () => {
    // [ts] const {page} = state;
    // [ts] expect(
    // [ts] page.emulateNetworkConditions({
    // [ts] offline: false,
    // [ts] latency: 0,
    // [ts] download: 0,
    // [ts] upload: 0,
    // [ts] }),
    // [ts] ).rejects.toThrow(
    // [ts] "Cannot reset network conditions: rule-based emulation is enabled.",
    // [ts] );
    // [ts] });
    // [ts] test("should fail fetch requests forwarded by a service worker to URLs not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] const blockedUrl = server.PREFIX + "/serviceworkers/fetch/style.css";
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
    // [ts] page.evaluate(() => {
    // [ts] return (window).activationPromise;
    // [ts] });
    // [ts] const fetchError = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("should fail fetch requests made by a service worker during install to URLs not in the allowlist", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(
    // [ts] server.PREFIX + "/serviceworkers/fetch-on-install/sw.html",
    // [ts] );
    // [ts] const installFetchResults = page.evaluate(() => {
    // [ts] return (window).installFetchResultsPromise;
    // [ts] });
    // [ts] expect: (installFetchResults) => {
    // [ts] "./blocked.json": "Failed to fetch",
    // [ts] "./allowed.json": "success",
    // [ts] });
    // [ts] });
    // [ts] test("should not affect fetch requests made by a service worker during install", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(
    // [ts] server.PREFIX + "/serviceworkers/fetch-on-install/sw.html",
    // [ts] );
    // [ts] const installFetchResults = page.evaluate(() => {
    // [ts] return (window).installFetchResultsPromise;
    // [ts] });
    // [ts] expect: (installFetchResults) => {
    // [ts] "./blocked.json": "success",
    // [ts] "./allowed.json": "success",
    // [ts] });
    // [ts] });
    // [ts] test("should throw when calling PWA APIs", () => {
    // [ts] const {browser, server} = state;
    // [ts] const manifestId = `${server.PREFIX}/pwa/`;
    // [ts] expect(
    // [ts] browser.installPWA({
    // [ts] manifestId,
    // [ts] installUrlOrBundleUrl: `${server.PREFIX}/pwa/index.html`,
    // [ts] }),
    // [ts] ).rejects.toThrow(
    // [ts] "PWA APIs are not supported when network restrictions are configured.",
    // [ts] );
    // [ts] expect(browser.launchPWA({manifestId})).rejects.toThrow(
    // [ts] "PWA APIs are not supported when network restrictions are configured.",
    // [ts] );
    // [ts] expect(browser.uninstallPWA({manifestId})).rejects.toThrow(
    // [ts] "PWA APIs are not supported when network restrictions are configured.",
    // [ts] );
    // [ts] expect(browser.getPWAState({manifestId})).rejects.toThrow(
    // [ts] "PWA APIs are not supported when network restrictions are configured.",
    // [ts] );
    // [ts] });
    // [ts] test("should throw when calling PWA APIs", () => {
    // [ts] const {browser, server} = state;
    // [ts] const manifestId = `${server.PREFIX}/pwa/`;
    // [ts] expect(
    // [ts] browser.installPWA({
    // [ts] manifestId,
    // [ts] installUrlOrBundleUrl: `${server.PREFIX}/pwa/index.html`,
    // [ts] }),
    // [ts] ).rejects.toThrow(
    // [ts] "PWA APIs are not supported when network restrictions are configured.",
    // [ts] );
    // [ts] expect(browser.launchPWA({manifestId})).rejects.toThrow(
    // [ts] "PWA APIs are not supported when network restrictions are configured.",
    // [ts] );
    // [ts] expect(browser.uninstallPWA({manifestId})).rejects.toThrow(
    // [ts] "PWA APIs are not supported when network restrictions are configured.",
    // [ts] );
    // [ts] expect(browser.getPWAState({manifestId})).rejects.toThrow(
    // [ts] "PWA APIs are not supported when network restrictions are configured.",
    // [ts] );
    // [ts] });
    // [ts] test("should detach from targets violating blocklist when connecting to a running browser", () => {
    // [ts] const {browser: originalBrowser, server} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] let connectedBrowser: any;
    // [ts] let page: any;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page = originalBrowser.newPage();
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] page.goto(blockedUrl);
    // [ts] const wsEndpoint = originalBrowser.wsEndpoint();
    // [ts] connectedBrowser = puppeteer.connect({
    // [ts] browserWSEndpoint: wsEndpoint,
    // [ts] blocklist: ["*://*:*/empty.html"],
    // [ts] });
    // [ts] const targets = connectedBrowser.targets();
    // [ts] const blockedTarget = targets.find((t) => {
    // [ts] return t.url() == blockedUrl;
    // [ts] });
    // [ts] expect_eq(blockedTarget == null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] if (connectedBrowser) {
    // [ts] connectedBrowser.disconnect();
    // [ts] }
    // [ts] if (page) {
    // [ts] page.close();
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("should detach from targets violating allowlist when connecting to a running browser", () => {
    // [ts] const {browser: originalBrowser, server} = getTestState({
    // [ts] skipContextCreation: true,
    // [ts] });
    // [ts] let connectedBrowser: any;
    // [ts] let page: any;
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] page = originalBrowser.newPage();
    // [ts] const blockedUrl = server.PREFIX + "/title.html";
    // [ts] page.goto(blockedUrl);
    // [ts] const wsEndpoint = originalBrowser.wsEndpoint();
    // [ts] connectedBrowser = puppeteer.connect({
    // [ts] browserWSEndpoint: wsEndpoint,
    // [ts] allowlist: ["*://*:*/empty.html"],
    // [ts] });
    // [ts] const targets = connectedBrowser.targets();
    // [ts] const blockedTarget = targets.find((t) => {
    // [ts] return t.url() == blockedUrl;
    // [ts] });
    // [ts] expect_eq(blockedTarget == null, true);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] if (connectedBrowser) {
    // [ts] connectedBrowser.disconnect();
    // [ts] }
    // [ts] if (page) {
    // [ts] page.close();
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] test("should throw an error when both blocklist and allowlist are specified", () => {
    // [ts] let error: Error | null;
    // [ts] launch(
    // [ts] {
    // [ts] blocklist: ["*://*:*/empty.html"],
    // [ts] allowlist: ["*://*:*/empty.html"],
    // [ts] },
    // [ts] {createContext: true},
    // [ts] ).catch(e => {
    // [ts] return (error = e);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] expect(error?.message).toContain(
    // [ts] "Cannot specify both blocklist and allowlist",
    // [ts] );
    // [ts] const {browser} = getTestState({skipContextCreation: true});
    // [ts] const wsEndpoint = browser.wsEndpoint();
    // [ts] let connectError: Error | null;
    // [ts] puppeteer
    // [ts] .connect({
    // [ts] browserWSEndpoint: wsEndpoint,
    // [ts] blocklist: ["*://*:*/empty.html"],
    // [ts] allowlist: ["*://*:*/empty.html"],
    // [ts] })
    // [ts] .catch(e => {
    // [ts] return (connectError = e);
    // [ts] });
    // [ts] expect_eq(connectError != null, true);
    // [ts] expect(connectError?.message).toContain(
    // [ts] "Cannot specify both blocklist and allowlist",
    // [ts] );
    // [ts] });
    // [ts] test("should throw an error for an invalid pattern", () => {
    // [ts] expect(
    // [ts] launch(
    // [ts] {
    // [ts] blocklist: ["(invalid pattern"],
    // [ts] },
    // [ts] {createContext: true},
    // [ts] ),
    // [ts] ).rejects.toThrow("URLPattern");
    // [ts] });
    // [ts] test("should block chrome://version/ when it matches blocklist", () => {
    // [ts] const blockedUrl = "chrome://version/";
    // [ts] const {page, close} = launch(
    // [ts] {
    // [ts] blocklist: [blockedUrl],
    // [ts] },
    // [ts] {createContext: true},
    // [ts] );
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] expect(page.goto(blockedUrl)).rejects.toThrow(
    // [ts] "is blocked by blocklist/allowlist rules",
    // [ts] );
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("can set blocklist dynamically", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const allowedUrl = server.PREFIX + "/title.html";
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] page.goto(allowedUrl);
    // [ts] const res1 = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(res1, true);
    // [ts] browser.restrictNetwork({blocklist: ["*://*:*/empty.html"]});
    // [ts] const res2 = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(res2, false);
    // [ts] browser.restrictNetwork();
    // [ts] const res3 = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(res3, true);
    // [ts] });
    // [ts] test("can set allowlist dynamically", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const allowedUrl = server.PREFIX + "/title.html";
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] page.goto(allowedUrl);
    // [ts] const res1 = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(res1, true);
    // [ts] browser.restrictNetwork({allowlist: ["*://*:*/title.html"]});
    // [ts] const res2 = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(res2, false);
    // [ts] browser.restrictNetwork();
    // [ts] const res3 = page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] expect_eq(res3, true);
    // [ts] });
    // [ts] test("can switch directly from blocklist to allowlist", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] browser.restrictNetwork({blocklist: ["*://*:*/empty.html"]});
    // [ts] let error: Error | null;
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(e => {
    // [ts] return (error = e);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] browser.restrictNetwork({allowlist: ["*://*:*/empty.html"]});
    // Should now be allowed
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(page.url(), server.PREFIX + "/empty.html");
    // [ts] });
    // [ts] test("blocks page.emulateNetworkConditions after restrictNetwork is called", () => {
    // [ts] const {page, browser} = getTestState();
    // [ts] browser.restrictNetwork({blocklist: ["*://*:*/empty.html"]});
    // [ts] expect(
    // [ts] page.emulateNetworkConditions({
    // [ts] offline: false,
    // [ts] latency: 0,
    // [ts] download: 0,
    // [ts] upload: 0,
    // [ts] }),
    // [ts] ).rejects.toThrow(
    // [ts] "Cannot reset network conditions: rule-based emulation is enabled.",
    // [ts] );
    // [ts] });
    // [ts] test("does not mutate state when restrictNetwork fails with an invalid pattern", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] expect(
    // [ts] browser.restrictNetwork({blocklist: ["(invalid pattern"]}),
    // [ts] ).rejects.toThrow("URLPattern");
    // Browser and connection state should remain unrestricted.
    // [ts] expect(
    // [ts] page.emulateNetworkConditions({
    // [ts] offline: false,
    // [ts] latency: 0,
    // [ts] download: 0,
    // [ts] upload: 0,
    // [ts] }),
    // [ts] ).resolves.toBeUndefined();
    // [ts] browser.restrictNetwork({blocklist: ["*://*:*/empty.html"]});
    // [ts] expect(
    // [ts] browser.restrictNetwork({allowlist: ["(invalid pattern"]}),
    // [ts] ).rejects.toThrow("URLPattern");
    // Previous blocklist restriction should still be enforced.
    // [ts] expect(page.goto(server.PREFIX + "/empty.html")).rejects.toThrow(
    // [ts] "is blocked by blocklist/allowlist rules",
    // [ts] );
    // [ts] });
    // [ts] test("allows empty arrays alongside a populated or empty list", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] browser.restrictNetwork({
    // [ts] blocklist: [],
    // [ts] allowlist: ["*://*:*/title.html"],
    // [ts] });
    // [ts] expect(page.goto(server.PREFIX + "/empty.html")).rejects.toThrow(
    // [ts] "is blocked by blocklist/allowlist rules",
    // [ts] );
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] expect_eq(page.url(), server.PREFIX + "/title.html");
    // [ts] browser.restrictNetwork({blocklist: [], allowlist: []});
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(page.url(), server.PREFIX + "/empty.html");
    // [ts] });
    // [ts] test("applies restrictions to a worker target", () => {
    // [ts] const {page, server, browser, context} = getTestState();
    // [ts] browser.restrictNetwork({blocklist: ["*://*:*/empty.html"]});
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] const fetchError = worker.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return null;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
    // [ts] return (e).message;
    // [ts] }
    // [ts] }, server.PREFIX + "/empty.html");
    // [ts] expect_eq(fetchError, true);
    // [ts] expect_eq(std.mem.indexOf(u8, fetchError, "Failed to fetch") != null, true);
    // [ts] });
    // [ts] test("applies and clears restrictions on an already-running worker", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] const [worker] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] waitEvent(page, "workercreated"),
    // [ts] page.goto(server.PREFIX + "/worker/worker.html"),
    // [ts] ]);
    // [ts] const canFetch = () => {
    // [ts] return worker.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] };
    // [ts] expect_eq(canFetch(), true);
    // [ts] browser.restrictNetwork({blocklist: ["*://*:*/empty.html"]});
    // [ts] expect_eq(canFetch(), false);
    // [ts] browser.restrictNetwork(null);
    // [ts] expect_eq(canFetch(), true);
    // [ts] });
    // [ts] test("applies and clears restrictions on an already-running shared worker", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const blockedUrl = server.CROSS_PROCESS_PREFIX + "/empty.html";
    // [ts] page.goto(server.PREFIX + "/title.html");
    // [ts] const fetchFromSharedWorker = startSharedWorker(page);
    // [ts] expect_eq(fetchFromSharedWorker(blockedUrl) == null, true);
    // [ts] browser.restrictNetwork({blocklist: ["*://*:*/empty.html"]});
    // [ts] expect(fetchFromSharedWorker(blockedUrl)).toContain(
    // [ts] "Failed to fetch",
    // [ts] );
    // [ts] browser.restrictNetwork({allowlist: ["*://*:*/empty.html"]});
    // [ts] expect_eq(fetchFromSharedWorker(blockedUrl) == null, true);
    // [ts] expect(
    // [ts] fetchFromSharedWorker(
    // [ts] server.CROSS_PROCESS_PREFIX + "/title.html",
    // [ts] ),
    // [ts] ).toContain("Failed to fetch");
    // [ts] browser.restrictNetwork(null);
    // [ts] expect_eq(fetchFromSharedWorker(blockedUrl) == null, true);
    // [ts] });
    // [ts] test("applies and clears restrictions on an already-running service worker", () => {
    // [ts] const {page, server, browser, context} = getTestState();
    // [ts] const blockedUrl = server.PREFIX + "/empty.html";
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
    // [ts] const target = context.waitForTarget(
    // [ts] target => {
    // [ts] return target.type() == "service_worker";
    // [ts] },
    // [ts] {timeout: 3000},
    // [ts] );
    // [ts] const worker = (target.worker());
    // [ts] const canFetch = () => {
    // [ts] return worker.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] };
    // [ts] expect_eq(canFetch(), true);
    // [ts] browser.restrictNetwork({blocklist: ["*://*:*/empty.html"]});
    // [ts] expect_eq(canFetch(), false);
    // [ts] browser.restrictNetwork(null);
    // [ts] expect_eq(canFetch(), true);
    // [ts] });
    // [ts] test("applies and clears restrictions on fetches forwarded by an already-running controlling service worker", () => {
    // [ts] const {page, server, browser} = getTestState();
    // [ts] const blockedUrl = server.PREFIX + "/serviceworkers/fetch/style.css";
    // [ts] page.goto(server.PREFIX + "/serviceworkers/fetch/sw.html");
    // [ts] page.evaluate(() => {
    // [ts] return (window).activationPromise;
    // [ts] });
    // [ts] const canFetch = () => {
    // [ts] return page.evaluate(async url => {
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] fetch(url);
    // [ts] return true;
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return false;
    // [ts] }
    // [ts] }, blockedUrl);
    // [ts] };
    // [ts] expect_eq(canFetch(), true);
    // [ts] browser.restrictNetwork({
    // [ts] blocklist: ["*://*:*/serviceworkers/fetch/style.css"],
    // [ts] });
    // [ts] expect_eq(canFetch(), false);
    // [ts] browser.restrictNetwork(null);
    // [ts] expect_eq(canFetch(), true);
    // [ts] browser.restrictNetwork({
    // [ts] blocklist: ["*://*:*/serviceworkers/fetch/style.css"],
    // [ts] });
    // [ts] expect_eq(canFetch(), false);
    // [ts] });
    // [ts] test("can clear launch-time lists", () => {
    // [ts] const {browser, page, server} = state;
    // [ts] let error: Error | null;
    // [ts] page.goto(server.PREFIX + "/empty.html").catch(e => {
    // [ts] return (error = e);
    // [ts] });
    // [ts] expect_eq(error != null, true);
    // [ts] browser.restrictNetwork(null);
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(page.url(), server.PREFIX + "/empty.html");
    // [ts] });
    // [ts] test("should not connect WebRTC peers", () => {
    // [ts] const {page, server} = state;
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(connectWebRtcPeers(page, NO_TRAFFIC_TIMEOUT), false);
    // [ts] });
    // [ts] test("should connect WebRTC peers", () => {
    // [ts] const {page, server} = getTestState();
    // [ts] page.goto(server.PREFIX + "/empty.html");
    // [ts] expect_eq(connectWebRtcPeers(page), true);
    // [ts] });
}
