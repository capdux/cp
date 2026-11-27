# @cap/unicode-bidi

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:unicode-bidi` ([https://github.com/servo/unicode-bidi](https://github.com/servo/unicode-bidi)) / `npm:bidi-js` ([https://github.com/lojjic/bidi-js](https://github.com/lojjic/bidi-js))

Zero-heap-allocation Unicode Bidirectional Algorithm (UAX #9) core library in pure CP (inspired by Rust `unicode-bidi`).

## Features

- Bidi character class constants (`BIDI_L`, `BIDI_R`, `BIDI_AL`, `BIDI_EN`, `BIDI_AN`, `BIDI_WS`, `BIDI_ON`, `BIDI_B`) and `bidiClass(cp: u32): u8`
- `baseDirection(text: string): u8` (UAX #9 P2/P3 first strong character rule)
- `hasRtl(text: string): bool` and `isLtrOnly(text: string): bool`
- `computeVisualRuns(text: string, out_runs: []BidiRun): usize` (splits mixed LTR/RTL text into contiguous same-level runs)
- `mirrorAsciiBracket(ch: u8): u8` (UAX #9 L4 bracket mirroring)

## License

MIT
