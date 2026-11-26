// @cap/property-information — Zero-allocation HTML/SVG attribute <-> DOM property mapping
// Clean-room CP implementation inspired by property-information (MIT).

pub struct PropInfo {
    property: string,
    attribute: string,
    is_boolean: bool,
    is_space_separated: bool,
    is_comma_separated: bool,
    is_numeric: bool,
    found: bool,
}

fn pinfToLowerByte(c: u8): u8 {
    if (c >= 'A' && c <= 'Z') {
        return c + 32;
    }
    return c;
}

fn pinfEqIgnoreCase(a: string, b: string): bool {
    if (a.len != b.len) return false;
    let i: usize = 0;
    while (i < a.len) : (i += 1) {
        if (pinfToLowerByte(a[i]) != pinfToLowerByte(b[i])) {
            return false;
        }
    }
    return true;
}

fn pinfHasPrefixIgnoreCase(s: string, prefix: string): bool {
    if (s.len < prefix.len) return false;
    let i: usize = 0;
    while (i < prefix.len) : (i += 1) {
        if (pinfToLowerByte(s[i]) != pinfToLowerByte(prefix[i])) {
            return false;
        }
    }
    return true;
}

fn pinfMake(
    property: string,
    attribute: string,
    is_boolean: bool,
    is_space_separated: bool,
    is_comma_separated: bool,
    is_numeric: bool,
): PropInfo {
    return PropInfo{
        property: property,
        attribute: attribute,
        is_boolean: is_boolean,
        is_space_separated: is_space_separated,
        is_comma_separated: is_comma_separated,
        is_numeric: is_numeric,
        found: true,
    };
}

fn pinfNotFound(name: string): PropInfo {
    return PropInfo{
        property: name,
        attribute: name,
        is_boolean: false,
        is_space_separated: false,
        is_comma_separated: false,
        is_numeric: false,
        found: false,
    };
}

fn pinfMatchPair(name: string, prop: string, attr: string): bool {
    if (pinfEqIgnoreCase(name, attr)) return true;
    if (pinfEqIgnoreCase(name, prop)) return true;
    return false;
}

