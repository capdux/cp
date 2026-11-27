// Type definitions for CP

export class PackageSpec {
    scope: string,
    name: string,
    full_name: string,
    version_spec: string,
    is_scoped: bool,
    valid: bool,
}

export fn isValidSegment(seg: string): bool;
export fn isValidPackageName(name: string): bool;
export fn parsePackageSpec(input: string): PackageSpec;
export fn appendSlice(out: []u8, pos: usize, s: string): i64;
export fn formatPackageSpec(spec: PackageSpec, out: []u8): usize;
