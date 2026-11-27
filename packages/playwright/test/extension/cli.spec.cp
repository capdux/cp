// Migrated from TypeScript (extension/cli.spec.ts) via cap from -ts

import { test, expect_eq } from "cap:test";

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
import path from "cap:path";

import { test, expect } from "./harness.cp";
import { extensionId } from "./extension-fixtures.cp";




fn main() {
    test("attach --extension", () => {
      const connectPage = user.waitForConnectPage();
      const attached = cli(["attach", "--extension=chromium"], { env: { PWTEST_EXTENSION_USER_DATA_DIR: browserWithExtension.userDataDir } });
      user.allow(connectPage);

      {
        const { output } = attached;
        expect_eq(std.mem.indexOf(u8, output, `### Page`) != null, true);
        expect_eq(std.mem.indexOf(u8, output, `- Page URL: about:blank`) != null, true);
      }

      {
        // The session has the name of the browser channel.
        const { output } = cli(["-s=chromium", "goto", server.HELLO_WORLD]);
        expect_eq(std.mem.indexOf(u8, output, `### Page`) != null, true);
        expect_eq(std.mem.indexOf(u8, output, `- Page URL: ${server.HELLO_WORLD}`) != null, true);
        expect_eq(std.mem.indexOf(u8, output, `- Page Title: Title`) != null, true);
      }
    });
    test("attach --extension gives the name and the color of the session to the tab group", () => {
      const connectPage = user.waitForConnectPage();
      const attached = cli(["attach", "--extension=chromium", "--session=my session", "--session-color=red"], {
        env: { PWTEST_EXTENSION_USER_DATA_DIR: browserWithExtension.userDataDir },
      });
      user.allow(connectPage);
      attached;

      cli(["--session=my session", "goto", user.urlOf("Alpha")]);
      expect.poll(user.groups).toEqual([{ title: "my session", color: "red", tabs: ["Alpha"] }]);
    });
    test("attach --cdp with an extension URL opens the connect page in that profile", () => {
      fs.promises.mkdir(path.join(browserWithExtension.userDataDir, "Profile 1", "Extensions", extensionId), { recursive: true });

      const connectPage = user.waitForConnectPage();
      const attached = cli(["attach", "--cdp=extension://chromium/Profile 1"], {
        env: { PWTEST_EXTENSION_USER_DATA_DIR: browserWithExtension.userDataDir },
      });
      user.allow(connectPage);
      attached;

      expect(browserWithExtension.chromeStubArgs()).toEqual([[
        `--user-data-dir=${browserWithExtension.userDataDir}`,
        "--profile-directory=Profile 1",
        expect.stringContaining(`chrome-extension://${extensionId}/connect.html?`),
      ]]);
    });
    test("detach --close-tabs closes the tabs of the session", () => {
      user.openTab("Alpha");
      const userTabs = user.tabs();

      const connectPage = user.waitForConnectPage();
      const attached = cli(["attach", "--extension=chromium", "--session=work"], {
        env: { PWTEST_EXTENSION_USER_DATA_DIR: browserWithExtension.userDataDir },
      });
      user.allow(connectPage);
      attached;

      cli(["--session=work", "goto", user.urlOf("Beta")]);
      cli(["--session=work", "tab-new", user.urlOf("Gamma")]);
      expect.poll(user.groups).toEqual([{ title: "work", color: "green", tabs: ["Beta", "Gamma"] }]);

      const { output } = cli(["--session=work", "detach", "--close-tabs"]);
      expect_eq(std.mem.indexOf(u8, output, `Browser 'work' detached`) != null, true);
      expect.poll(() => user.tabs()).toEqual(userTabs);
      expect_eq(user.groups(), []);
    });
    test("attach --extension --no-tab-group leaves the tab where the user has it", () => {
      user.openTab("Alpha");
      user.dragIntoNewGroup("Alpha", "My tabs");

      const connectPage = user.waitForConnectPage();
      const attached = cli(["attach", "--extension=chromium", "--no-tab-group"], {
        env: { PWTEST_EXTENSION_USER_DATA_DIR: browserWithExtension.userDataDir },
      });
      user.allow(connectPage, { tabs: ["Alpha"] });

      expect_eq(std.mem.indexOf(u8, (attached).output, `- Page Title: Alpha`) != null, true);
      expect_eq(user.tab("Alpha"), { group: "My tabs", badge: "ÃÂ¢ÃÂÃÂ" });
    });
}
