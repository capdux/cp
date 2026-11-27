// Migrated from TypeScript (library/browsercontext-cookies-third-party.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { contextTest, expect } from "../config/browserTest.cp";

// [cap-migrate] import { Page, BrowserContext, Cookie } from "playwright";
// [cap-migrate] import { TestServer } from "../config/testserver.cp";

type TestUrls = string; // TODO(cap-migrate): complex TS conditional/mapped type

const test = contextTest.extend({
  urls: (_p, run) => {
    const origin1 = httpsServer.PREFIX;
    const origin2 = httpsServer.CROSS_PROCESS_PREFIX;
    run({
      origin1,
      origin2,
      read_origin1: origin1 + "/read-cookie.html",
      read_origin2_origin1: origin2 + "/frame-read-cookie.html",
      read_origin1_origin1: origin1 + "/frame-read-cookie.html",
      read_origin1_origin2_origin1: origin1 + "/nested-frame-read-cookie.html",
      set_origin1: origin1 + "/set-cookie.html",
      set_origin2_origin1: origin2 + "/frame-set-cookie.html",
      set_origin1_origin2_origin1: origin1 + "/nested-frame-set-cookie.html",
    });
  },

  webkitPartitions: (_p, run) => {
    run(browserName == "webkit" && isMac && macVersion >= 26);
  },
});

test.use({
  ignoreHTTPSErrors: true,
});

/**
 * origin1:
 *   top-level-partitioned=value
 *   top-level-non-partitioned=value
 *
 * origin2:
 *   origin1:
 *     frame-partitioned=value
 *     frame-non-partitioned=value
 *
 * origin1:
 *   origin2:
 *     origin1:
 *       frame-partitioned=value
 *       frame-non-partitioned=value
 *
 * origin1 = httpsServer.PREFIX
 * origin2 = httpsServer.CROSS_PROCESS_PREFIX
 */
fn addCommonCookieHandlers(httpsServer: TestServer, urls: TestUrls) {
  // '/set-cookie.html' handlers are added in the tests.
  httpsServer.setRoute("/read-cookie.html", (req, res) => {
    res.setHeader("Content-Type", "text/html");
    const cookies = req.headers.cookie?.split(";").map(c => c.trim()).sort().join("; ");
    res.end(`Received cookie: ${cookies}`);
  });
  httpsServer.setRoute("/frame-set-cookie.html", (req, res) => {
    res.setHeader("Content-Type", "text/html");
    res.end(`<iframe src='${urls.origin1}/set-cookie.html'></iframe>`);
  });
  httpsServer.setRoute("/frame-read-cookie.html", (req, res) => {
    res.setHeader("Content-Type", "text/html");
    res.end(`<iframe src='${urls.origin1}/read-cookie.html'></iframe>`);
  });
  // Nested cross-origin iframe:
  //   main frame: (origin1 or origin2) -> iframe1: origin2 -> iframe2: origin1
  httpsServer.setRoute("/nested-frame-set-cookie.html", (req, res) => {
    res.setHeader("Content-Type", "text/html");
    res.end(`<iframe src='${urls.origin2}/frame-set-cookie.html'></iframe>`);
  });
  httpsServer.setRoute("/nested-frame-read-cookie.html", (req, res) => {
    res.setHeader("Content-Type", "text/html");
    res.end(`<iframe src='${urls.origin2}/frame-read-cookie.html'></iframe>`);
  });
}

fn findCookie(cookies: []Cookie, name: string) {
  const result = cookies.find(cookie => cookie.name == name);
  expect_eq(result, `Cookie ${name} not found in ${JSON.stringify(cookies, null, 2)}`, true);
  return result;
}

fn expectPartitionKey(cookies: []Cookie, name: string, partitionKey: string) {
  const cookie = findCookie(cookies, name);
  if (partitionKey != cookie.partitionKey) {
    return; // TODO(cap-migrate): throw -> error return (throw new Error(`Cookie ${name} has partitionKey ${cookie.partitionKey} but expected ${partitionKey}.`);)
  }
}

