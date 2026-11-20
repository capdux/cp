// Source: /data/dev/bun/src/napi/js_native_api_types.h
// lib/napi/_napi.cp
// Node-API (N-API v9) C-ABI Types, Status Codes, Struct Layouts & Extern Declarations

pub const NAPI_VERSION: u32 = 9;
pub const NAPI_MODULE_VERSION: i32 = 1;
pub const NAPI_AUTO_LENGTH: usize = 0xFFFFFFFFFFFFFFFF;

pub enum NapiStatus: i32 {
    Ok = 0,
    InvalidArg = 1,
    ObjectExpected = 2,
    StringExpected = 3,
    NameExpected = 4,
    FunctionExpected = 5,
    NumberExpected = 6,
    BooleanExpected = 7,
    ArrayExpected = 8,
    GenericFailure = 9,
    PendingException = 10,
    Cancelled = 11,
    EscapeCalledTwice = 12,
    HandleScopeMismatch = 13,
    CallbackScopeMismatch = 14,
    QueueFull = 15,
    Closing = 16,
    BigIntExpected = 17,
    DateExpected = 18,
    ArrayBufferExpected = 19,
    DetachableArrayBufferExpected = 20,
    WouldDeadlock = 21,
    NoExternalBuffersAllowed = 22,
    CannotRunJs = 23,
}

pub enum NapiValueType: i32 {
    Undefined = 0,
    Null = 1,
    Boolean = 2,
    Number = 3,
    String = 4,
    Symbol = 5,
    Object = 6,
    Function = 7,
    External = 8,
    BigInt = 9,
}

pub enum NapiTypedArrayType: i32 {
    Int8 = 0,
    Uint8 = 1,
    Uint8Clamped = 2,
    Int16 = 3,
    Uint16 = 4,
    Int32 = 5,
    Uint32 = 6,
    Float32 = 7,
    Float64 = 8,
    BigInt64 = 9,
    BigUint64 = 10,
    Float16 = 11,
}

pub enum NapiKeyCollectionMode: i32 {
    IncludePrototypes = 0,
    OwnOnly = 1,
}

pub enum NapiKeyFilter: u32 {
    AllProperties = 0,
    Writable = 1,
    Enumerable = 1 << 1,
    Configurable = 1 << 2,
    SkipStrings = 1 << 3,
    SkipSymbols = 1 << 4,
}

pub enum NapiKeyConversion: i32 {
    KeepNumbers = 0,
    NumbersToStrings = 1,
}

pub enum NapiTsfnReleaseMode: i32 {
    Release = 0,
    Abort = 1,
}

pub enum NapiTsfnCallMode: i32 {
    NonBlocking = 0,
    Blocking = 1,
}

pub struct NapiPropertyAttributes {
    pub const Default: u32 = 0;
    pub const Writable: u32 = 1 << 0;
    pub const Enumerable: u32 = 1 << 1;
    pub const Configurable: u32 = 1 << 2;
    pub const Static: u32 = 1 << 10;
    pub const DefaultMethod: u32 = (1 << 0) | (1 << 2);
    pub const DefaultJsProperty: u32 = (1 << 0) | (1 << 1) | (1 << 2);
}

pub type NapiEnv = *anyopaque;
pub type NapiValue = *anyopaque;
pub type NapiRef = *anyopaque;
pub type NapiHandleScope = *anyopaque;
pub type NapiEscapableHandleScope = *anyopaque;
pub type NapiCallbackInfo = *anyopaque;
pub type NapiDeferred = *anyopaque;
pub type NapiAsyncWork = *anyopaque;
pub type NapiAsyncContext = *anyopaque;
pub type NapiCallbackScope = *anyopaque;
pub type NapiThreadsafeFunction = *anyopaque;
pub type NapiAsyncCleanupHookHandle = *anyopaque;

// C-ABI compatible lowercase aliases
pub type napi_status = NapiStatus;
pub type napi_valuetype = NapiValueType;
pub type napi_typedarray_type = NapiTypedArrayType;
pub type napi_env = NapiEnv;
pub type napi_value = NapiValue;
pub type napi_ref = NapiRef;
pub type napi_handle_scope = NapiHandleScope;
pub type napi_escapable_handle_scope = NapiEscapableHandleScope;
pub type napi_callback_info = NapiCallbackInfo;
pub type napi_deferred = NapiDeferred;
pub type napi_async_work = NapiAsyncWork;
pub type napi_threadsafe_function = NapiThreadsafeFunction;
pub type napi_threadsafe_function_release_mode = NapiTsfnReleaseMode;
pub type napi_threadsafe_function_call_mode = NapiTsfnCallMode;

