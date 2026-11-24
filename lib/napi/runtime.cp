// Source: Absorbed from /data/dev/bun/src/napi/napi.cp | Synced: 2026-11-23T21:59:28Z
// lib/napi/runtime.cp
// Pure CP Node-API v9 Runtime Context, HandleScope/Ref/AsyncWork/TSFN State & Mock Env (cap:napi)

const std = @import("std");
import {
    NAPI_VERSION,
    NAPI_MODULE_VERSION,
    NapiStatus,
    NapiValueType,
    NapiTsfnReleaseMode,
    NapiTsfnCallMode,
    NapiExtendedErrorInfo,
    NapiNodeVersion,
    NapiPropertyDescriptor,
    NapiModule
} from "./_napi";
import { Napi, NapiAddonRegistry } from "./napi";

pub const NAPI_MAX_MOCK_VALUES: usize = 256;
pub const NAPI_MAX_MOCK_PROPS: usize = 64;
pub const NAPI_MAX_SCOPES: usize = 32;
pub const NAPI_MAX_REFS: usize = 64;
pub const NAPI_MAX_ASYNC_WORKS: usize = 32;
pub const NAPI_MAX_TSFN_QUEUE: usize = 32;

pub struct NapiCallbackInfoData {
    pub argc: usize = 0,
    pub argv: [16]usize = [0; 16],
    pub this_arg: usize = 0,
    pub data: (*anyopaque)? = null,
    pub new_target: usize = 0,

    pub static fn init(this_arg: usize, data: (*anyopaque)?): NapiCallbackInfoData {
        return NapiCallbackInfoData{
            argc: 0,
            argv: [0; 16],
            this_arg: this_arg,
            data: data,
            new_target: 0,
        };
    }

    pub fn pushArg(*_, val_id: usize): bool {
        if (_.argc >= 16) {
            return false;
        }
        _.argv[_.argc] = val_id;
        _.argc += 1;
        return true;
    }
}

pub enum NapiAsyncWorkStatus: u8 {
    Pending = 0,
    Queued = 1,
    Running = 2,
    Completed = 3,
    Cancelled = 4,
    Deleted = 5,
}

pub struct NapiAsyncWorkEntry {
    pub active: bool = false,
    pub id: usize = 0,
    pub resource_name: []const u8 = "",
    pub status: NapiAsyncWorkStatus = .Pending,
    pub data: (*anyopaque)? = null,
    pub execute_cb: (*const fn (usize, (*anyopaque)?): void)? = null,
    pub complete_cb: (*const fn (usize, NapiStatus, (*anyopaque)?): void)? = null,
    pub executed: bool = false,
    pub completed: bool = false,
}

pub struct NapiThreadsafeFunctionState {
    pub active: bool = true,
    pub resource_name: []const u8 = "",
    pub max_queue_size: usize = NAPI_MAX_TSFN_QUEUE,
    pub thread_count: usize = 1,
    pub ref_count: usize = 1,
    pub is_closing: bool = false,
    pub is_aborted: bool = false,
    pub finalized: bool = false,
    pub context: (*anyopaque)? = null,
    pub queue: [NAPI_MAX_TSFN_QUEUE]usize = [0; NAPI_MAX_TSFN_QUEUE],
    pub head: usize = 0,
    pub tail: usize = 0,
    pub count: usize = 0,
    pub dispatched_count: usize = 0,

    pub static fn init(name: []const u8, max_queue: usize, initial_threads: usize): NapiThreadsafeFunctionState {
        const cap = if (max_queue == 0 || max_queue > NAPI_MAX_TSFN_QUEUE) NAPI_MAX_TSFN_QUEUE else max_queue;
        return NapiThreadsafeFunctionState{
            active: true,
            resource_name: name,
            max_queue_size: cap,
            thread_count: initial_threads,
            ref_count: 1,
            is_closing: false,
            is_aborted: false,
            finalized: false,
            context: null,
            queue: [0; NAPI_MAX_TSFN_QUEUE],
            head: 0,
            tail: 0,
            count: 0,
            dispatched_count: 0,
        };
    }

    pub fn acquire(*_): NapiStatus {
        if (_.is_closing || _.is_aborted) {
            return NapiStatus.Closing;
        }
        _.thread_count += 1;
        return NapiStatus.Ok;
    }

