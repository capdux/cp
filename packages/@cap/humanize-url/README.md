# @cap/humanize-url

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:humanize-url` ([https://github.com/sindresorhus/humanize-url](https://github.com/sindresorhus/humanize-url))

Zero-allocation & zero-copy URL humanizer for CP — clean-room MIT parity with npm `humanize-url` v3.

纯 CP 零堆分配、零拷贝 URL 人性化展示库（对标 npm `humanize-url` v3），修剪首尾空白、剥离 `http://` / `https://` / `//` 协议头与前导 `www.`（大小写不敏感）并剥离尾部 `/`，保留路径、查询与锚点。

## Install / 安装

```bash
cap add @cap/humanize-url
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `humanizeUrlSlice` | `(url: string): string` | Zero-copy slice humanizing `url` |
| `humanizeUrl` | `(url: string, out: []u8): usize` | Writes humanized `url` into `out` and returns bytes written |

## Usage / 使用示例

```cp
import { humanizeUrl, humanizeUrlSlice } from "@cap/humanize-url";

const s = humanizeUrlSlice("https://www.sindresorhus.com/"); // "sindresorhus.com"

let buf: [128]u8 = undefined;
const n = humanizeUrl("  HTTP://WWW.Example.com/docs/?q=1#top ", buf[0..128]);
// "Example.com/docs/?q=1#top"
```
