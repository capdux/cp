[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# `metaphone`

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:metaphone` ([https://github.com/words/metaphone](https://github.com/words/metaphone))

Zero-heap-allocation Lawrence Philips Metaphone English phonetic encoder in pure CP.

## Features

- **`metaphone`**: Encodes English words into phonetic keys handling silent initial pairs (`KN`/`GN`/`PN`/`AE`/`WR`), `PH -> F`, `SH`/`CH`/`TIA`/`TIO -> X`, `TH -> 0`, `WH -> W`, `CK -> K`, silent `GH`, and initial-only vowels.
- **`soundsLike`**: Fast 4-character phonetic equality predicate.
- **Zero Heap Allocation**: Pure CP, stack-only, no FFI.

## API

```cp
import { metaphone, soundsLike } from "metaphone";
```

## License

MIT
