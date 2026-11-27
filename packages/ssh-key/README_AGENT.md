# ssh-key — AI Agent Quick Ref

Zero-heap OpenSSH public key (ssh-ed25519, ssh-rsa, ecdsa-sha2-*, sk-ssh-*) authorized_keys parser, validator, and formatter for CP (compatible with npm sshpk & Rust ssh-key) (`v1.0.0`, `MIT`)

## Import

```cp
import {
    SshPublicKey,
    isHspace,
    identifyAlgorithm,
    isValidBase64,
    parsePublicKey,
    formatPublicKey,
    countAuthorizedKeys,
    isModernAlgorithm,
} from "ssh-key";
```

## Core Types & Constants

```cp
class SshPublicKey { alg: u8, key_type: string, key_b64: string, comment: string, /* +1 fields */ }
```

## API Signatures

```cp
fn isHspace(c: u8): bool;
fn identifyAlgorithm(key_type: string): u8;
fn isValidBase64(b64: string): bool;
fn parsePublicKey(line: string): SshPublicKey;
fn formatPublicKey(key: SshPublicKey, out: []u8): usize;
fn countAuthorizedKeys(text: string): usize;
fn isModernAlgorithm(alg: u8): bool;
```

## Minimal Usage

```cp
import { isValidBase64 } from "ssh-key";

pub fn main() {
    let result = isValidBase64("example");
    _ = result;
}
```

## Borrowing, Errors & Pitfalls

- **Canonical import**: Always import directly from `"ssh-key"`; install via `cap add ssh-key`.
- **Caller-owned buffers**: Pre-allocate stack buffers (`let mut buf: [N]u8 = undefined;`) and pass slices `buf[0..N]` to `[]u8` parameters.
- **Zero-copy lifetimes**: Returned `string` slices may borrow from input arguments; keep source strings alive while reading results.