    pub fn releaseThread(*_, mode: NapiTsfnReleaseMode): NapiStatus {
        if (mode == NapiTsfnReleaseMode.Abort) {
            _.is_aborted = true;
            _.is_closing = true;
        }
        if (_.thread_count > 0) {
            _.thread_count -= 1;
        }
        if (_.thread_count == 0) {
            _.is_closing = true;
            _.finalized = true;
            _.active = false;
        }
        return NapiStatus.Ok;
    }

    pub fn ref(*_): NapiStatus {
        _.ref_count += 1;
        return NapiStatus.Ok;
    }

    pub fn unref(*_): NapiStatus {
        if (_.ref_count > 0) {
            _.ref_count -= 1;
        }
        return NapiStatus.Ok;
    }

    pub fn call(*_, payload: usize, mode: NapiTsfnCallMode): NapiStatus {
        * = mode;
        if (_.is_closing || _.is_aborted) {
            return NapiStatus.Closing;
        }
        if (_.count >= _.max_queue_size) {
            return NapiStatus.QueueFull;
        }
        _.queue[_.tail] = payload;
        _.tail = (_.tail + 1) % NAPI_MAX_TSFN_QUEUE;
        _.count += 1;
        return NapiStatus.Ok;
    }

    pub fn pop(*_): usize? {
        if (_.count == 0) {
            return null;
        }
        const val = _.queue[_.head];
        _.head = (_.head + 1) % NAPI_MAX_TSFN_QUEUE;
        _.count -= 1;
        _.dispatched_count += 1;
        return val;
    }

    pub fn drainAll(*_): usize {
        let drained: usize = 0;
        while (_.pop() != null) {
            drained += 1;
        }
        return drained;
    }
}

pub struct NapiRefEntry {
    pub active: bool = false,
    pub value_id: usize = 0,
    pub ref_count: u32 = 0,
    pub alive: bool = true,
}

pub struct NapiHandleScopeEntry {
    pub active: bool = false,
    pub is_escapable: bool = false,
    pub escaped: bool = false,
    pub start_handle_idx: usize = 0,
}

pub struct NapiMockProperty {
    pub active: bool = false,
    pub obj_id: usize = 0,
    pub name: []const u8 = "",
    pub value_id: usize = 0,
    pub attributes: u32 = 0,
}

pub struct NapiMockValue {
    pub active: bool = false,
    pub val_type: NapiValueType = .Undefined,
    pub scope_depth: usize = 0,
    pub bool_val: bool = false,
    pub num_val: f64 = 0.0,
    pub int_val: i64 = 0,
    pub str_val: []const u8 = "",
    pub ext_ptr: (*anyopaque)? = null,
    pub is_error: bool = false,
    pub error_code: []const u8 = "",
}

pub struct NapiRuntimeContext {
    pub values: [NAPI_MAX_MOCK_VALUES]NapiMockValue = undefined,
    pub value_count: usize = 0,
    pub props: [NAPI_MAX_MOCK_PROPS]NapiMockProperty = undefined,
    pub prop_count: usize = 0,
    pub scopes: [NAPI_MAX_SCOPES]NapiHandleScopeEntry = undefined,
    pub scope_depth: usize = 0,
    pub refs: [NAPI_MAX_REFS]NapiRefEntry = undefined,
    pub ref_count: usize = 0,
    pub async_works: [NAPI_MAX_ASYNC_WORKS]NapiAsyncWorkEntry = undefined,
    pub async_work_count: usize = 0,
    pub registry: NapiAddonRegistry = NapiAddonRegistry.init(),
    pub last_status: NapiStatus = .Ok,
    pub last_error_msg: []const u8 = "Success",
    pub pending_exception_id: usize? = null,
    pub node_version: NapiNodeVersion = NapiNodeVersion{ major: 20, minor: 0, patch: 0, release: null },

    pub static fn init(): NapiRuntimeContext {
        let ctx = NapiRuntimeContext{
            values: undefined,
            value_count: 0,
            props: undefined,
            prop_count: 0,
            scopes: undefined,
            scope_depth: 0,
            refs: undefined,
            ref_count: 0,
            async_works: undefined,
            async_work_count: 0,
            registry: NapiAddonRegistry.init(),
            last_status: .Ok,
            last_error_msg: "Success",
            pending_exception_id: null,
            node_version: NapiNodeVersion{ major: 20, minor: 0, patch: 0, release: "cp-napi-v9".ptr },
        };
        let i: usize = 0;
        while (i < NAPI_MAX_MOCK_VALUES) {
            ctx.values[i] = NapiMockValue{};
            i += 1;
        }
        i = 0;
        while (i < NAPI_MAX_MOCK_PROPS) {
            ctx.props[i] = NapiMockProperty{};
            i += 1;
        }
        i = 0;
        while (i < NAPI_MAX_SCOPES) {
            ctx.scopes[i] = NapiHandleScopeEntry{};
            i += 1;
        }
        i = 0;
        while (i < NAPI_MAX_REFS) {
            ctx.refs[i] = NapiRefEntry{};
            i += 1;
        }
        i = 0;
        while (i < NAPI_MAX_ASYNC_WORKS) {
            ctx.async_works[i] = NapiAsyncWorkEntry{};
            i += 1;
        }
        return ctx;
    }

