# normalize-package-data@1.0.0 (Agent Quickref)

> Zero-allocation package metadata normalizer and validator for CP, reusing @cap/validate-npm-package-name and @cap/validate-npm-package-license. | License: MIT
> Import: `import { ... } from "normalize-package-data";` | Full types: `src/index.d.cp`
> Deps: `validate-npm-package-license`, `validate-npm-package-name`

## Functions
- `export fn npdIsSpace(ch: u8): bool;`
- `export fn npdTrim(s: string): string;`
- `export fn npdStartsWith(s: string, prefix: string): bool;`
- `export fn npdEndsWith(s: string, suffix: string): bool;`
- `export fn npdPush(out: []u8, pos: usize, s: string): usize;`
- `export fn npdIsOwnerRepoShorthand(s: string): bool;`
- `export fn npdWriteHostedUrl(base_url: string, slug: string, out: []u8): usize;`
- `export fn expandRepoShorthand(repo: string, out: []u8): usize;`
- `export fn normalizeEntryPath(entry: string): string;`
- `export fn cleanVersionString(ver: string): string;`
- `export fn normalizePackageData(meta: PackageMetadata): NormalizedPackageResult;`
