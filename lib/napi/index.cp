// Source: Custom (agy-generated export index)
// lib/napi/index.cp
// Unified Public Exports for Node-API C-ABI & Safe Wrapper (cap:napi)

export {
    NAPI_VERSION,
    NAPI_MODULE_VERSION,
    NAPI_AUTO_LENGTH,
    NapiStatus,
    NapiValueType,
    NapiTypedArrayType,
    NapiPropertyAttributes,
    NapiEnv,
    NapiValue,
    NapiRef,
    NapiHandleScope,
    NapiEscapableHandleScope,
    NapiCallbackInfo,
    NapiDeferred,
    NapiAsyncWork,
    NapiExtendedErrorInfo,
    NapiPropertyDescriptor,
    NapiModule,
    napi_get_undefined,
    napi_get_null,
    napi_get_boolean,
    napi_typeof,
    napi_create_int32,
    napi_create_double,
    napi_create_string_utf8,
    napi_get_value_int32,
    napi_get_value_double,
    napi_create_object,
    napi_set_named_property,
    napi_open_handle_scope,
    napi_close_handle_scope,
    napi_throw_error,
    napi_module_register
} from "./_napi";

export {
    NapiError,
    NapiAddonInfo,
    NapiAddonRegistry,
    Napi
} from "./napi";
