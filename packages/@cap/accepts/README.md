# @cap/accepts

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:accepts` ([https://github.com/jshttp/accepts](https://github.com/jshttp/accepts))

Zero-allocation RFC 9110 HTTP content negotiation engine (`Accept`, `Accept-Encoding`, `Accept-Language`) for CP — clean-room MIT parity with npm `accepts` and `negotiator` (75M+/wk).

纯 CP 零堆分配 RFC 9110 HTTP 内容协商引擎，使用千分位整数质量权重 `0..1000` 消除浮点误差，支持精确特异度排序、BCP 47 语言前缀降级与 `q=0` 显式拒绝。

## Install / 安装

```bash
cap add @cap/accepts
```

## API

| Symbol | Signature | Description |
| --- | --- | --- |
| `parseQValue` | `(s: []const u8) -> i32` | Parse RFC 9110 q-value into `0..1000` (`-1` if invalid) |
| `negotiateEncoding` | `(accept_header: []const u8, available: []const []const u8) -> i64` | Pick best encoding index (supports `*` & `q=0` rejection) |
| `negotiateLanguage` | `(accept_header: []const u8, available: []const []const u8) -> i64` | Pick best BCP 47 language index (exact > prefix > `*`) |
| `negotiateMediaType` | `(accept_header: []const u8, available: []const []const u8) -> i64` | Pick best media type (`type/sub` > `type/*` > `*/*`) |

## Usage / 使用示例

```cp
import { negotiateEncoding, negotiateLanguage, negotiateMediaType } from "@cap/accepts";

const encs: [3][]const u8 = ["br", "gzip", "deflate"];
const best = negotiateEncoding("gzip;q=0.8, br;q=1.0", encs[0..3]); // 0 ("br")
```
