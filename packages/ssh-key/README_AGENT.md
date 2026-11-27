# ssh-key@1.0.0 (Agent Quickref)

> Zero-heap OpenSSH public key (ssh-ed25519, ssh-rsa, ecdsa-sha2-*, sk-ssh-*) authorized_keys parser, validator, and formatter for CP (compatible with npm sshpk & Rust ssh-key) | License: MIT
> Import: `import { ... } from "ssh-key";` | Full types: `src/index.d.cp`

## Functions
- `export fn isSpace(c: u8): bool;`
- `export fn isLineTrim(c: u8): bool;`
- `export fn strEq(a: string, b: string): bool;`
- `export fn identifyAlgorithm(key_type: string): u8;`
- `export fn isValidBase64(b64: string): bool;`
- `export fn parsePublicKey(line: string): SshPublicKey;`
- `export fn formatPublicKey(key: SshPublicKey, out: []u8): usize;`
- `export fn countAuthorizedKeys(text: string): usize;`
- `export fn isModernAlgorithm(alg: u8): bool;`
