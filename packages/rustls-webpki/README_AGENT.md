# rustls-webpki@1.0.0 (Agent Quickref)

> Pure CP zero-allocation WebPKI X.509 certificate verification engine (RFC 6125 DNS hostname matching, RFC 5280 validity & chain verification; reuses x509-cert) | License: MIT
> Import: `import { ... } from "rustls-webpki";` | Full types: `src/index.d.cp`
> Deps: `x509-cert`

## Functions
- `export fn verifyDnsHostnameBytes(pattern: &[]u8, hostname: &[]u8): bool;`
- `export fn verifyDnsHostname(pattern: string, hostname: string): bool;`
- `export fn parseX509TimeUtc(time_bytes: &[]u8): X509TimeResult;`
- `export fn verifyValidityWindow(validity: X509Validity, now_unix: u64): i32;`
- `export fn verifyValidityAt(cert: X509Certificate, now_unix: u64): i32;`
- `export fn verifyCertForDnsName(cert: X509Certificate, hostname: string, now_unix: u64): i32;`
- `export fn verifyDerCertForDnsName(der: &[]u8, hostname: string, now_unix: u64): i32;`
- `export fn verifyChainLink(leaf: X509Certificate, issuer: X509Certificate): bool;`
- `export fn verifyTrustAnchor(anchor: X509Certificate, now_unix: u64): i32;`
- `export fn verifyEndEntityChain(leaf: X509Certificate, issuer: X509Certificate, hostname: string, now_unix: u64): i32;`
