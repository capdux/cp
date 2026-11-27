# cidr-tools@1.0.0 (Agent Quickref)

> Zero-allocation IPv4 CIDR parser, formatter, overlap/containment checker, merge, and exclude engine (npm cidr-tools / ip-cidr parity) for CP | License: MIT
> Import: `import { ... } from "cidr-tools";` | Full types: `src/index.d.cp`

## Functions
- `export fn parseIpv4(s: &[]u8): i64;`
- `export fn parseCidr4(s: &[]u8): Cidr4;`
- `export fn formatCidr4(c: Cidr4, dst: []u8): usize;`
- `export fn cidrHostCount(c: Cidr4): u64;`
- `export fn cidrStart(c: Cidr4): u32;`
- `export fn cidrEnd(c: Cidr4): u32;`
- `export fn containsIp(c: Cidr4, ip: u32): bool;`
- `export fn containsCidr(outer: Cidr4, inner: Cidr4): bool;`
- `export fn overlaps(a: Cidr4, b: Cidr4): bool;`
- `export fn mergeCidrs(inputs: &[]Cidr4, out: []Cidr4): usize;`
- `export fn excludeCidr(base: Cidr4, remove: Cidr4, out: []Cidr4): usize;`
