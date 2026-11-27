// Type definitions for CP

export class HelmetOptions {
    csp_enabled: bool,
    csp_value: string,
    hsts_enabled: bool,
    hsts_max_age: u64,
    hsts_include_subdomains: bool,
    hsts_preload: bool,
    no_sniff: bool,
    frame_options: string,
    referrer_policy: string,
    coop: string,
    corp: string,
    dns_prefetch_control: string,
    permitted_cross_domain: string,
    origin_agent_cluster: bool,
    fn defaultStrict(): HelmetOptions;
    fn minimal(): HelmetOptions;
}

export fn hlmWriteByte(out: []u8, pos: &mut usize, b: u8);
export fn hlmWriteStr(out: []u8, pos: &mut usize, s: string);
export fn hlmWriteU64(out: []u8, pos: &mut usize, val: u64);
export fn hlmWriteHeaderLine(out: []u8, pos: &mut usize, name: string, val: string);
export fn hlmAppendCspDirective(out: []u8, pos: &mut usize, has_any: &mut bool, dir_name: string, val: string);
export fn isSafeHeaderValue(val: string): bool;
export fn formatHstsHeader(max_age: u64, include_subdomains: bool, preload: bool, out: []u8): usize;
export fn formatCspHeader(default_src: string, script_src: string, style_src: string, img_src: string, connect_src: string, out: []u8): usize;
export fn formatHelmetHeaders(opts: HelmetOptions, out: []u8): usize;
