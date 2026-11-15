import bun from "bun";
pub const SrcIndex = bun.bundle_v2.Index;
pub const SymbolList = bun.ast.Symbol.List;
pub const ImportRecord = bun.ImportRecord;
pub const ImportKind = bun.ImportKind;
import prefixes from "./prefixes.zig";
import dependencies_ from "./dependencies.zig";
pub const Dependency = dependencies_.Dependency;
import css_modules from "./css_modules.zig";
pub const CssModuleExports = css_modules.CssModuleExports;
pub const CssModule = css_modules.CssModule;
pub const CssModuleReferences = css_modules.CssModuleReferences;
pub const CssModuleReference = css_modules.CssModuleReference;
pub const CssModuleConfig = css_modules.Config;
pub const Composes = css_modules.Composes;
import css_rules from "./rules/rules.zig";
pub const CssRule = css_rules.CssRule;
pub const CssRuleList = css_rules.CssRuleList;
pub const LayerName = css_rules.layer.LayerName;
pub const LayerStatementRule = css_rules.layer.LayerStatementRule;
pub const SupportsCondition = css_rules.supports.SupportsCondition;
pub const CustomMedia = css_rules.custom_media.CustomMediaRule;
pub const NamespaceRule = css_rules.namespace.NamespaceRule;
pub const UnknownAtRule = css_rules.unknown.UnknownAtRule;
pub const ImportRule = css_rules.@"import".ImportRule;
pub const StyleRule = css_rules.style.StyleRule;
pub const StyleContext = css_rules.StyleContext;
pub const SupportsRule = css_rules.supports.SupportsRule;
pub const TailwindAtRule = css_rules.tailwind.TailwindAtRule;
pub const MinifyContext = css_rules.MinifyContext;
import media_query from "./media_query.zig";
pub const MediaList = media_query.MediaList;
pub let MediaFeatureType = media_query.MediaFeatureType;
import css_values from "./values/values.zig";
pub const DashedIdent = css_values.ident.DashedIdent;
pub const DashedIdentFns = css_values.ident.DashedIdentFns;
pub const CssColor = css_values.color.CssColor;
pub const ColorFallbackKind = css_values.color.ColorFallbackKind;
pub const CSSString = css_values.string.CSSString;
pub const CSSStringFns = css_values.string.CSSStringFns;
pub const CSSInteger = css_values.number.CSSInteger;
pub const CSSIntegerFns = css_values.number.CSSIntegerFns;
pub const CSSNumber = css_values.number.CSSNumber;
pub const CSSNumberFns = css_values.number.CSSNumberFns;
pub const Ident = css_values.ident.Ident;
pub const IdentFns = css_values.ident.IdentFns;
pub const CustomIdent = css_values.ident.CustomIdent;
pub const CustomIdentFns = css_values.ident.CustomIdentFns;
pub const Url = css_values.url.Url;
import declaration from "./declaration.zig";
pub const css_decls = declaration;
pub const DeclarationList = css_decls.DeclarationList;
pub const DeclarationBlock = css_decls.DeclarationBlock;
import css_properties from "./properties/properties.zig";
pub const Property = css_properties.Property;
pub const PropertyId = css_properties.PropertyId;
pub const PropertyIdTag = css_properties.PropertyIdTag;
pub const TokenList = css_properties.custom.TokenList;
pub const TokenListFns = css_properties.custom.TokenListFns;
pub const PropertyBitset = css_properties.PropertyBitset;
import selector from "./selectors/selector.zig";
pub const SelectorList = selector.parser.SelectorList;
pub const Selector = selector.parser.Selector;
pub const Component = selector.parser.Component;
pub const PseudoClass = selector.parser.PseudoClass;
pub const PseudoElement = selector.parser.PseudoElement;
import logical from "./logical.zig";
pub const PropertyCategory = logical.PropertyCategory;
pub const LogicalGroup = logical.LogicalGroup;
import css_printer from "./printer.zig";
pub const Printer = css_printer.Printer;
pub const PrinterOptions = css_printer.PrinterOptions;
import targets_ from "./targets.zig";
pub const Targets = targets_.Targets;
pub const ImportInfo = css_printer.ImportInfo;
import context from "./context.zig";
pub const PropertyHandlerContext = context.PropertyHandlerContext;
pub const DeclarationHandler = declaration.DeclarationHandler;
import errors_ from "./error.zig";
pub const Maybe = bun.jsc.Node.Maybe;
pub const Err = errors_.Err;
pub const PrinterErrorKind = errors_.PrinterErrorKind;
pub const PrinterError = errors_.PrinterError;
pub const ErrorLocation = errors_.ErrorLocation;
pub const ParseError = errors_.ParseError;
pub const ParserError = errors_.ParserError;
pub const BasicParseError = errors_.BasicParseError;
pub const BasicParseErrorKind = errors_.BasicParseErrorKind;
pub const SelectorError = errors_.SelectorError;
pub const MinifyErrorKind = errors_.MinifyErrorKind;
pub const MinifyError = errors_.MinifyError;
pub const MinifyErr = errors_.MinifyErr;
import generic from "./generics.zig";
pub const HASH_SEED = generic.HASH_SEED;
pub const ImportConditions = css_rules.@"import".ImportConditions;
import compat from "./compat.zig";
pub const Features = Targets.Features;
pub const Feature = compat.Feature;
pub const fmtPrinterError = errors_.fmtPrinterError;
pub const PrintErr = error{
    CSSPrintError,
};
pub const Allocator = std.mem.Allocator;
pub const ArrayList = std.ArrayList;
pub const Log = bun.logger.Log;

