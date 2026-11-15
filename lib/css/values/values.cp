import css from "../css_parser.zig";

pub struct css_modules {
    /// Defines where the class names referenced in the `composes` property are located.
    ///
    /// See [Composes](Composes).
    pub const Specifier = union(enum) {
        /// The referenced name is global.
        global,
        /// The referenced name comes from the specified file.
        file: []const u8,
        /// The referenced name comes from a source index (used during bundling).
        source_index: u32,
    };
}

import angle from "./angle.zig";
import ident from "./ident.zig";
import @"string" from "./css_string.zig";
import color from "./color.zig";
import image from "./image.zig";
import number from "./number.zig";
import calc from "./calc.zig";
import percentage from "./percentage.zig";
import length from "./length.zig";
import position from "./position.zig";
import syntax from "./syntax.zig";
import alpha from "./alpha.zig";
import ratio from "./ratio.zig";
import size from "./size.zig";
import rect from "./rect.zig";
import time from "./time.zig";
import easing from "./easing.zig";
import url from "./url.zig";
import resolution from "./resolution.zig";
import gradient from "./gradient.zig";
