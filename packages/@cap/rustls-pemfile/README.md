<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`rustls-pemfile`](https://github.com/rustls/pemfile)（`MIT OR Apache-2.0`）（包名 `@cap/rustls-pemfile`）。

# `@cap/rustls-pemfile`

Zero-allocation streaming TLS PEM bundle reader in pure CP (`rustls-pemfile` v2 parity). Reuses `@cap/pem-rfc7468` for RFC 7468 PEM framing, label validation, and Base64 decoding without duplicating any PEM or Base64 logic.

## Item Kinds

- `PEM_ITEM_NONE` (`0`) — No item / invalid label / EOF
- `PEM_ITEM_X509_CERT` (`1`) — `CERTIFICATE`
- `PEM_ITEM_PKCS8_KEY` (`2`) — `PRIVATE KEY`
- `PEM_ITEM_PKCS1_KEY` (`3`) — `RSA PRIVATE KEY`
- `PEM_ITEM_SEC1_KEY` (`4`) — `EC PRIVATE KEY`
- `PEM_ITEM_SPKI_PUBKEY` (`5`) — `PUBLIC KEY`
- `PEM_ITEM_CRL` (`6`) — `X509 CRL`
- `PEM_ITEM_CSR` (`7`) — `CERTIFICATE REQUEST`
- `PEM_ITEM_UNKNOWN` (`8`) — Any other valid RFC 7468 PEM label

## API

- `PemItem` — Decoded item descriptor (`kind: u8`, `der_len: usize`, `ok: bool`, `eof: bool`, plus `isCert()` / `isPrivateKey()`)
- `PemBundleReader.new(input: string): PemBundleReader` — Creates a zero-allocation cursor over a multi-block PEM bundle
- `reader.nextItem(der_out: []u8): PemItem` — Skips inter-block comments/whitespace, decodes the next PEM block into `der_out` via `@cap/pem-rfc7468::decodePem`, and classifies its label
- `reader.countItems(): usize` — Counts all well-framed PEM blocks from the current cursor without decoding Base64
- `reader.countCerts(): usize` — Counts `CERTIFICATE` blocks from the current cursor without decoding Base64
- `reader.peekNextKind(): u8` — Peeks the next PEM item kind via `@cap/pem-rfc7468::decodeLabel` without advancing the cursor
- `reader.remaining(): usize` / `reader.isEof(): bool` / `reader.reset()` — Cursor state helpers
- `classifyPemLabel(label: string): u8` — Maps RFC 7468 PEM label strings to `PEM_ITEM_*` constants
- `isPrivateKeyKind(kind: u8): bool` — Checks whether an item kind is PKCS#8, PKCS#1, or SEC1
- `readFirstPrivateKey(input: string, der_out: []u8): PemItem` — Scans a mixed certificate + key bundle and decodes the first private key (`PRIVATE KEY`, `RSA PRIVATE KEY`, or `EC PRIVATE KEY`)
