[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# slash

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-allocation Windows backslash <-> POSIX forward-slash path normalizer preserving extended-length (\\?\) paths for CP.

## 安装与导入

```bash
cap add slash
```

```cp
import { isExtendedLengthPath } from "slash";
```

## 核心导出 API

- `export fn isExtendedLengthPath(path: string): bool;`
- `export fn slash(path: string, out: []u8): usize;`
- `export fn toBackslash(path: string, out: []u8): usize;`
- `export fn countSeparators(path: string): usize;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`slash@1.0.0`)，许可证：MIT
