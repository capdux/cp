[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent Quickref](./README_AGENT.md)

# @cap/docker-ref

<!-- cap-source-attribution -->
> **参考 / 吸收来源**：`crates.io:oci-distribution` ([https://github.com/krustlet/oci-distribution](https://github.com/krustlet/oci-distribution)) / `npm:docker-image-parse` ([https://github.com/cp-lang/cp](https://github.com/cp-lang/cp))（原库名 `oci-distribution` → CP 包名 `@cap/docker-ref`）

Zero-allocation OCI / Docker image reference parser, validator, and canonical normalizer for CP.

## Features

- **Zero heap allocation**: zero-copy `ImageRef` slicing and caller-supplied `[]u8` normalization buffer.
- **Full OCI reference parsing**: handles `[registry/][namespace/]name[:tag][@sha256:...]`, accurately distinguishing registry port colons (`localhost:5000/app:v1`) from image tags.
- **Canonical normalization**: `formatNormalized` fills default `docker.io`, `library/` for official single-segment repos, and `:latest` when neither tag nor digest is specified.
- **Strict validation**: `isValidTag`, `isValidDigest` (`sha256:` 64-hex / `sha512:` 128-hex), and lowercase repository enforcement.

## Usage

```cp
import { parseImageRef, formatNormalized } from "@cap/docker-ref";

const img = parseImageRef("alpine:3.19");
let buf: [128]u8 = undefined;
const n = formatNormalized(img, buf[0..128]); // "docker.io/library/alpine:3.19"
```
