// Source: cp/lib/di/container.cp (Pure CP Dependency Injection & IoC Lifetime Container)
// Pure CP zero-external-dependency IoC container supporting Singleton, Scoped, and Transient lifetimes,
// factory resolution stack, circular dependency path reporting, captive dependency protection,
// multi-implementation extension points, and deterministic reverse-order deinit().

const std = @import("std");

pub const MAX_RESOLUTION_DEPTH: usize = 32;

pub err DiError {
    ServiceNotFound,
    DuplicateRegistration,
    CircularDependency,
    ScopeViolation,
    ContainerFrozen,
    MaxDepthExceeded,
    OutOfMemory,
}

pub enum ServiceLifetime {
    Singleton,
    Scoped,
    Transient,
}

pub type DestructorFn = *const fn (*!, std.mem.Allocator) void;
pub type FactoryFn = *const fn (*ServiceScope, std.mem.Allocator) *! ! DiError;

pub struct ServiceInstance {
    ptr: *!;
    type_tag: string;
    destructor: DestructorFn? = null;

    pub static fn init(ptr: *!, type_tag: string, destructor: DestructorFn?): ServiceInstance {
        return ServiceInstance{
            ptr: ptr,
            type_tag: type_tag,
            destructor: destructor,
        };
    }

    pub fn asPtr(comptime T: type): *T! {
        unsafe {
            return _.ptr as *T!;
        }
    }
}

struct OwnedInstance {
    ptr: *!;
    destructor: DestructorFn;
}

struct ServiceDescriptor {
    name: string;
    type_tag: string;
    lifetime: ServiceLifetime;
    factory: FactoryFn?;
    destructor: DestructorFn?;
    singleton_ptr: *!? = null;
}

struct ServiceEntry {
    name: string;
    is_multi: bool;
    descriptors: std.ArrayListUnmanaged(ServiceDescriptor) = .empty;
}

struct ResolutionFrame {
    name: string;
    lifetime: ServiceLifetime;
}

struct ScopedCacheItem {
    entry_idx: usize;
    desc_idx: usize;
    ptr: *!;
}

pub struct ServiceContainer {
    alloc: std.mem.Allocator;
    frozen: bool = false;
    entries: std.ArrayListUnmanaged(ServiceEntry) = .empty;
    name_map: std.StringHashMapUnmanaged(usize) = .empty;
    owned_singletons: std.ArrayListUnmanaged(OwnedInstance) = .empty;
    last_cycle: []u8 = "";

    pub fn init(alloc: std.mem.Allocator): ServiceContainer {
        return ServiceContainer{
            alloc: alloc,
            frozen: false,
            entries: .empty,
            name_map: .empty,
            owned_singletons: .empty,
            last_cycle: "",
        };
    }

    pub fn deinit(*_) {
        // Destroy singletons and root-owned instances strictly in reverse instantiation order
        let i: usize = _.owned_singletons.items.len;
        while (i > 0) {
            i -= 1;
            const item = _.owned_singletons.items[i];
            item.destructor(item.ptr, _.alloc);
        }
        _.owned_singletons.deinit(_.alloc);

        for (*entry of _.entries.items) {
            for (desc of entry.descriptors.items) {
                _.alloc.free(desc.type_tag);
            }
            entry.descriptors.deinit(_.alloc);
            _.alloc.free(entry.name);
        }
        _.entries.deinit(_.alloc);
        _.name_map.deinit(_.alloc);

        if (_.last_cycle.len > 0) {
            _.alloc.free(_.last_cycle);
            _.last_cycle = "";
        }
    }

    fn setLastCycle(*_, cycle: []const u8): void ! DiError {
        if (_.last_cycle.len > 0) {
            _.alloc.free(_.last_cycle);
            _.last_cycle = "";
        }
        const duped = _.alloc.dupe(u8, cycle) catch return DiError.OutOfMemory;
        _.last_cycle = duped;
    }

    pub fn lastErrorCycle(): string {
        return _.last_cycle;
    }

    pub fn isFrozen(): bool {
        return _.frozen;
    }

    pub fn has(name: string): bool {
        return _.name_map.contains(name);
    }

    pub fn count(): usize {
        return _.entries.items.len;
    }

