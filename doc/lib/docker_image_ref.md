# `cap:docker_image_ref` — OCI / Docker Image Reference Parser

Zero-allocation OCI / Docker image reference parser supporting `[registry[:port]/][namespace/]repository[:tag][@digest]`, `sha256`/`sha512` digest validation, and default Docker Hub / `latest` resolution.

## Exports

- `isValidOciDigest(digest): bool`
- `DockerImageRef` — `parse(raw): DockerImageRef?`, `effectiveRegistry()`, `effectiveTag()`, `isOfficialDockerHub()`.
