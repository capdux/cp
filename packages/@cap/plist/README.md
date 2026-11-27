# @cap/plist

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:plist` ([https://github.com/ebarnard/rust-plist](https://github.com/ebarnard/rust-plist))

Zero-heap-allocation Apple XML Property List (`.plist`) dictionary key-value parser and XML entry serializer in pure CP.

## Features

- `PlistValKind` enum: `String`, `Integer`, `Real`, `Bool`, `Data`, `Date`, `Invalid`
- `PlistValue { kind, str_val, int_val, bool_val }`
- `plistGetDictValue(xml, key_name)`: zero-allocation extraction of `<string>`, `<integer>`, `<real>`, `<true/>`, `<false/>`, `<data>`, and `<date>` values from `<dict>` elements (with nested `<dict>`/`<array>` skipping)
- `plistWriteDictEntry(key, val, out)`: bounds-checked serialization of `<key>...</key><value>...</value>` XML fragments into caller-provided buffers

## License

MIT
