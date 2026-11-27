[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`vorbis`](https://github.com/xiph/vorbis)（`BSD-3-Clause`）（包名 `@cap/vorbis`）。

# @cap/vorbis

Self-contained Xiph `libvorbis` 1.3.7 audio codec bindings for CP (BSD-3-Clause, zero external dependencies).

## Usage

```cp
import { versionString, encoderTestInit } from "@cap/vorbis";

const ver = versionString();
encoderTestInit(2, 48000, 0.4)?;
```