    fn addDescriptor(*_, name: string, type_tag: string, lifetime: ServiceLifetime, factory: FactoryFn?, destructor: DestructorFn?, singleton_ptr: *!?, allow_multi: bool): void ! DiError {
        if (_.frozen) {
            return DiError.ContainerFrozen;
        }
        if (name.len == 0) {
            return DiError.ServiceNotFound;
        }

        const duped_tag = _.alloc.dupe(u8, type_tag) catch return DiError.OutOfMemory;
        errdefer _.alloc.free(duped_tag);

        if (existing_idx := _.name_map.get(name)) {
            if (!allow_multi) {
                return DiError.DuplicateRegistration;
            }
            let entry = *_.entries.items[existing_idx];
            if (!entry.is_multi) {
                return DiError.DuplicateRegistration;
            }
            entry.descriptors.append(_.alloc, ServiceDescriptor{
                name: entry.name,
                type_tag: duped_tag,
                lifetime: lifetime,
                factory: factory,
                destructor: destructor,
                singleton_ptr: singleton_ptr,
            }) catch return DiError.OutOfMemory;
        } else {
            const duped_name = _.alloc.dupe(u8, name) catch return DiError.OutOfMemory;
            errdefer _.alloc.free(duped_name);

            let new_entry = ServiceEntry{
                name: duped_name,
                is_multi: allow_multi,
                descriptors: .empty,
            };
            errdefer new_entry.descriptors.deinit(_.alloc);

            new_entry.descriptors.append(_.alloc, ServiceDescriptor{
                name: duped_name,
                type_tag: duped_tag,
                lifetime: lifetime,
                factory: factory,
                destructor: destructor,
                singleton_ptr: singleton_ptr,
            }) catch return DiError.OutOfMemory;

            const new_idx = _.entries.items.len;
            _.entries.append(_.alloc, new_entry) catch return DiError.OutOfMemory;
            _.name_map.put(_.alloc, duped_name, new_idx) catch {
                * = _.entries.pop();
                return DiError.OutOfMemory;
            };
        }
    }

    pub fn registerSingleton(*_, name: string, type_tag: string, factory: FactoryFn, destructor: DestructorFn?): void ! DiError {
        return _.addDescriptor(name, type_tag, .Singleton, factory, destructor, null, false);
    }

    pub fn registerSingletonValue(*_, name: string, ptr: *!, type_tag: string, destructor: DestructorFn?): void ! DiError {
        if (_.frozen) {
            return DiError.ContainerFrozen;
        }
        _.addDescriptor(name, type_tag, .Singleton, null, destructor, ptr, false)?;
        if (dtor := destructor) {
            _.owned_singletons.append(_.alloc, OwnedInstance{
                ptr: ptr,
                destructor: dtor,
            }) catch return DiError.OutOfMemory;
        }
    }

    pub fn registerScoped(*_, name: string, type_tag: string, factory: FactoryFn, destructor: DestructorFn?): void ! DiError {
        return _.addDescriptor(name, type_tag, .Scoped, factory, destructor, null, false);
    }

    pub fn registerTransient(*_, name: string, type_tag: string, factory: FactoryFn, destructor: DestructorFn?): void ! DiError {
        return _.addDescriptor(name, type_tag, .Transient, factory, destructor, null, false);
    }

    pub fn registerMany(*_, name: string, lifetime: ServiceLifetime, type_tag: string, factory: FactoryFn, destructor: DestructorFn?): void ! DiError {
        return _.addDescriptor(name, type_tag, lifetime, factory, destructor, null, true);
    }

    pub fn registerManyValue(*_, name: string, ptr: *!, type_tag: string, destructor: DestructorFn?): void ! DiError {
        if (_.frozen) {
            return DiError.ContainerFrozen;
        }
        _.addDescriptor(name, type_tag, .Singleton, null, destructor, ptr, true)?;
        if (dtor := destructor) {
            _.owned_singletons.append(_.alloc, OwnedInstance{
                ptr: ptr,
                destructor: dtor,
            }) catch return DiError.OutOfMemory;
        }
    }

    pub fn build(*_): void ! DiError {
        _.frozen = true;
    }

    pub fn createScope(*_): ServiceScope {
        return ServiceScope.init(_, _.alloc, false);
    }

    pub fn resolve(*_, name: string): ServiceInstance ! DiError {
        let root_scope = ServiceScope.init(_, _.alloc, true);
        defer root_scope.deinit();
        return root_scope.resolve(name);
    }

    pub fn tryResolve(*_, name: string): ServiceInstance? {
        const inst = _.resolve(name) catch return null;
        return inst;
    }

    pub fn resolveAs(*_, comptime T: type, name: string): *T! ! DiError {
        const inst = _.resolve(name)?;
        unsafe {
            return inst.ptr as *T!;
        }
    }

