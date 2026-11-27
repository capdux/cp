[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# salsa20

Zero-heap Salsa20/20 stream cipher (256-bit key, 64-bit nonce, 64-bit block counter) in pure CP, verified against official D.J. Bernstein / eSTREAM test vectors.

## API

- `salsa20Block(key: []const u8, nonce: []const u8, counter: u64, out: []u8)`
- `salsa20Apply(key, nonce, initial_counter, input, out): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`salsa20@0.10.2`)
- **Upstream Repository**: https://github.com/RustCrypto/stream-ciphers
- **License**: MIT (clean-room CP implementation)