pub const ParserFlags = packed struct(u8) {
    nesting: bool = false,
    custom_media: bool = false,
    deep_combinators: bool = false,
    empty: u5 = 0,
};

pub const CopyOnWriteStr = union(enum) {
    borrowed: []const u8,
    owned: std.array_list.Managed(u8),
    pub fn append(self: *@This(), allocator: Allocator, slice: []const u8) void {}
    pub fn appendByte(self: *@This(), allocator: Allocator, b: u8) void {}
    pub fn toSlice(self: *@This()): []const u8 { return ""; }
};

pub const Opts = ParserOptions;
pub const NthResult = struct { a: i32, b: i32 };
pub const SeenStatus = enum { dont_care, looking_for_them, seen };
pub const ColorError = error{ parse };
pub const CssRef = packed struct(u32) {
    inner_index: u26 = 0,
    tag: Tag = .{},
    pub const Tag = packed struct(u6) {
        class: bool = false,
        id: bool = false,
        animation: bool = false,
        keyframes: bool = false,
        container: bool = false,
        counter_style: bool = false,
    };
};

export fn OOM(e: anyerror): noreturn;
export struct todo_stuff {
}

export struct VendorPrefix {
    none: bool,
    webkit: bool,
    moz: bool,
    ms: bool,
    o: bool,
    __unused: u3,
    fn toCss(dest: &mut Printer): void ! PrintErr;
    fn fromName(comptime name: &[]u8): VendorPrefix;
    fn orNone(): VendorPrefix;
    fn _or(other: VendorPrefix): VendorPrefix;
    fn difference(left: @This(), right: @This()): @This();
    fn isEmpty(): bool;
    fn bitwiseAnd(a: @This(), b: @This()): @This();
    fn asBits(vp: @This()): u8;
}

export struct SourceLocation {
    line: u32,
    column: u32,
    fn toLoggerLocation(file: &[]u8): bun.logger.Location;
    fn newBasicUnexpectedTokenError(_: auto, token: Token): ParseError(ParserError);
    fn newUnexpectedTokenError(_: auto, token: Token): ParseError(ParserError);
    fn newCustomError(_: auto, e: auto): ParseError(ParserError);
}

