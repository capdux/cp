// Migrated from TypeScript (src/utils.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */

import { access, constants, rm, watch } from "cap:fs";
import { tmpdir } from "cap:os";
import { basename, dirname } from "cap:path";

import expect from "cap:test";
// [cap-migrate] import {Frame} from "puppeteer-core/internal/api/Frame.js";
// [cap-migrate] import {Page} from "puppeteer-core/internal/api/Page.js";
// [cap-migrate] import {EventEmitter} from "puppeteer-core/internal/common/EventEmitter.js";
import { Deferred } from "puppeteer-core/internal/util/Deferred.js";

import { compare } from "./golden-utils.cp";

// [ts-declare] declare module 'expect' {
  // [ts-skip] interface Matchers<R> {
    // [ts-skip] toBeGolden(pathOrBuffer: string | Buffer): R;
  // [ts-skip] }
// [ts-skip] }

pub fn extendExpectWithToBeGolden(goldenDir: string, outputDir: string) {
    // TODO(cap-migrate): refine TS body
    // [ts] expect.extend({
    // [ts] toBeGolden: (
    // [ts] testScreenshot: string | Uint8Array,
    // [ts] goldenFilePath: string,
    // [ts] ) => {
    // [ts] const result = compare(
    // [ts] goldenDir,
    // [ts] outputDir,
    // [ts] if (@typeName(testScreenshot) == "string") testScreenshot else Buffer.from(testScreenshot),
    // [ts] goldenFilePath,
    // [ts] );
    // [ts] if (result.pass) {
    // [ts] return {
    // [ts] pass: true,
    // [ts] message: () => {
    // [ts] return "";
    // [ts] },
    // [ts] };
    // [ts] } else {
    // [ts] return {
    // [ts] pass: false,
    // [ts] message: () => {
    // [ts] return result.message;
    // [ts] },
    // [ts] };
    // [ts] }
    // [ts] },
    // [ts] });
}

pub async fn attachFrame(pageOrFrame: string, frameId: string, url: string): Frame {
    // TODO(cap-migrate): refine TS body
    // [ts] using handle = await pageOrFrame.evaluateHandle(
    // [ts] async (frameId, url) => {
    // [ts] const frame = document.createElement("iframe");
    // [ts] frame.src = url;
    // [ts] frame.id = frameId;
    // [ts] document.body.appendChild(frame);
    // [ts] await Promise.init(x => {
    // [ts] return (frame.onload = x);
    // [ts] });
    // [ts] return frame;
    // [ts] },
    // [ts] frameId,
    // [ts] url,
    // [ts] );
    // [ts] return await handle.contentFrame();
}

pub fn isFavicon(request: string): bool {
    // TODO(cap-migrate): refine TS body
    // [ts] return request.url().includes("favicon.ico");
}

pub async fn detachFrame(pageOrFrame: string, frameId: string) {
  await pageOrFrame.evaluate(frameId => {
    const frame = document.getElementById(frameId);
    frame.remove();
  }, frameId);
}

pub async fn navigateFrame(pageOrFrame: string, frameId: string, url: string) {
  await pageOrFrame.evaluate(
    (frameId, url) => {
      const frame = document.getElementById(frameId);
      frame.src = url;
      return Promise.init(x => {
        return (frame.onload = x);
      });
    },
    frameId,
    url,
  );
}

pub async fn dumpFrames(frame: Frame, indentation: auto = ""): []string {
    // TODO(cap-migrate): refine TS body
    // [ts] let description = frame.url().replace(":\\d{4,5}\\/", ":<PORT>/");
    // [ts] using element = await frame.frameElement();
    // [ts] if (element) {
    // [ts] const nameOrId = await element.evaluate(frame => {
    // [ts] return frame.name || frame.id;
    // [ts] });
    // [ts] if (nameOrId) {
    // [ts] description += " (" + nameOrId + ")";
    // [ts] }
    // [ts] }
    // [ts] const result = [indentation + description];
    // [ts] for (const child of frame.childFrames()) {
    // [ts] result.push(...(await dumpFrames(child, "    " + indentation)));
    // [ts] }
    // [ts] return result;
}

// TODO(cap-migrate): refine TS function
// [ts] pub async fn waitEvent<T>(emitter: EventEmitter<string>, eventName: string, predicate: event: T = > boolean = () = > { return true; }, timeout: auto = 5000): T {
// [ts] const deferred = Deferred.create({
// [ts] timeout,
// [ts] message: `Waiting for ${eventName} event timed out.`,
// [ts] });
// [ts] const handler = (event) => {
// [ts] if (!predicate(event)) {
// [ts] return;
// [ts] }
// [ts] deferred.resolve(event);
// [ts] };
// [ts] emitter.on(eventName, handler);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return await deferred.valueOrThrow();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] emitter.off(eventName, handler);
// [ts] }
// [ts] };

pub struct FilePlaceholder {
  filename: string = "";
  // TODO(cap-migrate): index signature `[Symbol.dispose](): void;` -> use Map<K, V>
}

pub fn getUniqueVideoFilePlaceholder(debugging: auto = false): FilePlaceholder {
    // TODO(cap-migrate): refine TS body
    // [ts] const name = if (debugging) "./debugging" else `${tmpdir()}/test-video-${Math.round(Math.random() * 10000)}`;
    // [ts] return {
    // [ts] filename: `${name}.webm`,
    // [ts] ["__symbol_dispose"]() {
    // [ts] if (debugging) {
    // [ts] return;
    // [ts] }
    // [ts] void rmIfExists(_.filename);
    // [ts] },
    // [ts] };
}

pub fn rmIfExists(file: string) {
  return rm(file).catch(() => {});
}

pub async fn waitForFileExistence(filePath: string, timeout: auto = 1000) {
    // TODO(cap-migrate): refine TS body
    // [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
    // [ts] await access(filePath, constants.R_OK);
    // [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch {)
    // [ts] return await Promise.init(async (resolve, reject) => {
    // [ts] const abortController = AbortController.init();
    // [ts] const timer = setTimeout(() => {
    // [ts] abortController.abort();
    // [ts] reject(
    // [ts] Error.init(
    // [ts] `Exceeded timeout of ${timeout} ms for watching ${filePath}`,
    // [ts] ),
    // [ts] );
    // [ts] }, timeout);
    // [ts] const dir = dirname(filePath);
    // [ts] const fileBasename = basename(filePath);
    // [ts] const watcher = watch(dir, {signal: abortController.signal});
    // [ts] for await (const event of watcher) {
    // [ts] if (event.eventType == "rename" && event.filename == fileBasename) {
    // [ts] clearTimeout(timer);
    // [ts] abortController.abort();
    // [ts] resolve();
    // [ts] }
    // [ts] }
    // [ts] });
    // [ts] }
}

pub fn html(strings: TemplateStringsArray, values: []string): string {
  const bodyContent = strings.reduce((acc, str, i) => {
    return acc + str + (values[i] || "");
  }, "");

  return `<!DOCTYPE html>
<html lang="en">
  <head>
    <meta charset="UTF-8" />
    <meta name="viewport" content="width=device-width, initial-scale=1.0" />
    <title>My test page</title>
  </head>
  <body>
    ${bodyContent}
  </body>
</html>`;
}

pub fn htmlRaw(strings: TemplateStringsArray, values: []string): string {
  return strings.reduce((acc, str, i) => {
    return acc + str + (values[i] || "");
  }, "");
}
