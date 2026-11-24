// Source: Absorbed from /data/dev/bun/src/napi/napi.cp | Synced: 2026-11-23T21:59:28Z
// lib/napi/napi.cp
// Safe CP High-Level Wrapper, Status/Error Mapping & Addon Registry for Node-API (cap:napi)

const std = @import("std");
import {
    NAPI_VERSION,
    NAPI_MODULE_VERSION,
    NapiStatus,
    NapiValueType,
    NapiTypedArrayType,
    NapiPropertyAttributes,
    NapiEnv,
    NapiValue,
    NapiRef,
    NapiHandleScope,
    NapiEscapableHandleScope,
    NapiExtendedErrorInfo,
    NapiNodeVersion,
    NapiTypeTag,
    NapiModule,
    NapiPropertyDescriptor,
    napi_get_undefined,
    napi_get_null,
    napi_get_boolean,
    napi_typeof,
    napi_create_int32,
    napi_create_double,
    napi_get_value_int32,
    napi_get_value_double,
    napi_create_object,
    napi_open_handle_scope,
    napi_close_handle_scope,
    napi_open_escapable_handle_scope,
    napi_close_escapable_handle_scope,
    napi_escape_handle,
    napi_create_reference,
    napi_delete_reference,
    napi_reference_ref,
    napi_reference_unref,
    napi_get_reference_value
} from "./_napi";

export error NapiError {
    InvalidArg,
    ObjectExpected,
    StringExpected,
    NameExpected,
    FunctionExpected,
    NumberExpected,
    BooleanExpected,
    ArrayExpected,
    GenericFailure,
    PendingException,
    Cancelled,
    EscapeCalledTwice,
    HandleScopeMismatch,
    CallbackScopeMismatch,
    QueueFull,
    Closing,
    BigIntExpected,
    DateExpected,
    ArrayBufferExpected,
    DetachableArrayBufferExpected,
    WouldDeadlock,
    NoExternalBuffersAllowed,
    CannotRunJs,
    UnknownError,
}

pub struct NapiAddonInfo {
    pub name: []const u8 = "";
    pub filename: []const u8 = "";
    pub napi_version: u32 = NAPI_VERSION;
    pub flags: u32 = 0;
}

pub struct NapiAddonRegistry {
    pub addons: [32]NapiAddonInfo = undefined;
    pub count: usize = 0;

    pub static fn init(): NapiAddonRegistry {
        return NapiAddonRegistry{
            addons: undefined,
            count: 0,
        };
    }

    pub fn register(*_, name: []const u8, filename: []const u8, flags: u32): bool {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.addons[i].name, name)) {
                _.addons[i].filename = filename;
                _.addons[i].flags = flags;
                return true;
            }
            i += 1;
        }
        if (_.count >= 32) {
            return false;
        }
        _.addons[_.count] = NapiAddonInfo{
            name: name,
            filename: filename,
            napi_version: NAPI_VERSION,
            flags: flags,
        };
        _.count += 1;
        return true;
    }

    pub fn find(name: []const u8): NapiAddonInfo? {
        let i: usize = 0;
        while (i < _.count) {
            if (std.mem.eql(u8, _.addons[i].name, name)) {
                return _.addons[i];
            }
            i += 1;
        }
        return null;
    }
}

pub struct Napi {
    pub static fn isOk(status: NapiStatus): bool {
        return status == NapiStatus.Ok;
    }

    pub static fn checkStatus(status: NapiStatus): !void {
        switch (status) {
            .Ok => { return; },
            .InvalidArg => { return error.InvalidArg; },
            .ObjectExpected => { return error.ObjectExpected; },
            .StringExpected => { return error.StringExpected; },
            .NameExpected => { return error.NameExpected; },
            .FunctionExpected => { return error.FunctionExpected; },
            .NumberExpected => { return error.NumberExpected; },
            .BooleanExpected => { return error.BooleanExpected; },
            .ArrayExpected => { return error.ArrayExpected; },
            .GenericFailure => { return error.GenericFailure; },
            .PendingException => { return error.PendingException; },
            .Cancelled => { return error.Cancelled; },
            .EscapeCalledTwice => { return error.EscapeCalledTwice; },
            .HandleScopeMismatch => { return error.HandleScopeMismatch; },
            .CallbackScopeMismatch => { return error.CallbackScopeMismatch; },
            .QueueFull => { return error.QueueFull; },
            .Closing => { return error.Closing; },
            .BigIntExpected => { return error.BigIntExpected; },
            .DateExpected => { return error.DateExpected; },
            .ArrayBufferExpected => { return error.ArrayBufferExpected; },
            .DetachableArrayBufferExpected => { return error.DetachableArrayBufferExpected; },
            .WouldDeadlock => { return error.WouldDeadlock; },
            .NoExternalBuffersAllowed => { return error.NoExternalBuffersAllowed; },
            .CannotRunJs => { return error.CannotRunJs; },
        }
    }