    pub fn tryResolveAs(*_, comptime T: type, name: string): *T!? {
        const inst = _.resolve(name) catch return null;
        unsafe {
            return inst.ptr as *T!;
        }
    }

    pub fn resolveAll(*_, name: string): ([]ServiceInstance) ! DiError {
        let root_scope = ServiceScope.init(_, _.alloc, true);
        defer root_scope.deinit();
        return root_scope.resolveAll(name);
    }
}

pub struct ServiceScope {
    container: *ServiceContainer;
    alloc: std.mem.Allocator;
    is_root: bool;
    scoped_cache: std.ArrayListUnmanaged(ScopedCacheItem) = .empty;
    owned_instances: std.ArrayListUnmanaged(OwnedInstance) = .empty;
    resolution_stack: std.ArrayListUnmanaged(ResolutionFrame) = .empty;

    pub fn init(container: *ServiceContainer, alloc: std.mem.Allocator, is_root: bool): ServiceScope {
        return ServiceScope{
            container: container,
            alloc: alloc,
            is_root: is_root,
            scoped_cache: .empty,
            owned_instances: .empty,
            resolution_stack: .empty,
        };
    }

    pub fn deinit(*_) {
        // Destroy scoped and scope-owned transient instances strictly in reverse instantiation order
        let i: usize = _.owned_instances.items.len;
        while (i > 0) {
            i -= 1;
            const item = _.owned_instances.items[i];
            item.destructor(item.ptr, _.alloc);
        }
        _.owned_instances.deinit(_.alloc);
        _.scoped_cache.deinit(_.alloc);
        _.resolution_stack.deinit(_.alloc);
    }

    pub fn lastErrorCycle(): string {
        return _.container.lastErrorCycle();
    }

    pub fn has(name: string): bool {
        return _.container.has(name);
    }

    fn hasSingletonAncestor(): bool {
        for (frame of _.resolution_stack.items) {
            if (frame.lifetime == .Singleton) {
                return true;
            }
        }
        return false;
    }

    fn findScopedCached(entry_idx: usize, desc_idx: usize): *!? {
        for (item of _.scoped_cache.items) {
            if (item.entry_idx == entry_idx && item.desc_idx == desc_idx) {
                return item.ptr;
            }
        }
        return null;
    }

    fn recordCycle(*_, target_name: string, start_idx: usize): void ! DiError {
        let buf: std.ArrayListUnmanaged(u8) = .empty;
        defer buf.deinit(_.alloc);

        let idx: usize = start_idx;
        while (idx < _.resolution_stack.items.len) : (idx += 1) {
            if (idx > start_idx) {
                buf.appendSlice(_.alloc, " -> ") catch return DiError.OutOfMemory;
            }
            buf.appendSlice(_.alloc, _.resolution_stack.items[idx].name) catch return DiError.OutOfMemory;
        }
        buf.appendSlice(_.alloc, " -> ") catch return DiError.OutOfMemory;
        buf.appendSlice(_.alloc, target_name) catch return DiError.OutOfMemory;

        _.container.setLastCycle(buf.items)?;
    }

