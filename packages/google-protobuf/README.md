[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/prost-wire

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:google-protobuf` ([https://github.com/protocolbuffers/protobuf-javascript](https://github.com/protocolbuffers/protobuf-javascript)) / `crates.io:prost` ([https://github.com/tokio-rs/prost](https://github.com/tokio-rs/prost)) / `npm:pbf` ([https://github.com/mapbox/pbf](https://github.com/mapbox/pbf))（原库名 `google-protobuf` → CP 包名 `@cap/prost-wire`）

Zero-heap-allocation Protocol Buffers Wire Format encoder/decoder in pure CP (inspired by Rust `prost`), directly reusing `@cap/varint` (ULEB128 + ZigZag).

## Dependencies

- `@cap/varint` (`^1.0.0`)

## Features

- Wire type constants: `WIRE_VARINT (0)`, `WIRE_I64 (1)`, `WIRE_LEN (2)`, `WIRE_I32 (5)`
- Tag codec: `encodeTag`, `decodeTag` (with field number range `1..2^29-1` & wire type validation)
- Field encoders/decoders: `encodeVarintField`, `encodeSint64Field`, `decodeVarintValue`, `decodeSint64Value`, `encodeFixed32Field`, `decodeFixed32`, `encodeFixed64Field`, `decodeFixed64`, `encodeBytesField`, `decodeBytesField`
- Unknown field skipping: `skipField(buf, pos, wire_type)`

## License

MIT
