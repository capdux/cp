[English](README.md) | [简体中文](README.zh-CN.md) | [AI Agent 速查手册](README_AGENT.md)

# schema-utils

面向 CP 的零堆内存配置项 Schema 校验器（`validate` / `ObjectSchema` / `SchemaProperty` / `SchemaValue`），内置零分配模式匹配引擎（`matchPattern`）、独立字段校验函数以及可读错误报告格式化器。

## 源包溯源与零成本迁移

| 属性 | 说明 |
| --- | --- |
| **上游源平台** | [npmjs.com/package/schema-utils](https://www.npmjs.com/package/schema-utils) |
| **上游原包名** | `schema-utils` v4.3.0 |
| **上游仓库** | https://github.com/webpack/schema-utils |
| **许可证** | MIT |

从 TypeScript/npm 迁移到 CP 保持原包名不变：

```ts
// TypeScript (npm)
import { validate } from "schema-utils";
validate(schema, options, { name: "MyPlugin" });
```

```cp
// CP —— 原包名直接导入，零堆配置 Schema 校验与报告生成
import {
    ObjectSchema, SchemaProperty, SchemaValue,
    validate, formatValidationReport,
} from "schema-utils";
```

## 安装

```bash
cap add schema-utils
```

## 实战示例

### 1. 对象配置 Schema 校验与报告输出（`validate` & `formatValidationReport`）

```cp
import {
    ObjectSchema, SchemaProperty, SchemaValue,
    validate, formatValidationReport, formatValidationError,
} from "schema-utils";

fn validateConfig() {
    let props: [3]SchemaProperty = [
        SchemaProperty.stringProp("host", true, 1, 64, "^[a-zA-Z0-9.-]+$"),
        SchemaProperty.intProp("port", true, true, 1, true, 65535),
        SchemaProperty.boolProp("tls", false),
    ];

    let schema = ObjectSchema {
        title: "ServerOptions",
        properties: &props,
        additional_properties: false,
    };

    let values: [3]SchemaValue = [
        SchemaValue.str("host", "api.example.com"),
        SchemaValue.int("port", 8080),
        SchemaValue.boolean("tls", true),
    ];

    let report = validate(schema, &values);
    if (!report.valid) {
        mut out: [512]u8 = [0; 512];
        let written = formatValidationReport("ServerOptions", report, &mut out);
        // out[0..written] 为格式化后的多条错误诊断文本
    }
}
```

### 2. 独立字段与模式校验函数

```cp
import {
    FieldConstraint, validateStringField, validateIntField,
    validateEnumField, validatePatternField, matchPattern,
    SchemaType, schemaTypeName,
} from "schema-utils";

fn demoFieldValidators() {
    let c = FieldConstraint {
        name: "username",
        required: true,
        min_len: 3,
        max_len: 20,
        has_int_range: false,
        min_int: 0,
        max_int: 0,
    };
    let r1 = validateStringField("alice", true, c); // r1.valid == true

    let allowed: [3]string = ["dev", "staging", "prod"];
    let r2 = validateEnumField("prod", &allowed);   // r2.valid == true

    let ok = matchPattern("^[a-z]+-[0-9]+$", "worker-42"); // true
    let r3 = validatePatternField("worker-42", "^[a-z]+-[0-9]+$");

    let tname = schemaTypeName(SchemaType.Integer); // "integer"
}
```

## 性能与安全亮点

- **零堆分配**：`ValidationReport` 在栈上内联保存最多 16 条 `ValidationError`，格式化输出直接写入调用方 `[]u8` 缓冲区。
- **内置轻量模式引擎**：`matchPattern` 零依赖支持起止锚点（`^`/`$`）、字符类（`[a-z0-9]`）与量词匹配。
- **严格多余字段拦截**：配置 `additional_properties: false` 可自动检测未声明的未知配置键。
- **单遍多错误聚合**：`validate` 一次性汇总缺失必填项、类型不匹配、长度/数值越界、正则不匹配与 `multiple_of` 错误。

## API 参考

完整类型定义见 [`src/index.d.cp`](src/index.d.cp)，AI 智能体极简速查见 [`README_AGENT.md`](README_AGENT.md)。
