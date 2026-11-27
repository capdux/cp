[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# rustls-webpki

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:rustls-webpki` ([https://github.com/rustls/webpki](https://github.com/rustls/webpki))

Pure CP, zero-allocation WebPKI X.509 certificate verification engine (clean-room MIT parity with `rustls-webpki`, strictly following RFC 6125 & RFC 5280).

Composes `x509-cert` with zero code duplication for X.509 DER parsing, Distinguished Name comparison, validity structures, and self-signed detection.

## API

- Error codes: `WPKI_OK (0)`, `WPKI_ERR_NOT_YET_VALID (-1)`, `WPKI_ERR_EXPIRED (-2)`, `WPKI_ERR_INVALID_TIME (-3)`, `WPKI_ERR_HOSTNAME_MISMATCH (-4)`, `WPKI_ERR_INVALID_CERT (-5)`, `WPKI_ERR_CHAIN_BROKEN (-6)`, `WPKI_ERR_NOT_SELF_SIGNED (-7)`
- `verifyDnsHostname(pattern: string, hostname: string): bool` / `verifyDnsHostnameBytes(pattern: []const u8, hostname: []const u8): bool` — strict RFC 6125 case-insensitive exact & single-label leftmost wildcard matching
- `parseX509TimeUtc(time_bytes: []const u8): X509TimeResult` — RFC 5280 UTCTime (`YYMMDDHHMMSSZ`) & GeneralizedTime (`YYYYMMDDHHMMSSZ`) to Unix timestamp conversion
- `verifyValidityAt(cert: X509Certificate, now_unix: u64): i32` / `verifyValidityWindow(validity: X509Validity, now_unix: u64): i32`
- `verifyCertForDnsName(cert: X509Certificate, hostname: string, now_unix: u64): i32` / `verifyDerCertForDnsName(der: []const u8, hostname: string, now_unix: u64): i32`
- `verifyChainLink(leaf: X509Certificate, issuer: X509Certificate): bool`
- `verifyTrustAnchor(anchor: X509Certificate, now_unix: u64): i32`
- `verifyEndEntityChain(leaf: X509Certificate, issuer: X509Certificate, hostname: string, now_unix: u64): i32`

## License

MIT