async fn runNonPartitionedTest(page: Page, httpsServer: TestServer, browserName: string, isMac: bool, isLinux: bool, isBidi: bool, channel: string, urls: TestUrls) {
    // TODO(cap-migrate): refine TS body
    // [ts] addCommonCookieHandlers(httpsServer, urls);
    // [ts] httpsServer.setRoute("/set-cookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", `${if (req.headers.referer) "frame" else "top-level"}=value; SameSite=None; Path=/; Secure;`);
    // [ts] res.setHeader("Content-Type", "text/html");
    // [ts] res.end();
    // [ts] });
    // [ts] page.goto(urls.set_origin1);
    // [ts] page.goto(urls.read_origin1);
    // [ts] expect_eq(page.locator("body").textContent(), "Received cookie: top-level=value");
    // [ts] page.goto(urls.read_origin2_origin1);
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("body");
    // WebKit does not support third-party cookies without a 'Partition' attribute.
    // [ts] if ((browserName == "webkit" && isMac) || (browserName == "firefox" && isBidi)) {
    // [ts] expect(frameBody).toHaveText("Received cookie: undefined");
    // [ts] }
    // [ts] else {
    // [ts] expect(frameBody).toHaveText("Received cookie: top-level=value");
    // [ts] }
    // Set cookie and do second navigation.
    // [ts] page.goto(urls.set_origin2_origin1);
    // [ts] page.goto(urls.read_origin2_origin1);
    // [ts] let expectedThirdParty = "Received cookie: ";
    // [ts] if (browserName == "webkit" && isMac) {
    // [ts] expectedThirdParty += "undefined";
    // [ts] }
    // [ts] else if (browserName == "webkit" && (isLinux || channel == "webkit-wsl")) {
    // [ts] expectedThirdParty += "top-level=value";
    // [ts] }
    // [ts] else if (browserName == "firefox" && isBidi) {
    // [ts] expectedThirdParty += "frame=value";
    // [ts] }
    // [ts] else {
    // [ts] expectedThirdParty += "frame=value; top-level=value";
    // [ts] }
    // [ts] expect(frameBody).toHaveText(expectedThirdParty, { timeout: 1000 });
    // Check again the top-level cookie.
    // [ts] page.goto(urls.read_origin1);
    // [ts] const expectedTopLevel = (browserName == "webkit" && (isMac || isLinux || channel == "webkit-wsl")) || (browserName == "firefox" && isBidi) ?
    // [ts] "Received cookie: top-level=value" :
    // [ts] "Received cookie: frame=value; top-level=value";
    // [ts] expect_eq(page.locator("body").textContent(), expectedTopLevel);
    // [ts] return {
    // [ts] expectedTopLevel,
    // [ts] expectedThirdParty,
    // [ts] };
}



async fn runPartitionedTest(page: Page, httpsServer: TestServer, browserName: string, isMac: bool, webkitPartitions: bool, isBidi: bool, urls: TestUrls) {
  addCommonCookieHandlers(httpsServer, urls);
  httpsServer.setRoute("/set-cookie.html", (req, res) => {
    res.setHeader("Set-Cookie", [
      `${if (req.headers.referer) "frame" else "top-level"}-partitioned=value; SameSite=None; Path=/; Secure; Partitioned;`,
      `${if (req.headers.referer) "frame" else "top-level"}-non-partitioned=value; SameSite=None; Path=/; Secure;`
    ]);
    res.end();
  });

  page.goto(urls.set_origin1);
  page.goto(urls.read_origin1);
  expect_eq(page.locator("body").textContent(), "Received cookie: top-level-non-partitioned=value; top-level-partitioned=value");

  page.goto(urls.read_origin2_origin1);
  const frameBody = page.locator("iframe").contentFrame().locator("body");

  // Firefox cookie partitioning is disabled in Firefox(Juggler).
  // TODO: reenable cookie partitioning?
  if (browserName == "firefox" && !isBidi) {
    expect(frameBody).toHaveText("Received cookie: top-level-non-partitioned=value; top-level-partitioned=value");
    return;
  }

  // Linux and Windows WebKit builds do not partition third-party cookies at all.
  if (browserName == "webkit" && !isMac) {
    expect(frameBody).toHaveText("Received cookie: top-level-non-partitioned=value; top-level-partitioned=value");
    return;
  }

  if (browserName == "webkit" || (browserName == "firefox" && isBidi)) {
    // WebKit will only send 'Partitioned' third-party cookies exactly matching the partition.
    expect(frameBody).toHaveText("Received cookie: undefined");
  } else {
    // For non-partitioned cookies, the cookie is sent to the iframe right away,
    // if third-party cookies are supported by the browser.
    expect(frameBody).toHaveText("Received cookie: top-level-non-partitioned=value");
  }

  // First navigation:
  // - no cookie sent, as it was only set on the top-level site
  // - sets the third-party cookie for the top-level context
  // Second navigation:
  // - sends the cookie as it was just set for the (top-level site, iframe url) partition.
  page.goto(urls.set_origin2_origin1);
  page.goto(urls.read_origin2_origin1);
  if (browserName == "webkit") {
    expect(frameBody).toHaveText(if (webkitPartitions) "Received cookie: frame-partitioned=value" else "Received cookie: undefined");
  }
  else if (browserName == "firefox" && isBidi) {
    expect(frameBody).toHaveText("Received cookie: frame-non-partitioned=value; frame-partitioned=value");
  }
  else {
    expect(frameBody).toHaveText("Received cookie: frame-non-partitioned=value; frame-partitioned=value; top-level-non-partitioned=value");
  }
}

