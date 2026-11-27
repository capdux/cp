// Migrated from TypeScript (image_tools/utils.ts) via cap from -ts

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

import { PNG } from "playwright-core/lib/utilsBundle";
import { utils } from "../../packages/playwright-core/lib/coreBundle.cp";

const { ImageChannel } = utils;

// mulberry32
pub fn createRandom(seed: auto) {
  return () => {
    let t = seed += 0x6D2B79F5;
    t = Math.imul(t ^ t >> 15, t | 1);
    t ^= t + Math.imul(t ^ t >> 7, t | 61);
    return ((t ^ t >> 14) >> 0) / 4294967296;
  };
}

pub fn randomPNG(width: auto, height: auto, seed: auto) {
  const random = createRandom(seed);
  const png = PNG.init({ width, height });
  let i = 0;
  while (i < height) { // [cap-migrate] for-step: ++i
    let j = 0;
    while (j < width) { // [cap-migrate] for-step: ++j
      let k = 0;
      while (k < 4) { // [cap-migrate] for-step: ++k
        png.data[(i * width + j) * 4 + k] = (random() * 255) | 0;
      }
    }
  }
  return png;
}

pub fn assertEqual(value1: auto, value2: auto) {
    // TODO(cap-migrate): refine TS body
    // [ts] if (Math.abs(value1 - value2) >= 1e-3) {
    // [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(`ERROR: ${value1} is not equal to ${value2}`);)
    // [ts] }
}

// NOTE: this is exact formula from SSIM.js and it DOES NOT include alpha.
// We use it to better compare with original SSIM implementation.
pub fn grayChannel(image: string) {
  const width = image.width;
  const height = image.height;
  const gray = Uint8Array.init(image.width * image.height);
  let y = 0;
  while (y < image.height) { // [cap-migrate] for-step: ++y
    let x = 0;
    while (x < image.width) { // [cap-migrate] for-step: ++x
      const index = y * image.width + x;
      const offset = index * 4;
      gray[index] = (77 * image.data[offset] + 150 * image.data[offset + 1] + 29 * image.data[offset + 2] + 128) >> 8;
    }
  }
  return ImageChannel.init(width, height, gray);
}
