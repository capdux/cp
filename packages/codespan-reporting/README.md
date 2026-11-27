[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# codespan-reporting

Zero-heap Rustc-style source code diagnostic renderer with 1-based line/column lookup, aligned gutter, `^^^` span underline, inline label, and `= note:` footer in pure CP.

## API

- `locateOffset(source: string, offset: usize): LineCol`
- `renderDiagnostic(source: string, diag: Diagnostic, out: []u8): usize`
- `severityLabel(sev: DiagSeverity): string`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: crates.io (`codespan-reporting@0.11.1`)
- **Upstream Repository**: https://github.com/brendanzab/codespan
- **License**: Apache-2.0 (clean-room CP implementation)
