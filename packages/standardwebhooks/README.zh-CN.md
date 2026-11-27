[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 快速参考](README_AGENT.md)

# standardwebhooks

纯 CP 实现的零堆分配 **Standard Webhooks**、**Stripe（`Stripe-Signature`）** 与 **GitHub（`X-Hub-Signature-256`）** 签名头解析器、重放时间窗口守卫、签名载荷格式化器与恒定时间校验库。

## 源包溯源与零成本迁移

| 字段 | 说明 |
| :--- | :--- |
| **源平台** | [npm](https://www.npmjs.com/package/standardwebhooks) & [crates.io](https://crates.io/crates/standardwebhooks) |
| **原包名与版本** | `standardwebhooks`（v1 对齐） |
| **上游仓库** | https://github.com/standard-webhooks/standard-webhooks |
| **CP 包名与版本** | `standardwebhooks`（`1.0.0`） |
| **开源协议** | MIT |

在 CP 中保持原包名直接导入，无需改名：

```typescript
// TypeScript (npm)
import { Webhook } from "standardwebhooks";

// CP (cap)
import {
    StripeSig,
    MAX_V1_SIGS,
    parseStripeHeader,
    parseGitHubSig256,
    parseStandardWebhookSig,
    isWithinTolerance,
    constantTimeEq,
    verifyStripeExpected,
    formatStripeSignedPayload,
} from "standardwebhooks";
```

## 安装

```bash
cap add standardwebhooks
```

## 核心特性与安全亮点

- **零堆分配**：解析结果直接存入栈结构 `StripeSig`（支持最多 `MAX_V1_SIGS = 4` 个密钥轮换 `v1` 签名）或调用方切片。
- **恒定时间校验**：`constantTimeEq` 与 `verifyStripeExpected` 基于 `cap:bin.ctEqual` 执行恒定时间比较，杜绝时序侧信道。
- **防重放时间窗口**：`isWithinTolerance` 同时校验过期与未来时钟偏移（`|now - ts| <= tolerance`）。
- **多平台 Webhook 覆盖**：一套轻量化 API 同时支持 Stripe、GitHub `sha256=` 与 Standard Webhooks `v1,<b64>` 轮换签名列表。

## 快速上手

```typescript
import {
    parseStripeHeader,
    formatStripeSignedPayload,
    verifyStripeExpected,
    parseGitHubSig256,
    parseStandardWebhookSig,
    isWithinTolerance,
    constantTimeEq,
} from "standardwebhooks";

fn verifyStripe(header: string, raw_body: string, now_secs: i64, expected_hex: string): bool {
    const parsed = parseStripeHeader(header);
    if (!parsed.valid) {
        return false;
    }

    let signed_buf: [2048]u8 = undefined;
    const n = formatStripeSignedPayload(parsed.timestamp, raw_body, signed_buf[0..]);
    if (n == 0) {
        return false;
    }

    return verifyStripeExpected(header, expected_hex, now_secs, 300);
}

fn verifyGitHub(header: string, expected_hex: string): bool {
    const sig = parseGitHubSig256(header);
    return sig.len > 0 && constantTimeEq(sig, expected_hex);
}
```

## API 速览

| 导出符号 | 签名 / 值 | 说明 |
| :--- | :--- | :--- |
| `MAX_V1_SIGS` | `4` | `StripeSig` 支持的最大轮换 `v1` 签名数 |
| `parseStripeHeader` | `(header: string) -> StripeSig` | 解析 Stripe `t=...,v1=...` 签名头 |
| `formatStripeSignedPayload` | `(timestamp_secs: i64, payload: string, out: []u8) -> usize` | 生成 `"<ts>.<payload>"` 待签名载荷 |
| `verifyStripeExpected` | `(header, expected_v1_hex, now_secs, tolerance_secs) -> bool` | 校验时间窗口并恒定时间比对 `v1` 签名 |
| `parseGitHubSig256` | `(header: string) -> string` | 提取 `sha256=<64hex>` 中的十六进制签名 |
| `parseStandardWebhookSig` | `(header: string, out_sigs: []string) -> usize` | 解析空格分隔的 `v1,<b64>` 签名列表 |
| `isWithinTolerance` | `(timestamp_secs, now_secs, tolerance_secs) -> bool` | 校验时间戳是否在允许误差窗口内 |
| `constantTimeEq` | `(a: string, b: string) -> bool` | 恒定时间字符串比对 |

## 开源协议

MIT
