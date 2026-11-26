# @cap/optionator

Zero-heap CLI option specification and auto-aligned `--help` documentation generator in pure CP.

## API

- `OptionSpec { long_name: string, short_alias: string, value_type: string, description: string, default_val: string, required: bool }`
- `formatOptionRow(spec: OptionSpec, col_width: usize, out: []u8): usize`
- `generateHelp(program: string, summary: string, specs: []const OptionSpec, out: []u8): usize`
- `findOptionSpec(specs: []const OptionSpec, flag: string): OptionSpec?`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`optionator@0.9.4`)
- **Upstream Repository**: https://github.com/gkz/optionator
- **License**: MIT (clean-room CP implementation)
