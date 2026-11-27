// Type definitions for CP

import {  } from "std";
export fn cp_expat_version(): [*:0]&u8!;
export fn cp_expat_count_elements(xml: [*]&u8!?, len: usize, out_elem_count: *usize!): i32;
export error ExpatError { InvalidInput, ParserInitFailed, ParseFailed }

export fn versionString(): string;
export fn countElements(xml: &[]u8): !usize;