export fn Result(comptime T: type): type;
export fn PrintResult(comptime T: type): type;
export fn todo(comptime fmt: &[]u8, args: auto): noreturn;
export fn voidWrap(comptime T: type, comptime parsefn: *const fn (*Parser) Result(T)): *const fn (void, *Parser) Result(T);
export fn DefineListShorthand(comptime T: type): type;
export fn DefineShorthand(comptime T: type, comptime property_name: PropertyIdTag, comptime PropertyFieldMap: auto): type;
export fn DefineRectShorthand(comptime T: type, comptime V: type): type;
export fn DefineSizeShorthand(comptime T: type, comptime V: type): type;
export fn DeriveParse(comptime T: type): type;
export fn DeriveToCss(comptime T: type): type;
export struct enum_property_util {
    fn asStr(comptime T: type, this: &T): &[]u8;
    fn parse(comptime T: type, input: &mut Parser): Result(T);
    fn toCss(comptime T: type, this: &T, dest: &mut Printer): void ! PrintErr;
}

export fn DefineEnumProperty(comptime T: type): type;
export fn DeriveValueType(comptime T: type, comptime ValueTypeMap: auto): type;
export fn consume_until_end_of_block(block_type: BlockType, tokenizer: &mut Tokenizer);
export fn parse_at_rule(allocator: Allocator, start: &ParserState, name: &[]u8, input: &mut Parser, comptime P: type, parser: &mut P): Result(P.AtRuleParser.AtRule);
export fn parse_custom_at_rule_prelude(name: &[]u8, input: &mut Parser, options: &ParserOptions, comptime T: type, at_rule_parser: &mut T): Result(AtRulePrelude(T.CustomAtRuleParser.Prelude));
export fn parse_custom_at_rule_without_block(comptime T: type, prelude: T.CustomAtRuleParser.Prelude, start: &ParserState, options: &ParserOptions, at_rule_parser: &mut T, is_nested: bool): Maybe(CssRule(T.CustomAtRuleParser.AtRule),void);
export fn parse_custom_at_rule_body(comptime T: type, prelude: T.CustomAtRuleParser.Prelude, input: &mut Parser, start: &ParserState, options: &ParserOptions, at_rule_parser: &mut T, is_nested: bool): Result(T.CustomAtRuleParser.AtRule);
export fn parse_qualified_rule(start: &ParserState, input: &mut Parser, comptime P: type, parser: &mut P, delimiters: Delimiters): Result(P.QualifiedRuleParser.QualifiedRule);
export fn parse_until_before(parser: &mut Parser, delimiters_: Delimiters, error_behavior: ParseUntilErrorBehavior, comptime T: type, closure: auto, comptime parse_fn: *const fn (@TypeOf(closure), *Parser) Result(T)): Result(T);
export fn parse_until_after(parser: &mut Parser, delimiters: Delimiters, error_behavior: ParseUntilErrorBehavior, comptime T: type, closure: auto, comptime parsefn: *const fn (@TypeOf(closure), *Parser) Result(T)): Result(T);
export fn parse_nested_block(parser: &mut Parser, comptime T: type, closure: auto, comptime parsefn: *const fn (@TypeOf(closure), *Parser) Result(T)): Result(T);
export fn ValidQualifiedRuleParser(comptime T: type);
export struct DefaultAtRule {
    fn toCss(_: &@This(), dest: &mut Printer): void ! PrintErr;
    fn deepClone(_: &@This(), _: std.mem.Allocator): @This();
}

export struct DefaultAtRuleParser {
}

export struct BundlerAtRuleParser {
    allocator: Allocator,
    import_records: &mut bun.BabyList(ImportRecord),
    layer_names: bun.BabyList(LayerName),
    options: &ParserOptions,
    anon_layer_count: u32,
    enclosing_layer: LayerName,
}

export fn ValidCustomAtRuleParser(comptime T: type);
export fn ValidAtRuleParser(comptime T: type);
export struct ImportPreludeLayer {
    value: LayerName?,
}

export struct KeyframesPrelude {
    name: css_rules.keyframes.KeyframesName,
    prefix: VendorPrefix,
}

