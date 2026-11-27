[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# binary-extensions

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

CP 官方生态包 binary-extensions

## 安装与导入

```bash
cap add binary-extensions
```

```cp
import { cmpLowerToAscii } from "binary-extensions";
```

## 核心导出 API

- `export fn cmpLowerToAscii(a: string, b: string): i32;`
- `export fn extensionCategory(ext: string): u8;`
- `export fn isBinaryExtension(ext: string): bool;`
- `export fn isBinaryPath(path: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`binary-extensions@1.0.0`)，许可证：MIT
