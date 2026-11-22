// Source: cp/cap/test/test_di_production_deep.cp
// Deep Production Verification Suite for cap:di & cap.di
// Covers:
//   1. Singleton, Scoped, and Transient lifetime semantics (global reuse vs scope isolation vs per-resolve creation)
//   2. Multi-layer dependency injection and strict reverse-order deinit() destruction
//   3. CircularDependency detection with full cycle path (lastErrorCycle) & Captive Dependency (ScopeViolation)
//   4. Multi-implementation extension points (registerMany + resolveAll) & build() ContainerFrozen guard
//   5. `import cap from "cap"; cap.di` automatic dynamic mounting

const std = @import("std");
import { test, expect_eq } from "cap:test";
import cap from "cap";
import {
    MAX_RESOLUTION_DEPTH,
    DiError,
    ServiceLifetime,
    ServiceInstance,
    ServiceContainer,
    ServiceScope,
} from "cap:sys";

// ============================================================================
// Shared test fixtures & destruction log
// ============================================================================

let g_singleton_creations: usize = 0;
let g_scoped_creations: usize = 0;
let g_transient_creations: usize = 0;

let g_singleton_destroys: usize = 0;
let g_scoped_destroys: usize = 0;
let g_transient_destroys: usize = 0;

let g_destroy_log_buf: [16]string = [""; 16];
let g_destroy_log_len: usize = 0;

fn resetCounters() {
    g_singleton_creations = 0;
    g_scoped_creations = 0;
    g_transient_creations = 0;
    g_singleton_destroys = 0;
    g_scoped_destroys = 0;
    g_transient_destroys = 0;
    g_destroy_log_len = 0;
}

fn pushDestroyLog(label: string) {
    if (g_destroy_log_len < g_destroy_log_buf.len) {
        g_destroy_log_buf[g_destroy_log_len] = label;
        g_destroy_log_len += 1;
    }
}

struct TrackedService {
    id: usize;
    label: string;
}

fn makeTracked(alloc: std.mem.Allocator, id: usize, label: string): *! ! DiError {
    let obj = alloc.create(TrackedService) catch return DiError.OutOfMemory;
    obj.* = TrackedService{ id: id, label: label };
    unsafe { return obj as *!; }
}

fn createSingletonSvc(_: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    g_singleton_creations += 1;
    return makeTracked(alloc, g_singleton_creations, "singleton");
}

fn destroySingletonSvc(ptr: *!, alloc: std.mem.Allocator) {
    g_singleton_destroys += 1;
    unsafe {
        alloc.destroy(ptr as *TrackedService!);
    }
}

fn createScopedSvc(_: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    g_scoped_creations += 1;
    return makeTracked(alloc, g_scoped_creations, "scoped");
}

fn destroyScopedSvc(ptr: *!, alloc: std.mem.Allocator) {
    g_scoped_destroys += 1;
    unsafe {
        alloc.destroy(ptr as *TrackedService!);
    }
}

fn createTransientSvc(_: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    g_transient_creations += 1;
    return makeTracked(alloc, g_transient_creations, "transient");
}

fn destroyTransientSvc(ptr: *!, alloc: std.mem.Allocator) {
    g_transient_destroys += 1;
    unsafe {
        alloc.destroy(ptr as *TrackedService!);
    }
}

// ============================================================================
// Test 1: Singleton / Scoped / Transient Lifetime Semantics
// ============================================================================