    pub static fn statusName(status: NapiStatus): []const u8 {
        return switch (status) {
            .Ok => "napi_ok",
            .InvalidArg => "napi_invalid_arg",
            .ObjectExpected => "napi_object_expected",
            .StringExpected => "napi_string_expected",
            .NameExpected => "napi_name_expected",
            .FunctionExpected => "napi_function_expected",
            .NumberExpected => "napi_number_expected",
            .BooleanExpected => "napi_boolean_expected",
            .ArrayExpected => "napi_array_expected",
            .GenericFailure => "napi_generic_failure",
            .PendingException => "napi_pending_exception",
            .Cancelled => "napi_cancelled",
            .EscapeCalledTwice => "napi_escape_called_twice",
            .HandleScopeMismatch => "napi_handle_scope_mismatch",
            .CallbackScopeMismatch => "napi_callback_scope_mismatch",
            .QueueFull => "napi_queue_full",
            .Closing => "napi_closing",
            .BigIntExpected => "napi_bigint_expected",
            .DateExpected => "napi_date_expected",
            .ArrayBufferExpected => "napi_arraybuffer_expected",
            .DetachableArrayBufferExpected => "napi_detachable_arraybuffer_expected",
            .WouldDeadlock => "napi_would_deadlock",
            .NoExternalBuffersAllowed => "napi_no_external_buffers_allowed",
            .CannotRunJs => "napi_cannot_run_js",
        };
    }

    pub static fn statusMessage(status: NapiStatus): []const u8 {
        return switch (status) {
            .Ok => "Success",
            .InvalidArg => "Invalid argument",
            .ObjectExpected => "An object was expected",
            .StringExpected => "A string was expected",
            .NameExpected => "A string or symbol was expected",
            .FunctionExpected => "A function was expected",
            .NumberExpected => "A number was expected",
            .BooleanExpected => "A boolean was expected",
            .ArrayExpected => "An array was expected",
            .GenericFailure => "Unknown failure",
            .PendingException => "An exception is pending",
            .Cancelled => "The async work item was cancelled",
            .EscapeCalledTwice => "napi_escape_handle already called on scope",
            .HandleScopeMismatch => "Handle scope mismatch",
            .CallbackScopeMismatch => "Callback scope mismatch",
            .QueueFull => "Threadsafe function queue is full",
            .Closing => "Threadsafe function is closing",
            .BigIntExpected => "A BigInt was expected",
            .DateExpected => "A Date was expected",
            .ArrayBufferExpected => "An ArrayBuffer was expected",
            .DetachableArrayBufferExpected => "A detachable ArrayBuffer was expected",
            .WouldDeadlock => "Operation would cause deadlock",
            .NoExternalBuffersAllowed => "External buffers are not allowed",
            .CannotRunJs => "Cannot run JavaScript",
        };
    }

    pub static fn valueTypeName(val_type: NapiValueType): []const u8 {
        return switch (val_type) {
            .Undefined => "undefined",
            .Null => "null",
            .Boolean => "boolean",
            .Number => "number",
            .String => "string",
            .Symbol => "symbol",
            .Object => "object",
            .Function => "function",
            .External => "external",
            .BigInt => "bigint",
        };
    }

    pub static fn typedArrayElementSize(arr_type: NapiTypedArrayType): usize {
        return switch (arr_type) {
            .Int8, .Uint8, .Uint8Clamped => 1,
            .Int16, .Uint16, .Float16 => 2,
            .Int32, .Uint32, .Float32 => 4,
            .Float64, .BigInt64, .BigUint64 => 8,
        };
    }

    pub static fn createExtendedErrorInfo(status: NapiStatus, engine_code: u32): NapiExtendedErrorInfo {
        const msg = Napi.statusMessage(status);
        return NapiExtendedErrorInfo{
            error_message: msg.ptr,
            engine_reserved: null,
            engine_error_code: engine_code,
            error_code: status,
        };
    }

    pub static fn createNodeVersion(major: u32, minor: u32, patch: u32, release: []const u8): NapiNodeVersion {
        return NapiNodeVersion{
            major: major,
            minor: minor,
            patch: patch,
            release: release.ptr,
        };
    }

    pub static fn createTypeTag(lower: u64, upper: u64): NapiTypeTag {
        return NapiTypeTag{
            lower: lower,
            upper: upper,
        };
    }

    pub static fn createModuleDescriptor(
        modname: [*]const u8,
        filename: [*]const u8,
        flags: u32,
        register_fn: (*anyopaque)?
    ): NapiModule {
        let mod = NapiModule{
            nm_version: NAPI_MODULE_VERSION,
            nm_flags: flags,
            nm_filename: filename,
            nm_register_func: register_fn,
            nm_modname: modname,
            nm_priv: null,
            reserved: undefined,
        };
        mod.reserved[0] = null;
        mod.reserved[1] = null;
        mod.reserved[2] = null;
        mod.reserved[3] = null;
        return mod;
    }

