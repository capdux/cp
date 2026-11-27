// Migrated from TypeScript (installation/.mocharc.js) via cap from -ts

/**
 * @license
 * Copyright 2022 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @type {import('mocha').MochaOptions}
 */
// TODO(cap-migrate): refine TS statement
// [ts] export default {
// [ts] spec: ["build/**/*.test.js"],
// [ts] require: ["source-map-support/register"],
// [ts] exit: !!process.env.CI,
// [ts] timeout: 240_000,
// This should make mocha crash on uncaught errors.
// See https://github.com/mochajs/mocha/blob/master/docs/index.md#--allow-uncaught.
// [ts] allowUncaught: true,
// See https://github.com/mochajs/mocha/blob/master/docs/index.md#--async-only--a.
// [ts] asyncOnly: true,
// [ts] };
