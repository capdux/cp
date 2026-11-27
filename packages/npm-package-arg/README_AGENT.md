# npm-package-arg@1.0.0 (Agent Quickref)

> Zero-heap npm package name validator and package specifier (@scope/name@version) parser and formatter. | License: MIT
> Import: `import { ... } from "npm-package-arg";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidSegment(seg: string): bool;`
- `export fn isValidPackageName(name: string): bool;`
- `export fn parsePackageSpec(input: string): PackageSpec;`
- `export fn appendSlice(out: []u8, pos: usize, s: string): i64;`
- `export fn formatPackageSpec(spec: PackageSpec, out: []u8): usize;`
