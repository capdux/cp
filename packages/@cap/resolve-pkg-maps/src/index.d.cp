// Type definitions for CP

export class ExportRule {
    pattern: string,
    condition: string,
    target: string,
}

export fn rpmEq(a: string, b: string): bool;
export fn rpmFindStar(s: string): usize;
export fn rpmHasParentTraversal(path: string): bool;
export fn rpmConditionRank(cond: string, conditions: &[]string): usize;
export fn matchSubpathPattern(pattern: string, subpath: string, matched_star: []u8): i64;
export fn resolveExportTarget(rules: &[]ExportRule, subpath: string, conditions: &[]string, out: []u8): usize;
