[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/ecdsa-sig-formatter

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:ecdsa-sig-formatter` ([https://github.com/Brightspace/node-ecdsa-sig-formatter](https://github.com/Brightspace/node-ecdsa-sig-formatter), `Apache-2.0`)

Zero-heap-allocation ASN.1 DER ↔ JOSE IEEE P1363 (`r || s`) ECDSA signature formatter for JWT/JWS (`ES256`, `ES384`, `ES512`) in pure CP.

## Features

- `EcdsaAlg` enum (`ES256`, `ES384`, `ES512`) and `paramBytes(alg): usize` (`32` / `48` / `66`)
- `derToJose(der: []const u8, alg: EcdsaAlg, out: []u8): i64` — strictly parses DER `SEQUENCE { INTEGER r, INTEGER s }`, strips positive sign padding, left-pads `r` and `s` to `paramBytes`, and returns `-1` on malformed DER
- `joseToDer(jose: []const u8, alg: EcdsaAlg, out: []u8): i64` — converts fixed-width `2 * paramBytes` JOSE signatures to canonical DER SEQUENCE, handling leading zero trimming, high-bit `0x00` sign padding, and `0x81` long-form SEQUENCE lengths for `ES512`

## License

Apache-2.0
