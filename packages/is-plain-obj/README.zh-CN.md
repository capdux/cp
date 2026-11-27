[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# is-plain-obj

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap plain object metadata checker, JSON object literal validator, and prototype-pollution key guard (clean-room port of npm is-plain-obj)

## 安装与导入

```bash
cap add is-plain-obj
```

```cp
import { ipoIsSpace } from "is-plain-obj";
```

## 核心导出 API

- `export fn ipoIsSpace(ch: u8): bool;`
- `export fn ipoSliceEq(s: string, start: usize, end: usize, target: string): bool;`
- `export fn ipoIsDangerousSlice(s: string, start: usize, end: usize): bool;`
- `export fn isPlainObjectMeta(m: ValueMeta): bool;`
- `export fn isPlainJsonObject(json_text: string): bool;`
- `export fn isPlainObjectKey(key: string): bool;`
- `export fn hasDangerousProtoKey(json_text: string): bool;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`npm` (`is-plain-obj@1.0.0`)，许可证：MIT