fn testLifetimeSemantics(): !void {
    resetCounters();
    const alloc = std.heap.page_allocator;

    let container = ServiceContainer.init(alloc);

    container.registerSingleton("cfg", "TrackedService", createSingletonSvc, destroySingletonSvc)?;
    container.registerScoped("req_ctx", "TrackedService", createScopedSvc, destroyScopedSvc)?;
    container.registerTransient("token", "TrackedService", createTransientSvc, destroyTransientSvc)?;
    container.build()?;

    {
        let scope1 = container.createScope();
        defer scope1.deinit();

        const s1_a = scope1.resolveAs(TrackedService, "cfg")?;
        const s1_b = scope1.resolveAs(TrackedService, "cfg")?;
        expect_eq(s1_a.id, 1);
        expect_eq(s1_b.id, 1);
        expect_eq(g_singleton_creations, 1);

        const sc1_a = scope1.resolveAs(TrackedService, "req_ctx")?;
        const sc1_b = scope1.resolveAs(TrackedService, "req_ctx")?;
        expect_eq(sc1_a.id, 1);
        expect_eq(sc1_b.id, 1);
        expect_eq(g_scoped_creations, 1);

        const t1 = scope1.resolveAs(TrackedService, "token")?;
        const t2 = scope1.resolveAs(TrackedService, "token")?;
        expect_eq(t1.id, 1);
        expect_eq(t2.id, 2);
        expect_eq(g_transient_creations, 2);

        // Second scope: shares Singleton, isolates Scoped, creates new Transient
        let scope2 = container.createScope();
        defer scope2.deinit();

        const s2 = scope2.resolveAs(TrackedService, "cfg")?;
        expect_eq(s2.id, 1);
        expect_eq(g_singleton_creations, 1);

        const sc2 = scope2.resolveAs(TrackedService, "req_ctx")?;
        expect_eq(sc2.id, 2);
        expect_eq(g_scoped_creations, 2);

        const t3 = scope2.resolveAs(TrackedService, "token")?;
        expect_eq(t3.id, 3);
        expect_eq(g_transient_creations, 3);
    }

    // After both scopes deinit: scoped (2) and transient (3) destroyed; singleton (0) still alive
    expect_eq(g_scoped_destroys, 2);
    expect_eq(g_transient_destroys, 3);
    expect_eq(g_singleton_destroys, 0);

    // Root container resolves the same singleton
    const s_root = container.resolveAs(TrackedService, "cfg")?;
    expect_eq(s_root.id, 1);
    expect_eq(g_singleton_creations, 1);

    container.deinit();
    expect_eq(g_singleton_destroys, 1);
}

// ============================================================================
// Test 2: Multi-layer Dependency Injection & Reverse-Order deinit()
// ============================================================================

struct LayeredNode {
    name: string;
    dep: *TrackedService!?;
}

fn createRepo(_: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    return makeTracked(alloc, 10, "DatabaseRepo");
}

fn createUserService(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    const repo = scope.resolveAs(TrackedService, "repo")?;
    if (repo.id != 10) return DiError.ServiceNotFound;
    return makeTracked(alloc, 20, "UserService");
}

fn createController(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    const svc = scope.resolveAs(TrackedService, "user_svc")?;
    if (svc.id != 20) return DiError.ServiceNotFound;
    return makeTracked(alloc, 30, "Controller");
}

fn destroyLogged(ptr: *!, alloc: std.mem.Allocator) {
    unsafe {
        const obj = ptr as *TrackedService!;
        pushDestroyLog(obj.label);
        alloc.destroy(obj);
    }
}

fn createSpan(_: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    return makeTracked(alloc, 101, "TraceSpan");
}

fn createSession(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    const span = scope.resolveAs(TrackedService, "span")?;
    if (span.id != 101) return DiError.ServiceNotFound;
    return makeTracked(alloc, 102, "Session");
}

fn createHandler(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    const sess = scope.resolveAs(TrackedService, "session")?;
    if (sess.id != 102) return DiError.ServiceNotFound;
    return makeTracked(alloc, 103, "Handler");
}

fn testMultiLayerAndReverseDestruction(): !void {
    resetCounters();
    const alloc = std.heap.page_allocator;

    let container = ServiceContainer.init(alloc);
    container.registerSingleton("repo", "TrackedService", createRepo, destroyLogged)?;
    container.registerSingleton("user_svc", "TrackedService", createUserService, destroyLogged)?;
    container.registerSingleton("controller", "TrackedService", createController, destroyLogged)?;

    container.registerTransient("span", "TrackedService", createSpan, destroyLogged)?;
    container.registerScoped("session", "TrackedService", createSession, destroyLogged)?;
    container.registerScoped("handler", "TrackedService", createHandler, destroyLogged)?;
    container.build()?;

    {
        let scope = container.createScope();
        const ctrl = scope.resolveAs(TrackedService, "controller")?;
        expect_eq(ctrl.id, 30);

        const handler = scope.resolveAs(TrackedService, "handler")?;
        expect_eq(handler.id, 103);

        scope.deinit();

        // Scope deinit destroys Handler -> Session -> TraceSpan (dependents before dependencies)
        expect_eq(g_destroy_log_len, 3);
        expect_eq(std.mem.eql(u8, g_destroy_log_buf[0], "Handler"), true);
        expect_eq(std.mem.eql(u8, g_destroy_log_buf[1], "Session"), true);
        expect_eq(std.mem.eql(u8, g_destroy_log_buf[2], "TraceSpan"), true);
    }

    container.deinit();

    // Container deinit destroys Controller -> UserService -> DatabaseRepo
    expect_eq(g_destroy_log_len, 6);
    expect_eq(std.mem.eql(u8, g_destroy_log_buf[3], "Controller"), true);
    expect_eq(std.mem.eql(u8, g_destroy_log_buf[4], "UserService"), true);
    expect_eq(std.mem.eql(u8, g_destroy_log_buf[5], "DatabaseRepo"), true);
}

