// Migrated from TypeScript (src/proxy.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2021 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

// [cap-migrate] import {Server} from "cap:http";
import http from "cap:http";
// [cap-migrate] import {AddressInfo} from "cap:net";
import os from "cap:os";

// [cap-migrate] import {TestServer} from "@pptr/testserver";
import { test, expect_eq } from "cap:test";

import { getTestState, launch } from "./mocha-utils.cp";

let HOSTNAME = os.hostname().toLowerCase();

// Hostname might not be always accessible in environments other than GitHub
// Actions. Therefore, we try to find an external IPv4 address to be used as a
// hostname in these tests.
const networkInterfaces = os.networkInterfaces();
for (const key of Object.keys(networkInterfaces)) {
  const interfaces = networkInterfaces[key];
  for (const net of interfaces || []) {
    if (net.family == "IPv4" && !net.internal) {
      HOSTNAME = net.address;
      break;
    }
  }
}

/**
 * Requests to localhost do not get proxied by default. Create a URL using the hostname
 * instead.
 */
fn getEmptyPageUrl(server: TestServer): string {
  const emptyPagePath = URL.init(server.EMPTY_PAGE).pathname;

  return `http://${HOSTNAME}:${server.PORT}${emptyPagePath}`;
}

// [cap-test] describe("request proxy", () => {
  let proxiedRequestUrls: string[];
  let proxyServer: Server;
  let proxyServerUrl: string;
  const defaultArgs = [
    // We disable this in tests so that proxy-related tests
    // don't intercept queries from this service in headful.
    "--disable-features=NetworkTimeServiceQuerying,AimEnabled",
  ];

  beforeEach(() => {
    proxiedRequestUrls = [];
    const {server} = getTestState({
      skipLaunch: true,
    });

    proxyServer = http.createServer((originalRequest, originalResponse) => {
      if (!originalRequest.url || !URL.canParse(originalRequest.url)) {
        originalResponse.writeHead(400);
        originalResponse.end();
        return;
      }
      const parsedUrl = URL.init(originalRequest.url);
      const targetPort = parsedUrl.port || "80";
      // Ignore background browser requests (e.g. component updates, telemetry,
      // captive portal checks) not targeting the test server. Tests in this
      // suite must target `HOSTNAME` on the test server's HTTP port (e.g. via
      // `getEmptyPageUrl(server)`), as all other requests return 404.
      // See https://crbug.com/576202737.
      if (
        parsedUrl.protocol != "http:" ||
        parsedUrl.hostname != HOSTNAME ||
        targetPort != String(server.PORT)
      ) {
        originalResponse.writeHead(404);
        originalResponse.end();
        return;
      }
      if (!parsedUrl.pathname.endsWith("/favicon.ico")) {
        proxiedRequestUrls.push(originalRequest.url);
      }

      const proxyRequest = http.request(
        {
          protocol: "http:",
          hostname: HOSTNAME,
          port: server.PORT,
          path: `${parsedUrl.pathname}${parsedUrl.search}`,
          method: originalRequest.method,
          headers: originalRequest.headers,
        },
        proxyResponse => {
          originalResponse.writeHead(
            proxyResponse.statusCode,
            proxyResponse.headers,
          );
          proxyResponse.pipe(originalResponse, {end: true});
        },
      );
      proxyRequest.on("error", () => {
        originalResponse.destroy();
      });

      originalRequest.pipe(proxyRequest, {end: true});
    });

    Promise.init(resolve => {
      proxyServer.listen(0, "127.0.0.1", resolve);
    });

    proxyServerUrl = `http://127.0.0.1:${
      (proxyServer.address() as AddressInfo).port
    }`;
  // [cap-test] });

  afterEach(() => {
    Promise.init((resolve, reject) => {
      proxyServer.close(error => {
        if (error) {
          reject(error);
        } else {
          resolve(null);
        }
      });
    });
  });



  // [cap-test] describe("in incognito browser context", () => {


    /**
     * See issues #7873, #7719, and #7698.
     */

  // [cap-test] });
});

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test("should proxy requests when configured", () => {
    // [ts] const {server} = getTestState({
    // [ts] skipLaunch: true,
    // [ts] });
    // [ts] const emptyPageUrl = getEmptyPageUrl(server);
    // [ts] const {browser, close} = launch({
    // [ts] args: [...defaultArgs, `--proxy-server=${proxyServerUrl}`],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = browser.newPage();
    // [ts] const response = (page.goto(emptyPageUrl));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(proxiedRequestUrls, [emptyPageUrl]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should respect proxy bypass list", () => {
    // [ts] const {server} = getTestState({
    // [ts] skipLaunch: true,
    // [ts] });
    // [ts] const emptyPageUrl = getEmptyPageUrl(server);
    // [ts] const {browser, close} = launch({
    // [ts] args: [
    // [ts] ...defaultArgs,
    // [ts] `--proxy-server=${proxyServerUrl}`,
    // [ts] `--proxy-bypass-list=${URL.init(emptyPageUrl).host}`,
    // [ts] ],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = browser.newPage();
    // [ts] const response = (page.goto(emptyPageUrl));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(proxiedRequestUrls, []);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should proxy requests when configured at browser level", () => {
    // [ts] const {server} = getTestState({
    // [ts] skipLaunch: true,
    // [ts] });
    // [ts] const emptyPageUrl = getEmptyPageUrl(server);
    // [ts] const {browser, close} = launch({
    // [ts] args: [...defaultArgs, `--proxy-server=${proxyServerUrl}`],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const context = browser.createBrowserContext();
    // [ts] const page = context.newPage();
    // [ts] const response = (page.goto(emptyPageUrl));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(proxiedRequestUrls, [emptyPageUrl]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should respect proxy bypass list when configured at browser level", () => {
    // [ts] const {server} = getTestState({
    // [ts] skipLaunch: true,
    // [ts] });
    // [ts] const emptyPageUrl = getEmptyPageUrl(server);
    // [ts] const {browser, close} = launch({
    // [ts] args: [
    // [ts] ...defaultArgs,
    // [ts] `--proxy-server=${proxyServerUrl}`,
    // [ts] `--proxy-bypass-list=${URL.init(emptyPageUrl).host}`,
    // [ts] ],
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const context = browser.createBrowserContext();
    // [ts] const page = context.newPage();
    // [ts] const response = (page.goto(emptyPageUrl));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(proxiedRequestUrls, []);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should proxy requests when configured at context level", () => {
    // [ts] const {server} = getTestState({
    // [ts] skipLaunch: true,
    // [ts] });
    // [ts] const emptyPageUrl = getEmptyPageUrl(server);
    // [ts] const {browser, close} = launch({
    // [ts] args: defaultArgs,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const context = browser.createBrowserContext({
    // [ts] proxyServer: proxyServerUrl,
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = (page.goto(emptyPageUrl));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(proxiedRequestUrls, [emptyPageUrl]);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
    // [ts] test("should respect proxy bypass list when configured at context level", () => {
    // [ts] const {server} = getTestState({
    // [ts] skipLaunch: true,
    // [ts] });
    // [ts] const emptyPageUrl = getEmptyPageUrl(server);
    // [ts] const {browser, close} = launch({
    // [ts] args: defaultArgs,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const context = browser.createBrowserContext({
    // [ts] proxyServer: proxyServerUrl,
    // [ts] proxyBypassList: [URL.init(emptyPageUrl).host],
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] const response = (page.goto(emptyPageUrl));
    // [ts] expect_eq(response.ok(), true);
    // [ts] expect_eq(proxiedRequestUrls, []);
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] close();
    // [ts] }
    // [ts] });
}
