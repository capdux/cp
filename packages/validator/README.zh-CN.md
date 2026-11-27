[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# `validator`

CP 零堆分配格式校验与多错误收集引擎 —— 对齐 npm [`validator`](https://github.com/validatorjs/validator.js)（v13）与 Rust `validator` 的纯 CP MIT 净室实现，内置 30+ 独立格式断言与栈上 `Validator` 收集器。

## 安装与导入

```bash
cap add validator
```

```cp
import {
    Validator,
    ValidationError,
    isEmail,
    isUrl,
    isUuid,
    isUlid,
    isIpv4,
    isIpv6,
    isCidr,
    isSemver,
    isIsoDate,
    isIsoDateTime,
    isLuhn,
    isE164Phone,
    isPort,
    inRangeI64,
} from "validator";
```

## 核心 API

### 核心类型

| 类型 | 字段 / 容量 | 说明 |
| :--- | :--- | :--- |
| `ValidationError` | `field: string`, `code: string`, `message: string` | 结构化校验错误条目 |
| `Validator` | `errors: [32]ValidationError`, `count: usize` | 栈上多错误收集器（最多 32 条错误，零堆分配） |

### 独立格式与范围校验函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isAscii` / `startsWith` / `endsWith` | `(s: string, ...) -> bool` | 从 `cap:ascii` 重导出的基础 ASCII 与前/后缀断言 |
| `isAlphanumeric` / `isNumeric` / `isHex` | `(s: string) -> bool` | 字母数字、数值、十六进制字符集校验 |
| `isBase64` / `isSlug` | `(s: string) -> bool` | 标准 Base64 与小写 URL Slug 校验 |
| `hasMinLen` / `hasMaxLen` / `hasLenBetween` | `(s: string, min/max: usize) -> bool` | 字节长度下界、上界与闭区间校验 |
| `containsStr` / `isOneOf` | `(s: string, needle: string / allowed: []string) -> bool` | 子串包含与枚举白名单匹配 |
| `isEmail` / `isUrl` | `(s: string) -> bool` | 邮箱地址与 `http(s)://` URL 校验 |
| `isIpv4` / `isIpv6` / `isCidr` | `(s: string) -> bool` | IPv4、IPv6 与 CIDR 网段格式校验 |
| `isIpv4Range` / `isIpv6Range` | `(s: string, start: usize, end: usize) -> bool` | 零拷贝切片区间 IP 校验 |
| `isUuid` / `isUlid` / `isSemver` | `(s: string) -> bool` | UUID（8-4-4-4-12）、Crockford Base32 ULID（26 字符）、SemVer 2.0 |
| `isIsoDate` / `isIsoDateTime` | `(s: string) -> bool` | 闰年感知的 `YYYY-MM-DD` 与 ISO-8601 日期时间校验 |
| `isLuhn` / `isE164Phone` | `(s: string) -> bool` | Luhn 模 10 校验和（银行卡/IMEI）与 E.164 国际电话号 |
| `isPort` / `inRangeI64` / `inRangeF64` / `isMultipleOf` | `(v, ...) -> bool` | 端口号（`1..=65535`）、整型/浮点闭区间与整除校验 |

### `Validator` 收集器方法

| 方法 | 签名 | 说明 |
| :--- | :--- | :--- |
| `Validator.init` | `() -> Validator` | 初始化栈上多错误收集器 |
| `isValid` / `errorCount` / `errorAt` | `() -> bool` / `() -> usize` / `(idx: usize) -> ValidationError` | 只读查询校验结果与指定索引错误 |
| `addError` | `(*_, field: string, code: string, message: string)` | 追加自定义错误（满 32 条后静默截断，不越界） |
| `required` / `minLen` / `maxLen` | `(*_, field, value, ...) -> bool` | 非空与长度校验，失败时自动记录错误 |
| `email` / `url` / `uuid` / `ulid` / `slug` | `(*_, field, value) -> bool` | 常用字符串格式校验 |
| `ipv4` / `ipv6` / `cidr` / `semver` | `(*_, field, value) -> bool` | 网络地址与语义化版本校验 |
| `isoDate` / `isoDateTime` / `luhn` / `phone` | `(*_, field, value) -> bool` | 日期时间、Luhn 与 E.164 手机号校验 |
| `oneOf` / `rangeI64` / `rangeF64` / `port` | `(*_, field, value, ...) -> bool` | 白名单、数值区间与端口校验 |

## 代码示例

### 1. 零拷贝独立格式断言

```cp
import { print } from "cap:io";
import { isEmail, isUrl, isUuid, isCidr, isSemver, isLuhn } from "validator";

fn main() {
    print("email:", isEmail("dev@cp-lang.org"));         // true
    print("url:", isUrl("https://cp-lang.org/docs"));    // true
    print("uuid:", isUuid("550e8400-e29b-41d4-a716-446655440000")); // true
    print("cidr:", isCidr("10.0.0.0/8"));                // true
    print("semver:", isSemver("2.1.0-rc.1+build.42"));   // true
    print("luhn:", isLuhn("79927398713"));               // true
}
```

### 2. 使用 `Validator` 一次性收集多字段错误

```cp
import { print } from "cap:io";
import { Validator } from "validator";

fn validateSignup(email: string, handle: string, port: i64, role: string) {
    // 必须使用 `let` 声明，因为校验方法需要可变接收者 `*_`
    let v = Validator.init();
    const roles: [3]string = ["admin", "editor", "viewer"];

    v.required("email", email);
    v.email("email", email);
    v.minLen("handle", handle, 3);
    v.slug("handle", handle);
    v.port("port", port);
    v.oneOf("role", role, roles[0..3]);

    if (!v.isValid()) {
        let i: usize = 0;
        while (i < v.errorCount()) {
            const err = v.errorAt(i);
            print(err.field, err.code, err.message);
            i += 1;
        }
    }
}
```

## 与源包差异及性能亮点

- **全程零堆分配**：不依赖正则表达式引擎，所有断言均采用单遍字节扫描（`O(n)` 时间、`O(1)` 栈空间），规避 ReDoS 风险与 GC 压力。
- **内联多错误收集**：`Validator` 在栈上固定预留 32 条 `ValidationError`，无需动态数组扩容，非常适合高并发 API 请求参数批校验。
- **强类型无隐式转换**：参数严格区分 `string`、`i64`、`f64`、`usize`，无异常抛出开销。
