// Source: cp/lib/ssh/index.cp (Unified Exports for cap:ssh)
// Pure CP zero-C-dependency OpenSSH public key parser, SHA256 fingerprint & authorized_keys engine.

export {
    SshError,
    SshKeyType,
    SshPublicKey,
    AuthorizedKeyEntry,
    AuthorizedKeys,
} from "./keys.cp";

export {
    SshHostConfig,
    matchHostPatterns,
    resolveSshConfig,
    getSshOption,
} from "./config.cp";

export {
    SshKeyType as PubKeyType,
    parseKeyType,
    SshPubKey,
    AuthorizedKeysIter,
} from "./pubkey.cp";
