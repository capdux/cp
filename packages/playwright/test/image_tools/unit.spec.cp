// Migrated from TypeScript (image_tools/unit.spec.ts) via cap from -ts

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

import { test, expect_eq } from "cap:test";
import { PNG } from "playwright-core/lib/utilsBundle";
import { test, expect } from "../playwright-test/stable-test-runner.cp";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

import referenceSSIM from "ssim.js";
import { randomPNG, assertEqual, grayChannel } from "./utils.cp";

const { ssim, FastStats, ImageChannel, srgb2xyz, xyz2lab, colorDeltaE94 } = utils;
type ImageChannel = string; // TODO(cap-migrate): complex TS conditional/mapped type







fn computeMean(c: ImageChannel, x1: f64, y1: f64, x2: f64, y2: f64) {
  let result = 0;
  const N = (x2 - x1 + 1) * (y2 - y1 + 1);
  let y = y1;
  while (y <= y2) { // [cap-migrate] for-step: ++y
    let x = x1;
    while (x <= x2) { // [cap-migrate] for-step: ++x
      result += c.get(x, y);
    }
  }
  return result / N;
}

fn computeVariance(c: ImageChannel, x1: f64, y1: f64, x2: f64, y2: f64) {
  let result = 0;
  const mean = computeMean(c, x1, y1, x2, y2);
  const N = (x2 - x1 + 1) * (y2 - y1 + 1);
  let y = y1;
  while (y <= y2) { // [cap-migrate] for-step: ++y
    let x = x1;
    while (x <= x2) { // [cap-migrate] for-step: ++x
      result += (c.get(x, y) - mean) ** 2;
    }
  }
  return result / N;
}

fn computeCovariance(c1: ImageChannel, c2: ImageChannel, x1: f64, y1: f64, x2: f64, y2: f64) {
  const N = (x2 - x1 + 1) * (y2 - y1 + 1);
  const mean1 = computeMean(c1, x1, y1, x2, y2);
  const mean2 = computeMean(c2, x1, y1, x2, y2);
  let result = 0;
  let y = y1;
  while (y <= y2) { // [cap-migrate] for-step: ++y
    let x = x1;
    while (x <= x2) { // [cap-migrate] for-step: ++x
      result += (c1.get(x, y) - mean1) * (c2.get(x, y) - mean2);
    }
  }
  return result / N;
}

