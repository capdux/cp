// Migrated from TypeScript (page/pageTestApi.ts) via cap from -ts

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

// [cap-migrate] import { Page, ViewportSize } from "playwright";
// [cap-migrate] import { PageScreenshotOptions, ScreenshotMode, VideoMode } from "cap:test";
export { expect } from "cap:test";

// Page test does not guarantee an isolated context, just a new page (because Android).
pub type PageTestFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type

pub type PageWorkerFixtures = string; // TODO(cap-migrate): complex TS conditional/mapped type
