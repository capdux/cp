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

pub @c struct NapiExtendedErrorInfo {
    pub error_message: ([*]const u8)? = null;
    pub engine_reserved: (*anyopaque)? = null;
    pub engine_error_code: u32 = 0;
    pub error_code: NapiStatus = NapiStatus.Ok;
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

pub @c {
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
    fn napi_throw_error(env: NapiEnv, code: ([*]const u8)?, msg: [*]const u8!): NapiStatus;
    fn napi_module_register(mod: *NapiModule!);
}
