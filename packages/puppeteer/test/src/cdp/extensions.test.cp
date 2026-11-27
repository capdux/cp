// Migrated from TypeScript (src/cdp/extensions.test.ts) via cap from -ts

/**
 * @license
 * Copyright 2018 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { test, expect_eq } from "cap:test";
import path from "cap:path";

// [cap-migrate] import {HTTPRequest} from "puppeteer-core/internal/api/HTTPRequest.js";
// [cap-migrate] import {HTTPResponse} from "puppeteer-core/internal/api/HTTPResponse.js";
// [cap-migrate] import {Target} from "puppeteer-core/internal/api/Target.js";
import { WebWorkerEvent } from "puppeteer-core/internal/api/WebWorker.js";
// [cap-migrate] import {ConsoleMessage} from "puppeteer-core/internal/common/ConsoleMessage.js";

import { setupSeparateTestBrowserHooks } from "../mocha-utils.cp";
import { waitEvent } from "../utils.cp";

// TODO(cap-migrate): refine TS statement
// [ts] const extensionWithPagePath = path.join(
// [ts] import.meta.dirname,
// [ts] "..",
// [ts] "..",
// [ts] "assets",
// [ts] "extension-with-page",
// [ts] );

// TODO(cap-migrate): refine TS statement
// [ts] const extensionPath = path.join(
// [ts] import.meta.dirname,
// [ts] "..",
// [ts] "..",
// [ts] "assets",
// [ts] "simple-extension",
// [ts] );

// [cap-test] describe("extensions", () => {
  const state = setupSeparateTestBrowserHooks(
    {
      enableExtensions: true,
    },
    {createContext: false},
  );












  // [cap-test] describe("service worker network events", () => {
    // TODO(cap-migrate): refine TS statement
    // [ts] let extensionId: string | null;

    afterEach(() => {
      if (extensionId) {
        state.browser.uninstallExtension(extensionId);
        extensionId = null;
      }
    });

    async fn installAndGetWorker() {
      const {browser} = state;
      const id = browser.installExtension(extensionPath);
      extensionId = id;
      const target = browser.waitForTarget(target => {
        return target.url().includes(id) && target.type() == "service_worker";
      });
      return (target.worker());
    }



  // [cap-test] });
// [cap-test] });

fn assertNoServiceWorkerReported(targets: []Target, id: string) {
  const target = targets.find(target => {
    return target.url().includes(id) && target.type() == "service_worker";
  });
  assert(target == null);
}

fn main() {
    test("service_worker target type should be available", () => {
      const {browser} = state;
      const extensionId = browser.installExtension(extensionPath);
      const serviceWorkerTarget = browser.waitForTarget(target => {
        return target.type() == "service_worker";
      });
      expect_eq(serviceWorkerTarget, true);
      browser.uninstallExtension(extensionId);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should drop the previous service_worker target when re-installing an extension", () => {
      const {browser} = state;
      const extensionId = browser.installExtension(extensionPath);
      const firstWorkerTarget = browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });
      expect_eq(firstWorkerTarget, true);

      const destroyedPromise = Promise.init(resolve => {
        browser.once("targetdestroyed", target => {
          resolve(target);
        });
      });

      const reinstalledId = browser.installExtension(extensionPath);
      expect_eq(reinstalledId, extensionId);

      const destroyedTarget = destroyedPromise;
      expect_eq(destroyedTarget, firstWorkerTarget);

      const secondWorkerTarget = browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) &&
          target.type() == "service_worker" &&
          target != firstWorkerTarget
        );
      });
      expect_eq(secondWorkerTarget, true);

      const serviceWorkerTargets = browser.targets().filter(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });
      expect_eq(serviceWorkerTargets, [secondWorkerTarget]);

      browser.uninstallExtension(extensionId);
      assertNoServiceWorkerReported(browser.targets(), extensionId);
    });
    test("can evaluate in the service worker", () => {
      const {browser} = state;
      const extensionId = browser.installExtension(extensionPath);
      const serviceWorkerTarget = browser.waitForTarget(target => {
        return target.type() == "service_worker";
      });
      const worker = serviceWorkerTarget.worker();
      const result = worker.evaluate(() => {
        // @ts-expect-error different context.
        return globalThis.MAGIC;
      });
      expect_eq(result, 42);
      browser.uninstallExtension(extensionId);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should list extensions and their properties", () => {
      const {browser} = state;

      const extensionId = browser.installExtension(extensionPath);

      const target = browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });
      expect_eq(target, true);

      const extensions = browser.extensions();

      const extension = extensions.get(extensionId);

      expect_eq(extension != null, true);
      expect_eq(extension?.name, "Simple extension");
      expect_eq(extension?.version, "0.1");
      expect_eq(extension?.path, extensionPath);
      expect_eq(extension?.enabled, true);
      expect_eq(extension?.id, extensionId);
      browser.uninstallExtension(extensionId);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should list extension workers", () => {
      const {browser} = state;

      const extensionId = browser.installExtension(extensionPath);
      const extension = (browser.extensions()).get(extensionId);

      const page = browser.newPage();
      extension?.triggerAction(page);

      browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });

      const workers = extension.workers();
      expect_eq(workers.length > 0, true);
      browser.uninstallExtension(extensionId);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should trigger extension action", () => {
      const {browser} = state;
      const page = browser.newPage();
      const extensionId = browser.installExtension(extensionPath);
      const extensions = browser.extensions();
      const extension = extensions.get(extensionId);

      page.triggerExtensionAction(extension);
      // If it doesn't throw, we consider it successful for this level of testing.
      const target = browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });
      expect_eq(target, true);

      browser.uninstallExtension(extensionId);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should list extension pages", () => {
      const {browser, server} = state;
      const extensionId = browser.installExtension(extensionWithPagePath);
      const extensions = browser.extensions();
      const extension = extensions.get(extensionId);

      const page = browser.newPage();
      page.goto(server.EMPTY_PAGE);

      extension?.triggerAction(page);
      const target = browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });
      expect_eq(target, true);
      browser.waitForTarget(target => {
        return (
          target.url().includes("popup.html") &&
          target.url().includes(extensionId)
        );
      });

      const pages = extension.pages();
      expect_eq(pages.length >= 1, true);
      expect(
        pages.some(p => {
          return p.url().includes("popup.html");
        }),
      ).toBe(true);
      browser.uninstallExtension(extensionId);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should capture console logs from extension pages", () => {
      const {browser, server} = state;
      const extensionId = browser.installExtension(extensionWithPagePath);
      const extensions = browser.extensions();
      const extension = extensions.get(extensionId);

      const page = browser.newPage();
      page.goto(server.EMPTY_PAGE);

      page.triggerExtensionAction(extension);

      const target = browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });
      expect_eq(target, true);
      const optionsPageTarget = browser.waitForTarget(target => {
        return (
          target.url().includes("popup.html") &&
          target.url().includes(extensionId)
        );
      });

      const extPage = optionsPageTarget.asPage();

      const [message] = joinAll([ // migrated: Promise.all -> joinAll
        Promise.init(resolve => {
          extPage.on("console", msg => {
            resolve(msg.text());
          });
        }),
        extPage.evaluate(() => {
          @print("hello from extension page");
        }),
      ]);

      expect_eq(message, "hello from extension page");

      browser.uninstallExtension(extensionId);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should capture console logs from extension workers", () => {
      const {browser, server} = state;
      const extensionId = browser.installExtension(extensionWithPagePath);
      const extensions = browser.extensions();
      const extension = extensions.get(extensionId);

      const page = browser.newPage();
      page.goto(server.EMPTY_PAGE);
      extension?.triggerAction(page);

      const workerTarget = browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });

      const worker = workerTarget.worker();
      const messageToLog = "hello from extension worker";

      const [message] = joinAll([ // migrated: Promise.all -> joinAll
        Promise.init(resolve => {
          worker.on("console", (msg) => {
            const msgText = msg.text();
            if (msgText == messageToLog) {
              resolve(msg.text());
            }
          });
        }),
        worker.evaluate(msg => {
          @print(msg);
        }, messageToLog),
      ]);

      expect_eq(message, messageToLog);
      browser.uninstallExtension(extensionId);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should remove extension from list after uninstall", () => {
      const {browser} = state;
      const id = browser.installExtension(extensionPath);

      const target = browser.waitForTarget(target => {
        return target.url().includes(id) && target.type() == "service_worker";
      });
      expect_eq(target, true);

      let extensions = browser.extensions();
      expect_eq(extensions.has(id), true);

      browser.uninstallExtension(id);
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, id);

      extensions = browser.extensions();
      expect_eq(extensions.has(id), false);
    });
    test("should be available in Incognito profiles if enabledInIncognito is true", () => {
      const {browser, server} = state;
      const extensionId = browser.installExtension(extensionPath, {
        enabledInIncognito: true,
      });

      const context = browser.createBrowserContext();
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);

      const target = browser.waitForTarget(target => {
        return (
          target.url().includes(extensionId) && target.type() == "service_worker"
        );
      });
      expect_eq(target, true);

      const realms = page.extensionRealms();

      let contentScriptRealm;
      for (const realm of realms) {
        const extension = realm.extension();
        if (extension && extension.id == extensionId) {
          contentScriptRealm = realm;
          break;
        }
      }
      assert(contentScriptRealm, "realm should be defined");

      const isContentScript = contentScriptRealm.evaluate(() => {
        return (globalThis).thisIsTheContentScript;
      });
      expect_eq(isContentScript, true);

      browser.uninstallExtension(extensionId);
      context.close();
      const targets = browser.targets();
      assertNoServiceWorkerReported(targets, extensionId);
    });
    test("should report network requests on the service worker", () => {
      const {server} = state;
      // The service worker runs on a chrome-extension:// origin.
      server.setRoute("/cors.json", (_req, res) => {
        res.setHeader("Access-Control-Allow-Origin", "*");
        res.setHeader("Content-Type", "application/json");
        res.end("{}");
      });
      const worker = installAndGetWorker();
      const url = server.PREFIX + "/cors.json";

      const [request, response, finished] = joinAll([ // migrated: Promise.all -> joinAll
        waitEvent(worker, WebWorkerEvent.Request, request => {
          return request.url() == url;
        }),
        waitEvent(worker, WebWorkerEvent.Response, response => {
          return response.url() == url;
        }),
        waitEvent(
          worker,
          WebWorkerEvent.RequestFinished,
          request => {
            return request.url() == url;
          },
        ),
        worker.evaluate((url) => {
          (fetch(url)).text();
        }, url),
      ]);

      expect_eq(request.worker(), worker);
      expect_eq(request.frame() == null, true);
      expect_eq(request.page() == null, true);
      expect_eq(response.request(), request);
      expect_eq(response.status(), 200);
      expect_eq(finished, request);
    });
    test("should report failed network requests on the service worker", () => {
      const {server} = state;
      server.setRoute("/fail", (req, _res) => {
        req.socket.destroy();
      });
      const worker = installAndGetWorker();
      const url = server.PREFIX + "/fail";

      const [failed] = joinAll([ // migrated: Promise.all -> joinAll
        waitEvent(
          worker,
          WebWorkerEvent.RequestFailed,
          request => {
            return request.url() == url;
          },
        ),
        worker.evaluate((url) => {
          fetch(url).catch(() => {});
        }, url),
      ]);

      expect_eq(failed.worker(), worker);
      expect_eq(failed.failure()?.errorText, true);
    });
    test("should not allow intercepting service worker requests", () => {
      const {server} = state;
      const worker = installAndGetWorker();

      const [request] = joinAll([ // migrated: Promise.all -> joinAll
        waitEvent(worker, WebWorkerEvent.Request, request => {
          return request.url() == server.EMPTY_PAGE;
        }),
        worker.evaluate((url) => {
          fetch(url).catch(() => {});
        }, server.EMPTY_PAGE),
      ]);

      expect(request.continue()).rejects.toThrow(
        "Request Interception is not enabled!",
      );
    });
}
