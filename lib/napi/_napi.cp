// Absorbed from Bun: /data/dev/bun/src/napi/
// Upstream Sync: Refer to /data/dev/bun/src/napi/ for future upstream updates.

@c {
    #include "js_native_api.h"
}

pub const NapiStatus = @c.napi_status;
pub const NapiEnv = @c.napi_env;
pub const NapiValue = @c.napi_value;

pub extern fn napi_get_undefined(env: NapiEnv, result: *NapiValue) NapiStatus;
