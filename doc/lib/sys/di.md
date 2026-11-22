# CP 标准库：依赖注入与生命周期容器手册 (`cap:di`)

> [!NOTE]
> 本子模块已归并至统一主包 `cap:sys`，导入请使用 `import { ... } from "cap:sys"`。

## 一、 架构总览与设计准则

`cap:di` 是 CP 语言官方内置的纯 CP **依赖注入（IoC）与生命周期管理服务容器**，**零外部依赖、零 `std.posix`、零 `@import("bun")`**，专为大型后端服务、CLI 工具与 AI Agent 插件系统设计。既可通过 `import { ... } from "cap:sys"` 导入，也支持 `import cap from "cap"; cap.di.*` 动态懒挂载调用。

```text
┌──────────────────────────────────────────────────────────────────────────┐
│                 CP 依赖注入与生命周期容器 (cap:di)                         │
├──────────────────────────────────┬───────────────────────────────────────┤
│ ServiceContainer (根容器)         │ ServiceScope (请求级子作用域)          │
│ - Singleton 实例缓存与逆序销毁    │ - Scoped 实例缓存与作用域隔离          │
│ - 服务注册表与 build() 冻结       │ - 嵌套依赖解析栈 (MAX_DEPTH = 32)       │
│ - registerMany 多实现扩展点       │ - 循环依赖环路检测 & Captive 拦截      │
└──────────────────────────────────┴───────────────────────────────────────┘
```

### 1. 模块文件拓扑与谱系来源

| 文件路径 | 谱系注释 | 核心职责 |
| :--- | :--- | :--- |
| `lib/di/container.cp` | `// Source: cp/lib/di/container.cp (Pure CP Dependency Injection & IoC Lifetime Container)` | `DiError`、`ServiceLifetime`、`ServiceInstance`、`ServiceContainer`、`ServiceScope` |
| `lib/di/index.cp` | `// Source: cp/lib/di/index.cp (Unified Exports for cap:di)` | `cap:di` 统一公开导出入口 |

---

## 二、 核心 API 详解

### 1. 错误、生命周期与句柄类型

- **`MAX_RESOLUTION_DEPTH: usize = 32`**：最大嵌套依赖解析深度。
- **`DiError`**：
  - `ServiceNotFound`：未找到指定服务名（或空服务名）。
  - `DuplicateRegistration`：单实例服务名重复注册，或单实例与多实现混用。
  - `CircularDependency`：检测到循环依赖（可通过 `lastErrorCycle()` 获取完整环路路径）。
  - `ScopeViolation`：违反作用域隔离（如从根容器直接解析 `Scoped` 服务，或 `Singleton` 工厂内部非法捕获 `Scoped` 服务即 Captive Dependency）。
  - `ContainerFrozen`：容器调用 `build()` 冻结后拒绝任何新增注册。
  - `MaxDepthExceeded`：依赖解析深度超过 `MAX_RESOLUTION_DEPTH`。
  - `OutOfMemory`：内存分配失败。
- **`ServiceLifetime`**：`Singleton`（全局唯一）、`Scoped`（作用域内复用、跨作用域隔离）、`Transient`（每次解析新建）。
- **`DestructorFn`**：`*const fn (*!, std.mem.Allocator) void`。
- **`FactoryFn`**：`*const fn (*ServiceScope, std.mem.Allocator) *! ! DiError`。
- **`ServiceInstance`**：封装 `ptr: *!`、`type_tag: string` 与 `destructor: DestructorFn?`，提供 `asPtr(comptime T: type): *T!` 类型转换辅助。

### 2. `ServiceContainer`（根容器）

- **`ServiceContainer.init(alloc)` / `deinit()`**：初始化根容器；`deinit()` 严格按实例化逆序调用所有 `Singleton`（及根作用域托管实例）的析构回调，先销毁依赖方、后销毁被依赖方。
- **`registerSingleton(name, type_tag, factory, destructor)`**：注册 `Singleton` 工厂。
- **`registerSingletonValue(name, ptr, type_tag, destructor)`**：直接注册已构造好的单例指针与可选析构回调。
- **`registerScoped(name, type_tag, factory, destructor)`**：注册 `Scoped` 工厂。
- **`registerTransient(name, type_tag, factory, destructor)`**：注册 `Transient` 工厂。
- **`registerMany(name, lifetime, type_tag, factory, destructor)` / `registerManyValue(name, ptr, type_tag, destructor)`**：为同一接口名追加多个实现。
- **`build()`**：冻结注册表，禁止后续修改。
- **`createScope(): ServiceScope`**：创建请求级子作用域。
- **`resolve(name)` / `tryResolve(name)` / `resolveAs(T, name)` / `tryResolveAs(T, name)` / `resolveAll(name)`**：根容器解析入口。
- **`lastErrorCycle(): string`**：返回最近一次 `CircularDependency` 的完整依赖环路径（如 `"A -> B -> C -> A"`）。

### 3. `ServiceScope`（请求级子作用域）

- **`deinit()`**：严格按实例化逆序销毁本作用域内所有 `Scoped` 与托管 `Transient` 实例，绝不销毁父容器 `Singleton`。
- **`resolve(name)` / `tryResolve(name)` / `resolveAs(T, name)` / `tryResolveAs(T, name)` / `resolveAll(name)`**：在当前作用域内解析服务，自动维护 `resolution_stack`，拦截循环依赖与 Captive Dependency。

---

## 三、 可运行代码示例

```typescript
const std = @import("std");
import { ServiceContainer, ServiceScope, DiError } from "cap:sys";
import cap from "cap";

struct Config {
    port: u16;
}

fn createConfig(_: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    let cfg = alloc.create(Config) catch return DiError.OutOfMemory;
    cfg.* = Config{ port: 8080 };
    unsafe { return cfg as *!; }
}

fn destroyConfig(ptr: *!, alloc: std.mem.Allocator) {
    unsafe {
        const cfg = ptr as *Config!;
        alloc.destroy(cfg);
    }
}

pub fn demoDi(): ! {
    const alloc = std.heap.page_allocator;
    let container = cap.di.ServiceContainer.init(alloc);
    defer container.deinit();

    container.registerSingleton("config", "Config", createConfig, destroyConfig)?;
    container.build()?;

    let scope = container.createScope();
    defer scope.deinit();

    const cfg = scope.resolveAs(Config, "config")?;
    * = cfg.port;
}
```
