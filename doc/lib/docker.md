# `cap:docker` — OCI / Docker 镜像引用与 Dockerfile 指令流式解析库

> 参考来源：/data/dev/bun/docs/guides/ecosystem/docker.mdx, /data/dev/bun/src/cli/cli.zig | 吸收日期：2026-11-22

## 概述

`cap:docker` 吸收自 Bun CLI 与容器化部署工具生态（`docs/guides/ecosystem/docker.mdx`、`src/cli/cli.zig`），提供纯 CP、零外部依赖、零堆分配的 OCI / Docker 镜像引用（Image Reference）解析规范化与 Dockerfile 指令流式解析能力，适用于容器构建工具、镜像安全扫描、依赖审计与部署编排器。

## 规范契约与边界防御

- **OCI 镜像引用解析规则（`imageref.cp`）**：
  - 完整解析 `[registry/]repository[:tag][@digest]` 四段结构，自动区分带点号/端口/`localhost` 的自定义仓库域（如 `ghcr.io/org/app:v1`、`localhost:5000/svc`）与默认 Docker Hub 命名空间；
  - `isValidOciDigest` 严格校验 `<algorithm>:<hex>` 摘要格式（如 `sha256:` 后跟 64 位小写十六进制字符），拒绝非法算法名、截断或非十六进制摘要。
- **Dockerfile 指令流式解析（`dockerfile.cp`）**：
  - `DockerfileIter` 按行流式产出 `DockerInstruction`，自动跳过空行与 `#` 注释、支持反斜杠续行（`\`），不依赖堆分配；
  - `parseFromArgs` 解析多阶段构建 `FROM [--platform=...] <image> [AS <stage>]` 参数；
  - `parseExposeToken` 解析 `EXPOSE 8080` / `8080/tcp` / `53/udp` 端口与传输协议，拒绝端口 `0`、超范围端口（`> 65535`）与未知协议。

## 模块结构与导出清单

| 文件 | 导出符号 |
| :--- | :--- |
| `lib/docker/imageref.cp` | `isValidOciDigest`, `DockerImageRef`（registry / repository / tag / digest 解析与规范化） |
| `lib/docker/dockerfile.cp` | `DockerInstruction`, `FromSpec`, `ExposePort`, `DockerfileIter`, `parseFromArgs`, `parseExposeToken` |

## 使用示例

```cp
import { DockerImageRef, isValidOciDigest, DockerfileIter, parseFromArgs, parseExposeToken } from "cap:docker";

fn demo() {
    if (ref := DockerImageRef.parse("ghcr.io/oven-sh/bun:1.3.14")) {
        * = ref;
    }
    const ok_digest = isValidOciDigest("sha256:0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef");
    let it = DockerfileIter.init("FROM oven/bun:1 AS builder\nEXPOSE 3000/tcp\n");
    while (inst := it.next()) {
        * = inst;
    }
    * = ok_digest;
    * = parseFromArgs;
    * = parseExposeToken;
}
```
