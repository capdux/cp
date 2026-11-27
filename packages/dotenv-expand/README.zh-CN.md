[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# dotenv-expand

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap .env parser and POSIX variable expansion engine (${VAR}, ${VAR:-default}, $VAR, escaped \$) (clean-room port of npm dotenv + dotenv-expand)

## 安装与导入

```bash
cap add dotenv-expand
```

```cp
import { deStrEq } from "dotenv-expand";
```

## 核心导出 API

- `export fn deStrEq(a: string, b: string): bool;`
- `export fn deIsWs(b: u8): bool;`
- `export fn deIsVarChar(b: u8): bool;`
- `export fn deTrim(s: string): string;`
- `export fn dePush(out: []u8, pos: usize, s: string): usize;`
- `export fn parseDotenv(src: string): EnvMap;`
- `export fn expandEnvValue(template: string, env: EnvMap, out: []u8): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`dotenv-expand@1.0.0`)，许可证：BSD-2-Clause
