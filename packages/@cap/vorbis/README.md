# @cap/vorbis

Self-contained Xiph `libvorbis` 1.3.7 audio codec bindings for CP (BSD-3-Clause, zero external dependencies).

## Usage

```cp
import { versionString, encoderTestInit } from "@cap/vorbis";

const ver = versionString();
encoderTestInit(2, 48000, 0.4)?;
```
