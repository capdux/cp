[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/pretty-bytes

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-bytes` ([https://github.com/sindresorhus/pretty-bytes](https://github.com/sindresorhus/pretty-bytes))

Pure CP, zero-allocation human-readable byte and bit size formatter (parity with npm `pretty-bytes`).

## Features

- **SI (1000-based) & IEC Binary (1024-based)**: `B, kB, MB, GB, TB, PB, EB` and `B, KiB, MiB, GiB, TiB, PiB, EiB`
- **Bit Units (`bits: true`)**: `b, kbit, Mbit, Gbit...` and `b, kibit, Mibit, Gibit...`
- **Formatting Options**: `signed_val` (`+42 B` / ` 0 B`), `space` (`1.34 kB` vs `1.34kB`), configurable `precision` with automatic trailing-zero trimming
- **Zero Heap Allocation**: Formats directly into caller-provided `[]u8` buffer with overflow protection

## Usage

```cp
import { prettyBytes, prettyBytesWith, PrettyBytesOpts } from "@cap/pretty-bytes";

let buf: [32]u8 = undefined;
let n1 = prettyBytes(1337.0, buf[0..32]); // "1.34 kB"

let opts = PrettyBytesOpts.defaults();
opts.binary = true;
let n2 = prettyBytesWith(1536.0, opts, buf[0..32]); // "1.5 KiB"
```

## License

MIT
