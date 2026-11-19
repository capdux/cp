# CP 标准库：Node-API (N-API v9) C-ABI 绑定与原生插件桥接手册 (`cap:napi`)

## 一、 架构总览与设计哲学

`cap:napi` 是 CP 语言面向 Node.js 与 Bun 原生插件生态的 **Node-API (N-API v9) 标准 C-ABI 互操作层**，吸收自 Bun 的 `/data/dev/bun/src/napi/`（`js_native_api_types.h`、`node_api_types.h` 与 `napi.zig`）。

设计采用经典的双层隔离架构：
- **底层协议层（`_napi.cp`）**：纯 CP `@c` 结构体与枚举定义，与 C 标准 `node_api.h` / `js_native_api.h` 保持字节级 ABI 等价，**无需依赖外部 C 头文件即可独立编译**；
- **安全封装层（`napi.cp`）**：将 `NapiStatus` 自动映射为 CP 强类型错误集 `NapiError`，支持 `?` 链式错误传播，并提供描述符构造器与本地插件注册表 `NapiAddonRegistry`。

```text
┌────────────────────────────────────────────────────────────────────────────┐
│                       cap:napi 跨运行时原生插件桥接                          │
│                                                                            │
│   CP 业务 / 原生扩展代码                                                    │
│     │  Napi.createInt32 / getUndefined / openHandleScope / checkStatus(?)  │
│     ▼                                                                      │
│   安全封装层 (lib/napi/napi.cp)                                             │
│     • NapiError 错误映射   • NapiAddonRegistry 本地插件元数据管理           │
│     • Napi.createModuleDescriptor / createPropertyDescriptor               │
│     ▼                                                                      │
│   C-ABI 契约层 (lib/napi/_napi.cp)                                          │
│     • NapiStatus (0..23) / NapiValueType (0..9) / NapiTypedArrayType       │
│     • @c struct NapiModule / NapiPropertyDescriptor / NapiExtendedErrorInfo│
│     • extern napi_* 函数声明 ──▶ 宿主运行时 (Bun / Node.js / CP Runtime)    │
└────────────────────────────────────────────────────────────────────────────┘
```

---

## 二、 核心 API 详解与代码示例

### 1. 状态码校验与类型元信息 (`Napi` / `NapiStatus` / `NapiValueType`)

```typescript
import {
    NAPI_VERSION,
    NapiStatus,
    NapiValueType,
    NapiTypedArrayType,
    Napi
} from "cap:napi";

pub fn demoNapiTypes(): !void {
    // 校验 C-ABI 返回码并自动转换为 CP 错误联合
    Napi.checkStatus(NapiStatus.Ok)?;

    const status_str = Napi.statusName(NapiStatus.PendingException); // "napi_pending_exception"
    const type_str = Napi.valueTypeName(NapiValueType.BigInt);       // "bigint"
    const elem_bytes = Napi.typedArrayElementSize(NapiTypedArrayType.Float64); // 8
}
```

### 2. 构造 C-ABI 模块描述符与属性描述符 (`NapiModule` / `NapiPropertyDescriptor`)

```typescript
import { Napi, NapiPropertyAttributes, NapiAddonRegistry } from "cap:napi";

pub fn demoDescriptors() {
    const mod_name = "fast_hash_addon";
    const file_name = "fast_hash.cp";
    const mod_desc = Napi.createModuleDescriptor(mod_name.ptr, file_name.ptr, 0, null);

    const fn_name = "sha256Hex";
    const prop = Napi.createPropertyDescriptor(fn_name.ptr, NapiPropertyAttributes.DefaultMethod);

    // 离线插件元数据注册表
    let registry = NapiAddonRegistry.init();
    * = registry.register("fast_hash_addon", "fast_hash.node", 0);
    if (info := registry.find("fast_hash_addon")) {
        // info.napi_version == 9
    }
}
```

### 3. 在宿主 Node-API 环境中桥接 JS 值

当 CP 编译为动态共享库（`.node` / `.so`）并加载到 Bun 或 Node.js 进程时，可直接使用 `Napi` 安全封装（自动传播 `NapiError` 与 `defer` 关闭句柄作用域）：

```typescript
import { Napi, NapiEnv, NapiValue } from "cap:napi";

pub fn createJsResult(env: NapiEnv, sum: i32): !NapiValue {
    const scope = Napi.openHandleScope(env)?;
    defer Napi.closeHandleScope(env, scope) catch {};

    const num = Napi.createInt32(env, sum)?;
    const t = Napi.typeofValue(env, num)?; // NapiValueType.Number
    return num;
}
```

---

## 三、 与 Node.js C/C++ N-API & Bun N-API 对照表

| 特性 | C/C++ `node-addon-api` | Bun `src/napi/napi.zig` | CP (`cap:napi`) |
|---|---|---|---|
| 头文件依赖 | 需安装 `node_api.h` / `node-gyp` | 内嵌 C 头文件 + Zig extern | 纯 CP `@c struct` 内建定义，零外部头文件依赖 |
| 错误处理 | 手动 `if (status != napi_ok)` 或 C++ 异常 | 返回 `napi_status` 枚举 | `Napi.checkStatus(st)?` 强类型错误联合传播 |
| 句柄作用域安全 | `Napi::HandleScope` RAII | 手动 `close` | `defer Napi.closeHandleScope(env, scope)` 确定性释放 |
| ABI 兼容性 | Node-API v9 | Node-API v9 | 与 Node.js / Bun 共享同一套二进制 C-ABI 布局 |