// ============================================================================
// Test 3: CircularDependency & Captive Dependency (ScopeViolation)
// ============================================================================

fn createCycleA(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    * = scope.resolve("cycle_b")?;
    return makeTracked(alloc, 1, "A");
}

fn createCycleB(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    * = scope.resolve("cycle_c")?;
    return makeTracked(alloc, 2, "B");
}

fn createCycleC(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    * = scope.resolve("cycle_a")?;
    return makeTracked(alloc, 3, "C");
}

fn createCycleEntry(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    * = scope.resolve("cycle_a")?;
    return makeTracked(alloc, 4, "Entry");
}

fn createBadCaptiveSingleton(scope: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    * = scope.resolve("scoped_uow")?;
    return makeTracked(alloc, 99, "BadSingleton");
}

fn testCycleAndCaptiveDependencyGuards(): !void {
    const alloc = std.heap.page_allocator;
    let container = ServiceContainer.init(alloc);
    defer container.deinit();

    container.registerTransient("cycle_entry", "TrackedService", createCycleEntry, destroyTransientSvc)?;
    container.registerTransient("cycle_a", "TrackedService", createCycleA, destroyTransientSvc)?;
    container.registerTransient("cycle_b", "TrackedService", createCycleB, destroyTransientSvc)?;
    container.registerTransient("cycle_c", "TrackedService", createCycleC, destroyTransientSvc)?;

    container.registerScoped("scoped_uow", "TrackedService", createScopedSvc, destroyScopedSvc)?;
    container.registerSingleton("bad_singleton", "TrackedService", createBadCaptiveSingleton, destroySingletonSvc)?;
    container.build()?;

    // 1. Direct cycle A -> B -> C -> A
    let hit_cycle = false;
    * = container.resolve("cycle_a") catch |e| {
        if (e == DiError.CircularDependency) {
            hit_cycle = true;
        }
    };
    expect_eq(hit_cycle, true);
    expect_eq(std.mem.eql(u8, container.lastErrorCycle(), "cycle_a -> cycle_b -> cycle_c -> cycle_a"), true);

    // 2. Entry into cycle: entry -> cycle_a -> cycle_b -> cycle_c -> cycle_a (cycle starts at cycle_a)
    let hit_entry_cycle = false;
    * = container.resolve("cycle_entry") catch |e| {
        if (e == DiError.CircularDependency) {
            hit_entry_cycle = true;
        }
    };
    expect_eq(hit_entry_cycle, true);
    expect_eq(std.mem.eql(u8, container.lastErrorCycle(), "cycle_a -> cycle_b -> cycle_c -> cycle_a"), true);

    // 3. Root container resolving Scoped service -> ScopeViolation
    let hit_root_scope_violation = false;
    * = container.resolve("scoped_uow") catch |e| {
        if (e == DiError.ScopeViolation) {
            hit_root_scope_violation = true;
        }
    };
    expect_eq(hit_root_scope_violation, true);

    // 4. Singleton depending on Scoped service (Captive Dependency) -> ScopeViolation
    let scope = container.createScope();
    defer scope.deinit();

    let hit_captive = false;
    * = scope.resolve("bad_singleton") catch |e| {
        if (e == DiError.ScopeViolation) {
            hit_captive = true;
        }
    };
    expect_eq(hit_captive, true);

    // 5. Missing service -> ServiceNotFound & tryResolve returns null
    let hit_not_found = false;
    * = scope.resolve("does_not_exist") catch |e| {
        if (e == DiError.ServiceNotFound) {
            hit_not_found = true;
        }
    };
    expect_eq(hit_not_found, true);
    expect_eq(scope.tryResolve("does_not_exist") == null, true);
    expect_eq(scope.tryResolveAs(TrackedService, "does_not_exist") == null, true);
}

// ============================================================================
// Test 4: Multi-Implementation Extension Points (registerMany + resolveAll) & ContainerFrozen
// ============================================================================

struct PluginInfo {
    name: string;
    priority: i32;
}

fn createAuthPlugin(_: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    let p = alloc.create(PluginInfo) catch return DiError.OutOfMemory;
    p.* = PluginInfo{ name: "auth-plugin", priority: 10 };
    unsafe { return p as *!; }
}

