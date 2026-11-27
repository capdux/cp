[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `zod`

CP 零堆分配类型安全 Schema 校验框架 —— 对齐 npm [`zod`](https://github.com/colinhacks/zod)（v3）的纯 CP MIT 净室实现，提供独立原子格式校验与栈上 `ZObjectSchema` JSON 对象结构化校验器。

## 安装与导入

```bash
cap add zod
```

```cp
import {
    Z_STRING, Z_NUMBER, Z_INT, Z_BOOL, Z_EMAIL, Z_URL, Z_UUID, Z_ENUM, Z_ARRAY, Z_OBJECT,
    Z_ERR_INVALID_JSON, Z_ERR_NOT_OBJECT, Z_ERR_REQUIRED, Z_ERR_INVALID_TYPE,
    Z_ERR_NOT_INT, Z_ERR_TOO_SMALL, Z_ERR_TOO_BIG,
    Z_ERR_INVALID_EMAIL, Z_ERR_INVALID_URL, Z_ERR_INVALID_UUID, Z_ERR_INVALID_ENUM,
    Z_MAX_RULES, Z_MAX_ISSUES,
    ZFieldRule, ZIssue, ZParseResult, ZObjectSchema,
    zString, zNumber, zEmail, zUrl, zUuid, zEnum,
} from "zod";
```

## 核心 API

### 规则类型与错误码常量

| 分类 | 常量 | 取值 |
| :--- | :--- | :--- |
| 规则类型 (`u8`) | `Z_STRING`, `Z_NUMBER`, `Z_INT`, `Z_BOOL`, `Z_EMAIL`, `Z_URL`, `Z_UUID`, `Z_ENUM`, `Z_ARRAY`, `Z_OBJECT` | `1..10` |
| 错误码 (`i32`) | `Z_ERR_INVALID_JSON`, `Z_ERR_NOT_OBJECT`, `Z_ERR_REQUIRED`, `Z_ERR_INVALID_TYPE`, `Z_ERR_NOT_INT`, `Z_ERR_TOO_SMALL`, `Z_ERR_TOO_BIG`, `Z_ERR_INVALID_EMAIL`, `Z_ERR_INVALID_URL`, `Z_ERR_INVALID_UUID`, `Z_ERR_INVALID_ENUM` | `1..11` |
| 容量上限 (`usize`) | `Z_MAX_RULES`, `Z_MAX_ISSUES` | `24`, `16` |

### 独立原子校验函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `zString` | `(raw: string, min_len: usize, max_len: usize): bool` | 校验字符串字节长度是否在 `[min_len, max_len]` 内 |
| `zNumber` | `(raw: string, min_v: f64, max_v: f64): bool` | 解析数字字符串并校验闭区间 `[min_v, max_v]` |
| `zEmail` | `(s: string): bool` | 邮箱格式校验 |
| `zUrl` | `(s: string): bool` | WHATWG URL 校验（复用 `url` 包） |
| `zUuid` | `(s: string): bool` | 标准 UUID（`8-4-4-4-12` 十六进制）校验 |
| `zEnum` | `(s: string, csv: string): bool` | 校验字符串是否命中逗号分隔的枚举项（如 `"admin,user,guest"`） |

### Schema 与结果类型

| 类型 | 核心方法 / 构造器 | 说明 |
| :--- | :--- | :--- |
| `ZFieldRule` | `empty()`, `basic(name, kind, required)`, `strRange(name, required, min_len, max_len)`, `numRange(name, kind, required, min_val, max_val)`, `enumRule(name, required, enum_csv)` | 声明式字段约束描述 |
| `ZIssue` | `empty()`, `fieldName(): []const u8` | 单条校验错误（内联 `field: [48]u8`、`field_len`、`code: i32`） |
| `ZParseResult` | `empty()`, `addIssue(*_, field_name, code)`, `hasIssue(field_name, code): bool` | 固定容量校验结果（`ok: bool`、`issue_count`、`issues: [16]ZIssue`） |
| `ZObjectSchema` | `new()`, `field(*_, rule: ZFieldRule): bool`, `safeParseJson(json: string): ZParseResult` | 零堆分配 JSON 对象校验器（最多 24 条规则） |

## 代码示例

### 1. 使用 `ZObjectSchema` 校验 JSON 负载

```cp
import { print } from "cap:io";
import {
    ZObjectSchema, ZFieldRule,
    Z_EMAIL, Z_INT, Z_BOOL, Z_ERR_TOO_SMALL,
} from "zod";

fn main() {
    let schema = ZObjectSchema.new();
    schema.field(ZFieldRule.strRange("username", true, 3, 20));
    schema.field(ZFieldRule.basic("email", Z_EMAIL, true));
    schema.field(ZFieldRule.numRange("age", Z_INT, true, 18.0, 120.0));
    schema.field(ZFieldRule.enumRule("role", true, "admin,member,guest"));
    schema.field(ZFieldRule.basic("active", Z_BOOL, false));

    const payload = "{\"username\":\"alice\",\"email\":\"alice@cp.dev\",\"age\":16,\"role\":\"admin\"}";
    const res = schema.safeParseJson(payload);
    if (!res.ok) {
        print("issues:", res.issue_count);
        print("age too small:", res.hasIssue("age", Z_ERR_TOO_SMALL)); // true
    }
}
```

### 2. 直接调用原子校验函数

```cp
import { print } from "cap:io";
import { zEmail, zUrl, zUuid, zEnum, zNumber } from "zod";

fn main() {
    print(zEmail("user@example.com"));                       // true
    print(zUrl("https://cp-lang.org/packages/zod"));         // true
    print(zUuid("550e8400-e29b-41d4-a716-446655440000"));    // true
    print(zEnum("staging", "dev,staging,prod"));             // true
    print(zNumber("42.5", 0.0, 100.0));                      // true
}
```

## 与源包差异及性能亮点

- **零堆 JSON 扫描**：`safeParseJson` 直接在原始 JSON 字符串切片上单遍扫描，不构造中间对象树或哈希表，组合复用 `kind-of` 与 `url`。
- **栈上固定容量**：规则表（`Z_MAX_RULES = 24`）与错误列表（`Z_MAX_ISSUES = 16`）均为值类型内联数组，无需分配器即可获得确定性延迟。
- **无异常设计**：全面采用 `safeParse` 风格，通过 `ZParseResult.ok` 与 `Z_ERR_*` 枚举错误码反馈结果。
