[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# number_prefix

Zero-heap SI decimal (`k`/`M`/`G`/`T`/`P`/`E`) and IEC binary (`Ki`/`Mi`/`Gi`/`Ti`/`Pi`/`Ei`) numeric prefix calculator and formatter in pure CP.

## API

- `decimalPrefix(amount: f64): PrefixedNumber`
- `binaryPrefix(amount: f64): PrefixedNumber`
- `formatPrefixed(pn: PrefixedNumber, decimals: u8, unit: string, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`number_prefix@0.4.0`)
- **Upstream Repository**: https://github.com/ogham/rust-number-prefix
- **License**: MIT (clean-room CP implementation)