export struct ContainerPrelude {
    name: css_rules.container.ContainerName?,
    condition: css_rules.container.ContainerCondition,
}

export struct ScopePrelude {
    scope_start: selector.parser.SelectorList?,
    scope_end: selector.parser.SelectorList?,
}

export struct UnknownPrelude {
    name: &[]u8,
    tokens: TokenList,
}

export struct ImportPreludeTuple {
    url: &[]u8,
    media: MediaList,
    supports: SupportsCondition?,
    layer: ImportPreludeLayer?,
}

export struct NamespacePreludeTuple {
    prefix: &[]u8?,
    url: &[]u8,
}

export struct CustomMediaPreludeTuple {
    name: DashedIdent,
    query: MediaList,
}

export struct PropertyPreludeTuple {
    name: DashedIdent,
}

export fn AtRulePrelude(comptime T: type): type;
export fn ParseNestedResult(comptime RuleT: type): type;
export fn TopLevelRuleParser(comptime AtRuleParserT: type): type;
export fn NestedRuleParser(comptime T: type): type;
export fn StyleSheetParser(comptime P: type): type;
export struct ToCssResult {
    code: &[]u8,
    exports: CssModuleExports?,
    references: CssModuleReferences?,
    dependencies: ArrayList(Dependency)?,
}

export struct ToCssResultInternal {
    exports: CssModuleExports?,
    references: CssModuleReferences?,
    dependencies: ArrayList(Dependency)?,
}

export struct MinifyOptions {
    targets: Targets,
    unused_symbols: std.StringArrayHashMapUnmanaged(void),
    fn default(): MinifyOptions;
}

export struct BundlerTailwindState {
    source: &[]u8,
    index: bun.bundle_v2.Index,
    output_from_tailwind: &[]u8?,
}

export struct StylesheetExtra {
    symbols: SymbolList,
}

export struct LocalEntry {
    ref: CssRef,
    loc: bun.logger.Loc,
}
pub const LocalScope = bun.StringArrayHashMapUnmanaged(LocalEntry);
pub const LocalsResultsMap = bun.bundle_v2.MangledProps;

export struct ParserExtra {
    symbols: SymbolList,
    local_scope: LocalScope,
    source_index: SrcIndex,
}



export struct ComposesEntry {
    composes: bun.BabyList(Composes),
}

export struct PropertyUsage {
    bitset: PropertyBitset,
    custom_properties: &[]&[]u8,
    range: bun.logger.Range,
    fn fill(_: auto, used: &PropertyBitset, custom_properties: &[]&[]u8);
}

pub const LocalPropertyUsage = bun.StringArrayHashMapUnmanaged(PropertyUsage);
export fn fillPropertyBitSet(allocator: Allocator, bitset: &mut PropertyBitset, block: &DeclarationBlock, custom_properties: &mut bun.BabyList(string));
export fn TailwindType(comptime AtRule: type): type;
export fn StyleSheet(comptime AtRule: type): type;
export struct StyleAttribute {
    declarations: DeclarationBlock,
    sources: ArrayList(string),
    fn parse(allocator: Allocator, code: &[]u8, options: ParserOptions, import_records: &mut bun.BabyList(ImportRecord), source_index: SrcIndex): Maybe(StyleAttribute,Err(ParserError));
    fn toCss(allocator: Allocator, options: PrinterOptions, import_info: ImportInfo?): ToCssResult ! PrintErr;
    fn minify(_: auto, allocator: Allocator, options: MinifyOptions);
}

