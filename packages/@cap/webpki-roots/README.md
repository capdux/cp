# `@cap/webpki-roots`

Zero-allocation Mozilla NSS / WebPKI root CA trust anchor catalog and dynamic trust store in pure CP (`webpki-roots` parity). Reuses `@cap/x509-cert` (`X509Certificate`, `X509Name`) and `@cap/spki` (`encodeSpkiDer`, `fingerprintSpki`).

## Constants

- `MOZILLA_ROOT_COUNT` (`20`) — Built-in authoritative Mozilla NSS / WebPKI root CA entries (ISRG Root X1/X2, DigiCert Global Root G2/G3, GlobalSign Root CA/R46, USERTrust RSA, Sectigo R46, Baltimore CyberTrust, Google Trust Services GTS Root R1/R4, Amazon Root CA 1/2, SSL.com, Entrust G2, QuoVadis, COMODO, Starfield G2, GoDaddy G2, Cloudflare Origin ECC)
- `TRUST_STORE_CAPACITY` (`32`) — Fixed-capacity zero-allocation store limit
- Key algorithm tags: `KEY_ALG_UNKNOWN` (`0`), `KEY_ALG_RSA` (`1`), `KEY_ALG_P256` (`2`), `KEY_ALG_P384` (`3`), `KEY_ALG_ED25519` (`4`)

## API

- `TrustAnchorInfo` — Root CA metadata (`name`, `org`, `country`, `key_alg`, `spki_fingerprint`, `not_after_year`, plus `isValidAtYear(year)` and `matchesIssuer(issuer: X509Name)`)
- `TrustAnchorStore.withMozillaDefaults()` / `TrustAnchorStore.empty()` — Initializes store with 20 default roots or empty
- `store.addAnchor(anchor: TrustAnchorInfo): bool` — Appends custom/private root CA with capacity checking
- `store.count(): usize` / `store.at(idx: usize): TrustAnchorInfo`
- `store.findByCn(cn: string): TrustAnchorResult` — Exact Common Name lookup
- `store.findByOrg(org: string): TrustAnchorResult` / `store.countByOrg(org: string): usize` — Organization lookup
- `store.isTrustedIssuer(cert: X509Certificate): bool` — Checks if an X.509 certificate's issuer matches any trust anchor
- `store.verifyIssuerTrusted(cert: X509Certificate, current_year: u16): bool` — Verifies trusted issuer and checks anchor + certificate expiration year
- `mozillaRootAt(idx: usize): TrustAnchorInfo` / `lookupMozillaRootByCn(cn: string): TrustAnchorResult`
- `certSpkiFingerprint(cert: X509Certificate): u64` — Computes 64-bit SPKI FNV-1a fingerprint via `@cap/spki`
