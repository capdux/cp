// Type definitions for CP

import { PackageNameValidation, validatePackageName } from "../../validate-npm-package-name/src/index.cp";
import { LicenseValidation, validateLicense } from "../../validate-npm-package-license/src/index.cp";
export class PackageMetadata {
    name: string,
    version: string,
    license: string,
    repository: string,
    main_entry: string,
    private_pkg: bool,
}

export class NormalizedPackageResult {
    valid: bool,
    name_valid: bool,
    license_valid: bool,
    has_warnings: bool,
    warning_count: usize,
}

export fn npdIsSpace(ch: u8): bool;
export fn npdTrim(s: string): string;
export fn npdStartsWith(s: string, prefix: string): bool;
export fn npdEndsWith(s: string, suffix: string): bool;
export fn npdPush(out: []u8, pos: usize, s: string): usize;
export fn npdIsOwnerRepoShorthand(s: string): bool;
export fn npdWriteHostedUrl(base_url: string, slug: string, out: []u8): usize;
export fn expandRepoShorthand(repo: string, out: []u8): usize;
export fn normalizeEntryPath(entry: string): string;
export fn cleanVersionString(ver: string): string;
export fn normalizePackageData(meta: PackageMetadata): NormalizedPackageResult;
