# `cap:docker` — OCI / Docker Image Reference & Dockerfile Parser

Zero-allocation, pure-CP OCI/Docker image reference parser and Dockerfile instruction streaming parser.

## Module Layout

| File | Source | Responsibilities |
| :--- | :--- | :--- |
| `lib/docker/imageref.cp` | `// Source: Absorbed from /data/dev/bun/src/cli.zig (OCI / Docker Image Reference Barrel)` | `isValidOciDigest`, `DockerImageRef` (registry, repository, tag, digest parsing & normalization) |
| `lib/docker/dockerfile.cp` | `// Source: Absorbed from /data/dev/bun/src/cli.zig (Dockerfile Instruction Parser)` | `DockerInstruction`, `FromSpec`, `ExposePort`, `DockerfileIter`, `parseFromArgs`, `parseExposeToken` |
| `lib/docker/index.cp` | `// Source: Absorbed from /data/dev/bun/src/cli.zig` | Unified `cap:docker` exports |

## Exports

- Image reference: `isValidOciDigest`, `DockerImageRef`
- Dockerfile parser: `DockerInstruction`, `FromSpec`, `ExposePort`, `DockerfileIter`, `parseFromArgs`, `parseExposeToken`
