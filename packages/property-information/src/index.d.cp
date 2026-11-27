// Type definitions for CP

import { eqIgnoreCase } from "cap:ascii";
export class PropInfo {
    property: string,
    attribute: string,
    is_boolean: bool,
    is_space_separated: bool,
    is_comma_separated: bool,
    is_numeric: bool,
    found: bool,
}

export fn pinfMake(property: string, attribute: string, is_boolean: bool, is_space_separated: bool, is_comma_separated: bool, is_numeric: bool): PropInfo;
export fn pinfNotFound(name: string): PropInfo;
export fn pinfMatchPair(name: string, prop: string, attr: string): bool;
export fn findHtmlProp(name: string): PropInfo;
export fn isBooleanAttr(name: string): bool;
export fn isSpaceSeparatedAttr(name: string): bool;
export fn isCommaSeparatedAttr(name: string): bool;
export fn attrToDomProp(attr: string): string;
export fn domPropToAttr(prop: string): string;
