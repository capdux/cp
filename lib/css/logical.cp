import css from "./css_parser.zig";
pub const Error = css.Error;

pub enum PropertyCategory {
    logical,
    physical,

    static fn default(): PropertyCategory {
        return .physical;
    }
}

pub enum LogicalGroup {
    border_color,
    border_style,
    border_width,
    border_radius,
    margin,
    scroll_margin,
    padding,
    scroll_padding,
    inset,
    size,
    min_size,
    max_size,
}
