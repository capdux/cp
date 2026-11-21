# `cap:ssh_pubkey` — OpenSSH Public Key & `authorized_keys` Parser

Zero-allocation OpenSSH public key and `authorized_keys` file parser supporting `ssh-ed25519`, `ssh-rsa`, `ecdsa-sha2-nistp{256,384,521}`, FIDO2 `sk-*` security keys, quoted options, Base64 validation, and comments.

## Exports

- `SshKeyType` & `parseKeyType(alg): SshKeyType`
- `SshPubKey` — `parseLine(raw_line): SshPubKey?`, `isSecurityKey(): bool`
- `AuthorizedKeysIter` — `init(content)`, `next(*_): SshPubKey?`