    pub static fn createPropertyDescriptor(utf8name: [*]const u8, attrs: u32): NapiPropertyDescriptor {
        return NapiPropertyDescriptor{
            utf8name: utf8name,
            name: null,
            method: null,
            getter: null,
            setter: null,
            value: null,
            attributes: attrs,
            data: null,
        };
    }

    pub static fn getUndefined(env: NapiEnv): !NapiValue {
        let value: NapiValue = undefined;
        Napi.checkStatus(napi_get_undefined(env, &value))?;
        return value;
    }

    pub static fn getNull(env: NapiEnv): !NapiValue {
        let value: NapiValue = undefined;
        Napi.checkStatus(napi_get_null(env, &value))?;
        return value;
    }

    pub static fn getBoolean(env: NapiEnv, b: bool): !NapiValue {
        let value: NapiValue = undefined;
        Napi.checkStatus(napi_get_boolean(env, b, &value))?;
        return value;
    }

    pub static fn createInt32(env: NapiEnv, val: i32): !NapiValue {
        let out: NapiValue = undefined;
        Napi.checkStatus(napi_create_int32(env, val, &out))?;
        return out;
    }

    pub static fn createDouble(env: NapiEnv, val: f64): !NapiValue {
        let out: NapiValue = undefined;
        Napi.checkStatus(napi_create_double(env, val, &out))?;
        return out;
    }

    pub static fn getValueInt32(env: NapiEnv, val: NapiValue): !i32 {
        let out: i32 = 0;
        Napi.checkStatus(napi_get_value_int32(env, val, &out))?;
        return out;
    }

    pub static fn getValueDouble(env: NapiEnv, val: NapiValue): !f64 {
        let out: f64 = 0.0;
        Napi.checkStatus(napi_get_value_double(env, val, &out))?;
        return out;
    }

    pub static fn typeofValue(env: NapiEnv, val: NapiValue): !NapiValueType {
        let out: NapiValueType = NapiValueType.Undefined;
        Napi.checkStatus(napi_typeof(env, val, &out))?;
        return out;
    }

    pub static fn createObject(env: NapiEnv): !NapiValue {
        let out: NapiValue = undefined;
        Napi.checkStatus(napi_create_object(env, &out))?;
        return out;
    }

    pub static fn openHandleScope(env: NapiEnv): !NapiHandleScope {
        let scope: NapiHandleScope = undefined;
        Napi.checkStatus(napi_open_handle_scope(env, &scope))?;
        return scope;
    }

    pub static fn closeHandleScope(env: NapiEnv, scope: NapiHandleScope): !void {
        Napi.checkStatus(napi_close_handle_scope(env, scope))?;
    }

    pub static fn openEscapableHandleScope(env: NapiEnv): !NapiEscapableHandleScope {
        let scope: NapiEscapableHandleScope = undefined;
        Napi.checkStatus(napi_open_escapable_handle_scope(env, &scope))?;
        return scope;
    }

    pub static fn escapeHandle(env: NapiEnv, scope: NapiEscapableHandleScope, escapee: NapiValue): !NapiValue {
        let out: NapiValue = undefined;
        Napi.checkStatus(napi_escape_handle(env, scope, escapee, &out))?;
        return out;
    }

    pub static fn closeEscapableHandleScope(env: NapiEnv, scope: NapiEscapableHandleScope): !void {
        Napi.checkStatus(napi_close_escapable_handle_scope(env, scope))?;
    }

    pub static fn createReference(env: NapiEnv, val: NapiValue, initial_refcount: u32): !NapiRef {
        let out: NapiRef = undefined;
        Napi.checkStatus(napi_create_reference(env, val, initial_refcount, &out))?;
        return out;
    }

    pub static fn referenceRef(env: NapiEnv, ref_handle: NapiRef): !u32 {
        let count: u32 = 0;
        Napi.checkStatus(napi_reference_ref(env, ref_handle, &count))?;
        return count;
    }

    pub static fn referenceUnref(env: NapiEnv, ref_handle: NapiRef): !u32 {
        let count: u32 = 0;
        Napi.checkStatus(napi_reference_unref(env, ref_handle, &count))?;
        return count;
    }

    pub static fn getReferenceValue(env: NapiEnv, ref_handle: NapiRef): !NapiValue {
        let out: NapiValue = undefined;
        Napi.checkStatus(napi_get_reference_value(env, ref_handle, &out))?;
        return out;
    }

    pub static fn deleteReference(env: NapiEnv, ref_handle: NapiRef): !void {
        Napi.checkStatus(napi_delete_reference(env, ref_handle))?;
    }
}
