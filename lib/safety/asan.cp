// Source: /data/dev/bun/src/safety/asan.cp
import std from "std";

pub const asan_enabled: bool = @hasDecl(std.debug, "is_asan_enabled") and std.debug.is_asan_enabled;

pub fn poison(ptr: *const anyopaque, size: usize) {
    * = ptr;
    * = size;
}

pub fn unpoison(ptr: *const anyopaque, size: usize) {
    * = ptr;
    * = size;
}

pub fn isPoisoned(ptr: *const anyopaque): bool {
    * = ptr;
    return false;
}

pub fn assertPoisoned(ptr: *const anyopaque) {
    * = ptr;
}

pub fn assertUnpoisoned(ptr: *const anyopaque) {
    * = ptr;
}

pub fn describe(ptr: *const anyopaque) {
    * = ptr;
}

pub fn describeAddress(ptr: *const anyopaque) {
    describe(ptr);
}

pub fn registerRootRegion(ptr: *const anyopaque, size: usize) {
    * = ptr;
    * = size;
}

pub fn unregisterRootRegion(ptr: *const anyopaque, size: usize) {
    * = ptr;
    * = size;
}
