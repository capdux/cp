# rustls-pemfile (AI Agent Quick Ref)

- **Import**: `import { PemBundleReader, PemItem, readFirstPrivateKey, classifyPemLabel, isPrivateKeyKind, PEM_ITEM_NONE, PEM_ITEM_X509_CERT, PEM_ITEM_PKCS8_KEY, PEM_ITEM_PKCS1_KEY, PEM_ITEM_SEC1_KEY, PEM_ITEM_SPKI_PUBKEY, PEM_ITEM_CRL, PEM_ITEM_CSR, PEM_ITEM_UNKNOWN } from "rustls-pemfile";`
- **Source**: `crates.io:rustls-pemfile` v2 (MIT, zero-alloc, wraps `pem-rfc7468`, `undefined` scratch buffers).

## Core Signatures

```cp
export class PemItem {
    kind: u8, der_len: usize, ok: bool, eof: bool,
    fn makeEof(): PemItem;
    fn makeErr(): PemItem;
    fn isCert(): bool;
    fn isPrivateKey(): bool;
}

export class PemBundleReader {
    input: string, offset: usize,
    fn new(input: string): PemBundleReader;
    fn remaining(): usize;
    fn isEof(): bool;
    fn reset(_: auto);
    fn peekNextKind(): u8;
    fn nextItem(_: auto, der_out: []u8): PemItem;
    fn countItems(): usize;
    fn countCerts(): usize;
}

export fn classifyPemLabel(label: string): u8;
export fn isPrivateKeyKind(kind: u8): bool;
export fn readFirstPrivateKey(input: string, der_out: []u8): PemItem;
```

## Constants (`u8`)

`PEM_ITEM_NONE=0`, `PEM_ITEM_X509_CERT=1`, `PEM_ITEM_PKCS8_KEY=2`, `PEM_ITEM_PKCS1_KEY=3`, `PEM_ITEM_SEC1_KEY=4`, `PEM_ITEM_SPKI_PUBKEY=5`, `PEM_ITEM_CRL=6`, `PEM_ITEM_CSR=7`, `PEM_ITEM_UNKNOWN=8`.

## Minimal Pattern

```cp
let mut r = PemBundleReader.new(bundle);
let mut der: [4096]u8 = undefined;
while (true) {
    let it = r.nextItem(der[0..4096]);
    if (it.eof) { break; }
    if (!it.ok) { return -1; }
    let slice = der[0..it.der_len];
}
```

## Pitfalls

1. **Check `item.eof` before `!item.ok`**: At EOF after trailing comments, `nextItem` returns `eof: true, ok: false`.
2. **Mutable reader required**: `let mut r = PemBundleReader.new(...)` is required because `nextItem` and `reset` mutate `offset`.
3. **Buffer reuse**: `der_out` is overwritten on each `nextItem` call; copy out `der[0..item.der_len]` if you need to retain multiple blocks.
4. **No-decode helpers**: `countItems()`, `countCerts()`, and `peekNextKind()` do not advance `offset` and do not decode Base64.
