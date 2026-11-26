# @cap/damerau-levenshtein

Zero-heap Damerau-Levenshtein (Optimal String Alignment) edit distance and similarity calculator in pure CP supporting insertions, deletions, substitutions, and adjacent transpositions.

## API

- `damerauLevenshteinSteps(a: string, b: string): usize`
- `damerauLevenshtein(a: string, b: string): DamLevResult` (`{ steps, relative, similarity }`)

<!-- cap-source-attribution -->
## Source & License Attribution

- **Upstream Ecosystem**: npm (`damerau-levenshtein@1.0.8`)
- **Upstream Repository**: https://github.com/tad-lispy/node-damerau-levenshtein
- **License**: MIT (clean-room CP implementation)