pub fn findHtmlProp(name: string): PropInfo {
    if (name.len == 0) return pinfNotFound(name);

    // Core HTML attribute <-> DOM property remappings
    if (pinfMatchPair(name, "className", "class")) return pinfMake("className", "class", false, true, false, false);
    if (pinfMatchPair(name, "htmlFor", "for")) return pinfMake("htmlFor", "for", false, true, false, false);
    if (pinfMatchPair(name, "tabIndex", "tabindex")) return pinfMake("tabIndex", "tabindex", false, false, false, true);
    if (pinfMatchPair(name, "readOnly", "readonly")) return pinfMake("readOnly", "readonly", true, false, false, false);
    if (pinfMatchPair(name, "maxLength", "maxlength")) return pinfMake("maxLength", "maxlength", false, false, false, true);
    if (pinfMatchPair(name, "minLength", "minlength")) return pinfMake("minLength", "minlength", false, false, false, true);
    if (pinfMatchPair(name, "colSpan", "colspan")) return pinfMake("colSpan", "colspan", false, false, false, true);
    if (pinfMatchPair(name, "rowSpan", "rowspan")) return pinfMake("rowSpan", "rowspan", false, false, false, true);
    if (pinfMatchPair(name, "autoFocus", "autofocus")) return pinfMake("autoFocus", "autofocus", true, false, false, false);
    if (pinfMatchPair(name, "autoPlay", "autoplay")) return pinfMake("autoPlay", "autoplay", true, false, false, false);
    if (pinfMatchPair(name, "noValidate", "novalidate")) return pinfMake("noValidate", "novalidate", true, false, false, false);
    if (pinfMatchPair(name, "formNoValidate", "formnovalidate")) return pinfMake("formNoValidate", "formnovalidate", true, false, false, false);
    if (pinfMatchPair(name, "allowFullScreen", "allowfullscreen")) return pinfMake("allowFullScreen", "allowfullscreen", true, false, false, false);
    if (pinfMatchPair(name, "playsInline", "playsinline")) return pinfMake("playsInline", "playsinline", true, false, false, false);
    if (pinfMatchPair(name, "isMap", "ismap")) return pinfMake("isMap", "ismap", true, false, false, false);
    if (pinfMatchPair(name, "noModule", "nomodule")) return pinfMake("noModule", "nomodule", true, false, false, false);
    if (pinfMatchPair(name, "itemScope", "itemscope")) return pinfMake("itemScope", "itemscope", true, false, false, false);
    if (pinfMatchPair(name, "contentEditable", "contenteditable")) return pinfMake("contentEditable", "contenteditable", false, false, false, false);
    if (pinfMatchPair(name, "crossOrigin", "crossorigin")) return pinfMake("crossOrigin", "crossorigin", false, false, false, false);
    if (pinfMatchPair(name, "autoComplete", "autocomplete")) return pinfMake("autoComplete", "autocomplete", false, true, false, false);
    if (pinfMatchPair(name, "accessKey", "accesskey")) return pinfMake("accessKey", "accesskey", false, true, false, false);
    if (pinfMatchPair(name, "acceptCharset", "accept-charset")) return pinfMake("acceptCharset", "accept-charset", false, true, false, false);
    if (pinfMatchPair(name, "httpEquiv", "http-equiv")) return pinfMake("httpEquiv", "http-equiv", false, true, false, false);
    if (pinfMatchPair(name, "srcSet", "srcset")) return pinfMake("srcSet", "srcset", false, false, true, false);
    if (pinfMatchPair(name, "imageSrcSet", "imagesrcset")) return pinfMake("imageSrcSet", "imagesrcset", false, false, true, false);
    if (pinfMatchPair(name, "srcDoc", "srcdoc")) return pinfMake("srcDoc", "srcdoc", false, false, false, false);
    if (pinfMatchPair(name, "useMap", "usemap")) return pinfMake("useMap", "usemap", false, false, false, false);
    if (pinfMatchPair(name, "dateTime", "datetime")) return pinfMake("dateTime", "datetime", false, false, false, false);
    if (pinfMatchPair(name, "encType", "enctype")) return pinfMake("encType", "enctype", false, false, false, false);
    if (pinfMatchPair(name, "spellCheck", "spellcheck")) return pinfMake("spellCheck", "spellcheck", false, false, false, false);
    if (pinfMatchPair(name, "referrerPolicy", "referrerpolicy")) return pinfMake("referrerPolicy", "referrerpolicy", false, false, false, false);
    if (pinfMatchPair(name, "viewBox", "viewBox")) return pinfMake("viewBox", "viewBox", false, false, false, false);
    if (pinfMatchPair(name, "preserveAspectRatio", "preserveAspectRatio")) return pinfMake("preserveAspectRatio", "preserveAspectRatio", false, false, false, false);

    // Boolean HTML attributes
    if (pinfEqIgnoreCase(name, "disabled")) return pinfMake("disabled", "disabled", true, false, false, false);
    if (pinfEqIgnoreCase(name, "checked")) return pinfMake("checked", "checked", true, false, false, false);
    if (pinfEqIgnoreCase(name, "selected")) return pinfMake("selected", "selected", true, false, false, false);
    if (pinfEqIgnoreCase(name, "multiple")) return pinfMake("multiple", "multiple", true, false, false, false);
    if (pinfEqIgnoreCase(name, "hidden")) return pinfMake("hidden", "hidden", true, false, false, false);
    if (pinfEqIgnoreCase(name, "required")) return pinfMake("required", "required", true, false, false, false);
    if (pinfEqIgnoreCase(name, "controls")) return pinfMake("controls", "controls", true, false, false, false);
    if (pinfEqIgnoreCase(name, "loop")) return pinfMake("loop", "loop", true, false, false, false);
    if (pinfEqIgnoreCase(name, "muted")) return pinfMake("muted", "muted", true, false, false, false);
    if (pinfEqIgnoreCase(name, "defer")) return pinfMake("defer", "defer", true, false, false, false);
    if (pinfEqIgnoreCase(name, "async")) return pinfMake("async", "async", true, false, false, false);
    if (pinfEqIgnoreCase(name, "open")) return pinfMake("open", "open", true, false, false, false);
    if (pinfEqIgnoreCase(name, "reversed")) return pinfMake("reversed", "reversed", true, false, false, false);
    if (pinfEqIgnoreCase(name, "inert")) return pinfMake("inert", "inert", true, false, false, false);
    if (pinfEqIgnoreCase(name, "default")) return pinfMake("default", "default", true, false, false, false);

    // Comma-separated HTML attributes
    if (pinfEqIgnoreCase(name, "accept")) return pinfMake("accept", "accept", false, false, true, false);
    if (pinfEqIgnoreCase(name, "coords")) return pinfMake("coords", "coords", false, false, true, false);

    // Space-separated HTML attributes
    if (pinfEqIgnoreCase(name, "rel")) return pinfMake("rel", "rel", false, true, false, false);
    if (pinfEqIgnoreCase(name, "sandbox")) return pinfMake("sandbox", "sandbox", false, true, false, false);
    if (pinfEqIgnoreCase(name, "headers")) return pinfMake("headers", "headers", false, true, false, false);
    if (pinfEqIgnoreCase(name, "ping")) return pinfMake("ping", "ping", false, true, false, false);
    if (pinfEqIgnoreCase(name, "itemProp") || pinfEqIgnoreCase(name, "itemprop")) return pinfMake("itemProp", "itemprop", false, true, false, false);
    if (pinfEqIgnoreCase(name, "itemRef") || pinfEqIgnoreCase(name, "itemref")) return pinfMake("itemRef", "itemref", false, true, false, false);
    if (pinfEqIgnoreCase(name, "itemType") || pinfEqIgnoreCase(name, "itemtype")) return pinfMake("itemType", "itemtype", false, true, false, false);
    if (pinfEqIgnoreCase(name, "sizes")) return pinfMake("sizes", "sizes", false, true, false, false);
    if (pinfEqIgnoreCase(name, "blocking")) return pinfMake("blocking", "blocking", false, true, false, false);

    // Numeric HTML attributes
    if (pinfEqIgnoreCase(name, "rows")) return pinfMake("rows", "rows", false, false, false, true);
    if (pinfEqIgnoreCase(name, "cols")) return pinfMake("cols", "cols", false, false, false, true);
    if (pinfEqIgnoreCase(name, "size")) return pinfMake("size", "size", false, false, false, true);
    if (pinfEqIgnoreCase(name, "span")) return pinfMake("span", "span", false, false, false, true);
    if (pinfEqIgnoreCase(name, "start")) return pinfMake("start", "start", false, false, false, true);
    if (pinfEqIgnoreCase(name, "width")) return pinfMake("width", "width", false, false, false, true);
    if (pinfEqIgnoreCase(name, "height")) return pinfMake("height", "height", false, false, false, true);

    // Common standard attributes
    if (pinfEqIgnoreCase(name, "id")) return pinfMake("id", "id", false, false, false, false);
    if (pinfEqIgnoreCase(name, "href")) return pinfMake("href", "href", false, false, false, false);
    if (pinfEqIgnoreCase(name, "src")) return pinfMake("src", "src", false, false, false, false);
    if (pinfEqIgnoreCase(name, "alt")) return pinfMake("alt", "alt", false, false, false, false);
    if (pinfEqIgnoreCase(name, "title")) return pinfMake("title", "title", false, false, false, false);
    if (pinfEqIgnoreCase(name, "style")) return pinfMake("style", "style", false, false, false, false);
    if (pinfEqIgnoreCase(name, "type")) return pinfMake("type", "type", false, false, false, false);
    if (pinfEqIgnoreCase(name, "name")) return pinfMake("name", "name", false, false, false, false);
    if (pinfEqIgnoreCase(name, "value")) return pinfMake("value", "value", false, false, false, false);
    if (pinfEqIgnoreCase(name, "role")) return pinfMake("role", "role", false, true, false, false);

    // ARIA attributes (aria-*)
    if (pinfHasPrefixIgnoreCase(name, "aria-") && name.len > 5) {
        const is_space: bool = pinfEqIgnoreCase(name, "aria-labelledby") ||
            pinfEqIgnoreCase(name, "aria-describedby") ||
            pinfEqIgnoreCase(name, "aria-controls") ||
            pinfEqIgnoreCase(name, "aria-owns") ||
            pinfEqIgnoreCase(name, "aria-flowto") ||
            pinfEqIgnoreCase(name, "aria-relevant");
        return pinfMake(name, name, false, is_space, false, false);
    }

    // Custom data attributes (data-*)
    if (pinfHasPrefixIgnoreCase(name, "data-") && name.len > 5) {
        return pinfMake(name, name, false, false, false, false);
    }

    return pinfNotFound(name);
}

pub fn isBooleanAttr(name: string): bool {
    const info = findHtmlProp(name);
    return info.found && info.is_boolean;
}

pub fn isSpaceSeparatedAttr(name: string): bool {
    const info = findHtmlProp(name);
    return info.found && info.is_space_separated;
}

pub fn isCommaSeparatedAttr(name: string): bool {
    const info = findHtmlProp(name);
    return info.found && info.is_comma_separated;
}

pub fn attrToDomProp(attr: string): string {
    const info = findHtmlProp(attr);
    if (info.found) return info.property;
    return attr;
}

pub fn domPropToAttr(prop: string): string {
    const info = findHtmlProp(prop);
    if (info.found) return info.attribute;
    return prop;
}
