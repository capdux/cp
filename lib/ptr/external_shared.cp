// Source: Absorbed from /data/dev/bun/src/ptr/external_shared.cp | Synced: 2026-11-19T20:38:57Z
// External reference counted pointer for C/FFI interop
import std from "std";

fn ExternalSharedOptional(comptime T: type, comptime NonOptional: type): type {
    return struct {
        const Self = @This();
        impl: (*T)? = null,

        pub static fn initNull(): Self {
            return Self{ impl: null };
        }

        pub static fn adopt(incremented_raw: (*T)?): Self {
            return Self{ impl: incremented_raw };
        }

        pub fn deinit(*_) {
            if (val := _.impl) {
                T.external_shared_descriptor.deref(val);
            }
        }

        pub fn get(): (*T)? {
            return _.impl;
        }

        pub fn take(*_): NonOptional? {
            if (val := _.impl) {
                _.impl = null;
                return NonOptional{ impl: val };
            }
            return null;
        }

        pub fn clone(): Self {
            if (val := _.impl) {
                T.external_shared_descriptor.ref(val);
            }
            return Self{ impl: _.impl };
        }

        pub static fn cloneFromRaw(raw: (*T)?): Self {
            if (some_raw := raw) {
                T.external_shared_descriptor.ref(some_raw);
            }
            return Self{ impl: raw };
        }

        pub fn leak(*_): (*T)? {
            const p = _.impl;
            _.impl = null;
            return p;
        }
    };
}

pub fn ExternalShared(comptime T: type): type {
    return struct {
        const Self = @This();
        pub const Optional = ExternalSharedOptional(T, Self);

        impl: *T,

        pub static fn adopt(incremented_raw: *T): Self {
            return Self{ impl: incremented_raw };
        }

        pub fn deinit(*_) {
            T.external_shared_descriptor.deref(_.impl);
        }

        pub fn get(): *T {
            return _.impl;
        }

        pub fn clone(): Self {
            T.external_shared_descriptor.ref(_.impl);
            return Self{ impl: _.impl };
        }

        pub static fn cloneFromRaw(raw: *T): Self {
            T.external_shared_descriptor.ref(raw);
            return Self{ impl: raw };
        }

        pub fn leak(*_): *T {
            const p = _.impl;
            return p;
        }

        pub fn intoOptional(*_): Optional {
            return Optional{ impl: _.impl };
        }
    };
}
