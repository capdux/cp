// Migrated from TypeScript (config/testMode.ts) via cap from -ts

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

import { oop, client } from "../../packages/playwright-core/lib/coreBundle.cp";

pub type TestModeName = string;

const { start } = oop;

struct TestMode {
  // TODO(cap-migrate): interface method `setup(): Promise<client.Playwright>;` -> move to trait/impl
  // TODO(cap-migrate): interface method `teardown(): Promise<void>;` -> move to trait/impl
}

pub class DriverTestMode {
  _impl: string = "";

  pub async fn setup(*_) {
    _._impl = await start({
      NODE_OPTIONS: null,  // Hide driver process while debugging.
    });
    return _._impl.playwright;
  }

  pub async fn teardown() {
    await _._impl.stop();
  }
}

pub class DefaultTestMode {
  pub async fn setup() {
    return require("playwright-core");
  }

  pub async fn teardown() {
  }
}
