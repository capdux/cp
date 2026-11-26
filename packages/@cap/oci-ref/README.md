<!-- cap-source-attribution -->
> **参考 / 吸收来源**：吸收自 **crates.io** [`oci-distribution`](https://github.com/oras-project/rust-oci-distribution)（`Apache-2.0`）（原库名 `oci-distribution`，CP 包名 `@cap/oci-ref`）。

# @cap/oci-ref

Zero-heap OCI / Docker container image reference parser, validator, and canonicalizer in pure CP.

Supports Docker Hub bare names (`nginx` → `docker.io/library/nginx:latest`), custom registries with ports (`localhost:5000/app:dev`), multi-segment namespaces (`ghcr.io/capdux/caprun/worker:v1.2`), and immutable `@sha256:...` digests.

## Install

```bash
cap add @cap/oci-ref
```

## Usage

```cp
import { parseOciRef, formatCanonical, isPinned } from "@cap/oci-ref";

const ref = parseOciRef("ghcr.io/capdux/runner:v1.0.0");
// ref.registry   == "ghcr.io"
// ref.repository == "capdux/runner"
// ref.tag        == "v1.0.0"

let buf: [256]u8 = undefined;
const canonical = formatCanonical(parseOciRef("alpine"), buf[0..256]);
// "docker.io/library/alpine:latest"
```

## API

| Function | Description |
| --- | --- |
| `parseOciRef(raw)` | Parse OCI/Docker reference into `OciRef` with zero heap allocation |
| `formatCanonical(r, out)` | Format into canonical `registry/repository[:tag][@digest]` |
| `isPinned(r)` | Check whether reference includes an immutable digest |