    pub fn setLastError(*_, status: NapiStatus): NapiStatus {
        _.last_status = status;
        _.last_error_msg = Napi.statusMessage(status);
        return status;
    }

    pub fn getLastErrorInfo(): NapiExtendedErrorInfo {
        return NapiExtendedErrorInfo{
            error_message: _.last_error_msg.ptr,
            engine_reserved: null,
            engine_error_code: (@intFromEnum(_.last_status)) as u32,
            error_code: _.last_status,
        };
    }

    pub fn getNodeVersion(): NapiNodeVersion {
        return _.node_version;
    }

    fn allocValue(*_, val: NapiMockValue): usize? {
        if (_.value_count >= NAPI_MAX_MOCK_VALUES) {
            * = _.setLastError(.GenericFailure);
            return null;
        }
        const id = _.value_count;
        _.values[id] = val;
        _.values[id].active = true;
        _.values[id].scope_depth = _.scope_depth;
        _.value_count += 1;
        * = _.setLastError(.Ok);
        return id;
    }

    pub fn getUndefined(*_): usize? {
        return _.allocValue(NapiMockValue{ val_type: .Undefined });
    }

    pub fn getNull(*_): usize? {
        return _.allocValue(NapiMockValue{ val_type: .Null });
    }

    pub fn getBoolean(*_, b: bool): usize? {
        return _.allocValue(NapiMockValue{ val_type: .Boolean, bool_val: b });
    }

    pub fn createInt32(*_, v: i32): usize? {
        return _.allocValue(NapiMockValue{
            val_type: .Number,
            num_val: v as f64,
            int_val: v as i64,
        });
    }

    pub fn createDouble(*_, v: f64): usize? {
        return _.allocValue(NapiMockValue{
            val_type: .Number,
            num_val: v,
            int_val: v as i64,
        });
    }

    pub fn createStringUtf8(*_, s: []const u8): usize? {
        return _.allocValue(NapiMockValue{
            val_type: .String,
            str_val: s,
        });
    }

    pub fn createObject(*_): usize? {
        return _.allocValue(NapiMockValue{ val_type: .Object });
    }

    pub fn createFunction(*_, name: []const u8): usize? {
        return _.allocValue(NapiMockValue{
            val_type: .Function,
            str_val: name,
        });
    }

    pub fn createExternal(*_, ptr: (*anyopaque)?): usize? {
        return _.allocValue(NapiMockValue{
            val_type: .External,
            ext_ptr: ptr,
        });
    }

    pub fn createBigInt64(*_, v: i64): usize? {
        return _.allocValue(NapiMockValue{
            val_type: .BigInt,
            int_val: v,
        });
    }

