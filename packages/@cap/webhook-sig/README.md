# `@cap/webhook-sig`

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
} from "@cap/webhook-sig";
```

## License

MIT
