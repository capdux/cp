[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# standardwebhooks

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Stripe, GitHub (X-Hub-Signature-256), and Standard Webhooks signature parser, replay window guard, and constant-time verifier in pure CP

## 安装与导入

```bash
cap add standardwebhooks
```

```cp
import { isSpace } from "standardwebhooks";
```

## 核心导出 API

- `export fn isSpace(b: u8): bool;`
- `export fn isHex(b: u8): bool;`
- `export fn trimSlice(s: string): string;`
- `export fn parseNonNegI64(s: string): i64;`
- `export fn parseStripeHeader(header: string): StripeSig;`
- `export fn parseGitHubSig256(header: string): string;`
- `export fn parseStandardWebhookSig(header: string, out_sigs: []string): usize;`
- `export fn isWithinTolerance(timestamp_secs: i64, now_secs: i64, tolerance_secs: i64): bool;`
- `export fn constantTimeEq(a: string, b: string): bool;`
- `export fn verifyStripeExpected(header: string, expected_v1_hex: string, now_secs: i64, tolerance_secs: i64): bool;`
- `export fn formatStripeSignedPayload(timestamp_secs: i64, payload: string, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`standardwebhooks@1.0.0`)，许可证：MIT
- **上游生态**：`crates.io` (`standardwebhooks@1.0.0`)，许可证：MIT
