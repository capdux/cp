# CP 标准库：Node-API (N-API v9) C-ABI 绑定与离线运行时模拟手册 (`cap:napi`)

> 参考来源：`/data/dev/bun/src/napi/napi.cp`, `/data/dev/bun/src/napi/node_api.h` | 吸收日期：2026-11-22
> **模块路径**: `lib/napi/`  
> **导入协议**: `import { NAPI_VERSION, NapiStatus, NapiValueType, NapiTypedArrayType, NapiPropertyAttributes, NapiModule, NapiPropertyDescriptor, NapiExtendedErrorInfo, NapiNodeVersion, NapiThreadsafeFunctionState, NapiRuntimeContext, NapiEnvMock, Napi } from "cap:napi";`

---

## 一、 架构总览与模块布局

`cap:napi` 提供与 Node.js / Bun 完全一致的 **Node-API v9 C-ABI 类型布局**、**完整 24 项状态码与错误消息映射**，以及**纯 CP 可离线测试的运行时上下文 `NapiRuntimeContext`（别名 `NapiEnvMock`）**：

```text
lib/napi/
├── index.cp    # 统一公开导出入口
├── _napi.cp    # N-API v9 C-ABI 枚举、@c struct 布局与 extern 声明（含 C 小写别名 napi_*）
├── napi.cp     # 安全封装 NapiError、checkStatus、statusName/statusMessage、TypedArray 大小与描述符构造
└── runtime.cp  # 纯 CP 离线运行时状态机：HandleScope 栈、Ref 引用槽表、AsyncWork、TSFN 队列与 NapiEnvMock
```

### 核心补全能力矩阵

1. **完备 N-API v9 枚举与错误映射**：
   - `NapiStatus` 全部 24 项（`Ok = 0` .. `CannotRunJs = 23`），`Napi.checkStatus`、`Napi.statusName` 与 `Napi.statusMessage` 覆盖全部分支。
   - `NapiTypedArrayType` 覆盖 12 种类型（含 `Float16 = 11`、`BigInt64`、`BigUint64`），`Napi.typedArrayElementSize` 精准返回元素字节数（`Float16 => 2`）。
   - 包含 `NapiKeyCollectionMode`、`NapiKeyFilter`、`NapiKeyConversion`、`NapiTsfnReleaseMode`、`NapiTsfnCallMode`。
2. **完整 C-ABI 结构体与运行时状态机**：
   - `@c struct`: `NapiExtendedErrorInfo` (`napi_extended_error_info`)、`NapiNodeVersion` (`napi_node_version`)、`NapiTypeTag` (`napi_type_tag`)、`NapiPropertyDescriptor` (`napi_property_descriptor`)、`NapiModule` (`napi_module`)。
   - 运行时结构体：`NapiCallbackInfoData`、`NapiAsyncWorkEntry` (`Pending`/`Queued`/`Running`/`Completed`/`Cancelled`/`Deleted`)、`NapiThreadsafeFunctionState`（环形调度队列、`acquire`/`release`/`call`/`drainAll`/`ref`/`unref`、`QueueFull`/`Closing` 状态）、`NapiRefEntry` 引用计数槽表、`NapiHandleScopeEntry`（LIFO 作用域校验与 `EscapableHandleScope` 防二次逃逸）。
3. **纯 CP 离线测试环境 (`NapiEnvMock` / `NapiRuntimeContext`)**：
   - 无需挂载 Node.js 或 JSC 引擎即可在纯 CP 单元测试中验证 HandleScope、EscapableHandleScope、Reference ref/unref、PropertyDescriptor 注册、Module 注册、AsyncWork、TSFN 与异常捕获。

---

## 二、 核心 API 与代码示例

### 1. 状态码、TypedArray 大小与 C-ABI 描述符

```typescript
import { NAPI_VERSION, NapiStatus, NapiTypedArrayType, Napi } from "cap:napi";

pub fn demoNapiStatus(): !void {
    Napi.checkStatus(NapiStatus.Ok)?;
    const name = Napi.statusName(NapiStatus.QueueFull);       // "napi_queue_full"
    const msg = Napi.statusMessage(NapiStatus.CannotRunJs);   // "Cannot run JavaScript"
    const f16_sz = Napi.typedArrayElementSize(NapiTypedArrayType.Float16); // 2
}
```

### 2. 使用 `NapiEnvMock` 进行离线 HandleScope、Ref、Property 与 TSFN 测试

```typescript
import {
    NapiStatus,
    NapiPropertyAttributes,
    NapiTsfnCallMode,
    NapiTsfnReleaseMode,
    NapiThreadsafeFunctionState,
    NapiEnvMock,
    Napi
} from "cap:napi";

pub fn demoMockRuntime() {
    let env = NapiEnvMock.init();

    // 1. HandleScope & EscapableHandleScope
    const outer = env.openHandleScope().?;
    const esc = env.openEscapableHandleScope().?;
    const s = env.createStringUtf8("hello_napi").?;
    const escaped = env.escapeHandle(esc, s).?;
    * = env.closeEscapableHandleScope(esc);

    // 2. Object 属性与 Reference 引用计数
    const obj = env.createObject().?;
    * = env.setNamedProperty(obj, "msg", escaped);
    const ref_id = env.createReference(obj, 1).?;
    * = env.referenceRef(ref_id);   // count -> 2
    * = env.referenceUnref(ref_id); // count -> 1
    * = env.deleteReference(ref_id);

    // 3. ThreadsafeFunction 队列调度
    let tsfn = NapiThreadsafeFunctionState.init("bg_worker", 4, 1);
    * = tsfn.call(42, NapiTsfnCallMode.NonBlocking);
    * = tsfn.drainAll();
    * = tsfn.releaseThread(NapiTsfnReleaseMode.Release);

    * = env.closeHandleScope(outer);
}
```

---

## 上游 Bun Node-API 行为契约与安全边界

1. **`HandleScope` 严格 LIFO 嵌套与单次逃逸契约**：
   - `openHandleScope` 与 `closeHandleScope` 必须严格按后进先出（LIFO）配对关闭；`EscapableHandleScope` 仅允许调用一次 `escapeHandle`，二次逃逸返回 `EscapeCalledTwice`，防止句柄生命周期越界。
2. **`NapiStatus` 检查优先于返回值消费**：
   - 所有 N-API 原语均返回 `NapiStatus`，调用方必须先通过 `Napi.checkStatus(status)` 校验 `NapiStatus.Ok` 后再读取出参；出现 `PendingException` 时不得继续调用会触发 JS 执行的 API。
3. **`ThreadsafeFunction` 跨线程背压与关闭序列**：
   - 跨线程调用 `NapiThreadsafeFunctionState.call` 时区分 `NonBlocking`（队列满立即返回 `QueueFull`）与 `Blocking`；工作线程退出前须调用 `releaseThread(NapiTsfnReleaseMode.Release)` 或 `Abort`，避免主事件循环永久挂起。
4. **ABI 稳定性保障（`NAPI_VERSION = 9`）**：
   - 所有 `@c struct`（`NapiModule`、`NapiPropertyDescriptor`、`NapiTypeTag`、`NapiNodeVersion`）严格对齐 Node-API v9 C 头文件内存布局，并包含 `Float16`（2 字节）在内的 12 种 `NapiTypedArrayType`。
