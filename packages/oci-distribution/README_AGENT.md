# oci-distribution@1.0.0 (Agent Quickref)

> Zero-allocation OCI / Docker image reference parser, validator, and normalizer for CP. | License: MIT
> Import: `import { ... } from "oci-distribution";` | Full types: `src/index.d.cp`

## Functions
- `export fn isValidTag(tag: string): bool;`
- `export fn isValidDigest(digest: string): bool;`
- `export fn parseImageRef(input: string): ImageRef;`
- `export fn formatNormalized(ref_in: ImageRef, out: []u8): usize;`