    fn resolveDescriptor(*_, entry_idx: usize, desc_idx: usize): ServiceInstance ! DiError {
        const desc_snapshot = _.container.entries.items[entry_idx].descriptors.items[desc_idx];
        const name = desc_snapshot.name;
        const lifetime = desc_snapshot.lifetime;

        // 1. Captive Dependency & Root Scope guard:
        //    - Scoped service cannot be resolved directly from root container.
        //    - Scoped service cannot be resolved inside a Singleton factory.
        if (lifetime == .Scoped) {
            if (_.is_root || _.hasSingletonAncestor()) {
                return DiError.ScopeViolation;
            }
        }

        // 2. Fast path: already cached Singleton or Scoped instance
        if (lifetime == .Singleton) {
            if (cached_ptr := desc_snapshot.singleton_ptr) {
                return ServiceInstance.init(cached_ptr, desc_snapshot.type_tag, desc_snapshot.destructor);
            }
        } else if (lifetime == .Scoped) {
            if (cached_ptr := _.findScopedCached(entry_idx, desc_idx)) {
                return ServiceInstance.init(cached_ptr, desc_snapshot.type_tag, desc_snapshot.destructor);
            }
        }

        // 3. Circular dependency detection on active resolution stack
        let stack_idx: usize = 0;
        while (stack_idx < _.resolution_stack.items.len) : (stack_idx += 1) {
            if (std.mem.eql(u8, _.resolution_stack.items[stack_idx].name, name)) {
                _.recordCycle(name, stack_idx)?;
                return DiError.CircularDependency;
            }
        }

        // 4. Max resolution depth guard
        if (_.resolution_stack.items.len >= MAX_RESOLUTION_DEPTH) {
            return DiError.MaxDepthExceeded;
        }

        if (factory := desc_snapshot.factory) {
            _.resolution_stack.append(_.alloc, ResolutionFrame{
                name: name,
                lifetime: lifetime,
            }) catch return DiError.OutOfMemory;
            defer {
                * = _.resolution_stack.pop();
            }

            const raw_ptr = factory(_, _.container.alloc)?;

            // 5. Store instance & register destructor according to lifetime
            if (lifetime == .Singleton) {
                _.container.entries.items[entry_idx].descriptors.items[desc_idx].singleton_ptr = raw_ptr;
                if (dtor := desc_snapshot.destructor) {
                    _.container.owned_singletons.append(_.container.alloc, OwnedInstance{
                        ptr: raw_ptr,
                        destructor: dtor,
                    }) catch return DiError.OutOfMemory;
                }
            } else if (lifetime == .Scoped) {
                _.scoped_cache.append(_.alloc, ScopedCacheItem{
                    entry_idx: entry_idx,
                    desc_idx: desc_idx,
                    ptr: raw_ptr,
                }) catch return DiError.OutOfMemory;
                if (dtor := desc_snapshot.destructor) {
                    _.owned_instances.append(_.alloc, OwnedInstance{
                        ptr: raw_ptr,
                        destructor: dtor,
                    }) catch return DiError.OutOfMemory;
                }
            } else {
                // Transient: if created inside a Singleton factory, bind lifetime to root container
                // so the Singleton never holds a dangling pointer after a child scope exits.
                if (dtor := desc_snapshot.destructor) {
                    if (_.hasSingletonAncestor()) {
                        _.container.owned_singletons.append(_.container.alloc, OwnedInstance{
                            ptr: raw_ptr,
                            destructor: dtor,
                        }) catch return DiError.OutOfMemory;
                    } else {
                        _.owned_instances.append(_.alloc, OwnedInstance{
                            ptr: raw_ptr,
                            destructor: dtor,
                        }) catch return DiError.OutOfMemory;
                    }
                }
            }

            return ServiceInstance.init(raw_ptr, desc_snapshot.type_tag, desc_snapshot.destructor);
        } else {
            return DiError.ServiceNotFound;
        }
    }

    pub fn resolve(*_, name: string): ServiceInstance ! DiError {
        if (entry_idx := _.container.name_map.get(name)) {
            const entry = _.container.entries.items[entry_idx];
            if (entry.descriptors.items.len == 0) {
                return DiError.ServiceNotFound;
            }
            // For single or multi registration, resolve() returns the latest/primary descriptor
            const desc_idx = entry.descriptors.items.len - 1;
            return _.resolveDescriptor(entry_idx, desc_idx);
        } else {
            return DiError.ServiceNotFound;
        }
    }

    pub fn tryResolve(*_, name: string): ServiceInstance? {
        const inst = _.resolve(name) catch return null;
        return inst;
    }

    pub fn resolveAs(*_, comptime T: type, name: string): *T! ! DiError {
        const inst = _.resolve(name)?;
        unsafe {
            return inst.ptr as *T!;
        }
    }

    pub fn tryResolveAs(*_, comptime T: type, name: string): *T!? {
        const inst = _.resolve(name) catch return null;
        unsafe {
            return inst.ptr as *T!;
        }
    }

    pub fn resolveAll(*_, name: string): ([]ServiceInstance) ! DiError {
        if (entry_idx := _.container.name_map.get(name)) {
            const desc_count = _.container.entries.items[entry_idx].descriptors.items.len;
            if (desc_count == 0) {
                return DiError.ServiceNotFound;
            }

            let results: std.ArrayListUnmanaged(ServiceInstance) = .empty;
            errdefer results.deinit(_.alloc);

            let desc_idx: usize = 0;
            while (desc_idx < desc_count) : (desc_idx += 1) {
                const inst = _.resolveDescriptor(entry_idx, desc_idx)?;
                results.append(_.alloc, inst) catch return DiError.OutOfMemory;
            }

            return results.toOwnedSlice(_.alloc) catch return DiError.OutOfMemory;
        } else {
            return DiError.ServiceNotFound;
        }
    }
}