export fn ValidDeclarationParser(comptime P: type);
export fn ValidRuleBodyItemParser(comptime P: type);
export fn RuleBodyParser(comptime P: type): type;
export struct ParserOptions {
    filename: &[]u8,
    css_modules: css_modules.Config?,
    source_index: u32,
    error_recovery: bool,
    logger: &mut Log?,
    flags: ParserFlags,
    allocator: Allocator,
    fn warn(warning: ParseError(ParserError));
    fn warnFmt(comptime text: &[]u8, args: auto, line: u32, column: u32);
    fn warnFmtWithNotes(comptime text: &[]u8, args: auto, line: u32, column: u32, notes: []bun.logger.Data);
    fn warnFmtWithNote(comptime text: &[]u8, args: auto, line: u32, column: u32, note_fmt: &[]u8, note_args: auto, note_range: bun.logger.Range);
    fn default(allocator: std.mem.Allocator, log: &mut Log?): ParserOptions;
}

export enum ParseUntilErrorBehavior {
    consume,
    stop
}

export struct ParserOpts {
    css_modules: bool,
    __unused: u7,
}

export struct Parser {
    input: &mut ParserInput,
    at_start_of: BlockType?,
    stop_before: Delimiters,
    flags: Opts,
    import_records: &mut bun.BabyList(ImportRecord)?,
    extra: &mut ParserExtra?,
    fn addSymbolForName(_: auto, name: &[]u8, tag: CssRef.Tag, loc: bun.logger.Loc): bun.bundle_v2.Ref;
    fn addImportRecord(_: auto, url: &[]u8, start_position: usize, kind: ImportKind): Result(u32);
    fn allocator(_: auto): Allocator;
    fn new(input: &mut ParserInput, import_records: &mut bun.BabyList(ImportRecord)?, flags: Opts, extra: &mut ParserExtra?): Parser;
    fn newCustomError(e: ParserError): ParseError(ParserError);
    fn newBasicError(kind: BasicParseErrorKind): BasicParseError;
    fn newError(kind: BasicParseErrorKind): ParseError(ParserError);
    fn newUnexpectedTokenError(token: Token): ParseError(ParserError);
    fn newBasicUnexpectedTokenError(token: Token): ParseError(ParserError);
    fn currentSourceLocation(): SourceLocation;
    fn currentSourceMapUrl(): &[]u8?;
    fn sliceFrom(start_position: usize): &[]u8;
    fn parseList(_: auto, comptime T: type, comptime parse_one: *const fn (*Parser) Result(T)): Result(ArrayList(T));
    fn parseCommaSeparated(_: auto, comptime T: type, comptime parse_one: *const fn (*Parser) Result(T)): Result(ArrayList(T));
    fn parseCommaSeparatedWithCtx(_: auto, comptime T: type, closure: auto, comptime parse_one: *const fn (@TypeOf(closure), *Parser) Result(T)): Result(ArrayList(T));
    fn parseCommaSeparatedInternal(_: auto, comptime T: type, closure: auto, comptime parse_one: *const fn (@TypeOf(closure), *Parser) Result(T), ignore_errors: bool): Result(ArrayList(T));
    fn tryParse(_: auto, comptime func: auto, args_: auto): bun.@"meta".ReturnOf(func);
    fn tryParseImpl(_: auto, comptime Ret: type, comptime func: auto, args: auto): Ret;
    fn parseNestedBlock(_: auto, comptime T: type, closure: auto, comptime parsefn: *const fn (@TypeOf(closure), *Parser) Result(T)): Result(T);
    fn isExhausted(_: auto): bool;
    fn expectNoErrorToken(_: auto): Result(void);
    fn expectPercentage(_: auto): Result(f32);
    fn expectComma(_: auto): Result(void);
    fn expectInteger(_: auto): Result(i32);
    fn expectNumber(_: auto): Result(f32);
    fn expectDelim(_: auto, delim: u8): Result(void);
    fn expectParenthesisBlock(_: auto): Result(void);
    fn expectColon(_: auto): Result(void);
    fn expectString(_: auto): Result(string);
    fn expectIdent(_: auto): Result(string);
    fn expectIdentOrString(_: auto): Result(string);
    fn expectIdentMatching(_: auto, name: &[]u8): Result(void);
    fn expectFunction(_: auto): Result(string);
    fn expectFunctionMatching(_: auto, name: &[]u8): Result(void);
    fn expectCurlyBracketBlock(_: auto): Result(void);
    fn expectSquareBracketBlock(_: auto): Result(void);
    fn expectUrl(_: auto): Result(string);
    fn expectUrlOrString(_: auto): Result(string);
    fn position(_: auto): usize;
    fn parseEmpty(_: &mut Parser): Result(void);
    fn parseUntilAfter(_: auto, delimiters: Delimiters, comptime T: type, closure: auto, comptime parse_fn: *const fn (@TypeOf(closure), *Parser) Result(T)): Result(T);
    fn parseUntilBefore(_: auto, delimiters: Delimiters, comptime T: type, closure: auto, comptime parse_fn: *const fn (@TypeOf(closure), *Parser) Result(T)): Result(T);
    fn parseEntirely(_: auto, comptime T: type, closure: auto, comptime parsefn: *const fn (@TypeOf(closure), *Parser) Result(T)): Result(T);
    fn expectExhausted(_: auto): Result(void);
    fn @"skip cdc and cdo"(_: auto);
    fn skipWhitespace(_: auto);
    fn next(_: auto): Result(*Token);
    fn nextIncludingWhitespace(_: auto): Result(*Token);
    fn nextByte(_: auto): u8?;
    fn reset(_: auto, state_: &ParserState);
    fn state(_: auto): ParserState;
    fn nextIncludingWhitespaceAndComments(_: auto): Result(*Token);
    fn newErrorForNextToken(_: auto): ParseError(ParserError);
}