fn createAuditPlugin(_: *ServiceScope, alloc: std.mem.Allocator): *! ! DiError {
    let p = alloc.create(PluginInfo) catch return DiError.OutOfMemory;
    p.* = PluginInfo{ name: "audit-plugin", priority: 30 };
    unsafe { return p as *!; }
}

fn destroyPlugin(ptr: *!, alloc: std.mem.Allocator) {
    unsafe {
        alloc.destroy(ptr as *PluginInfo!);
    }
}

fn testMultiImplementationAndFreeze(): !void {
    const alloc = std.heap.page_allocator;
    let container = ServiceContainer.init(alloc);
    defer container.deinit();

    // Duplicate single registration guard
    container.registerSingleton("unique_svc", "TrackedService", createSingletonSvc, destroySingletonSvc)?;
    let hit_dup = false;
    container.registerSingleton("unique_svc", "TrackedService", createSingletonSvc, destroySingletonSvc) catch |e| {
        if (e == DiError.DuplicateRegistration) {
            hit_dup = true;
        }
    };
    expect_eq(hit_dup, true);

    // Multi-implementation registration (mix factory + pre-built value)
    container.registerMany("plugin", .Singleton, "PluginInfo", createAuthPlugin, destroyPlugin)?;

    let rate_limit = alloc.create(PluginInfo) catch return DiError.OutOfMemory;
    rate_limit.* = PluginInfo{ name: "rate-limit-plugin", priority: 20 };
    unsafe {
        container.registerManyValue("plugin", rate_limit as *!, "PluginInfo", destroyPlugin)?;
    }
    container.registerMany("plugin", .Singleton, "PluginInfo", createAuditPlugin, destroyPlugin)?;

    expect_eq(container.isFrozen(), false);
    container.build()?;
    expect_eq(container.isFrozen(), true);

    // ContainerFrozen guard after build()
    let hit_frozen = false;
    container.registerTransient("late_svc", "TrackedService", createTransientSvc, destroyTransientSvc) catch |e| {
        if (e == DiError.ContainerFrozen) {
            hit_frozen = true;
        }
    };
    expect_eq(hit_frozen, true);

    let scope = container.createScope();
    defer scope.deinit();

    const plugins = scope.resolveAll("plugin")?;
    defer alloc.free(plugins);

    expect_eq(plugins.len, 3);
    expect_eq(std.mem.eql(u8, plugins[0].asPtr(PluginInfo).name, "auth-plugin"), true);
    expect_eq(plugins[0].asPtr(PluginInfo).priority, 10);
    expect_eq(std.mem.eql(u8, plugins[1].asPtr(PluginInfo).name, "rate-limit-plugin"), true);
    expect_eq(plugins[1].asPtr(PluginInfo).priority, 20);
    expect_eq(std.mem.eql(u8, plugins[2].asPtr(PluginInfo).name, "audit-plugin"), true);
    expect_eq(plugins[2].asPtr(PluginInfo).priority, 30);
}

// ============================================================================
// Test 5: Dynamic Lazy Namespace Mounting via `import cap from "cap"; cap.di`
// ============================================================================

fn testCapDiDynamicMount(): !void {
    const alloc = std.heap.page_allocator;
    let container = cap.sys.ServiceContainer.init(alloc);
    defer container.deinit();

    expect_eq(cap.sys.MAX_RESOLUTION_DEPTH, 32);

    container.registerSingleton("dynamic_svc", "TrackedService", createSingletonSvc, destroySingletonSvc)?;
    container.build()?;

    let scope = container.createScope();
    defer scope.deinit();

    const svc = scope.resolveAs(TrackedService, "dynamic_svc")?;
    expect_eq(std.mem.eql(u8, svc.label, "singleton"), true);
    expect_eq(container.has("dynamic_svc"), true);
    expect_eq(container.count(), 1);
}

fn main() {
    test("1. Singleton, Scoped, and Transient lifetime semantics", () => {
        testLifetimeSemantics()?;
    });
    test("2. Multi-layer dependency injection and strict reverse-order deinit()", () => {
        testMultiLayerAndReverseDestruction()?;
    });
    test("3. CircularDependency path reporting & Captive Dependency ScopeViolation", () => {
        testCycleAndCaptiveDependencyGuards()?;
    });
    test("4. Multi-implementation registerMany/resolveAll & build() ContainerFrozen", () => {
        testMultiImplementationAndFreeze()?;
    });
    test("5. import cap from \"cap\"; cap.di Automatic Dynamic Mounting", () => {
        testCapDiDynamicMount()?;
    });
}
