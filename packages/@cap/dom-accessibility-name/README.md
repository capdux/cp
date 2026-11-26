# @cap/dom-accessibility-name

Zero-heap W3C Accessible Name and Description Computation (AccName) library in pure CP.

## API

- `A11yNodeInput { tag: string, aria_label: string, labelled_by_text: string, inner_text: string, alt: string, title: string, placeholder: string, aria_hidden: bool }`
- `collapseWhitespace(input: string, out: []u8): usize`
- `computeAccessibleName(node: A11yNodeInput, out: []u8): usize`
- `computeAccessibleDescription(aria_desc: string, title: string, used_title_as_name: bool, out: []u8): usize`

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`dom-accessibility-api@0.6.3`)
- **Upstream Repository**: https://github.com/eps1lon/dom-accessibility-api
- **License**: MIT (clean-room CP implementation)
