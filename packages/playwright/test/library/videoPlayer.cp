// Migrated from TypeScript (library/videoPlayer.ts) via cap from -ts

/**
 * Copyright Microsoft Corporation. All rights reserved.
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

import { spawnSync } from "cap:process";
import fs from "cap:fs";
import { PNG } from "playwright-core/lib/utilsBundle";
import { registry } from "../../packages/playwright-core/lib/coreBundle.cp";

pub class VideoPlayer {

  fileName: string = "";
  output: string = "";
  duration: f64 = 0.0;
  frames: f64 = 0.0;
  videoWidth: f64 = 0.0;
  videoHeight: f64 = 0.0;
  fps: f64 = 0.0;
  cache: string = Map.init();

  pub static fn init(fileName: string): VideoPlayer {
      let self_val: VideoPlayer = VideoPlayer{};
    self_val.fileName = fileName;
    const ffmpeg = registry.registry.findExecutable("ffmpeg").executablePathOrDie("javascript");
    // Force output frame rate to 25 fps as otherwise it would produce one image per timebase unit
    // which is 1 / (25 * 1000).
    self_val.output = spawnSync(ffmpeg, ["-i", self_val.fileName, "-r", "25", `${_.fileName}-%04d.png`]).stderr.toString();

    const lines = self_val.output.split("\n");
    let framesLine = lines.find(l => l.startsWith("frame="));
    if (!framesLine) {
      return; // TODO(cap-migrate): throw -> error return (throw new Error(`No frame data in the output:\n${this.output}`);)
    }
    framesLine = framesLine.substring(framesLine.lastIndexOf("frame="));
    const framesMatch = framesLine.match("frame=\\s+(\\d+)");
    const streamLine = lines.find(l => l.trim().startsWith("Stream #0:0"));
    const resolutionMatch = streamLine.match(", (\\d+)x(\\d+),");
    const fpsMatch = streamLine.match(", ([\\d.]+) fps,");
    const durationMatch = lines.find(l => l.trim().startsWith("Duration")).match("Duration: (\\d+):(\\d\\d):(\\d\\d.\\d\\d)");
    self_val.duration = (((parseInt(durationMatch[1], 10) * 60) + parseInt(durationMatch[2], 10)) * 60 + parseFloat(durationMatch[3])) * 1000;
    self_val.frames = parseInt(framesMatch[1], 10);
    self_val.videoWidth = parseInt(resolutionMatch[1], 10);
    self_val.videoHeight = parseInt(resolutionMatch[2], 10);
    self_val.fps = parseFloat(fpsMatch[1]);
      return self_val;
  }

  // TODO(cap-migrate): refine TS function
  // [ts] pub fn findFrame(*_, framePredicate: pixels: Buffer = > boolean, offset: string? = null): string {
  // [ts] let f = 1;
  // [ts] while (f <= _.frames) { // [cap-migrate] for-step: ++f
  // [ts] const frame = _.frame(f, offset);
  // [ts] if (framePredicate(frame.data)) {
  // [ts] return frame;
  // [ts] }
  // [ts] }
  // [ts] }

  pub fn seekLastFrame(*_, offset: string? = null): string {
    return _.frame(_.frames, offset);
  }

  pub fn frame(*_, frame: f64, offset: auto = { x: 10, y: 10 }): string {
    if (!_.cache.has(frame)) {
      const gap = "0".repeat(4 - String(frame).length);
      const buffer = fs.readFileSync(`${_.fileName}-${gap}${frame}.png`);
      _.cache.set(frame, PNG.sync.read(buffer));
    }
    const decoded = _.cache.get(frame);
    const dst = PNG.init({ width: 10, height: 10 });
    PNG.bitblt(decoded, dst, offset.x, offset.y, 10, 10, 0, 0);
    return dst;
  }
}
