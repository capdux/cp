// Migrated from TypeScript (src/golden-utils.ts) via cap from -ts

/**
 * @license
 * Copyright 2017 Google Inc.
 * SPDX-License-Identifier: Apache-2.0
 */
import assert from "cap:test";
import fs from "cap:fs";
import path from "cap:path";

import { diffLines } from "diff";
import jpeg from "jpeg-js";
import mime from "cap:mime";
import pixelmatch from "pixelmatch";
import { PNG } from "cap:png";

struct DiffFile {
  diff: string = "";
  ext: string? = null;
}

// TODO(cap-migrate): refine TS statement
// [ts] const GoldenComparators = new Map<
// [ts] string,
// [ts] (
// [ts] actualBuffer: string | Buffer,
// [ts] expectedBuffer: string | Buffer,
// [ts] mimeType: string,
// [ts] ) => DiffFile | null;
// TODO(cap-migrate): refine TS statement
// [ts] >();

fn addSuffix(filePath: string, suffix: string, customExtension: string? = null): string {
    // TODO(cap-migrate): refine TS body
    // [ts] const dirname = path.dirname(filePath);
    // [ts] const ext = path.extname(filePath);
    // [ts] const name = path.basename(filePath, ext);
    // [ts] return path.join(dirname, name + suffix + (customExtension || ext));
}

fn compareImages(actualBuffer: string, expectedBuffer: string, mimeType: string): DiffFile? {
    // TODO(cap-migrate): refine TS body
    // [ts] assert(@typeName(actualBuffer) != "string");
    // [ts] assert(@typeName(expectedBuffer) != "string");
    // [ts] const actual =
    // [ts] if (mimeType == "image/png") PNG.sync.read(actualBuffer) else jpeg.decode(actualBuffer);
    // [ts] const expected =
    // [ts] if (mimeType == "image/png") PNG.sync.read(expectedBuffer) else jpeg.decode(expectedBuffer);
    // [ts] if (expected.width != actual.width || expected.height != actual.height) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error()
    // [ts-throw] `Sizes differ: expected image ${expected.width}px X ${expected.height}px, but got ${actual.width}px X ${actual.height}px.`,
    // [ts-throw] );
    // [ts] }
    // [ts] const diff = PNG.init({width: expected.width, height: expected.height});
    // [ts] const count = pixelmatch(
    // [ts] expected.data,
    // [ts] actual.data,
    // [ts] diff.data,
    // [ts] expected.width,
    // [ts] expected.height,
    // [ts] {threshold: 0.1},
    // [ts] );
    // [ts] return if (count > 0) {diff: PNG.sync.write(diff)} else null;
}

fn compareText(actual: string, expectedBuffer: string): DiffFile? {
    // TODO(cap-migrate): refine TS body
    // [ts] assert(
    // [ts] @typeName(actual) == "string",
    // [ts] `Expected type string got ${@typeName(actual)}`,
    // [ts] );
    // [ts] const expected = expectedBuffer.toString("utf-8");
    // [ts] if (expected == actual) {
    // [ts] return;
    // [ts] }
    // [ts] const result = diffLines(expected, actual);
    // [ts] const html = result.reduce(
    // [ts] (text, change) => {
    // [ts] text += if (change.added) `<span class='ins'>${change.value}</span>` else if (change.removed) `<span class='del'>${change.value}</span>` else change.value;
    // [ts] return text;
    // [ts] },
    // [ts] `<link rel="stylesheet" href="file://${path.join(
    // [ts] import.meta.dirname,
    // [ts] 'diffstyle.css',
    // [ts] )}">`,
    // [ts] );
    // [ts] return {
    // [ts] diff: html,
    // [ts] ext: ".html",
    // [ts] };
}

GoldenComparators.set("image/png", compareImages);
GoldenComparators.set("image/jpeg", compareImages);
GoldenComparators.set("text/plain", compareText);

pub fn compare(goldenPath: string, outputPath: string, actual: string, goldenName: string): string {
    // TODO(cap-migrate): refine TS body
    // [ts] goldenPath = path.normalize(goldenPath);
    // [ts] outputPath = path.normalize(outputPath);
    // [ts] const expectedPath = path.join(goldenPath, goldenName);
    // [ts] const actualPath = path.join(outputPath, goldenName);
    // [ts] const messageSuffix = `Output is saved in "${path.basename(
    // [ts] outputPath + '" directory',
    // [ts] )}`;
    // [ts] if (!fs.existsSync(expectedPath)) {
    // [ts] ensureOutputDir();
    // [ts] fs.writeFileSync(actualPath, actual);
    // [ts] return {
    // [ts] pass: false,
    // [ts] message: `${goldenName} is missing in golden results. ${messageSuffix}`,
    // [ts] };
    // [ts] }
    // [ts] const expected = fs.readFileSync(expectedPath);
    // [ts] const mimeType = mime.getType(goldenName);
    // [ts] assert(mimeType);
    // [ts] const comparator = GoldenComparators.get(mimeType);
    // [ts] if (!comparator) {
    // [ts] return {
    // [ts] pass: false,
    // [ts] message: `Failed to find comparator with type ${mimeType}: ${goldenName}`,
    // [ts] };
    // [ts] }
    // [ts] const result = comparator(actual, expected, mimeType);
    // [ts] if (!result) {
    // [ts] return {pass: true};
    // [ts] }
    // [ts] ensureOutputDir();
    // [ts] if (goldenPath == outputPath) {
    // [ts] fs.writeFileSync(addSuffix(actualPath, "-actual"), actual);
    // [ts] } else {
    // [ts] fs.writeFileSync(actualPath, actual);
    // Copy expected to the output/ folder for convenience.
    // [ts] fs.writeFileSync(addSuffix(actualPath, "-expected"), expected);
    // [ts] }
    // [ts] if (result) {
    // [ts] const diffPath = addSuffix(actualPath, "-diff", result.ext);
    // [ts] fs.writeFileSync(diffPath, result.diff);
    // [ts] }
    // [ts] return {
    // [ts] pass: false,
    // [ts] message: `${goldenName} mismatch! ${messageSuffix}`,
    // [ts] };
    // [ts] fn ensureOutputDir() {
    // [ts] if (!fs.existsSync(outputPath)) {
    // [ts] fs.mkdirSync(outputPath);
    // [ts] }
    // [ts] }
}
