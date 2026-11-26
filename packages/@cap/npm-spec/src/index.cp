// @cap/npm-spec — Zero-heap npm package name validator & specifier parser
// MIT License (clean-room CP implementation)

pub const MAX_PACKAGE_NAME_LEN: usize = 214;

pub struct PackageSpec {
    pub scope: string,
    pub name: string,
    pub full_name: string,
    pub version_spec: string,
    pub is_scoped: bool,
    pub valid: bool,
}

fn isValidSegment(seg: string) -> bool {
    if (seg.len == 0) {
        return false;
    }
    const c0: u8 = seg[0];
    if (c0 == '.' as u8 || c0 == '_' as u8) {
        return false;
    }
    let i: usize = 0;
    while (i < seg.len) {
        const c: u8 = seg[i];
        const ok: bool = (c >= 'a' as u8 && c <= 'z' as u8)
            || (c >= '0' as u8 && c <= '9' as u8)
            || c == '-' as u8
            || c == '_' as u8
            || c == '.' as u8
            || c == '~' as u8;
        if (!ok) {
            return false;
        }
        i += 1;
    }
    return true;
}

pub fn isValidPackageName(name: string) -> bool {
    if (name.len == 0 || name.len > MAX_PACKAGE_NAME_LEN) {
        return false;
    }
    if (name[0] == '@' as u8) {
        let slash: usize = 1;
        while (slash < name.len && name[slash] != '/' as u8) {
            slash += 1;
        }
        if (slash >= name.len) {
            return false;
        }
        const scope: string = name[1..slash];
        const pkg: string = name[(slash + 1)..name.len];
        return isValidSegment(scope) && isValidSegment(pkg);
    }
    return isValidSegment(name);
}

pub fn parsePackageSpec(input: string) -> PackageSpec {
    const fail = PackageSpec {
        scope: "",
        name: "",
        full_name: "",
        version_spec: "",
        is_scoped: false,
        valid: false,
    };

    if (input.len == 0) {
        return fail;
    }

    let search_from: usize = 0;
    if (input[0] == '@' as u8) {
        search_from = 1;
    }

    let at_pos: usize = input.len;
    let i: usize = search_from;
    while (i < input.len) {
        if (input[i] == '@' as u8) {
            at_pos = i;
            break;
        }
        i += 1;
    }

    let full_name: string = input;
    let version_spec: string = "";
    if (at_pos < input.len) {
        full_name = input[0..at_pos];
        version_spec = input[(at_pos + 1)..input.len];
        if (version_spec.len == 0) {
            return fail;
        }
    }

    if (!isValidPackageName(full_name)) {
        return fail;
    }

    if (full_name[0] == '@' as u8) {
        let slash: usize = 1;
        while (slash < full_name.len && full_name[slash] != '/' as u8) {
            slash += 1;
        }
        return PackageSpec {
            scope: full_name[1..slash],
            name: full_name[(slash + 1)..full_name.len],
            full_name: full_name,
            version_spec: version_spec,
            is_scoped: true,
            valid: true,
        };
    }

    return PackageSpec {
        scope: "",
        name: full_name,
        full_name: full_name,
        version_spec: version_spec,
        is_scoped: false,
        valid: true,
    };
}

fn appendSlice(out: []u8, pos: usize, s: string) -> i64 {
    if (s.len > out.len - pos) {
        return -1;
    }
    let i: usize = 0;
    while (i < s.len) {
        out[pos + i] = s[i];
        i += 1;
    }
    return (pos + s.len) as i64;
}

pub fn formatPackageSpec(spec: PackageSpec, out: []u8) -> usize {
    if (!isValidPackageName(spec.full_name)) {
        return 0;
    }
    let p: i64 = appendSlice(out, 0, spec.full_name);
    if (p < 0) {
        return 0;
    }
    if (spec.version_spec.len > 0) {
        p = appendSlice(out, p as usize, "@");
        if (p < 0) {
            return 0;
        }
        p = appendSlice(out, p as usize, spec.version_spec);
        if (p < 0) {
            return 0;
        }
    }
    return p as usize;
}
