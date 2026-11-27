# ssri@1.0.0 (Agent Quickref)

> Zero-allocation W3C Subresource Integrity (SRI) parser, validator, constant-time matcher, and formatter for CP | License: MIT
> Import: `import { ... } from "ssri";` | Full types: `src/index.d.cp`

## Types & Constants
- `export enum SriAlgo`

## Functions
- `export fn algoName(algo: SriAlgo): string;`
- `export fn algoStrength(algo: SriAlgo): u8;`
- `export fn parseIntegrity(raw: string): SriIntegrity;`
- `export fn strongestHash(sri: SriIntegrity): SriHash?;`
- `export fn pickAlgorithm(sri: SriIntegrity, algo: SriAlgo): SriHash?;`
- `export fn matchesIntegrity(expected: SriIntegrity, actual: SriIntegrity): bool;`
- `export fn formatHash(algo: SriAlgo, base64_digest: string, out: []u8): string;`
- `export fn formatIntegrity(sri: SriIntegrity, out: []u8): string;`
