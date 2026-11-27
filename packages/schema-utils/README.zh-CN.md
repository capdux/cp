[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# schema-utils

纯 CP 实现的零堆配置项 Schema 校验库，提供结构化多错误聚合报告、轻量正则模式匹配、单字段校验以及 webpack 风格错误格式化输出。

- **源包对齐**：参考 npm [`schema-utils`](https://github.com/webpack/schema-utils) `4.3.0`（`MIT`）。
- **包版本**：`4.3.0`

## 安装与导入

```bash
cap add schema-utils
```

在 `cap.json` 中声明依赖：

```json
{
  "dependencies": {
    "schema-utils": "^4.3.0"
  }
}
```

```typescript
import {
    SchemaType, SchemaProperty, SchemaValue, ObjectSchema,
    FieldConstraint, ValidationResult, ValidationError, ValidationReport,
    validate, validateStringField, validateIntField, validateEnumField,
    validatePatternField, matchPattern, schemaTypeName,
    formatValidationError, formatValidationReport,
    SCHU_OK, SCHU_ERR_REQUIRED, SCHU_MAX_ERRORS
} from "schema-utils";
```

## 核心 API 参考

### 常量与枚举

| 符号 | 取值 | 说明 |
| :--- | :--- | :--- |
| `SchemaType` | `Any`, `String`, `Integer`, `Number`, `Boolean`, `Array`, `Object`, `Null` | JSON Schema 值类型枚举 |
| 错误码常量 | `SCHU_OK` (0)、`SCHU_ERR_REQUIRED` (1)、`SCHU_ERR_MIN_LEN` (2)、`SCHU_ERR_MAX_LEN` (3)、`SCHU_ERR_INT_MIN` (4)、`SCHU_ERR_INT_MAX` (5)、`SCHU_ERR_ENUM` (6)、`SCHU_ERR_TYPE` (7)、`SCHU_ERR_PATTERN` (8)、`SCHU_ERR_ADDITIONAL_PROP` (9)、`SCHU_ERR_MULTIPLE_OF` (10) | 机器可读的校验错误码 |
| `SCHU_MAX_ERRORS` | `16` | `ValidationReport.errors` 固定错误槽位上限 |

### 数据结构与构造方法

| 类型 | 关键字段 / 静态构造 | 说明 |
| :--- | :--- | :--- |
| `SchemaProperty` | `stringProp(name, required, min_length, max_length, pattern)`、`intProp(name, required, has_minimum, minimum, has_maximum, maximum)`、`boolProp(name, required)` | 属性约束定义（亦支持 `multiple_of: i64`） |
| `SchemaValue` | `str(name, v)`、`int(name, v)`、`boolean(name, v)` | 待校验的强类型键值项 |
| `ObjectSchema` | `title: string`、`properties: &[]SchemaProperty`、`additional_properties: bool` | 对象级 Schema 描述 |
| `FieldConstraint` | `name`、`required`、`min_len`、`max_len`、`has_int_range`、`min_int`、`max_int` | 单字段校验约束 |
| `ValidationResult` | `valid: bool`、`error_code: u32`、`message: string` | 单字段校验结果 |
| `ValidationError` | `field`、`code`、`message`、`expected`、`actual` | 报告中的单条结构化错误 |
| `ValidationReport` | `valid: bool`、`error_count: usize`、`errors: [16]ValidationError` | `validate` 返回的多错误聚合报告 |

### 核心函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `validate` | `validate(schema: ObjectSchema, values: &[]SchemaValue): ValidationReport` | 按 `ObjectSchema` 校验配置项切片 |
| `validateStringField` | `validateStringField(value: string, present: bool, c: FieldConstraint): ValidationResult` | 校验字符串必填与长度上下限 |
| `validateIntField` | `validateIntField(value: i64, present: bool, c: FieldConstraint): ValidationResult` | 校验整数必填与 `[min_int, max_int]` 区间 |
| `validateEnumField` | `validateEnumField(value: string, allowed: &[]string): ValidationResult` | 校验值是否属于允许列表 |
| `matchPattern` | `matchPattern(pattern: string, text: string): bool` | 轻量模式匹配（支持 `^`、`$`、`.`、`*`、`+`、`?`、`[a-z]`、转义） |
| `validatePatternField` | `validatePatternField(value: string, pattern: string): ValidationResult` | 按模式校验字符串字段 |
| `schemaTypeName` | `schemaTypeName(t: SchemaType): string` | 返回小写 JSON Schema 类型名称 |
| `formatValidationError` | `formatValidationError(schema_name: string, err: ValidationError, out: []u8): usize` | 将单条错误格式化写入 `out` |
| `formatValidationReport` | `formatValidationReport(schema_name: string, report: ValidationReport, out: []u8): usize` | 将完整校验报告格式化写入 `out` |

## 使用示例

### 1. 使用 `ObjectSchema` 校验插件配置

```typescript
import {
    SchemaProperty, SchemaValue, ObjectSchema,
    validate, formatValidationReport
} from "schema-utils";

fn validateConfig(): bool {
    let props = [
        SchemaProperty.stringProp("mode", true, 1, 16, "^(dev|prod)$"),
        SchemaProperty.intProp("port", true, true, 1, true, 65535),
        SchemaProperty.boolProp("minify", false)
    ];
    let schema = ObjectSchema {
        title: "BuildOptions",
        properties: &props,
        additional_properties: false
    };

    let values = [
        SchemaValue.str("mode", "prod"),
        SchemaValue.int("port", 8080),
        SchemaValue.boolean("minify", true)
    ];

    let report = validate(schema, &values);
    if (!report.valid) {
        let mut buf: [512]u8 = undefined;
        let _ = formatValidationReport(schema.title, report, &mut buf);
    }
    return report.valid;
}
```

### 2. 单字段枚举与模式校验

```typescript
import { validateEnumField, validatePatternField } from "schema-utils";

fn checkTarget(target: string, version: string): bool {
    let allowed = ["node", "web", "bun"];
    let r1 = validateEnumField(target, &allowed);
    let r2 = validatePatternField(version, "^[0-9]+\\.[0-9]+\\.[0-9]+$");
    return r1.valid && r2.valid;
}
```

## 与源包差异与 CP 安全亮点

- **零堆结构化报告**：`ValidationReport` 在栈上固定收集最多 `SCHU_MAX_ERRORS`（16）条错误，无需抛异常或动态拼接字符串。
- **内置零依赖模式引擎**：`matchPattern` 无需外部正则引擎即可完成常见配置项格式校验。
- **调用方缓冲输出**：所有诊断格式化函数写入 `out: []u8` 并返回写入字节数，彻底避免堆分配。