export struct Delimiters {
    curly_bracket: bool,
    semicolon: bool,
    bang: bool,
    comma: bool,
    close_curly_bracket: bool,
    close_square_bracket: bool,
    close_parenthesis: bool,
    __unused: u1,
    fn getDelimiter(comptime tag: @TypeOf(.enum_literal)): Delimiters;
    fn fromByte(byte: u8?): Delimiters;
}

export struct ParserInput {
    tokenizer: Tokenizer,
    cached_token: CachedToken?,
    fn new(allocator: Allocator, code: &[]u8): ParserInput;
}

export struct ParserState {
    position: usize,
    current_line_start_position: usize,
    current_line_number: u32,
    import_record_count: u32,
    at_start_of: BlockType?,
    fn sourceLocation(): SourceLocation;
}

export enum BlockType {
    parenthesis,
    square_bracket,
    curly_bracket
}

export struct nth {
    fn parse_number_saturate(allocator: Allocator, str: &[]u8): Maybe(i32,void);
    fn parse_n_dash_digits(allocator: Allocator, str: &[]u8): Maybe(i32,void);
    fn parse_signless_b(input: &mut Parser, a: i32, b_sign: i32): Result(NthResult);
    fn parse_b(input: &mut Parser, a: i32): Result(NthResult);
    fn parse_nth(input: &mut Parser): Result(NthResult);
}

export struct CachedToken {
    token: Token,
    start_position: usize,
    end_state: ParserState,
}

export struct QuotedStringResult {
    str: &[]u8,
    bad: bool,
}

export struct HexDigitsResult {
    value: u32,
    num_digits: u32,
}

