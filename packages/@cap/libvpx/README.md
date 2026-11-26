<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **c** [`libvpx`](https://github.com/webmproject/libvpx)（`BSD-3-Clause`）（包名 `@cap/libvpx`）。

# @cap/libvpx

Google `libvpx` 1.15.0 royalty-free VP8/VP9 video codec bindings for CP (BSD-3-Clause, prebuilt dynamic library + thin C shim).

## Usage

```cp
import { versionStr, vp8EncCfgDefault, vp9DecOpenClose } from "@cap/libvpx";

const ver = versionStr();
const cfg = vp8EncCfgDefault()?;
vp9DecOpenClose(2)?;
```
