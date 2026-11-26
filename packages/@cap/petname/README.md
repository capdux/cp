# @cap/petname

Zero-allocation deterministic human-readable name generator for CP — clean-room MIT parity with npm `unique-names-generator` / `human-id` and Rust `petname` / `names`.

## Install

```bash
cap add @cap/petname
```

## Features

- **Deterministic SplitMix64 PRNG**: identical seed always reproduces identical names.
- **DNS-Safe Slugs & Subdomains**: `generateSlug(out)` (`"swift-falcon"`) and `generateSubdomain(out)` (`"amber-otter-3f9a"` — ideal for `*.capdux.run` sandboxes and container IDs).
- **Flexible Word Count & Separators**: 1-word, 2-word (`adj-noun`), 3-word (`adv-adj-noun`), or N-word combinations with custom separators.

## Usage

```cp
import { Petname, petnameFromSeed, subdomainFromSeed } from "@cap/petname";

let buf: [64]u8 = undefined;
const slug = petnameFromSeed(42, 2, "-", buf[0..64]);
const host = subdomainFromSeed(42, buf[0..64]);
```