export struct Tokenizer {
    src: &[]u8,
    position: usize,
    source_map_url: &[]u8?,
    current_line_start_position: usize,
    current_line_number: u32,
    allocator: Allocator,
    var_or_env_functions: SeenStatus,
    current: Token,
    previous: Token,
    fn init(allocator: Allocator, src: &[]u8): Tokenizer;
    fn currentSourceMapUrl(): &[]u8?;
    fn getPosition(): usize;
    fn state(): ParserState;
    fn skipWhitespace(_: auto);
    fn currentSourceLocation(): SourceLocation;
    fn prev(_: auto): Token;
    fn isEof(_: auto): bool;
    fn seeFunction(_: auto, name: &[]u8);
    fn next(_: auto): Maybe(Token,void);
    fn nextImpl(_: auto): Maybe(Token,void);
    fn reset(_: auto, state2: &ParserState);
    fn @"skip cdc and cdo"(_: auto);
    fn consumeNumeric(_: auto): Token;
    fn consumeWhitespace(_: auto, comptime newline: bool): Token;
    fn consumeString(_: auto, comptime single_quote: bool): Token;
    fn consumeIdentLike(_: auto): Token;
    fn consumeName(_: auto): &[]u8;
    fn consumeQuotedString(_: auto, comptime single_quote: bool): QuotedStringResult;
    fn consumeUnquotedUrl(_: auto): Token?;
    fn consumeUnquotedUrlInternal(_: auto): Token;
    fn consumeUrlEnd(_: auto, start_pos: usize, str: &mut CopyOnWriteStr): Token;
    fn consumeBadUrl(_: auto, start_pos: usize): Token;
    fn consumeEscapeAndWrite(_: auto, bytes: &mut CopyOnWriteStr);
    fn consumeEscape(_: auto): u32;
    fn consumeHexDigits(_: auto): HexDigitsResult;
    fn consumeChar(_: auto): u32;
    fn lenUtf8(code: u32): usize;
    fn lenUtf16(ch: u32): usize;
    fn byteToHexDigit(b: u8): u32?;
    fn byteToDecimalDigit(b: u8): u32?;
    fn consumeComment(_: auto): &[]u8;
    fn checkForSourceMap(_: auto, contents: &[]u8);
    fn splitSourceMap(contents: &[]u8): &[]u8?;
    fn consumeNewline(_: auto);
    fn consumeContinuationByte(_: auto);
    fn consume4byteIntro(_: auto);
    fn isIdentStart(_: auto): bool;
    fn hasAtLeast(_: auto, n: usize): bool;
    fn hasNewlineAt(_: auto, offset: usize): bool;
    fn startsWith(_: auto, comptime needle: &[]u8): bool;
    fn advance(_: auto, n: usize);
    fn consumeKnownByte(_: auto, byte: u8);
    fn byteAt(_: auto, n: usize): u8;
    fn nextByte(_: auto): u8?;
    fn nextChar(_: auto): u32;
    fn nextByteUnchecked(_: auto): u8;
    fn sliceFrom(_: auto, start: usize): &[]u8;
}

export enum TokenKind {
    ident,
    function,
    at_keyword,
    unrestrictedhash,
    idhash,
    quoted_string,
    bad_string,
    unquoted_url,
    bad_url,
    delim,
    number,
    percentage,
    dimension,
    whitespace,
    cdo,
    cdc,
    include_match,
    dash_match,
    prefix_match,
    suffix_match,
    substring_match,
    colon,
    semicolon,
    comma,
    open_square,
    close_square,
    open_paren,
    close_paren,
    open_curly,
    close_curly,
    comment
}

export struct TokenPercentage {
    has_sign: bool,
    unit_value: f32,
    int_value: i32?,
    fn eql(lhs: &@This(), rhs: &@This()): bool;
    fn __generateHash();
}

export union Token {
    ident: &[]u8,
    function: &[]u8,
    at_keyword: &[]u8,
    unrestrictedhash: &[]u8,
    idhash: &[]u8,
    quoted_string: &[]u8,
    bad_string: &[]u8,
    unquoted_url: &[]u8,
    bad_url: &[]u8,
    delim: u32,
    number: Num,
    percentage: TokenPercentage,
    dimension: Dimension,
    whitespace: &[]u8,
    cdo: void,
    cdc: void,
    include_match: void,
    dash_match: void,
    prefix_match: void,
    suffix_match: void,
    substring_match: void,
    colon: void,
    semicolon: void,
    comma: void,
    open_square: void,
    close_square: void,
    open_paren: void,
    close_paren: void,
    open_curly: void,
    close_curly: void,
    comment: &[]u8,
    fn eql(lhs: &Token, rhs: &Token): bool;
    fn hash(hasher: &mut std.hash.Wyhash);
    fn isParseError(): bool;
    fn format(writer: &mut std.Io.Writer): !void;
    fn raw(): &[]u8;
    fn kind(): TokenKind;
    fn kindString(): &[]u8;
    fn toCssGeneric(writer: auto): !void;
    fn toCss(dest: &mut Printer): void ! PrintErr;
}