    pub fn typeofValue(*_, val_id: usize): NapiValueType? {
        if (val_id >= _.value_count || !_.values[val_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        * = _.setLastError(.Ok);
        return _.values[val_id].val_type;
    }

    pub fn getValueInt32(*_, val_id: usize): i32? {
        if (val_id >= _.value_count || !_.values[val_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        if (_.values[val_id].val_type != .Number) {
            * = _.setLastError(.NumberExpected);
            return null;
        }
        * = _.setLastError(.Ok);
        return _.values[val_id].int_val as i32;
    }

    pub fn getValueDouble(*_, val_id: usize): f64? {
        if (val_id >= _.value_count || !_.values[val_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        if (_.values[val_id].val_type != .Number) {
            * = _.setLastError(.NumberExpected);
            return null;
        }
        * = _.setLastError(.Ok);
        return _.values[val_id].num_val;
    }

    pub fn getValueBool(*_, val_id: usize): bool? {
        if (val_id >= _.value_count || !_.values[val_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        if (_.values[val_id].val_type != .Boolean) {
            * = _.setLastError(.BooleanExpected);
            return null;
        }
        * = _.setLastError(.Ok);
        return _.values[val_id].bool_val;
    }

    pub fn getValueString(*_, val_id: usize): ([]const u8)? {
        if (val_id >= _.value_count || !_.values[val_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        if (_.values[val_id].val_type != .String) {
            * = _.setLastError(.StringExpected);
            return null;
        }
        * = _.setLastError(.Ok);
        return _.values[val_id].str_val;
    }

    // --- Object Property Management ---
    pub fn setNamedProperty(*_, obj_id: usize, name: []const u8, val_id: usize): NapiStatus {
        if (obj_id >= _.value_count || !_.values[obj_id].active || val_id >= _.value_count || !_.values[val_id].active) {
            return _.setLastError(.InvalidArg);
        }
        if (_.values[obj_id].val_type != .Object && _.values[obj_id].val_type != .Function) {
            return _.setLastError(.ObjectExpected);
        }
        let i: usize = 0;
        while (i < _.prop_count) {
            if (_.props[i].active && _.props[i].obj_id == obj_id && std.mem.eql(u8, _.props[i].name, name)) {
                _.props[i].value_id = val_id;
                return _.setLastError(.Ok);
            }
            i += 1;
        }
        if (_.prop_count >= NAPI_MAX_MOCK_PROPS) {
            return _.setLastError(.GenericFailure);
        }
        _.props[_.prop_count] = NapiMockProperty{
            active: true,
            obj_id: obj_id,
            name: name,
            value_id: val_id,
            attributes: 0,
        };
        _.prop_count += 1;
        return _.setLastError(.Ok);
    }

    pub fn getNamedProperty(*_, obj_id: usize, name: []const u8): usize? {
        if (obj_id >= _.value_count || !_.values[obj_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        if (_.values[obj_id].val_type != .Object && _.values[obj_id].val_type != .Function) {
            * = _.setLastError(.ObjectExpected);
            return null;
        }
        let i: usize = 0;
        while (i < _.prop_count) {
            if (_.props[i].active && _.props[i].obj_id == obj_id && std.mem.eql(u8, _.props[i].name, name)) {
                * = _.setLastError(.Ok);
                return _.props[i].value_id;
            }
            i += 1;
        }
        return _.getUndefined();
    }

    pub fn hasNamedProperty(*_, obj_id: usize, name: []const u8): bool {
        let i: usize = 0;
        while (i < _.prop_count) {
            if (_.props[i].active && _.props[i].obj_id == obj_id && std.mem.eql(u8, _.props[i].name, name)) {
                * = _.setLastError(.Ok);
                return true;
            }
            i += 1;
        }
        * = _.setLastError(.Ok);
        return false;
    }

    pub fn defineProperties(*_, obj_id: usize, descs: []const NapiPropertyDescriptor): NapiStatus {
        if (obj_id >= _.value_count || !_.values[obj_id].active) {
            return _.setLastError(.InvalidArg);
        }
        if (_.values[obj_id].val_type != .Object && _.values[obj_id].val_type != .Function) {
            return _.setLastError(.ObjectExpected);
        }
        let i: usize = 0;
        while (i < descs.len) {
            const d = descs[i];
            if (c_name := d.utf8name) {
                let prop_name: []const u8 = "";
                zig {
                    prop_name = std.mem.span(@as([*:0]const u8, @ptrCast(c_name)));
                }
                const val_id_opt = if (d.method != null) _.createFunction(prop_name) else _.getUndefined();
                if (val_id := val_id_opt) {
                    const st = _.setNamedProperty(obj_id, prop_name, val_id);
                    if (st != .Ok) return st;
                } else {
                    return _.last_status;
                }
            }
            i += 1;
        }
        return _.setLastError(.Ok);
    }

    // --- HandleScope & EscapableHandleScope Stack Management ---
    pub fn openHandleScope(*_): usize? {
        if (_.scope_depth >= NAPI_MAX_SCOPES) {
            * = _.setLastError(.GenericFailure);
            return null;
        }
        const idx = _.scope_depth;
        _.scopes[idx] = NapiHandleScopeEntry{
            active: true,
            is_escapable: false,
            escaped: false,
            start_handle_idx: _.value_count,
        };
        _.scope_depth += 1;
        * = _.setLastError(.Ok);
        return idx;
    }

    pub fn closeHandleScope(*_, scope_id: usize): NapiStatus {
        if (_.scope_depth == 0 || scope_id != _.scope_depth - 1) {
            return _.setLastError(.HandleScopeMismatch);
        }
        const entry = _.scopes[scope_id];
        if (!entry.active || entry.is_escapable) {
            return _.setLastError(.HandleScopeMismatch);
        }
        _.invalidateScopeValues(scope_id + 1);
        _.scopes[scope_id].active = false;
        _.scope_depth -= 1;
        return _.setLastError(.Ok);
    }

    pub fn openEscapableHandleScope(*_): usize? {
        if (_.scope_depth >= NAPI_MAX_SCOPES) {
            * = _.setLastError(.GenericFailure);
            return null;
        }
        const idx = _.scope_depth;
        _.scopes[idx] = NapiHandleScopeEntry{
            active: true,
            is_escapable: true,
            escaped: false,
            start_handle_idx: _.value_count,
        };
        _.scope_depth += 1;
        * = _.setLastError(.Ok);
        return idx;
    }

    pub fn escapeHandle(*_, scope_id: usize, val_id: usize): usize? {
        if (_.scope_depth == 0 || scope_id != _.scope_depth - 1 || !_.scopes[scope_id].is_escapable) {
            * = _.setLastError(.HandleScopeMismatch);
            return null;
        }
        if (_.scopes[scope_id].escaped) {
            * = _.setLastError(.EscapeCalledTwice);
            return null;
        }
        if (val_id >= _.value_count || !_.values[val_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        _.scopes[scope_id].escaped = true;
        // Promote handle to outer scope depth
        if (_.values[val_id].scope_depth > scope_id) {
            _.values[val_id].scope_depth = scope_id;
        }
        * = _.setLastError(.Ok);
        return val_id;
    }

    pub fn closeEscapableHandleScope(*_, scope_id: usize): NapiStatus {
        if (_.scope_depth == 0 || scope_id != _.scope_depth - 1) {
            return _.setLastError(.HandleScopeMismatch);
        }
        const entry = _.scopes[scope_id];
        if (!entry.active || !entry.is_escapable) {
            return _.setLastError(.HandleScopeMismatch);
        }
        _.invalidateScopeValues(scope_id + 1);
        _.scopes[scope_id].active = false;
        _.scope_depth -= 1;
        return _.setLastError(.Ok);
    }

    fn invalidateScopeValues(*_, depth_to_close: usize) {
        let i: usize = 0;
        while (i < _.value_count) {
            if (_.values[i].active && _.values[i].scope_depth >= depth_to_close) {
                if (!_.isStronglyReferenced(i)) {
                    _.values[i].active = false;
                }
            }
            i += 1;
        }
    }

    fn isStronglyReferenced(val_id: usize): bool {
        let i: usize = 0;
        while (i < _.ref_count) {
            if (_.refs[i].active && _.refs[i].value_id == val_id && _.refs[i].ref_count > 0) {
                return true;
            }
            i += 1;
        }
        return false;
    }

    // --- Reference Counting Table ---
    pub fn createReference(*_, val_id: usize, initial_refcount: u32): usize? {
        if (val_id >= _.value_count || !_.values[val_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        if (_.ref_count >= NAPI_MAX_REFS) {
            * = _.setLastError(.GenericFailure);
            return null;
        }
        const id = _.ref_count;
        _.refs[id] = NapiRefEntry{
            active: true,
            value_id: val_id,
            ref_count: initial_refcount,
            alive: true,
        };
        _.ref_count += 1;
        * = _.setLastError(.Ok);
        return id;
    }

    pub fn referenceRef(*_, ref_id: usize): u32? {
        if (ref_id >= _.ref_count || !_.refs[ref_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        if (!_.refs[ref_id].alive) {
            * = _.setLastError(.GenericFailure);
            return null;
        }
        _.refs[ref_id].ref_count += 1;
        * = _.setLastError(.Ok);
        return _.refs[ref_id].ref_count;
    }

    pub fn referenceUnref(*_, ref_id: usize): u32? {
        if (ref_id >= _.ref_count || !_.refs[ref_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        if (_.refs[ref_id].ref_count == 0) {
            * = _.setLastError(.GenericFailure);
            return null;
        }
        _.refs[ref_id].ref_count -= 1;
        * = _.setLastError(.Ok);
        return _.refs[ref_id].ref_count;
    }

    pub fn getReferenceValue(*_, ref_id: usize): usize? {
        if (ref_id >= _.ref_count || !_.refs[ref_id].active) {
            * = _.setLastError(.InvalidArg);
            return null;
        }
        const vid = _.refs[ref_id].value_id;
        if (!_.refs[ref_id].alive || vid >= _.value_count || !_.values[vid].active) {
            * = _.setLastError(.Ok);
            return null;
        }
        * = _.setLastError(.Ok);
        return vid;
    }

    pub fn deleteReference(*_, ref_id: usize): NapiStatus {
        if (ref_id >= _.ref_count || !_.refs[ref_id].active) {
            return _.setLastError(.InvalidArg);
        }
        _.refs[ref_id].active = false;
        _.refs[ref_id].alive = false;
        return _.setLastError(.Ok);
    }

    // --- Error & Exception State Capture ---
    pub fn throwError(*_, code: []const u8, msg: []const u8): NapiStatus {
        if (_.pending_exception_id != null) {
            return _.setLastError(.PendingException);
        }
        if (err_id := _.allocValue(NapiMockValue{
            val_type: .Object,
            str_val: msg,
            is_error: true,
            error_code: code,
        })) {
            _.pending_exception_id = err_id;
            return _.setLastError(.Ok);
        }
        return _.last_status;
    }

    pub fn isExceptionPending(): bool {
        return _.pending_exception_id != null;
    }

    pub fn getAndClearLastException(*_): usize? {
        const exc = _.pending_exception_id;
        _.pending_exception_id = null;
        * = _.setLastError(.Ok);
        return exc;
    }

    // --- Module Registration ---
    pub fn registerModule(*_, mod: NapiModule): bool {
        let modname: []const u8 = "";
        let filename: []const u8 = "";
        if (mn := mod.nm_modname) {
            zig { modname = std.mem.span(@as([*:0]const u8, @ptrCast(mn))); }
        }
        if (fn_ptr := mod.nm_filename) {
            zig { filename = std.mem.span(@as([*:0]const u8, @ptrCast(fn_ptr))); }
        }
        return _.registry.register(modname, filename, mod.nm_flags);
    }

    // --- AsyncWork Lifecycle ---
    pub fn createAsyncWork(*_, resource_name: []const u8, data: (*anyopaque)?): usize? {
        if (_.async_work_count >= NAPI_MAX_ASYNC_WORKS) {
            * = _.setLastError(.GenericFailure);
            return null;
        }
        const id = _.async_work_count;
        _.async_works[id] = NapiAsyncWorkEntry{
            active: true,
            id: id,
            resource_name: resource_name,
            status: .Pending,
            data: data,
            execute_cb: null,
            complete_cb: null,
            executed: false,
            completed: false,
        };
        _.async_work_count += 1;
        * = _.setLastError(.Ok);
        return id;
    }

    pub fn queueAsyncWork(*_, work_id: usize): NapiStatus {
        if (work_id >= _.async_work_count || !_.async_works[work_id].active) {
            return _.setLastError(.InvalidArg);
        }
        if (_.async_works[work_id].status != .Pending) {
            return _.setLastError(.GenericFailure);
        }
        _.async_works[work_id].status = .Queued;
        return _.setLastError(.Ok);
    }

    pub fn cancelAsyncWork(*_, work_id: usize): NapiStatus {
        if (work_id >= _.async_work_count || !_.async_works[work_id].active) {
            return _.setLastError(.InvalidArg);
        }
        if (_.async_works[work_id].status != .Queued && _.async_works[work_id].status != .Pending) {
            return _.setLastError(.GenericFailure);
        }
        _.async_works[work_id].status = .Cancelled;
        return _.setLastError(.Ok);
    }

    pub fn runAsyncWork(*_, work_id: usize): NapiStatus {
        if (work_id >= _.async_work_count || !_.async_works[work_id].active) {
            return _.setLastError(.InvalidArg);
        }
        if (_.async_works[work_id].status == .Cancelled) {
            _.async_works[work_id].completed = true;
            return _.setLastError(.Cancelled);
        }
        if (_.async_works[work_id].status != .Queued) {
            return _.setLastError(.GenericFailure);
        }
        _.async_works[work_id].status = .Running;
        _.async_works[work_id].executed = true;
        _.async_works[work_id].status = .Completed;
        _.async_works[work_id].completed = true;
        return _.setLastError(.Ok);
    }

    pub fn deleteAsyncWork(*_, work_id: usize): NapiStatus {
        if (work_id >= _.async_work_count || !_.async_works[work_id].active) {
            return _.setLastError(.InvalidArg);
        }
        _.async_works[work_id].status = .Deleted;
        _.async_works[work_id].active = false;
        return _.setLastError(.Ok);
    }
}

pub type NapiEnvMock = NapiRuntimeContext;
