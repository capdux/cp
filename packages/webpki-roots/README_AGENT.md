# webpki-roots@1.0.0 (Agent Quickref)

> Zero-allocation Mozilla NSS / WebPKI root CA trust anchor catalog and dynamic trust store reusing @cap/x509-cert and @cap/spki (webpki-roots parity) in pure CP. | License: MIT
> Import: `import { ... } from "webpki-roots";` | Full types: `src/index.d.cp`
> Deps: `spki`, `x509-cert`

## Functions
- `export fn wrStrEq(a: string, b: string): bool;`
- `export fn wrBytesEqStr(b: &[]u8, s: string): bool;`
- `export fn wrIsDigit(ch: u8): bool;`
- `export fn wrParseCertNotAfterYear(cert: X509Certificate): u16;`
- `export fn wrMakeRoot(name: string, org: string, country: string, key_alg: u8, not_after_year: u16): TrustAnchorInfo;`
- `export fn mozillaRootAt(idx: usize): TrustAnchorInfo;`
- `export fn lookupMozillaRootByCn(cn: string): TrustAnchorResult;`
- `export fn certSpkiFingerprint(cert: X509Certificate): u64;`
- `export fn wrEmptyAnchors(): [32]TrustAnchorInfo;`
