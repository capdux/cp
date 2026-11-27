[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI 智能体速查](./README_AGENT.md)

# mint

> 版本：`1.0.0` | 许可证：`MIT` | 类型声明：[src/index.d.cp](./src/index.d.cp)

Zero-heap Math Interoperability Standard Types: Point2/3, Vector2/3/4, Quaternion, RowMatrix2/3/4, ColumnMatrix4, EulerAngles (crates.io mint equivalent)

## 安装与导入

```bash
cap add mint
```

```cp
import { point3ToVector3 } from "mint";
```

## 核心导出 API

- `export fn point3ToVector3(p: Point3): Vector3;`
- `export fn vector3ToPoint3(v: Vector3): Point3;`

<!-- cap-source-attribution -->
## 上游来源与许可证说明

- **上游生态**：`crates.io` (`mint@1.0.0`)，许可证：MIT
