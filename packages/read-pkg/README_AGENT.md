# read-pkg@1.0.0 (Agent Quickref)

> Zero-allocation package.json / cap.json parser and normalizer for CP, reusing normalize-package-data. | License: MIT
> Import: `import { ... } from "read-pkg";` | Full types: `src/index.d.cp`
> Deps: `normalize-package-data`

## Functions
- `export fn rpkgIsSpace(ch: u8): bool;`
- `export fn rpkgStrEq(a: string, b: string): bool;`
- `export fn rpkgStartsWithAt(s: string, pos: usize, kw: string): bool;`
- `export fn rpkgIsValueBoundary(s: string, pos: usize): bool;`
- `export fn rpkgSkipWs(s: string, pos: usize): usize;`
- `export fn rpkgFindStringEnd(s: string, open_quote: usize): usize?;`
- `export fn rpkgSkipValue(s: string, pos: usize): usize?;`
- `export fn rpkgValidateTopObject(json_text: string): bool;`
- `export fn rpkgFindTopLevelValue(json_text: string, key: string): usize?;`
- `export fn extractJsonStringField(json_text: string, key: string): string?;`
- `export fn extractJsonBoolField(json_text: string, key: string): bool?;`
- `export fn rpkgExtractRepository(json_text: string): string;`
- `export fn parsePackageJson(json_text: string, normalize: bool): ReadPkgResult;`
