[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# hosted-git-info

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap parser & URL formatter for GitHub, GitLab, and Bitbucket shorthands, SSH, and HTTPS git URLs (npm hosted-git-info equivalent)

## 安装与导入

```bash
cap add hosted-git-info
```

```cp
import { hgiPush } from "hosted-git-info";
```

## 核心导出 API

- `export fn hgiPush(out: []u8, pos: usize, s: string): usize;`
- `export fn hgiStartsWith(s: string, prefix: string): bool;`
- `export fn hgiEndsWith(s: string, suffix: string): bool;`
- `export fn hgiFindChar(s: string, ch: u8): usize?;`
- `export fn hgiBuildFromPath(host_type: u8, domain: string, path_and_hash: string): HostedGitInfo;`
- `export fn fromUrl(raw: string): HostedGitInfo;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`hosted-git-info@1.0.0`)，许可证：ISC
