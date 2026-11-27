[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# pretty-ms

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:pretty-ms` ([https://github.com/sindresorhus/pretty-ms](https://github.com/sindresorhus/pretty-ms))

零堆分配的 CP 毫秒高精度分解与多风格可读时长格式化引擎 — 洁净室 MIT 实现，对标 npm `pretty-ms` + `parse-ms`（~1.8 亿/周）。

Zero-allocation millisecond decomposer and human-readable duration formatter for CP — clean-room MIT parity with npm `pretty-ms` and `parse-ms` (~180M/wk).

## 安装 / Install

```bash
cap add pretty-ms
```

## API 参考 / API Reference

| 符号 / Symbol | 说明 / Description |
| --- | --- |
| `ParsedMs` | `{ negative, days, hours, minutes, seconds, milliseconds }` |
| `PrettyMsOptions` | `{ verbose, compact, colon_notation, separate_ms, unit_count }` + `defaults()` |
| `parseMs(ms)` | 将有符号毫秒精确分解为天/时/分/秒/毫秒 |
| `prettyMs(ms, out)` | 默认紧凑风格格式化（如 `"15d 11h 23m 20s"`、`"1.3s"`、`"42ms"`） |
| `prettyMsWith(ms, opts, out)` | 支持 `verbose` 单复数、`compact`、`colon_notation`、`separate_ms` 与 `unit_count` |

## 使用示例 / Usage

```cp
import { parseMs, prettyMs, prettyMsWith, PrettyMsOptions } from "pretty-ms";

let buf: [128]u8 = undefined;
const s1 = prettyMs(1337000000, buf[0..128]); // "15d 11h 23m 20s"
const s2 = prettyMs(1337, buf[0..128]);       // "1.3s"

let opts = PrettyMsOptions.defaults();
opts.colon_notation = true;
const clk = prettyMsWith(3723000, opts, buf[0..128]); // "1:02:03"
```

## License

MIT
