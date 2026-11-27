[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-binary-path

> 版本：`3.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation check if a file path has a known binary file extension (covers binary-extensions)

## 安装与导入

```bash
cap add is-binary-path
```

```cp
import { extractExtension } from "is-binary-path";
```

## 核心导出 API

- `export fn extractExtension(file_path: string): string;`
- `export fn isBinaryExtension(ext: string): bool;`
- `export fn isBinaryPath(file_path: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-binary-path@3.0.0`)（https://github.com/sindresorhus/is-binary-path），许可证：MIT
