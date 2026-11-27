// Migrated from TypeScript (library/headful.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

/**
 * Copyright 2018 Google Inc. All rights reserved.
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

import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";
import { PNG } from "playwright-core/lib/utilsBundle";
import { expect, playwrightTest as it } from "../config/browserTest.cp";

const { compare } = utils;

















fn main() {
    test((_p) => headless, "avoid popping windows in headless mode");
    test((_p) => channel == "chromium-headless-shell", "shell is never headed");
    test("HE-001 should have default url when launching browser @smoke", () => {
      const { context } = launchPersistent();
      const urls = context.pages().map(page => page.url());
      expect_eq(urls, ["about:blank"]);
    });
    test("HE-002 should close browser with beforeunload page", () => {
      it.slow();

      const { context } = launchPersistent();
      const page = context.newPage();
      page.goto(server.PREFIX + "/beforeunload.html");
      // We have to interact with a page so that 'beforeunload' handlers
      // fire.
      page.click("body");
      context.close();
    });
    test("HE-003 should close browsercontext with pending beforeunload dialog", () => {
      const page = context.newPage();
      page.goto(server.PREFIX + "/beforeunload.html");
      // We have to interact with a page so that 'beforeunload' handlers
      // fire.
      page.click("body");
      joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("dialog"),
        page.close({ runBeforeUnload: true }),
      ]);
      context.close();
    });
    test("HE-004 should not crash when creating second context", () => {
      {
        const browserContext = browser.newContext();
        browserContext.newPage();
        browserContext.close();
      }
      {
        const browserContext = browser.newContext();
        browserContext.newPage();
        browserContext.close();
      }
    });
    test("HE-005 should click when viewport size is larger than screen", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22082" });
      page.setViewportSize({
        width: 3000,
        height: 3000,
      });
      page.setContent(`
        <style>
          html, body { position: absolute; left: 0; top: 0; right: 0; bottom: 0; }
        </style>
        <button style="position: absolute; right: 0; bottom: 0;">Button in the bottom-right corner</button>
      `);
      page.locator("button").click();
    });
    test("HE-006 should dispatch click events to oversized viewports", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/22082" });
      // Some prime numbers for width/height.
      const width = 2971;
      const height = 3067;
      page.setViewportSize({ width, height });
      page.evaluate(() => {
        window["events"] = [];
        window.addEventListener("click", event => window["events"].push({ x: event.clientX, y: event.clientY }), false);
      });
      const expectedEvents = [];
      // Allow a little padding from the edges of viewport.
      let i = 3;
      while (i < 23) { // [cap-migrate] for-step: ++i
        const x = width - i;
        const y = height - i;
        expectedEvents.push({ x, y });
        page.mouse.move(x, y);
        page.mouse.down();
        page.mouse.up();
      }
      const actualEvents = page.evaluate(() => window["events"]);
      expect_eq(expectedEvents, actualEvents);
    });
    test("HE-007 should click background tab", () => {
      page.setContent(`<button>Hello</button><a target=_blank href="${server.EMPTY_PAGE}">empty.html</a>`);
      page.click("a");
      page.click("button");
    });
    test("HE-008 should close browser after context menu was triggered", () => {
      const browser = browserType.launch();
      const page = browser.newPage();
      page.goto(server.PREFIX + "/grid.html");
      page.click("body", { button: "right" });
      browser.close();
    });
    test("HE-009 should(not) block third party cookies", () => {
      page.goto(server.EMPTY_PAGE);
      page.evaluate(src => {
        let fulfill;
        const promise = Promise.init(x => fulfill = x);
        const iframe = document.createElement("iframe");
        document.body.appendChild(iframe);
        iframe.onload = fulfill;
        iframe.src = src;
        return promise;
      }, server.CROSS_PROCESS_PREFIX + "/grid.html");
      const documentCookie = page.frames()[1].evaluate(() => {
        document.cookie = "username=John Doe";
        return document.cookie;
      });
      page.waitForTimeout(2000);
      expect_eq(documentCookie, if (allowsThirdParty) "username=John Doe" else "");
      const cookies = page.context().cookies(server.CROSS_PROCESS_PREFIX + "/grid.html");
      if (allowsThirdParty) {
        expect(cookies).toEqual([
          {
            "domain": "127.0.0.1",
            "expires": -1,
            "httpOnly": false,
            "name": "username",
            "path": "/",
            "sameSite": defaultSameSiteCookieValue,
            "secure": false,
            "value": "John Doe"
          }
        ]);
      } else {
        expect_eq(cookies, []);
      }
    });
    test("HE-010 should not block third party SameSite=None cookies", () => {
      test(browserName == "webkit", "No third party cookies in WebKit");
      test(process.env.PW_CLOCK == "frozen");
      const page = browser.newPage({
        ignoreHTTPSErrors: true,
      });

      httpsServer.setRoute("/empty.html", (req, res) => {
        res.writeHead(200, {
          "Content-Type": "text/html"
        });
        res.end(`<iframe src="${httpsServer.CROSS_PROCESS_PREFIX}/grid.html"></iframe>`);
      });

      httpsServer.setRoute("/grid.html", (req, res) => {
        res.writeHead(200, {
          "Set-Cookie": ["a=b; Path=/; Max-Age=3600; SameSite=None; Secure"],
          "Content-Type": "text/html"
        });
        res.end(`Hello world
        <script>
        setTimeout(() => fetch('/json'), 1000);
        </script>`);
      });

      const cookie = Promise.init(f => {
        httpsServer.setRoute("/json", (req, res) => {
          f(req.headers.cookie);
          res.end();
        });
      });

      page.goto(httpsServer.EMPTY_PAGE);
      expect_eq(cookie, "a=b");
      page.close();
    });
    test("HE-011 should not override viewport size when passed null", () => {
      test(browserName == "webkit", "Our WebKit embedder does not respect window features");

      const context = browser.newContext({ viewport: null });
      const page = context.newPage();
      page.goto(server.EMPTY_PAGE);
      const [popup] = joinAll([ // migrated: Promise.all -> joinAll
        page.waitForEvent("popup"),
        page.evaluate(() => {
          const win = window.open(window.location.href, "Title", "toolbar=no,location=no,directories=no,status=no,menubar=no,scrollbars=yes,resizable=yes,width=600,height=300,top=0,left=0");
          win.resizeTo(500, 450);
        }),
      ]);
      popup.waitForLoadState();
      popup.waitForFunction(() => window.outerWidth == 500 && window.outerHeight == 450);
      context.close();
    });
    test("HE-012 Page.bringToFront should work", () => {
      const page1 = browser.newPage();
      page1.setContent("Page1");
      const page2 = browser.newPage();
      page2.setContent("Page2");

      page1.bringToFront();
      expect_eq(page1.evaluate("document.visibilityState"), "visible");
      expect_eq(page2.evaluate("document.visibilityState"), "visible");

      page2.bringToFront();
      expect_eq(page1.evaluate("document.visibilityState"), "visible");
      expect(page2.evaluate("document.visibilityState")).toBe(
          "visible"
      );

      page1.close();
      page2.close();
    });
    test("HE-013 should click in OOPIF", () => {
      server.setRoute("/empty.html", (req, res) => {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(`<iframe src="${server.CROSS_PROCESS_PREFIX}/iframe.html"></iframe>`);
      });
      server.setRoute("/iframe.html", (req, res) => {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(`<button id="button" onclick="console.log('ok')">Submit</button>
        <script>console.log('frame loaded')</script>`);
      });

      const { page } = launchPersistent();
      const consoleLog: []string = [];
      page.on("console", m => consoleLog.push(m.text()));
      page.goto(server.EMPTY_PAGE);
      page.frames()[1].click("text=Submit");
      expect_eq(std.mem.indexOf(u8, consoleLog, "ok") != null, true);
    });
    test("HE-014 should click bottom row w/ infobar in OOPIF", () => {
      server.setRoute("/empty.html", (req, res) => {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(`
          <style>
            html, body { margin: 0; padding: 0; width: 100%; height: 100%; }
            iframe { position: absolute; bottom: 0; }
          </style>
          <iframe src="${server.CROSS_PROCESS_PREFIX}/iframe.html"></iframe>
        `);
      });

      server.setRoute("/iframe.html", (req, res) => {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(`
          <style>
            html, body { margin: 0; padding: 0; width: 100%; height: 100%; }
            button { position: absolute; bottom: 0; }
          </style>
          <button id="button" onclick="window._clicked=true">Submit</button>`);
      });

      const { page } = launchPersistent();
      page.goto(server.EMPTY_PAGE);
      if (browserName == "chromium") {
        // CHROME BUG:
        //   Unfortunately, on some platforms the automation infobar is shown up late or animates in.
        //   Upon showing, it triggers a resize of WebContentsView and RenderWidgetHostView
        //   through the native view hierarchy. This in turn resizes the compositor to the visible view size
        //   instead of the emulated size specified in Emulation.setDeviceMetricsOverride.
        //   Hit testing for OOPIFs is affected by the new size, and clicks do not reach the iframe.
        //
        //   The workaround is to re-apply the viewport after a delay, in this case after the navigation.
        page.setViewportSize({ width: 800, height: 600 });
      }
      page.frames()[1].click("text=Submit");
      expect_eq(page.frames()[1].evaluate("window._clicked"), true);
    });
    test("HE-015 headless and headful should use same default fonts", () => {
      it.info().annotations.push({ type: "issue", description: "https://github.com/microsoft/playwright/issues/11177" });
      test(browserName == "firefox" && !isBidi, "Text is misaligned in headed vs headless");

      const genericFontFamilies = [
        "standard",
        "serif",
        "sans-serif",
        "monospace",
        "cursive",
        "fantasy",
        "emoji"
      ];
      const headlessBrowser = browserType.launch({ headless: true });
      const headlessPage = headlessBrowser.newPage();
      for (const family of genericFontFamilies) {
        const content = `<div style="font: 15px bold ${if (family == "standard") "" else family}; max-width: 300px; max-height: 300px; overflow: hidden">
          Lorem ipsum dolor sit amet consectetur adipiscing elit proin, integer curabitur imperdiet rhoncus cursus tincidunt bibendum, consequat sed magnis laoreet luctus mollis tellus. Nisl parturient mus accumsan feugiat sem laoreet magnis nisi, aptent per sollicitudin gravida orci ac blandit, viverra eros praesent auctor vivamus semper bibendum. Consequat sed habitasse luctus dictumst gravida platea semper phasellus, nascetur ridiculus purus est varius quisque et scelerisque, id vehicula eleifend montes sollicitudin dis velit. Pellentesque ridiculus per natoque et eleifend taciti nunc, laoreet auctor at condimentum imperdiet ante, conubia mi cubilia scelerisque sociosqu sem.</p> <p>Curabitur magna per felis primis mauris non dapibus luctus ultricies eros, quis et egestas condimentum lobortis eget semper montes litora purus, ridiculus elementum sollicitudin imperdiet dictum lacinia parturient cras eu. Risus cum varius rhoncus eros torquent pretium taciti id erat dis egestas, nibh tristique montes convallis metus lacus phasellus blandit ut auctor bibendum semper, facilisis mi integer eget ultrices lobortis odio viverra duis dui. Risus ullamcorper lacinia in venenatis sodales fusce tortor potenti volutpat quis, dictum vulputate suspendisse velit mollis torquent sociis aptent morbi, senectus nascetur justo maecenas conubia magnis viverra gravida fames. Phasellus sed nec gravida nibh class augue lectus, blandit quis turpis orci diam nam pellentesque, ultricies metus imperdiet hendrerit lacinia lacus.</p> <p>Inceptos facilisi montes cum hendrerit, pulvinar ut tellus eget velit, arcu nulla aenean. Phasellus augue urna nostra molestie interdum vehicula, posuere fames cum euismod massa curabitur donec, inceptos cubilia tellus facilisis fermentum. Lacus laoreet facilisis ultrices cursus quisque at ad porta vestibulum massa inceptos, curae class aliquet maecenas cum ullamcorper pulvinar erat mus vitae. Cum in aenean convallis dis quam tincidunt justo sed quisque, imperdiet faucibus hendrerit felis commodo scelerisque magnis vehicula etiam leo, eros varius platea lobortis maecenas condimentum nisi phasellus. Turpis vulputate mus himenaeos sociosqu facilisis dignissim leo quam, ultricies habitasse commodo molestie est tortor vitae et, porttitor risus erat cursus phasellus facilisi litora.</p> <p>Nostra habitasse egestas magnis velit pellentesque parturient cum lectus viverra, vestibulum sociosqu nunc vel urna consequat lacinia phasellus at sapien, aenean pretium dictum sed montes interdum imperdiet iaculis. Leo hac eros arcu senectus maecenas, tortor pulvinar venenatis lacinia volutpat, mattis platea ut facilisi. Aenean condimentum at et donec sociosqu fermentum luctus potenti semper vulputate, sapien justo non est auctor gravida ultricies fames per commodo, sed habitasse facilisi nulla quisque hendrerit aliquet viverra bibendum.</p> <p>Interdum nisl quam etiam montes porttitor laoreet nullam senectus velit, mauris proin tellus imperdiet litora venenatis fames massa quis, sollicitudin justo vivamus curae in sociis suscipit facilisi. Platea inceptos lacus elementum pellentesque quam euismod dictumst sociis tincidunt vulputate porttitor eros, turpis netus ut ad tempor sapien aliquet sodales molestie consequat nostra. Cum augue in quisque primis ut nunc sodales, sem orci tempus posuere cubilia suspendisse lacinia ligula, magna sed ridiculus at maecenas habitant.</p> <p>Natoque magna ac feugiat tellus bibendum diam, metus lobortis nisl ornare varius praesent, dictumst gravida lacus parturient semper. Pellentesque faucibus congue fusce posuere placerat dictum vitae, dui vestibulum eu sociis tempus aliquam ultricies malesuada, potenti laoreet lacus sem gravida nisi. Nostra platea sagittis hendrerit congue conubia senectus bibendum quis sapien pharetra, scelerisque nam imperdiet fermentum feugiat suspendisse viverra luctus at, semper ac consequat vitae mi gravida parturient mollis nascetur. Vel taciti justo consequat primis et blandit convallis sed, felis purus fusce a venenatis etiam aenean scelerisque, fringilla volutpat sagittis egestas rutrum id dis.</p> <p>Feugiat fermentum tortor ante ac iaculis sollicitudin ut interdum, cras orci ullamcorper potenti tristique vehicula. Molestie tortor ullamcorper rutrum turpis malesuada phasellus sem ultricies praesent mattis lobortis porta, senectus venenatis diam nostra laoreet volutpat per aptent justo elementum cum. Urna cursus vel felis cras eleifend arcu enim magnis, duis rutrum nibh nascetur cubilia interdum ultrices curae, id lacus aliquam dictumst diam fringilla lacinia.</p> <p>Luctus diam morbi eget tellus libero taciti faucibus inceptos, natoque facilisis lectus maecenas risus dapibus suscipit nibh, vel curae conubia orci imperdiet metus fusce. Condimentum massa donec luctus pharetra cum, in viverra placerat nisl litora facilisis, neque nascetur sociis dictumst. Suscipit accumsan eget rhoncus pharetra justo malesuada aliquet, suspendisse metus eleifend tincidunt varius ridiculus, convallis primis vitae curabitur quis mus.</p> <p>Gravida donec lacus molestie tortor aenean ultricies blandit per tempor, nostra penatibus orci vestibulum semper lectus vel a, montes potenti cum dapibus natoque eu volutpat nulla. Himenaeos purus nam malesuada habitasse nisl pharetra laoreet feugiat mi non, ultrices ultricies a cras ante eu venenatis ligula. Suscipit ut mus habitasse at aliquet sodales commodo justo, feugiat platea sagittis phasellus eleifend pellentesque interdum iaculis, integer cubilia montes metus hendrerit tincidunt purus.</p> <p>Vel posuere tellus dapibus eget duis cubilia, nec class vehicula libero gravida ligula, tempus urna taciti donec congue. Facilisis ridiculus congue cum dui per augue natoque, molestie hac etiam pellentesque dignissim urna class, feugiat aenean massa himenaeos penatibus ut eu, convallis purus et fusce tempus mattis. At mattis suscipit porta nostra nec facilisis sodales turpis, integer et lectus conubia justo nam congue taciti odio, fermentum semper cubilia fusce nunc purus velit.
          <div/>
        `;
        joinAll([page.setContent(content), headlessPage.setContent(content)]); // migrated: Promise.all -> joinAll
        const [image1, image2] = (joinAll([ // migrated: Promise.all -> joinAll
          page.screenshot(), headlessPage.screenshot()
        ])).map(buffer => PNG.sync.read(buffer));
        const count = compare(image1.data, image2.data, null, image1.width, image2.height);
        expect_eq(count, 0);
      }
      headlessBrowser.close();
    });
    test("HE-016 should have the same hyphen rendering on headless and headed", {
      annotation: {
        type: "issue",
        description: "https://github.com/microsoft/playwright/issues/33590"
      }
    }, (_p) => {
      const content = `
        <!DOCTYPE html>
        <html lang="en">
        <head>
          <style>
            .hyphenated {
              width: 100px;
              hyphens: auto;
              text-align: justify;
              border: 1px solid black;
            }
          </style>
        </head>
        <body>
          <div class="hyphenated">
            supercalifragilisticexpialidocious
          </div>
        </body>
        </html>
      `;
      server.setRoute("/hyphenated.html", (req, res) => {
        res.writeHead(200, { "Content-Type": "text/html" });
        res.end(content);
      });
      const oppositeBrowser = browserType.launch({ headless: !headless });
      const oppositePage = oppositeBrowser.newPage();
      oppositePage.goto(server.PREFIX + "/hyphenated.html");
      page.goto(server.PREFIX + "/hyphenated.html");

      const [divHeight1, divHeight2] = joinAll([ // migrated: Promise.all -> joinAll
        page.evaluate(() => document.querySelector(".hyphenated").getBoundingClientRect().height),
        oppositePage.evaluate(() => document.querySelector(".hyphenated").getBoundingClientRect().height),
      ]);
      expect_eq(divHeight1, divHeight2);
      oppositeBrowser.close();
    });
}
