[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/stylis

Zero-heap CSS minifier and nested selector expansion preprocessor in pure CP.

## API

- `minifyCss(css, out): usize` — strips `/* ... */` and `//` comments, collapses redundant whitespace, preserves quoted strings and required selector/value spaces
- `expandNestedSelector(parent, child, out): usize` — expands `&` placeholders (e.g. `.btn` + `&:hover` → `.btn:hover`, `.card` + `&--active` → `.card--active`) or joins as descendant selector (`.card .title`)
- `compileNestedCss(parent_sel, body_css, out): usize` — flattens base declarations and single-level nested child blocks into standard minified CSS

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`stylis@4.3.6`)
- **Upstream Repository**: https://github.com/thysultan/stylis
- **License**: MIT (clean-room CP implementation)
