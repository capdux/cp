// Absorbed from Bun: /data/dev/bun/src/napi/
// Upstream Sync: Refer to /data/dev/bun/src/napi/ for future upstream updates.
import { NapiEnv, NapiValue, NapiStatus, napi_get_undefined } from "./_napi";

pub const Napi = struct {
    pub fn getUndefined(env: NapiEnv): !NapiValue {
        var value: NapiValue = undefined;
        const status = napi_get_undefined(env, &value);
        if (status != 0) { // 0 is napi_ok
            return error.NapiError;
        }
        return value;
    }
};
