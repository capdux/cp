# @cap/ssh-key

Zero-heap OpenSSH public key (`authorized_keys`) parser, algorithm classifier, Base64 validator, and formatter for CP.
Compatible with npm `sshpk` and Rust `ssh-key`.

## API

- Algorithms: `ALG_ED25519`, `ALG_RSA`, `ALG_ECDSA_P256`, `ALG_ECDSA_P384`, `ALG_ECDSA_P521`, `ALG_SK_ED25519`, `ALG_SK_ECDSA_P256`
- `SshPublicKey`, `parsePublicKey`, `formatPublicKey`, `identifyAlgorithm`, `isValidBase64`, `countAuthorizedKeys`, `isModernAlgorithm`

## License

MIT