pub @c struct NapiExtendedErrorInfo {
    pub error_message: ([*]const u8)? = null;
    pub engine_reserved: (*anyopaque)? = null;
    pub engine_error_code: u32 = 0;
    pub error_code: NapiStatus = NapiStatus.Ok;
}

pub @c struct NapiNodeVersion {
    pub major: u32 = 20;
    pub minor: u32 = 0;
    pub patch: u32 = 0;
    pub release: ([*]const u8)? = null;
}

pub @c struct NapiTypeTag {
    pub lower: u64 = 0;
    pub upper: u64 = 0;
}

pub @c struct NapiPropertyDescriptor {
    pub utf8name: ([*]const u8)? = null;
    pub name: (*anyopaque)? = null;
    pub method: (*anyopaque)? = null;
    pub getter: (*anyopaque)? = null;
    pub setter: (*anyopaque)? = null;
    pub value: (*anyopaque)? = null;
    pub attributes: u32 = 0;
    pub data: (*anyopaque)? = null;
}

pub @c struct NapiModule {
    pub nm_version: i32 = NAPI_MODULE_VERSION;
    pub nm_flags: u32 = 0;
    pub nm_filename: ([*]const u8)? = null;
    pub nm_register_func: (*anyopaque)? = null;
    pub nm_modname: ([*]const u8)? = null;
    pub nm_priv: (*anyopaque)? = null;
    pub reserved: [4]((*anyopaque)?) = undefined;
}

pub type napi_extended_error_info = NapiExtendedErrorInfo;
pub type napi_node_version = NapiNodeVersion;
pub type napi_type_tag = NapiTypeTag;
pub type napi_property_descriptor = NapiPropertyDescriptor;
pub type napi_module = NapiModule;

pub @c {
    fn napi_get_last_error_info(env: NapiEnv, result: *((*const NapiExtendedErrorInfo)?)): NapiStatus;
    fn napi_get_node_version(env: NapiEnv, version: *((*const NapiNodeVersion)?)): NapiStatus;
    fn napi_get_undefined(env: NapiEnv, result: *NapiValue!): NapiStatus;
    fn napi_get_null(env: NapiEnv, result: *NapiValue!): NapiStatus;
    fn napi_get_boolean(env: NapiEnv, value: bool, result: *NapiValue!): NapiStatus;
    fn napi_typeof(env: NapiEnv, value: NapiValue, result: *NapiValueType!): NapiStatus;
    fn napi_create_int32(env: NapiEnv, value: i32, result: *NapiValue!): NapiStatus;
    fn napi_create_double(env: NapiEnv, value: f64, result: *NapiValue!): NapiStatus;
    fn napi_create_string_utf8(env: NapiEnv, str: [*]const u8!, length: usize, result: *NapiValue!): NapiStatus;
    fn napi_get_value_int32(env: NapiEnv, value: NapiValue, result: *i32!): NapiStatus;
    fn napi_get_value_double(env: NapiEnv, value: NapiValue, result: *f64!): NapiStatus;
    fn napi_create_object(env: NapiEnv, result: *NapiValue!): NapiStatus;
    fn napi_set_named_property(env: NapiEnv, object: NapiValue, utf8name: [*]const u8!, value: NapiValue): NapiStatus;
    fn napi_open_handle_scope(env: NapiEnv, result: *NapiHandleScope!): NapiStatus;
    fn napi_close_handle_scope(env: NapiEnv, scope: NapiHandleScope): NapiStatus;
    fn napi_open_escapable_handle_scope(env: NapiEnv, result: *NapiEscapableHandleScope!): NapiStatus;
    fn napi_close_escapable_handle_scope(env: NapiEnv, scope: NapiEscapableHandleScope): NapiStatus;
    fn napi_escape_handle(env: NapiEnv, scope: NapiEscapableHandleScope, escapee: NapiValue, result: *NapiValue!): NapiStatus;
    fn napi_create_reference(env: NapiEnv, value: NapiValue, initial_refcount: u32, result: *NapiRef!): NapiStatus;
    fn napi_delete_reference(env: NapiEnv, ref_handle: NapiRef): NapiStatus;
    fn napi_reference_ref(env: NapiEnv, ref_handle: NapiRef, result: *u32!): NapiStatus;
    fn napi_reference_unref(env: NapiEnv, ref_handle: NapiRef, result: *u32!): NapiStatus;
    fn napi_get_reference_value(env: NapiEnv, ref_handle: NapiRef, result: *NapiValue!): NapiStatus;
    fn napi_throw_error(env: NapiEnv, code: ([*]const u8)?, msg: [*]const u8!): NapiStatus;
    fn napi_module_register(mod: *NapiModule!);
}
