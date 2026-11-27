# @cap/figures

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`npm:figures` ([https://github.com/sindresorhus/figures](https://github.com/sindresorhus/figures), `MIT`)

Zero-heap-allocation Unicode terminal symbols with automatic fallback strings in pure CP, modeled after Sindre Sorhus's `figures` v6 and composing `@cap/is-unicode-supported`.

## Features

- `FigureKind` enum covering 20 common CLI symbols (`Tick`, `Cross`, `Star`, `Square`, `Circle`, `Info`, `Warning`, `Pointer`, `ArrowUp`, `ArrowDown`, `ArrowLeft`, `ArrowRight`, `Ellipsis`, `Bullet`, `Heart`, `RadioOn`, `RadioOff`, `CheckboxOn`, `CheckboxOff`, `Line`)
- `figureFor(kind: FigureKind, unicode_ok: bool): string` — returns Unicode or fallback symbol
- `figureForEnv(kind: FigureKind, env: EnvInfo): string` — detects terminal capability via `@cap/is-unicode-supported`
- `replaceSymbols(input: string, unicode_ok: bool, out: []u8): usize` — replaces Unicode figures in text with fallbacks when `unicode_ok` is `false`

## License

MIT
