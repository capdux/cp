[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `standardwebhooks`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:standardwebhooks` ([https://github.com/standard-webhooks/standard-webhooks](https://github.com/standard-webhooks/standard-webhooks)) / `crates.io:standardwebhooks` ([https://github.com/standard-webhooks/standard-webhooks](https://github.com/standard-webhooks/standard-webhooks))（原库名 `standardwebhooks` → CP 包名 `standardwebhooks`）

Zero-heap-allocation Webhook signature parser, replay-window guard, and constant-time verifier in pure CP.

## Supported Protocols

- **Stripe (`Stripe-Signature`)**: Multi-`v1` rotated signature parsing (`parseStripeHeader`), signed payload assembly (`formatStripeSignedPayload`), and tolerance-checked verification (`verifyStripeExpected`).
- **GitHub (`X-Hub-Signature-256`)**: Strict 64-hex `sha256=` digest extractor (`parseGitHubSig256`).
- **Standard Webhooks (`webhook-signature`)**: Multi-version `v1,<b64>` extractor (`parseStandardWebhookSig`).
- **Security Primitives**: `isWithinTolerance` replay/skew guard and `constantTimeEq`.

## API

```cp
import {
    MAX_V1_SIGS,
    StripeSig,
    parseStripeHeader,
    parseGitHubSig256,
    parseStandardWebhookSig,
    isWithinTolerance,
    constantTimeEq,
    verifyStripeExpected,
    formatStripeSignedPayload,
} from "standardwebhooks";
```

## License

MIT
