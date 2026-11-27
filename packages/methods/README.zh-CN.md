[English](./README.md) | [简体中文](./README.zh-CN.md) | [AI Agent 速查](./README_AGENT.md)

# methods

面向 CP 语言的零堆分配 HTTP 方法注册表与 RFC 9110 / WebDAV 语义判断库，支持 34 个标准 HTTP 方法的 O(log n) 不区分大小写检索，对齐 npm [`methods`](https://github.com/jshttp/methods)（周下载 1.2 亿+）。

## 源包对齐说明

- **上游源包**：npm `methods`（`jshttp/methods`，MIT，Express / Koa 核心依赖）
- **CP 实现**：`methods` v1.0.0，纯 CP 净室 MIT 实现
- **架构特点**：全程零堆分配，编译期有序 `[34]string` 静态表（小写 `METHODS` 与大写 `METHODS_UPPER`）配合二分查找与 RFC 9110 §9.2 / WebDAV 语义谓词。

## 安装与导入

```bash
cap add methods
```

```cp
import {
    HTTP_METHOD_COUNT,
    METHODS,
    METHODS_UPPER,
    hmCmpLower,
    findMethodIndex,
    isHttpMethod,
    toLowerMethod,
    methodToLower,
    toUpperMethod,
    methodToUpper,
    methodAt,
    methodUpperAt,
    isSafeMethod,
    isIdempotentMethod,
    isCacheableMethod,
    mayHaveRequestBody,
    isStandardRfc9110Method,
    isWebDavMethod,
    allowsResponseBody,
} from "methods";
```

## 核心 API

### 常量表

| 符号 | 类型 | 说明 |
| :--- | :--- | :--- |
| `HTTP_METHOD_COUNT` | `const HTTP_METHOD_COUNT: usize`（`34`） | 支持的标准 HTTP / WebDAV 方法总数 |
| `METHODS` | `const METHODS: [34]string` | 按字典序升序排列的小写方法名表（`"acl"` .. `"unsubscribe"`） |
| `METHODS_UPPER` | `const METHODS_UPPER: [34]string` | 按字典序升序排列的大写方法名表（`"ACL"` .. `"UNSUBSCRIBE"`） |

### 检索与规范化函数

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `findMethodIndex` | `fn findMethodIndex(method: string): i32` | 不区分大小写二分查找，命中返回 `0..33`，未知返回 `-1` |
| `isHttpMethod` | `fn isHttpMethod(method: string): bool` | 判断是否为 34 个标准 HTTP/WebDAV 方法之一（不区分大小写） |
| `toLowerMethod` / `methodToLower` | `fn toLowerMethod(method: string): string` | 返回静态规范小写名（如 `"get"`），未知返回 `""` |
| `toUpperMethod` / `methodToUpper` | `fn toUpperMethod(method: string): string` | 返回静态规范大写名（如 `"GET"`），未知返回 `""` |
| `methodAt` | `fn methodAt(idx: usize): string` | 按下标获取小写方法名（越界返回 `""`） |
| `methodUpperAt` | `fn methodUpperAt(idx: usize): string` | 按下标获取大写方法名（越界返回 `""`） |
| `hmCmpLower` | `fn hmCmpLower(input: string, lower_target: string): i32` | 不区分大小写与小写目标串比较（`-1`/`0`/`1`） |

### RFC 9110 与 WebDAV 语义谓词

| 函数 | 签名 | 说明 |
| :--- | :--- | :--- |
| `isSafeMethod` | `fn isSafeMethod(method: string): bool` | RFC 9110 §9.2.1 只读安全方法（`GET`/`HEAD`/`OPTIONS`/`TRACE`/`PROPFIND`/`REPORT`/`SEARCH`） |
| `isIdempotentMethod` | `fn isIdempotentMethod(method: string): bool` | RFC 9110 §9.2.2 幂等方法（安全方法 + `PUT`/`DELETE`/`COPY`/`MOVE`/`MKCOL`/`PROPPATCH`/`LOCK`/`UNLOCK`/`UNLINK`） |
| `isCacheableMethod` | `fn isCacheableMethod(method: string): bool` | RFC 9110 §9.2.3 可缓存方法（`GET`/`HEAD`/`POST`） |
| `mayHaveRequestBody` | `fn mayHaveRequestBody(method: string): bool` | 是否常规携带请求体（`POST`/`PUT`/`PATCH` 等） |
| `isStandardRfc9110Method` | `fn isStandardRfc9110Method(method: string): bool` | 是否为 9 个核心 RFC 9110 / RFC 5789 方法之一 |
| `isWebDavMethod` | `fn isWebDavMethod(method: string): bool` | 是否为 WebDAV / CalDAV / Versioning 扩展方法 |
| `allowsResponseBody` | `fn allowsResponseBody(method: string): bool` | 是否允许返回响应体（除 `HEAD` 外的合法方法均返回 `true`） |

## 使用示例

### 1. 不区分大小写校验与规范化

```cp
import { isHttpMethod, toUpperMethod, toLowerMethod } from "methods";
import { print } from "cap:io";

pub fn main() {
    if (isHttpMethod("pAtCh")) {
        print(toUpperMethod("pAtCh")); // "PATCH"
        print(toLowerMethod("PATCH")); // "patch"
    }
}
```

### 2. 路由与重试策略判断

```cp
import { isSafeMethod, isIdempotentMethod, allowsResponseBody } from "methods";

pub fn canRetryRequest(method: string): bool {
    return isSafeMethod(method) || isIdempotentMethod(method);
}

pub fn shouldWriteBody(method: string): bool {
    return allowsResponseBody(method); // 对 "HEAD" 或未知方法返回 false
}
```

### 3. 遍历全部标准方法

```cp
import { HTTP_METHOD_COUNT, methodAt } from "methods";
import { print } from "cap:io";

pub fn listMethods() {
    let i: usize = 0;
    while (i < HTTP_METHOD_COUNT) {
        print(methodAt(i));
        i += 1;
    }
}
```

## 与源包差异与安全亮点

- **零堆分配**：`toLowerMethod`、`toUpperMethod`、`methodAt` 均直接返回编译期静态常量字符串，无需缓冲区拷贝。
- **O(log n) 零拷贝二分查找**：查询时直接按字节转小写比较，不构造临时小写字符串。
- **完整协议语义谓词**：内置 RFC 9110 安全性/幂等性/可缓存性/请求体/响应体判断及 WebDAV 分类。