export struct Num {
    has_sign: bool,
    value: f32,
    int_value: i32?,
    fn eql(lhs: &Num, rhs: &Num): bool;
    fn hash(_: &@This(), hasher: &mut std.hash.Wyhash);
}

export struct Dimension {
    num: Num,
    unit: &[]u8,
    fn eql(lhs: &@This(), rhs: &@This()): bool;
    fn hash(_: &@This(), hasher: &mut std.hash.Wyhash);
}

export struct color {
    fn parseNamedColor(ident: &[]u8): [u8, u8, u8]?;
    fn parseHashColor(value: &[]u8): [u8, u8, u8, f32]?;
    fn parseHashColorImpl(value: &[]u8): [u8, u8, u8, f32] ! ColorError;
    fn fromHex(c: u8): u8 ! ColorError;
    fn hslToRgb(hue: f32, saturation: f32, lightness: f32): [f32, f32, f32];
}

export struct serializer {
    fn serializeName(value: &[]u8, writer: auto): !void;
    fn serializeString(value: &[]u8, writer: auto): !void;
    fn serializeDimension(value: f32, unit: &[]u8, dest: &mut Printer): void ! PrintErr;
    fn serializeIdentifier(value: &[]u8, writer: auto): !void;
    fn serializeUnquotedUrl(value: &[]u8, writer: auto): !void;
    fn writeNumeric(value: f32, int_value: i32?, has_sign: bool, writer: auto): !void;
    fn hexEscape(ascii_byte: u8, writer: auto): !void;
    fn charEscape(ascii_byte: u8, writer: auto): !void;
    fn CssStringWriter(comptime W: type): type;
}

export struct parse_utility {
    fn parseString(allocator: Allocator, comptime T: type, input: &[]u8, comptime parse_one: *const fn (*Parser) Result(T)): Result(T);
}

export struct to_css {
    fn string(allocator: Allocator, comptime T: type, val: &T, options: PrinterOptions, import_info: ImportInfo?, local_names: &LocalsResultsMap?, symbols: &bun.ast.Symbol.Map): &[]u8 ! PrintErr;
    fn fromList(comptime T: type, this: &[]T, dest: &mut Printer): void ! PrintErr;
    fn fromBabyList(comptime T: type, this: &bun.BabyList(T), dest: &mut Printer): void ! PrintErr;
    fn integer(comptime T: type, this: T, dest: &mut Printer): void ! PrintErr;
    fn float32(writer: &mut Printer): !void;
    fn maxDigits(comptime T: type): usize;
}

export fn parseImportant(input: &mut Parser): Result(void);
export struct signfns {
    fn isSignPositive(x: f32): bool;
    fn isSignNegative(x: f32): bool;
    fn signum(x: f32): f32;
    fn signF32(x: f32): f32;
}

export fn copysign(client: f32, sign: f32): f32;
export fn deepClone(comptime V: type, allocator: Allocator, list: &ArrayList(V)): ArrayList(V);
export fn deepDeinit(comptime V: type, allocator: Allocator, list: &mut ArrayList(V));
export struct Notation {
    decimal_point: bool,
    scientific: bool,
    fn integer(): Notation;
}

export fn dtoa_short(buf: &mut u8[129], value: f32, comptime precision: u8): ![[]u8, Notation?];
export fn dtoa_short_impl(buf: &mut u8[129], value: f32, comptime precision: u8): [[]u8, Notation];
export fn restrict_prec(buf: []u8, comptime prec: u8): [[]u8, Notation];
export fn fract(val: f32): f32;
export fn f32_length_with_5_digits(n_input: f32): usize;
import {  } from "./context.zig";
import {  } from "./declaration.zig";
import {  } from "./error.zig";