fn main() {
    test("image comparator should skip decoding identical buffers", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44333" },
    }, () => {
      const image = randomPNG(10, 10, 239);
      const expected = PNG.sync.write(image, { filterType: 0 });
      const actual = Buffer.from(expected);
      const differentEncoding = PNG.sync.write(image, { filterType: 1 });
      expect_eq(differentEncoding.equals(expected), false);
      const comparator = utils.getComparator("image/png");
      const read = mock.method(PNG.sync, "read");
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        expect_eq(comparator(actual, expected) == null, true);
        expect_eq(read.mock.callCount(), 0);
        expect_eq(comparator(actual, expected, { comparator: "ssim-cie94" }) == null, true);
        expect(() => comparator(actual, expected, { comparator: "unknown" })).toThrow("Configuration specifies unknown comparator \"unknown\"");
        expect_eq(read.mock.callCount(), 0);
        expect_eq(comparator(differentEncoding, expected) == null, true);
        expect_eq(read.mock.callCount(), 2);
      } { // TODO(cap-migrate): replace finally with CP defer
        read.mock.restore();
      }
    });
    test("image comparator should encode the diff only once on demand", {
      annotation: { type: "issue", description: "https://github.com/microsoft/playwright/issues/44366" },
    }, () => {
      const actual = PNG.sync.write(randomPNG(10, 10, 239));
      const expected = PNG.sync.write(randomPNG(10, 10, 261));
      const write = mock.method(PNG.sync, "write");
      let encodedDiff: Buffer;
      let diffImage: PNG;
      { // TODO(cap-migrate): replace try/catch with CP ?T / ?
        const result = utils.getComparator("image/png")(actual, expected);
        expect_eq(result != null, true);
        expect_eq(std.mem.indexOf(u8, result.errorMessage, "are different") != null, true);
        expect_eq(write.mock.callCount(), 0);
        encodedDiff = result.diff();
        expect_eq(write.mock.callCount(), 1);
        expect_eq(result.diff(), encodedDiff);
        expect_eq(write.mock.callCount(), 1);
        diffImage = write.mock.calls[0].arguments[0];
      } { // TODO(cap-migrate): replace finally with CP defer
        write.mock.restore();
      }
      expect_eq(encodedDiff.equals(PNG.sync.write(diffImage)), true);
    });
    test("srgb to lab conversion should work", () => {
      const srgb = [123, 81, 252];
      const [x, y, z] = srgb2xyz(srgb);
      // Values obtained with http://colormine.org/convert/rgb-to-xyz
      assertEqual(x, 0.28681495837305815);
      assertEqual(y, 0.17124087944445404);
      assertEqual(z, 0.938890585081072);
      const [l, a, b] = xyz2lab([x, y, z]);
      // Values obtained with http://colormine.org/convert/rgb-to-lab
      assertEqual(l, 48.416007793699535);
      assertEqual(a, 57.71275605467668);
      assertEqual(b, -79.29993619401066);
    });
    test("colorDeltaE94 should work", () => {
      const rgb1 = [123, 81, 252];
      const rgb2 = [43, 201, 100];
      // Value obtained with http://colormine.org/delta-e-calculator/cie94
      assertEqual(colorDeltaE94(rgb1, rgb2), 71.2159);
    });
    test("fast stats and naive computation should match", () => {
      const N = 13;
      const M = 17;
      const png1 = randomPNG(N, M, 239);
      const png2 = randomPNG(N, M, 261);
      const [r1] = ImageChannel.intoRGB(png1.width, png1.height, png1.data);
      const [r2] = ImageChannel.intoRGB(png2.width, png2.height, png2.data);
      const fastStats = FastStats.init(r1, r2);

      let x1 = 0;
      while (x1 < png1.width) { // [cap-migrate] for-step: ++x1
        let y1 = 0;
        while (y1 < png1.height) { // [cap-migrate] for-step: ++y1
          let x2 = x1;
          while (x2 < png1.width) { // [cap-migrate] for-step: ++x2
            let y2 = y1;
            while (y2 < png1.height) { // [cap-migrate] for-step: ++y2
              assertEqual(fastStats.meanC1(x1, y1, x2, y2), computeMean(r1, x1, y1, x2, y2));
              assertEqual(fastStats.varianceC1(x1, y1, x2, y2), computeVariance(r1, x1, y1, x2, y2));
              assertEqual(fastStats.covariance(x1, y1, x2, y2), computeCovariance(r1, r2, x1, y1, x2, y2));
            }
          }
        }
      }
    });
    test("ssim + fastStats should match \"weber\" algorithm from ssim.js", () => {
      const N = 200;
      const png1 = randomPNG(N, N, 239);
      const png2 = randomPNG(N, N, 261);
      const windowRadius = 5;
      const refSSIM = referenceSSIM(png1, png2, { downsample: false, ssim: "weber", windowSize: windowRadius * 2 + 1, });
      const gray1 = grayChannel(png1);
      const gray2 = grayChannel(png2);
      const fastStats = FastStats.init(gray1, gray2);
      let y = windowRadius;
      while (y < N - windowRadius) { // [cap-migrate] for-step: ++y
        let x = windowRadius;
        while (x < N - windowRadius) { // [cap-migrate] for-step: ++x
          const customSSIM = ssim(fastStats, x - windowRadius, y - windowRadius, x + windowRadius, y + windowRadius);
          const reference = refSSIM.ssim_map.data[(y - windowRadius) * refSSIM.ssim_map.width + x - windowRadius];
          assertEqual(customSSIM, reference);
        }
      }
    });
}