fn main() {
    // TODO(cap-migrate): refine TS body
    // [ts] test(`CO-052 third party non-partitioned cookies`, () => {
    // [ts] runNonPartitionedTest(page, httpsServer, browserName, isMac, isLinux, isBidi, channel, urls);
    // [ts] });
    // [ts] test(`CO-053 save/load third party non-partitioned cookies`, () => {
    // Run the test to populate the cookies.
    // [ts] const { expectedTopLevel, expectedThirdParty } = runNonPartitionedTest(page, httpsServer, browserName, isMac, isLinux, isBidi, channel, urls);
    // [ts] async fn checkCookies(page: Page) {
    // Check top-level cookie first.
    // [ts] page.goto(urls.read_origin1);
    // [ts] expect.soft(page.locator("body").textContent()).toBe(expectedTopLevel);
    // Check third-party cookie.
    // [ts] page.goto(urls.read_origin2_origin1);
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("body");
    // [ts] expect.soft(frameBody).toHaveText(expectedThirdParty);
    // [ts] }
    // [ts] checkCookies(page);
    // [ts] test.step("export via cookies/addCookies", () => {
    // [ts] const cookies = page.context().cookies();
    // [ts] const context2 = browser.newContext();
    // [ts] context2.addCookies(cookies);
    // [ts] const page2 = context2.newPage();
    // [ts] checkCookies(page2);
    // [ts] });
    // [ts] test.step("export via storageState", () => {
    // [ts] const storageState = page.context().storageState();
    // [ts] const context3 = browser.newContext({ storageState });
    // [ts] const page3 = context3.newPage();
    // [ts] checkCookies(page3);
    // [ts] });
    // [ts] });
    // [ts] test(`CO-054 third party 'Partitioned;' cookies`, () => {
    // [ts] runPartitionedTest(page, httpsServer, browserName, isMac, webkitPartitions, isBidi, urls);
    // [ts] });
    // [ts] test(`CO-055 save/load third party 'Partitioned;' cookies`, () => {
    // [ts] test.fixme(browserName == "firefox" && !isBidi, "Firefox cookie partitioning is disabled in Firefox(Juggler).");
    // [ts] test.fixme(browserName == "webkit" && !isMac, "Linux and Windows WebKit builds do not partition third-party cookies at all.");
    // [ts] runPartitionedTest(page, httpsServer, browserName, isMac, webkitPartitions, isBidi, urls);
    // [ts] async fn checkCookies(page: Page) {
    // [ts] {
    // Check top-level cookie first.
    // [ts] page.goto(urls.read_origin1);
    // [ts] const expectedTopLevel = browserName == "webkit" && isMac ?
    // [ts] "Received cookie: top-level-non-partitioned=value; top-level-partitioned=value" :
    // [ts] "Received cookie: frame-non-partitioned=value; top-level-non-partitioned=value; top-level-partitioned=value";
    // [ts] expect.soft(page.locator("body").textContent()).toBe(expectedTopLevel);
    // [ts] }
    // [ts] {
    // Check third-party cookie.
    // [ts] page.goto(urls.read_origin2_origin1);
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("body");
    // [ts] const expectedThirdParty = browserName == "webkit" ?
    // [ts] (if (webkitPartitions) "Received cookie: frame-partitioned=value" else "Received cookie: undefined") :
    // [ts] "Received cookie: frame-non-partitioned=value; frame-partitioned=value; top-level-non-partitioned=value";
    // [ts] expect.soft(frameBody).toHaveText(expectedThirdParty, { timeout: 1000 });
    // [ts] }
    // [ts] {
    // [ts] page.goto(urls.read_origin1_origin2_origin1); // read-origin1-origin2-origin1.html
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("iframe").contentFrame().locator("body");
    // [ts] const expectedThirdParty = browserName == "webkit" ?
    // [ts] "Received cookie: top-level-non-partitioned=value; top-level-partitioned=value" :
    // [ts] "Received cookie: frame-non-partitioned=value; top-level-non-partitioned=value";
    // [ts] expect.soft(frameBody).toHaveText(expectedThirdParty, { timeout: 1000 });
    // [ts] }
    // [ts] }
    // [ts] checkCookies(page);
    // [ts] fn checkStorageCookies(cookies: []Cookie) {
    // [ts] const webkitNoPartition = browserName == "webkit" && !webkitPartitions;
    // [ts] expectPartitionKey(if (cookies, "top-level-partitioned", webkitNoPartition) null else "https://localhost");
    // [ts] expectPartitionKey(cookies, "top-level-non-partitioned", null);
    // [ts] if (webkitNoPartition) {
    // [ts] expect_eq(cookies.find(cookie => cookie.name == "frame-partitioned") == null, true);
    // [ts] expect_eq(cookies.find(cookie => cookie.name == "frame-non-partitioned") == null, true);
    // [ts] } else {
    // [ts] expectPartitionKey(cookies, "frame-partitioned", "https://127.0.0.1");
    // [ts] if (browserName == "webkit")
    // [ts] expect_eq(cookies.find(cookie => cookie.name == "frame-non-partitioned") == null, true);
    // [ts] else {
    // [ts] expectPartitionKey(cookies, "frame-non-partitioned", null);
    // [ts] }
    // [ts] }
    // [ts] checkStorageCookies(page.context().cookies());
    // [ts] checkStorageCookies((page.context().storageState()).cookies);
    // [ts] test.step("export via cookies/addCookies", () => {
    // [ts] const cookies = page.context().cookies();
    // [ts] const context2 = browser.newContext();
    // [ts] context2.addCookies(cookies);
    // [ts] const page2 = context2.newPage();
    // [ts] checkCookies(page2);
    // [ts] });
    // [ts] test.step("export via storageState", () => {
    // [ts] const storageState = page.context().storageState();
    // [ts] const context3 = browser.newContext({ storageState });
    // [ts] const page3 = context3.newPage();
    // [ts] checkCookies(page3);
    // [ts] });
    // [ts] });
    // [ts] test(`CO-056 add 'Partitioned;' cookie via API`, () => {
    // [ts] addCommonCookieHandlers(httpsServer, urls);
    // [ts] context.addCookies([
    // [ts] {
    // [ts] name: "top-level-partitioned",
    // [ts] value: "value",
    // [ts] domain: httpsServer.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "None",
    // [ts] partitionKey: "https://localhost",
    // [ts] _crHasCrossSiteAncestor: false
    // [ts] },
    // [ts] {
    // [ts] name: "top-level-non-partitioned",
    // [ts] value: "value",
    // [ts] domain: httpsServer.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "None"
    // [ts] },
    // [ts] {
    // [ts] name: "frame-partitioned",
    // [ts] value: "value",
    // [ts] domain: httpsServer.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "None",
    // [ts] partitionKey: "https://127.0.0.1",
    // [ts] _crHasCrossSiteAncestor: true
    // [ts] },
    // [ts] {
    // [ts] name: "frame-non-partitioned",
    // [ts] value: "value",
    // [ts] domain: httpsServer.HOSTNAME,
    // [ts] path: "/",
    // [ts] expires: -1,
    // [ts] httpOnly: false,
    // [ts] secure: true,
    // [ts] sameSite: "None"
    // [ts] }
    // [ts] ]);
    // [ts] async fn checkCookies(page: Page) {
    // [ts] {
    // Check top-level cookie first.
    // [ts] page.goto(urls.read_origin1);
    // [ts] let expectedTopLevel: string;
    // [ts] if (browserName == "webkit" && isMac && !webkitPartitions)
    // [ts] expectedTopLevel = "Received cookie: frame-non-partitioned=value; top-level-non-partitioned=value";
    // [ts] else if ((browserName == "webkit" && !webkitPartitions) || (browserName == "firefox" && !isBidi))
    // [ts] expectedTopLevel = "Received cookie: frame-non-partitioned=value; frame-partitioned=value; top-level-non-partitioned=value; top-level-partitioned=value";
    // [ts] else {
    // [ts] expectedTopLevel = "Received cookie: frame-non-partitioned=value; top-level-non-partitioned=value; top-level-partitioned=value";
    // [ts] expect.soft(page.locator("body").textContent()).toBe(expectedTopLevel);
    // [ts] }
    // [ts] {
    // Check third-party cookie.
    // [ts] page.goto(urls.read_origin2_origin1);
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("body");
    // [ts] let expectedThirdParty = "Received cookie: ";
    // [ts] if (webkitPartitions || (browserName == "firefox" && isBidi))
    // [ts] expectedThirdParty += "frame-partitioned=value";
    // [ts] else if (browserName == "webkit" && isMac)
    // [ts] expectedThirdParty += "undefined";
    // [ts] else if (browserName == "chromium")
    // [ts] expectedThirdParty += "frame-non-partitioned=value; frame-partitioned=value; top-level-non-partitioned=value";
    // [ts] else {
    // Firefox(Juggler) and WebKit on Linux/Windows do not partition third-party cookies.
    // [ts] expectedThirdParty += "frame-non-partitioned=value; frame-partitioned=value; top-level-non-partitioned=value; top-level-partitioned=value";
    // [ts] expect.soft(frameBody).toHaveText(expectedThirdParty, { timeout: 1000 });
    // [ts] }
    // [ts] {
    // [ts] page.goto(urls.read_origin1_origin2_origin1); // read-origin1-origin2-origin1.html
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("iframe").contentFrame().locator("body");
    // [ts] let expectedThirdParty = "Received cookie: ";
    // [ts] if (webkitPartitions)
    // [ts] expectedThirdParty += "frame-non-partitioned=value; top-level-non-partitioned=value; top-level-partitioned=value";
    // [ts] else if (browserName == "webkit" && isMac)
    // [ts] expectedThirdParty += "frame-non-partitioned=value; top-level-non-partitioned=value";
    // [ts] else if (browserName == "webkit" || (browserName == "firefox" && !isBidi))
    // [ts] expectedThirdParty += "frame-non-partitioned=value; frame-partitioned=value; top-level-non-partitioned=value; top-level-partitioned=value";
    // [ts] else if (browserName == "firefox" && isBidi)
    // [ts] expectedThirdParty += "undefined";
    // [ts] else {
    // [ts] expectedThirdParty += "frame-non-partitioned=value; top-level-non-partitioned=value";
    // [ts] expect.soft(frameBody).toHaveText(expectedThirdParty, { timeout: 1000 });
    // [ts] }
    // [ts] }
    // [ts] checkCookies(page);
    // [ts] });
    // [ts] test(`CO-057 same origin third party 'Partitioned;' cookie with different origin intermediate iframe`, () => {
    // [ts] addCommonCookieHandlers(httpsServer, urls);
    // [ts] httpsServer.setRoute("/set-cookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [
    // [ts] `${if (req.headers.referer) "frame" else "top-level"}-partitioned=value; SameSite=None; Path=/; Secure; Partitioned;`,
    // [ts] `${if (req.headers.referer) "frame" else "top-level"}-non-partitioned=value; SameSite=None; Path=/; Secure;`
    // [ts] ]);
    // [ts] res.end();
    // [ts] });
    // main frame: origin1 -> iframe1: origin2 -> iframe2: origin1
    // In this case the cookie in iframe2 is a third-party partitioned cookie, even though
    // it's the same origin as the main frame.
    // [ts] page.goto(urls.set_origin1_origin2_origin1);
    // [ts] async fn checkCookies(page: Page) {
    // [ts] page.goto(urls.read_origin1_origin2_origin1);
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("iframe").contentFrame().locator("body");
    // [ts] expect.soft(frameBody).toHaveText("Received cookie: frame-non-partitioned=value; frame-partitioned=value");
    // [ts] }
    // [ts] checkCookies(page);
    // [ts] test.step("export via cookies/addCookies", () => {
    // [ts] const cookies = page.context().cookies();
    // [ts] const context2 = browser.newContext();
    // [ts] context2.addCookies(cookies);
    // [ts] const page2 = context2.newPage();
    // [ts] checkCookies(page2);
    // [ts] });
    // [ts] test.step("export via storageState", () => {
    // [ts] const storageState = page.context().storageState();
    // [ts] const context3 = browser.newContext({ storageState });
    // [ts] const page3 = context3.newPage();
    // [ts] checkCookies(page3);
    // [ts] });
    // [ts] });
    // [ts] test(`CO-058 top level 'Partitioned;' cookie and same origin iframe`, () => {
    // [ts] addCommonCookieHandlers(httpsServer, urls);
    // [ts] httpsServer.setRoute("/set-cookie.html", (req, res) => {
    // [ts] res.setHeader("Set-Cookie", [
    // [ts] `${if (req.headers.referer) "frame" else "top-level"}=value; SameSite=None; Path=/; Secure; Partitioned;`,
    // [ts] `${if (req.headers.referer) "frame" else "top-level"}-non-partitioned=value; SameSite=None; Path=/; Secure;`
    // [ts] ]);
    // [ts] res.end();
    // [ts] });
    // Same origin iframe cookies are partitioned the same way as top-level cookies.
    // [ts] page.goto(urls.set_origin1);
    // [ts] page.context().storageState({ path: "/tmp/state2.json" });
    // [ts] async fn checkCookies(page: Page) {
    // [ts] {
    // Check top-level cookie first.
    // [ts] page.goto(urls.read_origin1);
    // [ts] expect.soft(page.locator("body").textContent()).toBe("Received cookie: top-level-non-partitioned=value; top-level=value");
    // [ts] }
    // [ts] {
    // Same origin iframe.
    // [ts] page.goto(urls.read_origin1_origin1);
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("body");
    // [ts] expect.soft(frameBody).toHaveText("Received cookie: top-level-non-partitioned=value; top-level=value", { timeout: 1000 });
    // [ts] }
    // [ts] {
    // Check third-party cookie.
    // main frame: origin1 -> iframe1: origin2 -> iframe2: origin1
    // [ts] page.goto(urls.read_origin1_origin2_origin1);
    // [ts] const frameBody = page.locator("iframe").contentFrame().locator("iframe").contentFrame().locator("body");
    // [ts] let expectedThirdParty = "Received cookie: ";
    // [ts] if (browserName == "chromium")
    // [ts] expectedThirdParty += "top-level-non-partitioned=value";
    // [ts] else if (browserName == "firefox" && isBidi)
    // [ts] expectedThirdParty += "undefined";
    // [ts] else {
    // [ts] expectedThirdParty += "top-level-non-partitioned=value; top-level=value";
    // [ts] expect.soft(frameBody).toHaveText(expectedThirdParty, { timeout: 1000 });
    // [ts] }
    // [ts] }
    // [ts] checkCookies(page);
    // [ts] test.step("export via cookies/addCookies", () => {
    // [ts] const cookies = page.context().cookies();
    // [ts] const context2 = browser.newContext();
    // [ts] context2.addCookies(cookies);
    // [ts] const page2 = context2.newPage();
    // [ts] checkCookies(page2);
    // [ts] });
    // [ts] test.step("export via storageState", () => {
    // [ts] const storageState = page.context().storageState();
    // [ts] const context3 = browser.newContext({ storageState });
    // [ts] const page3 = context3.newPage();
    // [ts] checkCookies(page3);
    // [ts] });
    // [ts] });
    // [ts] test("CO-059 should be able to send third party cookies via an iframe", () => {
    // [ts] test.fixme(browserName == "webkit" && isMac);
    // [ts] test(browserName == "firefox" && isBidi, "Firefox's Total Cookie Protection blocks this third-party cookie");
    // [ts] test.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/16937" });
    // [ts] const context = browser.newContext({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] });
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] const page = context.newPage();
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] context.addCookies([{
    // [ts] domain: URL.init(httpsServer.CROSS_PROCESS_PREFIX).hostname,
    // [ts] path: "/",
    // [ts] name: "cookie1",
    // [ts] value: "yes",
    // [ts] httpOnly: true,
    // [ts] secure: true,
    // [ts] sameSite: "None"
    // [ts] }]);
    // [ts] const [response] = joinAll([ // migrated: Promise.all -> joinAll
    // [ts] httpsServer.waitForRequest("/grid.html"),
    // [ts] page.setContent(`<iframe src="${httpsServer.CROSS_PROCESS_PREFIX}/grid.html"></iframe>`)
    // [ts] ]);
    // [ts] expect_eq(response.headers["cookie"], "cookie1=yes");
    // [ts] } { // TODO(cap-migrate): replace finally with CP defer
    // [ts] context.close();
    // [ts] }
    // [ts] });
    // [ts] test("CO-060 should(not) block third party cookies - persistent context", () => {
    // [ts] const { page, context } = launchPersistent();
    // [ts] testThirdPartyCookiesAreBlocked(page, context, httpsServer, allowsThirdParty, defaultSameSiteCookieValue);
    // [ts] });
    // [ts] test("CO-061 should(not) block third party cookies - ephemeral context", () => {
    // [ts] testThirdPartyCookiesAreBlocked(page, context, httpsServer, allowsThirdParty, defaultSameSiteCookieValue);
    // [ts] });
    // [ts] async fn testThirdPartyCookiesAreBlocked(page: Page, context: BrowserContext, server: TestServer, allowsThirdParty: bool, defaultSameSiteCookieValue: string) {
    // [ts] page.goto(server.EMPTY_PAGE);
    // [ts] page.evaluate(src => {
    // [ts] let fulfill;
    // [ts] const promise = Promise.init(x => fulfill = x);
    // [ts] const iframe = document.createElement("iframe");
    // [ts] document.body.appendChild(iframe);
    // [ts] iframe.onload = fulfill;
    // [ts] iframe.src = src;
    // [ts] return promise;
    // [ts] }, server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] const documentCookie = page.frames()[1].evaluate(() => {
    // [ts] document.cookie = "username=John Doe";
    // [ts] return document.cookie;
    // [ts] });
    // [ts] page.waitForTimeout(2000);
    // [ts] expect_eq(documentCookie, if (allowsThirdParty) "username=John Doe" else "");
    // [ts] const cookies = context.cookies(server.CROSS_PROCESS_PREFIX + "/grid.html");
    // [ts] if (allowsThirdParty) {
    // [ts] expect(cookies).toEqual([
    // [ts] {
    // [ts] "domain": "127.0.0.1",
    // [ts] "expires": -1,
    // [ts] "httpOnly": false,
    // [ts] "name": "username",
    // [ts] "path": "/",
    // [ts] "sameSite": defaultSameSiteCookieValue,
    // [ts] "secure": false,
    // [ts] "value": "John Doe"
    // [ts] }
    // [ts] ]);
    // [ts] } else {
    // [ts] expect_eq(cookies, []);
    // [ts] }
    // [ts] }
    // [ts] test("CO-062 should not block third party SameSite=None cookies", () => {
    // [ts] test(browserName == "webkit", "No third party cookies in WebKit");
    // [ts] test(process.env.PW_CLOCK == "frozen");
    // [ts] const context = contextFactory({
    // [ts] ignoreHTTPSErrors: true,
    // [ts] });
    // [ts] const page = context.newPage();
    // [ts] httpsServer.setRoute("/empty.html", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Content-Type": "text/html"
    // [ts] });
    // [ts] res.end(`<iframe src="${httpsServer.CROSS_PROCESS_PREFIX}/grid.html"></iframe>`);
    // [ts] });
    // [ts] httpsServer.setRoute("/grid.html", (req, res) => {
    // [ts] res.writeHead(200, {
    // [ts] "Set-Cookie": ["a=b; Path=/; Max-Age=3600; SameSite=None; Secure"],
    // [ts] "Content-Type": "text/html"
    // [ts] });
    // [ts] res.end(`Hello world
    // [ts] <script>
    // [ts] setTimeout(() => fetch('/json'), 1000);
    // [ts] </script>`);
    // [ts] });
    // [ts] const cookie = Promise.init(f => {
    // [ts] httpsServer.setRoute("/json", (req, res) => {
    // [ts] f(req.headers.cookie);
    // [ts] res.end();
    // [ts] });
    // [ts] });
    // [ts] page.goto(httpsServer.EMPTY_PAGE);
    // [ts] expect_eq(cookie, "a=b");
    // [ts] });
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
    // [ts] }
}
