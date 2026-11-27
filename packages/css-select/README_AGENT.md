# css-select@1.0.0 (Agent Quickref)

> Zero-heap CSS selector parser, W3C Selectors Level 4 specificity calculator, and compound/list element matcher (clean-room port of css-select + css-what) | License: MIT
> Import: `import { ... } from "css-select";` | Full types: `src/index.d.cp`
> Deps: `unicase`

## Functions
- `export fn csssIsCombinator(c: u8): bool;`
- `export fn csssIsIdentStart(c: u8): bool;`
- `export fn csssIsIdentChar(c: u8): bool;`
- `export fn csssEqMaybeCi(a: string, b: string, ci: bool): bool;`
- `export fn csssSkipIdent(s: string, pos: usize): usize;`
- `export fn csssFindBracketEnd(s: string, pos: usize): usize;`
- `export fn csssSkipParen(s: string, pos: usize): usize;`
- `export fn csssHasToken(list: string, target: string, ci: bool): bool;`
- `export fn csssContains(haystack: string, needle: string, ci: bool): bool;`
- `export fn csssIsLegacyPseudoElement(name: string): bool;`
- `export fn csssComputeSingleSpecificity(s: string, depth: u32): Specificity;`
- `export fn csssComputeListSpecificity(selector: string, depth: u32): Specificity;`
- `export fn computeSpecificity(selector: string): Specificity;`
- `export fn csssMatchAttr(raw_inner: string, el: CssElement): bool;`
- `export fn matchesCompoundSelector(selector: string, el: CssElement): bool;`
- `export fn matchesSelectorList(selector_list: string, el: CssElement): bool;`
- `export fn countSelectorParts(selector: string): usize;`
