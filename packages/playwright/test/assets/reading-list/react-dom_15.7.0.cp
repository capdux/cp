// Migrated from TypeScript (assets/reading-list/react-dom_15.7.0.js) via cap from -ts

 /**
  * ReactDOM v15.7.0
  */

// TODO(cap-migrate): refine TS statement
// [ts] ;((f) => {
// CommonJS
// [ts] if (@typeName(exports) == "object" && @typeName(module) != "undefined") {
// [ts] module.exports = f(require("react"));
// RequireJS
// [ts] } else if (@typeName(define) == "function" && define.amd) {
// [ts] define(["react"], f);
// <script>
// [ts] } else {
// [ts] let g;
// [ts] if (@typeName(window) != "undefined") {
// [ts] g = window;
// [ts] } else if (@typeName(global) != "undefined") {
// [ts] g = global;
// [ts] } else if (@typeName(self) != "undefined") {
// [ts] g = self;
// [ts] } else {
// works providing we're not in "use strict";
// needed for Java 8 Nashorn
// see https://github.com/facebook/react/issues/3037
// [ts] g = this;
// [ts] }
// [ts] g.ReactDOM = f(g.React);
// [ts] }
// [ts] })((React) => {
// [ts] return ((f) => {return f()})(() => {let define,module,exports;return ((t, n, r) => {(o, u) => {if(!n[o]){if(!t[o]){let a=@typeName(require)=="function"&&require;if(!u&&a)return a(o,!0);if(i)return i(o,!0);let f=Error.init("Cannot find module '"+o+"'");throw f.code="MODULE_NOT_FOUND",f}let l=n[o]={exports:{}};t[o][0].call(l.exports,(e) => {let n=t[o][1][e];return s(if (n) n else e)},l,l.exports,e,t,n,r)}return n[o].exports}let i=@typeName(require)=="function"&&require;for(let o=0;o<r.length;o++)s(r[o]);return s})({1:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ARIADOMPropertyConfig = {
// [ts] Properties: {
// Global States and Properties
// [ts] "aria-current": 0, // state
// [ts] "aria-details": 0,
// [ts] "aria-disabled": 0, // state
// [ts] "aria-hidden": 0, // state
// [ts] "aria-invalid": 0, // state
// [ts] "aria-keyshortcuts": 0,
// [ts] "aria-label": 0,
// [ts] "aria-roledescription": 0,
// Widget Attributes
// [ts] "aria-autocomplete": 0,
// [ts] "aria-checked": 0,
// [ts] "aria-expanded": 0,
// [ts] "aria-haspopup": 0,
// [ts] "aria-level": 0,
// [ts] "aria-modal": 0,
// [ts] "aria-multiline": 0,
// [ts] "aria-multiselectable": 0,
// [ts] "aria-orientation": 0,
// [ts] "aria-placeholder": 0,
// [ts] "aria-pressed": 0,
// [ts] "aria-readonly": 0,
// [ts] "aria-required": 0,
// [ts] "aria-selected": 0,
// [ts] "aria-sort": 0,
// [ts] "aria-valuemax": 0,
// [ts] "aria-valuemin": 0,
// [ts] "aria-valuenow": 0,
// [ts] "aria-valuetext": 0,
// Live Region Attributes
// [ts] "aria-atomic": 0,
// [ts] "aria-busy": 0,
// [ts] "aria-live": 0,
// [ts] "aria-relevant": 0,
// Drag-and-Drop Attributes
// [ts] "aria-dropeffect": 0,
// [ts] "aria-grabbed": 0,
// Relationship Attributes
// [ts] "aria-activedescendant": 0,
// [ts] "aria-colcount": 0,
// [ts] "aria-colindex": 0,
// [ts] "aria-colspan": 0,
// [ts] "aria-controls": 0,
// [ts] "aria-describedby": 0,
// [ts] "aria-errormessage": 0,
// [ts] "aria-flowto": 0,
// [ts] "aria-labelledby": 0,
// [ts] "aria-owns": 0,
// [ts] "aria-posinset": 0,
// [ts] "aria-rowcount": 0,
// [ts] "aria-rowindex": 0,
// [ts] "aria-rowspan": 0,
// [ts] "aria-setsize": 0
// [ts] },
// [ts] DOMAttributeNames: {},
// [ts] DOMPropertyNames: {}
// [ts] };
// [ts] module.exports = ARIADOMPropertyConfig;
// [ts] },{}],2:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let focusNode = _dereq_(146);
// [ts] let AutoFocusUtils = {
// [ts] focusDOMComponent: () => {
// [ts] focusNode(ReactDOMComponentTree.getNodeFromInstance(this));
// [ts] }
// [ts] };
// [ts] module.exports = AutoFocusUtils;
// [ts] },{"146":146,"34":34}],3:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let EventPropagators = _dereq_(20);
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let FallbackCompositionState = _dereq_(21);
// [ts] let SyntheticCompositionEvent = _dereq_(89);
// [ts] let SyntheticInputEvent = _dereq_(93);
// [ts] let END_KEYCODES = [9, 13, 27, 32]; // Tab, Return, Esc, Space
// [ts] let START_KEYCODE = 229;
// [ts] let canUseCompositionEvent = ExecutionEnvironment.canUseDOM && "CompositionEvent" == window;
// [ts] let documentMode = null;
// [ts] if (ExecutionEnvironment.canUseDOM && "documentMode" == document) {
// [ts] documentMode = document.documentMode;
// [ts] }
// Webkit offers a very useful `textInput` event that can be used to
// directly represent `beforeInput`. The IE `textinput` event is not as
// useful, so we don't use it.
// [ts] let canUseTextInputEvent = ExecutionEnvironment.canUseDOM && "TextEvent" == window && !documentMode && !isPresto();
// In IE9+, we have access to composition events, but the data supplied
// by the native compositionend event may be incorrect. Japanese ideographic
// spaces, for instance (\u3000) are not recorded correctly.
// [ts] let useFallbackCompositionData = ExecutionEnvironment.canUseDOM && (!canUseCompositionEvent || documentMode && documentMode > 8 && documentMode <= 11);
// [ts] /**
// [ts] * Opera <= 12 includes TextEvent in window, but does not fire
// [ts] * text input events. Rely on keypress instead.
// [ts] */
// [ts] fn isPresto() {
// [ts] let opera = window.opera;
// [ts] return @typeName(opera) == "object" && @typeName(opera.version) == "function" && parseInt(opera.version(), 10) <= 12;
// [ts] }
// [ts] let SPACEBAR_CODE = 32;
// [ts] let SPACEBAR_CHAR = String.fromCharCode(SPACEBAR_CODE);
// Events and their corresponding property names.
// [ts] let eventTypes = {
// [ts] beforeInput: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onBeforeInput",
// [ts] captured: "onBeforeInputCapture"
// [ts] },
// [ts] dependencies: ["topCompositionEnd", "topKeyPress", "topTextInput", "topPaste"]
// [ts] },
// [ts] compositionEnd: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onCompositionEnd",
// [ts] captured: "onCompositionEndCapture"
// [ts] },
// [ts] dependencies: ["topBlur", "topCompositionEnd", "topKeyDown", "topKeyPress", "topKeyUp", "topMouseDown"]
// [ts] },
// [ts] compositionStart: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onCompositionStart",
// [ts] captured: "onCompositionStartCapture"
// [ts] },
// [ts] dependencies: ["topBlur", "topCompositionStart", "topKeyDown", "topKeyPress", "topKeyUp", "topMouseDown"]
// [ts] },
// [ts] compositionUpdate: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onCompositionUpdate",
// [ts] captured: "onCompositionUpdateCapture"
// [ts] },
// [ts] dependencies: ["topBlur", "topCompositionUpdate", "topKeyDown", "topKeyPress", "topKeyUp", "topMouseDown"]
// [ts] }
// [ts] };
// Track whether we've ever handled a keypress on the space key.
// [ts] let hasSpaceKeypress = false;
// [ts] /**
// [ts] * Return whether a native keypress event is assumed to be a command.
// [ts] * This is required because Firefox fires `keypress` events for key commands
// [ts] * (cut, copy, select-all, etc.) even though no character is inserted.
// [ts] */
// [ts] fn isKeypressCommand(nativeEvent: auto) {
// [ts] return (nativeEvent.ctrlKey || nativeEvent.altKey || nativeEvent.metaKey) &&
// ctrlKey && altKey is equivalent to AltGr, and is not a command.
// [ts] !(nativeEvent.ctrlKey && nativeEvent.altKey);
// [ts] }
// [ts] /**
// [ts] * Translate native top level events into event types.
// [ts] *
// [ts] * @param {string} topLevelType
// [ts] * @return {object}
// [ts] */
// [ts] fn getCompositionEventType(topLevelType: auto) {
// [ts] switch (topLevelType) {
// [ts] "topCompositionStart" => {
// [ts] return eventTypes.compositionStart;
// [ts] },
// [ts] "topCompositionEnd" => {
// [ts] return eventTypes.compositionEnd;
// [ts] },
// [ts] "topCompositionUpdate" => {
// [ts] return eventTypes.compositionUpdate;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Does our fallback best-guess model think this event signifies that
// [ts] * composition has begun?
// [ts] *
// [ts] * @param {string} topLevelType
// [ts] * @param {object} nativeEvent
// [ts] * @return {boolean}
// [ts] */
// [ts] fn isFallbackCompositionStart(topLevelType: auto, nativeEvent: auto) {
// [ts] return topLevelType == "topKeyDown" && nativeEvent.keyCode == START_KEYCODE;
// [ts] }
// [ts] /**
// [ts] * Does our fallback mode think that this event is the end of composition?
// [ts] *
// [ts] * @param {string} topLevelType
// [ts] * @param {object} nativeEvent
// [ts] * @return {boolean}
// [ts] */
// [ts] fn isFallbackCompositionEnd(topLevelType: auto, nativeEvent: auto) {
// [ts] switch (topLevelType) {
// Command keys insert or clear IME input.
// [ts] "topKeyUp" => {
// [ts] return END_KEYCODES.indexOf(nativeEvent.keyCode) != -1;
// [ts] },
// Expect IME keyCode on each keydown. If we get any other
// code we must have exited earlier.
// [ts] "topKeyDown" => {
// [ts] return nativeEvent.keyCode != START_KEYCODE;
// [ts] },
// Events are not possible without cancelling IME.
// [ts] "topKeyPress", "topMouseDown", "topBlur" => {
// [ts] return true;
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Google Input Tools provides composition data via a CustomEvent,
// [ts] * with the `data` property populated in the `detail` object. If this
// [ts] * is available on the event object, use it. If not, this is a plain
// [ts] * composition event and we have nothing special to extract.
// [ts] *
// [ts] * @param {object} nativeEvent
// [ts] * @return {?string}
// [ts] */
// [ts] fn getDataFromCustomEvent(nativeEvent: auto) {
// [ts] let detail = nativeEvent.detail;
// [ts] if (@typeName(detail) == "object" && "data" == detail) {
// [ts] return detail.data;
// [ts] }
// [ts] return null;
// [ts] }
// Track the current IME composition fallback object, if any.
// [ts] let currentComposition = null;
// [ts] /**
// [ts] * @return {?object} A SyntheticCompositionEvent.
// [ts] */
// [ts] fn extractCompositionEvent(topLevelType: auto, targetInst: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] let eventType;
// [ts] let fallbackData;
// [ts] if (canUseCompositionEvent) {
// [ts] eventType = getCompositionEventType(topLevelType);
// [ts] } else if (!currentComposition) {
// [ts] if (isFallbackCompositionStart(topLevelType, nativeEvent)) {
// [ts] eventType = eventTypes.compositionStart;
// [ts] }
// [ts] } else if (isFallbackCompositionEnd(topLevelType, nativeEvent)) {
// [ts] eventType = eventTypes.compositionEnd;
// [ts] }
// [ts] if (!eventType) {
// [ts] return null;
// [ts] }
// [ts] if (useFallbackCompositionData) {
// The current composition is stored statically and must not be
// overwritten while composition continues.
// [ts] if (!currentComposition && eventType == eventTypes.compositionStart) {
// [ts] currentComposition = FallbackCompositionState.getPooled(nativeEventTarget);
// [ts] } else if (eventType == eventTypes.compositionEnd) {
// [ts] if (currentComposition) {
// [ts] fallbackData = currentComposition.getData();
// [ts] }
// [ts] }
// [ts] }
// [ts] let event = SyntheticCompositionEvent.getPooled(eventType, targetInst, nativeEvent, nativeEventTarget);
// [ts] if (fallbackData) {
// Inject data generated from fallback path into the synthetic event.
// This matches the property of native CompositionEventInterface.
// [ts] event.data = fallbackData;
// [ts] } else {
// [ts] let customData = getDataFromCustomEvent(nativeEvent);
// [ts] if (customData != null) {
// [ts] event.data = customData;
// [ts] }
// [ts] }
// [ts] EventPropagators.accumulateTwoPhaseDispatches(event);
// [ts] return event;
// [ts] }
// [ts] /**
// [ts] * @param {string} topLevelType Record from `EventConstants`.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {?string} The string corresponding to this `beforeInput` event.
// [ts] */
// [ts] fn getNativeBeforeInputChars(topLevelType: auto, nativeEvent: auto) {
// [ts] switch (topLevelType) {
// [ts] "topCompositionEnd" => {
// [ts] return getDataFromCustomEvent(nativeEvent);
// [ts] },
// [ts] /**
// [ts] * If native `textInput` events are available, our goal is to make
// [ts] * use of them. However, there is a special case: the spacebar key.
// [ts] * In Webkit, preventing default on a spacebar `textInput` event
// [ts] * cancels character insertion, but it *also* causes the browser
// [ts] * to fall back to its default spacebar behavior of scrolling the
// [ts] * page.
// [ts] *
// [ts] * Tracking at:
// [ts] * https://code.google.com/p/chromium/issues/detail?id=355103
// [ts] *
// [ts] * To avoid this issue, use the keypress event as if no `textInput`
// [ts] * event is available.
// [ts] */
// [ts] "topKeyPress" => {
// [ts] let which = nativeEvent.which;
// [ts] if (which != SPACEBAR_CODE) {
// [ts] return null;
// [ts] }
// [ts] hasSpaceKeypress = true;
// [ts] return SPACEBAR_CHAR;
// [ts] },
// Record the characters to be added to the DOM.
// [ts] "topTextInput" => {
// [ts] let chars = nativeEvent.data;
// If it's a spacebar character, assume that we have already handled
// it at the keypress level and bail immediately. Android Chrome
// doesn't give us keycodes, so we need to blacklist it.
// [ts] if (chars == SPACEBAR_CHAR && hasSpaceKeypress) {
// [ts] return null;
// [ts] }
// [ts] return chars;
// [ts] },
// [ts] else => {
// For other native event types, do nothing.
// [ts] return null;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * For browsers that do not provide the `textInput` event, extract the
// [ts] * appropriate string to use for SyntheticInputEvent.
// [ts] *
// [ts] * @param {string} topLevelType Record from `EventConstants`.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {?string} The fallback string for this `beforeInput` event.
// [ts] */
// [ts] fn getFallbackBeforeInputChars(topLevelType: auto, nativeEvent: auto) {
// If we are currently composing (IME) and using a fallback to do so,
// try to extract the composed characters from the fallback object.
// If composition event is available, we extract a string only at
// compositionevent, otherwise extract it at fallback events.
// [ts] if (currentComposition) {
// [ts] if (topLevelType == "topCompositionEnd" || !canUseCompositionEvent && isFallbackCompositionEnd(topLevelType, nativeEvent)) {
// [ts] let chars = currentComposition.getData();
// [ts] FallbackCompositionState.release(currentComposition);
// [ts] currentComposition = null;
// [ts] return chars;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] switch (topLevelType) {
// If a paste event occurs after a keypress, throw out the input
// chars. Paste events should not lead to BeforeInput events.
// [ts] "topPaste" => {
// [ts] return null;
// [ts] },
// [ts] /**
// [ts] * As of v27, Firefox may fire keypress events even when no character
// [ts] * will be inserted. A few possibilities:
// [ts] *
// [ts] * - `which` is `0`. Arrow keys, Esc key, etc.
// [ts] *
// [ts] * - `which` is the pressed key code, but no char is available.
// [ts] *   Ex: 'AltGr + d` in Polish. There is no modified character for
// [ts] *   this key combination and no character is inserted into the
// [ts] *   document, but FF fires the keypress for char code `100` anyway.
// [ts] *   No `input` event will occur.
// [ts] *
// [ts] * - `which` is the pressed key code, but a command combination is
// [ts] *   being used. Ex: `Cmd+C`. No character is inserted, and no
// [ts] *   `input` event will occur.
// [ts] */
// [ts] "topKeyPress" => {
// [ts] if (nativeEvent.which && !isKeypressCommand(nativeEvent)) {
// [ts] return String.fromCharCode(nativeEvent.which);
// [ts] }
// [ts] return null;
// [ts] },
// [ts] "topCompositionEnd" => {
// [ts] return if (useFallbackCompositionData) null else nativeEvent.data;
// [ts] },
// [ts] else => {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Extract a SyntheticInputEvent for `beforeInput`, based on either native
// [ts] * `textInput` or fallback behavior.
// [ts] *
// [ts] * @return {?object} A SyntheticInputEvent.
// [ts] */
// [ts] fn extractBeforeInputEvent(topLevelType: auto, targetInst: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] let chars;
// [ts] if (canUseTextInputEvent) {
// [ts] chars = getNativeBeforeInputChars(topLevelType, nativeEvent);
// [ts] } else {
// [ts] chars = getFallbackBeforeInputChars(topLevelType, nativeEvent);
// [ts] }
// If no characters are being inserted, no BeforeInput event should
// be fired.
// [ts] if (!chars) {
// [ts] return null;
// [ts] }
// [ts] let event = SyntheticInputEvent.getPooled(eventTypes.beforeInput, targetInst, nativeEvent, nativeEventTarget);
// [ts] event.data = chars;
// [ts] EventPropagators.accumulateTwoPhaseDispatches(event);
// [ts] return event;
// [ts] }
// [ts] /**
// [ts] * Create an `onBeforeInput` event to match
// [ts] * http://www.w3.org/TR/2013/WD-DOM-Level-3-Events-20131105/#events-inputevents.
// [ts] *
// [ts] * This event plugin is based on the native `textInput` event
// [ts] * available in Chrome, Safari, Opera, and IE. This event fires after
// [ts] * `onKeyPress` and `onCompositionEnd`, but before `onInput`.
// [ts] *
// [ts] * `beforeInput` is spec'd but not implemented in any browsers, and
// [ts] * the `input` event does not provide any useful information about what has
// [ts] * actually been added, contrary to the spec. Thus, `textInput` is the best
// [ts] * available event to identify the characters that have actually been inserted
// [ts] * into the target node.
// [ts] *
// [ts] * This plugin is also responsible for emitting `composition` events, thus
// [ts] * allowing us to share composition fallback code for both `beforeInput` and
// [ts] * `composition` event types.
// [ts] */
// [ts] let BeforeInputEventPlugin = {
// [ts] eventTypes: eventTypes,
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget) => {
// [ts] return [extractCompositionEvent(topLevelType, targetInst, nativeEvent, nativeEventTarget), extractBeforeInputEvent(topLevelType, targetInst, nativeEvent, nativeEventTarget)];
// [ts] }
// [ts] };
// [ts] module.exports = BeforeInputEventPlugin;
// [ts] },{"138":138,"20":20,"21":21,"89":89,"93":93}],4:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * CSS properties which accept numbers but are not in units of "px".
// [ts] */
// [ts] let isUnitlessNumber = {
// [ts] animationIterationCount: true,
// [ts] borderImageOutset: true,
// [ts] borderImageSlice: true,
// [ts] borderImageWidth: true,
// [ts] boxFlex: true,
// [ts] boxFlexGroup: true,
// [ts] boxOrdinalGroup: true,
// [ts] columnCount: true,
// [ts] columns: true,
// [ts] flex: true,
// [ts] flexGrow: true,
// [ts] flexPositive: true,
// [ts] flexShrink: true,
// [ts] flexNegative: true,
// [ts] flexOrder: true,
// [ts] gridRow: true,
// [ts] gridRowEnd: true,
// [ts] gridRowSpan: true,
// [ts] gridRowStart: true,
// [ts] gridColumn: true,
// [ts] gridColumnEnd: true,
// [ts] gridColumnSpan: true,
// [ts] gridColumnStart: true,
// [ts] fontWeight: true,
// [ts] lineClamp: true,
// [ts] lineHeight: true,
// [ts] opacity: true,
// [ts] order: true,
// [ts] orphans: true,
// [ts] tabSize: true,
// [ts] widows: true,
// [ts] zIndex: true,
// [ts] zoom: true,
// SVG-related properties
// [ts] fillOpacity: true,
// [ts] floodOpacity: true,
// [ts] stopOpacity: true,
// [ts] strokeDasharray: true,
// [ts] strokeDashoffset: true,
// [ts] strokeMiterlimit: true,
// [ts] strokeOpacity: true,
// [ts] strokeWidth: true
// [ts] };
// [ts] /**
// [ts] * @param {string} prefix vendor-specific prefix, eg: Webkit
// [ts] * @param {string} key style name, eg: transitionDuration
// [ts] * @return {string} style name prefixed with `prefix`, properly camelCased, eg:
// [ts] * WebkitTransitionDuration
// [ts] */
// [ts] fn prefixKey(prefix: auto, key: auto) {
// [ts] return prefix + key.charAt(0).toUpperCase() + key.substring(1);
// [ts] }
// [ts] /**
// [ts] * Support style names that may come passed in prefixed by adding permutations
// [ts] * of vendor prefixes.
// [ts] */
// [ts] let prefixes = ["Webkit", "ms", "Moz", "O"];
// Using Object.keys here, or else the vanilla for-in loop makes IE8 go into an
// infinite loop, because it iterates over the newly added props too.
// [ts] Object.keys(isUnitlessNumber).forEach((prop) => {
// [ts] prefixes.forEach((prefix) => {
// [ts] isUnitlessNumber[prefixKey(prefix, prop)] = isUnitlessNumber[prop];
// [ts] });
// [ts] });
// [ts] /**
// [ts] * Most style properties can be unset by doing .style[prop] = '' but IE8
// [ts] * doesn't like doing that with shorthand properties so for the properties that
// [ts] * IE8 breaks on, which are listed here, we instead unset each of the
// [ts] * individual properties. See http://bugs.jquery.com/ticket/12385.
// [ts] * The 4-value 'clock' properties like margin, padding, border-width seem to
// [ts] * behave without any problems. Curiously, list-style works too without any
// [ts] * special prodding.
// [ts] */
// [ts] let shorthandPropertyExpansions = {
// [ts] background: {
// [ts] backgroundAttachment: true,
// [ts] backgroundColor: true,
// [ts] backgroundImage: true,
// [ts] backgroundPositionX: true,
// [ts] backgroundPositionY: true,
// [ts] backgroundRepeat: true
// [ts] },
// [ts] backgroundPosition: {
// [ts] backgroundPositionX: true,
// [ts] backgroundPositionY: true
// [ts] },
// [ts] border: {
// [ts] borderWidth: true,
// [ts] borderStyle: true,
// [ts] borderColor: true
// [ts] },
// [ts] borderBottom: {
// [ts] borderBottomWidth: true,
// [ts] borderBottomStyle: true,
// [ts] borderBottomColor: true
// [ts] },
// [ts] borderLeft: {
// [ts] borderLeftWidth: true,
// [ts] borderLeftStyle: true,
// [ts] borderLeftColor: true
// [ts] },
// [ts] borderRight: {
// [ts] borderRightWidth: true,
// [ts] borderRightStyle: true,
// [ts] borderRightColor: true
// [ts] },
// [ts] borderTop: {
// [ts] borderTopWidth: true,
// [ts] borderTopStyle: true,
// [ts] borderTopColor: true
// [ts] },
// [ts] font: {
// [ts] fontStyle: true,
// [ts] fontVariant: true,
// [ts] fontWeight: true,
// [ts] fontSize: true,
// [ts] lineHeight: true,
// [ts] fontFamily: true
// [ts] },
// [ts] outline: {
// [ts] outlineWidth: true,
// [ts] outlineStyle: true,
// [ts] outlineColor: true
// [ts] }
// [ts] };
// [ts] let CSSProperty = {
// [ts] isUnitlessNumber: isUnitlessNumber,
// [ts] shorthandPropertyExpansions: shorthandPropertyExpansions
// [ts] };
// [ts] module.exports = CSSProperty;
// [ts] },{}],5:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let CSSProperty = _dereq_(4);
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let camelizeStyleName = _dereq_(140);
// [ts] let dangerousStyleValue = _dereq_(106);
// [ts] let hyphenateStyleName = _dereq_(151);
// [ts] let memoizeStringOnly = _dereq_(155);
// [ts] let warning = _dereq_(159);
// [ts] let processStyleName = memoizeStringOnly((styleName) => {
// [ts] return hyphenateStyleName(styleName);
// [ts] });
// [ts] let hasShorthandPropertyBug = false;
// [ts] let styleFloatAccessor = "cssFloat";
// [ts] if (ExecutionEnvironment.canUseDOM) {
// [ts] let tempStyle = document.createElement("div").style;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// IE8 throws "Invalid argument." if resetting shorthand style properties.
// [ts] tempStyle.font = "";
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] hasShorthandPropertyBug = true;
// [ts] }
// IE8 only supports accessing cssFloat (standard) as styleFloat
// [ts] if (document.documentElement.style.cssFloat == null) {
// [ts] styleFloatAccessor = "styleFloat";
// [ts] }
// [ts] }
// [ts] if ("development" != "production") {
// 'msTransform' is correct, but the other prefixes should be capitalized
// [ts] let badVendoredStyleNamePattern = "^(?:webkit|moz|o)[A-Z]";
// style values shouldn't contain a semicolon
// [ts] let badStyleValueWithSemicolonPattern = ";\\s*$";
// [ts] let warnedStyleNames = {};
// [ts] let warnedStyleValues = {};
// [ts] let warnedForNaNValue = false;
// [ts] let warnHyphenatedStyleName = (name, owner) => {
// [ts] if (warnedStyleNames.hasOwnProperty(name) && warnedStyleNames[name]) {
// [ts] return;
// [ts] }
// [ts] warnedStyleNames[name] = true;
// [ts] if ("development" != "production") warning(false, "Unsupported style property %s. Did you mean %s?%s", name, camelizeStyleName(name), checkRenderMessage(owner)) else void 0;
// [ts] };
// [ts] let warnBadVendoredStyleName = (name, owner) => {
// [ts] if (warnedStyleNames.hasOwnProperty(name) && warnedStyleNames[name]) {
// [ts] return;
// [ts] }
// [ts] warnedStyleNames[name] = true;
// [ts] if ("development" != "production") warning(false, "Unsupported vendor-prefixed style property %s. Did you mean %s?%s", name, name.charAt(0).toUpperCase() + name.slice(1), checkRenderMessage(owner)) else void 0;
// [ts] };
// [ts] let warnStyleValueWithSemicolon = (name, value, owner) => {
// [ts] if (warnedStyleValues.hasOwnProperty(value) && warnedStyleValues[value]) {
// [ts] return;
// [ts] }
// [ts] warnedStyleValues[value] = true;
// [ts] if ("development" != "production") warning(false, "Style property values shouldn't contain a semicolon.%s " + "Try \"%s: %s\" instead.", checkRenderMessage(owner), name, value.replace(badStyleValueWithSemicolonPattern, "")) else void 0;
// [ts] };
// [ts] let warnStyleValueIsNaN = (name, value, owner) => {
// [ts] if (warnedForNaNValue) {
// [ts] return;
// [ts] }
// [ts] warnedForNaNValue = true;
// [ts] if ("development" != "production") warning(false, "`NaN` is an invalid value for the `%s` css style property.%s", name, checkRenderMessage(owner)) else void 0;
// [ts] };
// [ts] let checkRenderMessage = (owner) => {
// [ts] if (owner) {
// [ts] let name = owner.getName();
// [ts] if (name) {
// [ts] return " Check the render method of `" + name + "`.";
// [ts] }
// [ts] }
// [ts] return "";
// [ts] };
// [ts] /**
// [ts] * @param {string} name
// [ts] * @param {*} value
// [ts] * @param {ReactDOMComponent} component
// [ts] */
// [ts] let warnValidStyle = (name, value, component) => {
// [ts] let owner;
// [ts] if (component) {
// [ts] owner = component._currentElement._owner;
// [ts] }
// [ts] if (name.indexOf("-") > -1) {
// [ts] warnHyphenatedStyleName(name, owner);
// [ts] } else if (badVendoredStyleNamePattern.test(name)) {
// [ts] warnBadVendoredStyleName(name, owner);
// [ts] } else if (badStyleValueWithSemicolonPattern.test(value)) {
// [ts] warnStyleValueWithSemicolon(name, value, owner);
// [ts] }
// [ts] if (@typeName(value) == "number" && isNaN(value)) {
// [ts] warnStyleValueIsNaN(name, value, owner);
// [ts] }
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Operations for dealing with CSS properties.
// [ts] */
// [ts] let CSSPropertyOperations = {
// [ts] /**
// [ts] * Serializes a mapping of style properties for use as inline styles:
// [ts] *
// [ts] *   > createMarkupForStyles({width: '200px', height: 0})
// [ts] *   "width:200px;height:0;"
// [ts] *
// [ts] * Undefined values are ignored so that declarative programming is easier.
// [ts] * The result should be HTML-escaped before insertion into the DOM.
// [ts] *
// [ts] * @param {object} styles
// [ts] * @param {ReactDOMComponent} component
// [ts] * @return {?string}
// [ts] */
// [ts] createMarkupForStyles: (styles, component) => {
// [ts] let serialized = "";
// [ts] for (let styleName in styles) {
// [ts] if (!styles.hasOwnProperty(styleName)) {
// [ts] continue;
// [ts] }
// [ts] let isCustomProperty = styleName.indexOf("--") == 0;
// [ts] let styleValue = styles[styleName];
// [ts] if ("development" != "production") {
// [ts] if (!isCustomProperty) {
// [ts] warnValidStyle(styleName, styleValue, component);
// [ts] }
// [ts] }
// [ts] if (styleValue != null) {
// [ts] serialized += processStyleName(styleName) + ":";
// [ts] serialized += dangerousStyleValue(styleName, styleValue, component, isCustomProperty) + ";";
// [ts] }
// [ts] }
// [ts] return serialized || null;
// [ts] },
// [ts] /**
// [ts] * Sets the value for multiple styles on a node.  If a value is specified as
// [ts] * '' (empty string), the corresponding style property will be unset.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {object} styles
// [ts] * @param {ReactDOMComponent} component
// [ts] */
// [ts] setValueForStyles: (node, styles, component) => {
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: component._debugID,
// [ts] type: "update styles",
// [ts] payload: styles
// [ts] });
// [ts] }
// [ts] let style = node.style;
// [ts] for (let styleName in styles) {
// [ts] if (!styles.hasOwnProperty(styleName)) {
// [ts] continue;
// [ts] }
// [ts] let isCustomProperty = styleName.indexOf("--") == 0;
// [ts] if ("development" != "production") {
// [ts] if (!isCustomProperty) {
// [ts] warnValidStyle(styleName, styles[styleName], component);
// [ts] }
// [ts] }
// [ts] let styleValue = dangerousStyleValue(styleName, styles[styleName], component, isCustomProperty);
// [ts] if (styleName == "float" || styleName == "cssFloat") {
// [ts] styleName = styleFloatAccessor;
// [ts] }
// [ts] if (isCustomProperty) {
// [ts] style.setProperty(styleName, styleValue);
// [ts] } else if (styleValue) {
// [ts] style[styleName] = styleValue;
// [ts] } else {
// [ts] let expansion = hasShorthandPropertyBug && CSSProperty.shorthandPropertyExpansions[styleName];
// [ts] if (expansion) {
// Shorthand property that IE8 won't like unsetting, so unset each
// component to placate it
// [ts] for (let individualStyleName in expansion) {
// [ts] style[individualStyleName] = "";
// [ts] }
// [ts] } else {
// [ts] style[styleName] = "";
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = CSSPropertyOperations;
// [ts] },{"106":106,"138":138,"140":140,"151":151,"155":155,"159":159,"4":4,"64":64}],6:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] fn _classCallCheck(instance: auto, Constructor: auto) { if (!(instance == Constructor)) { throw TypeError.init("Cannot call a class as a function"); } }
// [ts] let PooledClass = _dereq_(25);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * A specialized pseudo-event module to help keep track of components waiting to
// [ts] * be notified when their DOM representations are available for use.
// [ts] *
// [ts] * This implements `PooledClass`, so you should never need to instantiate this.
// [ts] * Instead, use `CallbackQueue.getPooled()`.
// [ts] *
// [ts] * @class ReactMountReady
// [ts] * @implements PooledClass
// [ts] * @internal
// [ts] */
// [ts] let CallbackQueue = () => {
// [ts] fn CallbackQueue(arg: auto) {
// [ts] _classCallCheck(this, CallbackQueue);
// [ts] _._callbacks = null;
// [ts] _._contexts = null;
// [ts] _._arg = arg;
// [ts] }
// [ts] /**
// [ts] * Enqueues a callback to be invoked when `notifyAll` is invoked.
// [ts] *
// [ts] * @param {function} callback Invoked when `notifyAll` is invoked.
// [ts] * @param {?object} context Context to call `callback` with.
// [ts] * @internal
// [ts] */
// [ts] CallbackQueue.prototype.enqueue = (callback, context) => {
// [ts] _._callbacks = _._callbacks || [];
// [ts] _._callbacks.push(callback);
// [ts] _._contexts = _._contexts || [];
// [ts] _._contexts.push(context);
// [ts] };
// [ts] /**
// [ts] * Invokes all enqueued callbacks and clears the queue. This is invoked after
// [ts] * the DOM representation of a component has been created or updated.
// [ts] *
// [ts] * @internal
// [ts] */
// [ts] CallbackQueue.prototype.notifyAll = () => {
// [ts] let callbacks = _._callbacks;
// [ts] let contexts = _._contexts;
// [ts] let arg = _._arg;
// [ts] if (callbacks && contexts) {
// [ts] if (!(callbacks.length == contexts.length)) if ("development" != "production") invariant(false, "Mismatched list of contexts in callback queue") else _prodInvariant("24") else void 0;
// [ts] _._callbacks = null;
// [ts] _._contexts = null;
// [ts] let i = 0;
// [ts] while (i < callbacks.length) { // [cap-migrate] for-step: i++
// [ts] callbacks[i].call(contexts[i], arg);
// [ts] }
// [ts] callbacks.length = 0;
// [ts] contexts.length = 0;
// [ts] }
// [ts] };
// [ts] CallbackQueue.prototype.checkpoint = () => {
// [ts] return if (_._callbacks) _._callbacks.length else 0;
// [ts] };
// [ts] CallbackQueue.prototype.rollback = (len) => {
// [ts] if (_._callbacks && _._contexts) {
// [ts] _._callbacks.length = len;
// [ts] _._contexts.length = len;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Resets the internal queue.
// [ts] *
// [ts] * @internal
// [ts] */
// [ts] CallbackQueue.prototype.reset = () => {
// [ts] _._callbacks = null;
// [ts] _._contexts = null;
// [ts] };
// [ts] /**
// [ts] * `PooledClass` looks for this.
// [ts] */
// [ts] CallbackQueue.prototype.destructor = () => {
// [ts] _.reset();
// [ts] };
// [ts] return CallbackQueue;
// [ts] }();
// [ts] module.exports = PooledClass.addPoolingTo(CallbackQueue);
// [ts] },{"126":126,"152":152,"25":25}],7:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let EventPluginHub = _dereq_(17);
// [ts] let EventPropagators = _dereq_(20);
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] let inputValueTracking = _dereq_(120);
// [ts] let getEventTarget = _dereq_(114);
// [ts] let isEventSupported = _dereq_(122);
// [ts] let isTextInputElement = _dereq_(123);
// [ts] let eventTypes = {
// [ts] change: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onChange",
// [ts] captured: "onChangeCapture"
// [ts] },
// [ts] dependencies: ["topBlur", "topChange", "topClick", "topFocus", "topInput", "topKeyDown", "topKeyUp", "topSelectionChange"]
// [ts] }
// [ts] };
// [ts] fn createAndAccumulateChangeEvent(inst: auto, nativeEvent: auto, target: auto) {
// [ts] let event = SyntheticEvent.getPooled(eventTypes.change, inst, nativeEvent, target);
// [ts] event.type = "change";
// [ts] EventPropagators.accumulateTwoPhaseDispatches(event);
// [ts] return event;
// [ts] }
// [ts] /**
// [ts] * For IE shims
// [ts] */
// [ts] let activeElement = null;
// [ts] let activeElementInst = null;
// [ts] /**
// [ts] * SECTION: handle `change` event
// [ts] */
// [ts] fn shouldUseChangeEvent(elem: auto) {
// [ts] let nodeName = elem.nodeName && elem.nodeName.toLowerCase();
// [ts] return nodeName == "select" || nodeName == "input" && elem.type == "file";
// [ts] }
// [ts] let doesChangeEventBubble = false;
// [ts] if (ExecutionEnvironment.canUseDOM) {
// See `handleChange` comment below
// [ts] doesChangeEventBubble = isEventSupported("change") && (!document.documentMode || document.documentMode > 8);
// [ts] }
// [ts] fn manualDispatchChangeEvent(nativeEvent: auto) {
// [ts] let event = createAndAccumulateChangeEvent(activeElementInst, nativeEvent, getEventTarget(nativeEvent));
// If change and propertychange bubbled, we'd just bind to it like all the
// other events and have it go through ReactBrowserEventEmitter. Since it
// doesn't, we manually listen for the events and so we have to enqueue and
// process the abstract event manually.
//
// Batching is necessary here in order to ensure that all event handlers run
// before the next rerender (including event handlers attached to ancestor
// elements instead of directly on the input). Without this, controlled
// components don't work properly in conjunction with event bubbling because
// the component is rerendered and the value reverted before all the event
// handlers can run. See https://github.com/facebook/react/issues/708.
// [ts] ReactUpdates.batchedUpdates(runEventInBatch, event);
// [ts] }
// [ts] fn runEventInBatch(event: auto) {
// [ts] EventPluginHub.enqueueEvents(event);
// [ts] EventPluginHub.processEventQueue(false);
// [ts] }
// [ts] fn startWatchingForChangeEventIE8(target: auto, targetInst: auto) {
// [ts] activeElement = target;
// [ts] activeElementInst = targetInst;
// [ts] activeElement.attachEvent("onchange", manualDispatchChangeEvent);
// [ts] }
// [ts] fn stopWatchingForChangeEventIE8() {
// [ts] if (!activeElement) {
// [ts] return;
// [ts] }
// [ts] activeElement.detachEvent("onchange", manualDispatchChangeEvent);
// [ts] activeElement = null;
// [ts] activeElementInst = null;
// [ts] }
// [ts] fn getInstIfValueChanged(targetInst: auto, nativeEvent: auto) {
// [ts] let updated = inputValueTracking.updateValueIfChanged(targetInst);
// [ts] let simulated = nativeEvent.simulated == true && ChangeEventPlugin._allowSimulatedPassThrough;
// [ts] if (updated || simulated) {
// [ts] return targetInst;
// [ts] }
// [ts] }
// [ts] fn getTargetInstForChangeEvent(topLevelType: auto, targetInst: auto) {
// [ts] if (topLevelType == "topChange") {
// [ts] return targetInst;
// [ts] }
// [ts] }
// [ts] fn handleEventsForChangeEventIE8(topLevelType: auto, target: auto, targetInst: auto) {
// [ts] if (topLevelType == "topFocus") {
// stopWatching() should be a noop here but we call it just in case we
// missed a blur event somehow.
// [ts] stopWatchingForChangeEventIE8();
// [ts] startWatchingForChangeEventIE8(target, targetInst);
// [ts] } else if (topLevelType == "topBlur") {
// [ts] stopWatchingForChangeEventIE8();
// [ts] }
// [ts] }
// [ts] /**
// [ts] * SECTION: handle `input` event
// [ts] */
// [ts] let isInputEventSupported = false;
// [ts] if (ExecutionEnvironment.canUseDOM) {
// IE9 claims to support the input event but fails to trigger it when
// deleting text, so we ignore its input events.
// [ts] isInputEventSupported = isEventSupported("input") && (!document.documentMode || document.documentMode > 9);
// [ts] }
// [ts] /**
// [ts] * (For IE <=9) Starts tracking propertychange events on the passed-in element
// [ts] * and override the value property so that we can distinguish user events from
// [ts] * value changes in JS.
// [ts] */
// [ts] fn startWatchingForValueChange(target: auto, targetInst: auto) {
// [ts] activeElement = target;
// [ts] activeElementInst = targetInst;
// [ts] activeElement.attachEvent("onpropertychange", handlePropertyChange);
// [ts] }
// [ts] /**
// [ts] * (For IE <=9) Removes the event listeners from the currently-tracked element,
// [ts] * if any exists.
// [ts] */
// [ts] fn stopWatchingForValueChange() {
// [ts] if (!activeElement) {
// [ts] return;
// [ts] }
// [ts] activeElement.detachEvent("onpropertychange", handlePropertyChange);
// [ts] activeElement = null;
// [ts] activeElementInst = null;
// [ts] }
// [ts] /**
// [ts] * (For IE <=9) Handles a propertychange event, sending a `change` event if
// [ts] * the value of the active element has changed.
// [ts] */
// [ts] fn handlePropertyChange(nativeEvent: auto) {
// [ts] if (nativeEvent.propertyName != "value") {
// [ts] return;
// [ts] }
// [ts] if (getInstIfValueChanged(activeElementInst, nativeEvent)) {
// [ts] manualDispatchChangeEvent(nativeEvent);
// [ts] }
// [ts] }
// [ts] fn handleEventsForInputEventPolyfill(topLevelType: auto, target: auto, targetInst: auto) {
// [ts] if (topLevelType == "topFocus") {
// In IE8, we can capture almost all .value changes by adding a
// propertychange handler and looking for events with propertyName
// equal to 'value'
// In IE9, propertychange fires for most input events but is buggy and
// doesn't fire when text is deleted, but conveniently, selectionchange
// appears to fire in all of the remaining cases so we catch those and
// forward the event if the value has changed
// In either case, we don't want to call the event handler if the value
// is changed from JS so we redefine a setter for `.value` that updates
// our activeElementValue variable, allowing us to ignore those changes
//
// stopWatching() should be a noop here but we call it just in case we
// missed a blur event somehow.
// [ts] stopWatchingForValueChange();
// [ts] startWatchingForValueChange(target, targetInst);
// [ts] } else if (topLevelType == "topBlur") {
// [ts] stopWatchingForValueChange();
// [ts] }
// [ts] }
// For IE8 and IE9.
// [ts] fn getTargetInstForInputEventPolyfill(topLevelType: auto, targetInst: auto, nativeEvent: auto) {
// [ts] if (topLevelType == "topSelectionChange" || topLevelType == "topKeyUp" || topLevelType == "topKeyDown") {
// On the selectionchange event, the target is just document which isn't
// helpful for us so just check activeElement instead.
//
// 99% of the time, keydown and keyup aren't necessary. IE8 fails to fire
// propertychange on the first input event after setting `value` from a
// script and fires only keydown, keypress, keyup. Catching keyup usually
// gets it and catching keydown lets us fire an event for the first
// keystroke if user does a key repeat (it'll be a little delayed: right
// before the second keystroke). Other input methods (e.g., paste) seem to
// fire selectionchange normally.
// [ts] return getInstIfValueChanged(activeElementInst, nativeEvent);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * SECTION: handle `click` event
// [ts] */
// [ts] fn shouldUseClickEvent(elem: auto) {
// Use the `click` event to detect changes to checkbox and radio inputs.
// This approach works across all browsers, whereas `change` does not fire
// until `blur` in IE8.
// [ts] let nodeName = elem.nodeName;
// [ts] return nodeName && nodeName.toLowerCase() == "input" && (elem.type == "checkbox" || elem.type == "radio");
// [ts] }
// [ts] fn getTargetInstForClickEvent(topLevelType: auto, targetInst: auto, nativeEvent: auto) {
// [ts] if (topLevelType == "topClick") {
// [ts] return getInstIfValueChanged(targetInst, nativeEvent);
// [ts] }
// [ts] }
// [ts] fn getTargetInstForInputOrChangeEvent(topLevelType: auto, targetInst: auto, nativeEvent: auto) {
// [ts] if (topLevelType == "topInput" || topLevelType == "topChange") {
// [ts] return getInstIfValueChanged(targetInst, nativeEvent);
// [ts] }
// [ts] }
// [ts] fn handleControlledInputBlur(inst: auto, node: auto) {
// TODO: In IE, inst is occasionally null. Why?
// [ts] if (inst == null) {
// [ts] return;
// [ts] }
// Fiber and ReactDOM keep wrapper state in separate places
// [ts] let state = inst._wrapperState || node._wrapperState;
// [ts] if (!state || !state.controlled || node.type != "number") {
// [ts] return;
// [ts] }
// If controlled, assign the value attribute to the current value on blur
// [ts] let value = "" + node.value;
// [ts] if (node.getAttribute("value") != value) {
// [ts] node.setAttribute("value", value);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * This plugin creates an `onChange` event that normalizes change events
// [ts] * across form elements. This event fires at a time when it's possible to
// [ts] * change the element's value without seeing a flicker.
// [ts] *
// [ts] * Supported elements are:
// [ts] * - input (see `isTextInputElement`)
// [ts] * - textarea
// [ts] * - select
// [ts] */
// [ts] let ChangeEventPlugin = {
// [ts] eventTypes: eventTypes,
// [ts] _allowSimulatedPassThrough: true,
// [ts] _isInputEventSupported: isInputEventSupported,
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget) => {
// [ts] let targetNode = if (targetInst) ReactDOMComponentTree.getNodeFromInstance(targetInst) else window;
// [ts] let getTargetInstFunc, handleEventFunc;
// [ts] if (shouldUseChangeEvent(targetNode)) {
// [ts] if (doesChangeEventBubble) {
// [ts] getTargetInstFunc = getTargetInstForChangeEvent;
// [ts] } else {
// [ts] handleEventFunc = handleEventsForChangeEventIE8;
// [ts] }
// [ts] } else if (isTextInputElement(targetNode)) {
// [ts] if (isInputEventSupported) {
// [ts] getTargetInstFunc = getTargetInstForInputOrChangeEvent;
// [ts] } else {
// [ts] getTargetInstFunc = getTargetInstForInputEventPolyfill;
// [ts] handleEventFunc = handleEventsForInputEventPolyfill;
// [ts] }
// [ts] } else if (shouldUseClickEvent(targetNode)) {
// [ts] getTargetInstFunc = getTargetInstForClickEvent;
// [ts] }
// [ts] if (getTargetInstFunc) {
// [ts] let inst = getTargetInstFunc(topLevelType, targetInst, nativeEvent);
// [ts] if (inst) {
// [ts] let event = createAndAccumulateChangeEvent(inst, nativeEvent, nativeEventTarget);
// [ts] return event;
// [ts] }
// [ts] }
// [ts] if (handleEventFunc) {
// [ts] handleEventFunc(topLevelType, targetNode, targetInst);
// [ts] }
// When blurring, set the value attribute for number inputs
// [ts] if (topLevelType == "topBlur") {
// [ts] handleControlledInputBlur(targetInst, targetNode);
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ChangeEventPlugin;
// [ts] },{"114":114,"120":120,"122":122,"123":123,"138":138,"17":17,"20":20,"34":34,"82":82,"91":91}],8:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMLazyTree = _dereq_(9);
// [ts] let Danger = _dereq_(13);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let createMicrosoftUnsafeLocalFunction = _dereq_(105);
// [ts] let setInnerHTML = _dereq_(128);
// [ts] let setTextContent = _dereq_(129);
// [ts] fn getNodeAfter(parentNode: auto, node: auto) {
// Special case for text components, which return [open, close] comments
// from getHostNode.
// [ts] if (Array.isArray(node)) {
// [ts] node = node[1];
// [ts] }
// [ts] return if (node) node.nextSibling else parentNode.firstChild;
// [ts] }
// [ts] /**
// [ts] * Inserts `childNode` as a child of `parentNode` at the `index`.
// [ts] *
// [ts] * @param {DOMElement} parentNode Parent node in which to insert.
// [ts] * @param {DOMElement} childNode Child node to insert.
// [ts] * @param {number} index Index at which to insert the child.
// [ts] * @internal
// [ts] */
// [ts] let insertChildAt = createMicrosoftUnsafeLocalFunction((parentNode, childNode, referenceNode) => {
// We rely exclusively on `insertBefore(node, null)` instead of also using
// `appendChild(node)`. (Using `undefined` is not allowed by all browsers so
// we are careful to use `null`.)
// [ts] parentNode.insertBefore(childNode, referenceNode);
// [ts] });
// [ts] fn insertLazyTreeChildAt(parentNode: auto, childTree: auto, referenceNode: auto) {
// [ts] DOMLazyTree.insertTreeBefore(parentNode, childTree, referenceNode);
// [ts] }
// [ts] fn moveChild(parentNode: auto, childNode: auto, referenceNode: auto) {
// [ts] if (Array.isArray(childNode)) {
// [ts] moveDelimitedText(parentNode, childNode[0], childNode[1], referenceNode);
// [ts] } else {
// [ts] insertChildAt(parentNode, childNode, referenceNode);
// [ts] }
// [ts] }
// [ts] fn removeChild(parentNode: auto, childNode: auto) {
// [ts] if (Array.isArray(childNode)) {
// [ts] let closingComment = childNode[1];
// [ts] childNode = childNode[0];
// [ts] removeDelimitedText(parentNode, childNode, closingComment);
// [ts] parentNode.removeChild(closingComment);
// [ts] }
// [ts] parentNode.removeChild(childNode);
// [ts] }
// [ts] fn moveDelimitedText(parentNode: auto, openingComment: auto, closingComment: auto, referenceNode: auto) {
// [ts] let node = openingComment;
// [ts] while (true) {
// [ts] let nextNode = node.nextSibling;
// [ts] insertChildAt(parentNode, node, referenceNode);
// [ts] if (node == closingComment) {
// [ts] break;
// [ts] }
// [ts] node = nextNode;
// [ts] }
// [ts] }
// [ts] fn removeDelimitedText(parentNode: auto, startNode: auto, closingComment: auto) {
// [ts] while (true) {
// [ts] let node = startNode.nextSibling;
// [ts] if (node == closingComment) {
// The closing comment is removed by ReactMultiChild.
// [ts] break;
// [ts] } else {
// [ts] parentNode.removeChild(node);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn replaceDelimitedText(openingComment: auto, closingComment: auto, stringText: auto) {
// [ts] let parentNode = openingComment.parentNode;
// [ts] let nodeAfterComment = openingComment.nextSibling;
// [ts] if (nodeAfterComment == closingComment) {
// There are no text nodes between the opening and closing comments; insert
// a new one if stringText isn't empty.
// [ts] if (stringText) {
// [ts] insertChildAt(parentNode, document.createTextNode(stringText), nodeAfterComment);
// [ts] }
// [ts] } else {
// [ts] if (stringText) {
// Set the text content of the first node after the opening comment, and
// remove all following nodes up until the closing comment.
// [ts] setTextContent(nodeAfterComment, stringText);
// [ts] removeDelimitedText(parentNode, nodeAfterComment, closingComment);
// [ts] } else {
// [ts] removeDelimitedText(parentNode, openingComment, closingComment);
// [ts] }
// [ts] }
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: ReactDOMComponentTree.getInstanceFromNode(openingComment)._debugID,
// [ts] type: "replace text",
// [ts] payload: stringText
// [ts] });
// [ts] }
// [ts] }
// [ts] let dangerouslyReplaceNodeWithMarkup = Danger.dangerouslyReplaceNodeWithMarkup;
// [ts] if ("development" != "production") {
// [ts] dangerouslyReplaceNodeWithMarkup = (oldChild, markup, prevInstance) => {
// [ts] Danger.dangerouslyReplaceNodeWithMarkup(oldChild, markup);
// [ts] if (prevInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: prevInstance._debugID,
// [ts] type: "replace with",
// [ts] payload: markup.toString()
// [ts] });
// [ts] } else {
// [ts] let nextInstance = ReactDOMComponentTree.getInstanceFromNode(markup.node);
// [ts] if (nextInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: nextInstance._debugID,
// [ts] type: "mount",
// [ts] payload: markup.toString()
// [ts] });
// [ts] }
// [ts] }
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Operations for updating with DOM children.
// [ts] */
// [ts] let DOMChildrenOperations = {
// [ts] dangerouslyReplaceNodeWithMarkup: dangerouslyReplaceNodeWithMarkup,
// [ts] replaceDelimitedText: replaceDelimitedText,
// [ts] /**
// [ts] * Updates a component's children by processing a series of updates. The
// [ts] * update configurations are each expected to have a `parentNode` property.
// [ts] *
// [ts] * @param {array<object>} updates List of update configurations.
// [ts] * @internal
// [ts] */
// [ts] processUpdates: (parentNode, updates) => {
// [ts] if ("development" != "production") {
// [ts] let parentNodeDebugID = ReactDOMComponentTree.getInstanceFromNode(parentNode)._debugID;
// [ts] }
// [ts] let k = 0;
// [ts] while (k < updates.length) { // [cap-migrate] for-step: k++
// [ts] let update = updates[k];
// [ts] switch (update.type) {
// [ts] "INSERT_MARKUP" => {
// [ts] insertLazyTreeChildAt(parentNode, update.content, getNodeAfter(parentNode, update.afterNode));
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: parentNodeDebugID,
// [ts] type: "insert child",
// [ts] payload: {
// [ts] toIndex: update.toIndex,
// [ts] content: update.content.toString()
// [ts] }
// [ts] });
// [ts] }
// break;
// [ts] },
// [ts] "MOVE_EXISTING" => {
// [ts] moveChild(parentNode, update.fromNode, getNodeAfter(parentNode, update.afterNode));
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: parentNodeDebugID,
// [ts] type: "move child",
// [ts] payload: { fromIndex: update.fromIndex, toIndex: update.toIndex }
// [ts] });
// [ts] }
// break;
// [ts] },
// [ts] "SET_MARKUP" => {
// [ts] setInnerHTML(parentNode, update.content);
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: parentNodeDebugID,
// [ts] type: "replace children",
// [ts] payload: update.content.toString()
// [ts] });
// [ts] }
// break;
// [ts] },
// [ts] "TEXT_CONTENT" => {
// [ts] setTextContent(parentNode, update.content);
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: parentNodeDebugID,
// [ts] type: "replace text",
// [ts] payload: update.content.toString()
// [ts] });
// [ts] }
// break;
// [ts] },
// [ts] "REMOVE_NODE" => {
// [ts] removeChild(parentNode, update.fromNode);
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: parentNodeDebugID,
// [ts] type: "remove child",
// [ts] payload: { fromIndex: update.fromIndex }
// [ts] });
// [ts] }
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = DOMChildrenOperations;
// [ts] },{"105":105,"128":128,"129":129,"13":13,"34":34,"64":64,"9":9}],9:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2015-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMNamespaces = _dereq_(10);
// [ts] let setInnerHTML = _dereq_(128);
// [ts] let createMicrosoftUnsafeLocalFunction = _dereq_(105);
// [ts] let setTextContent = _dereq_(129);
// [ts] let ELEMENT_NODE_TYPE = 1;
// [ts] let DOCUMENT_FRAGMENT_NODE_TYPE = 11;
// [ts] /**
// [ts] * In IE (8-11) and Edge, appending nodes with no children is dramatically
// [ts] * faster than appending a full subtree, so we essentially queue up the
// [ts] * .appendChild calls here and apply them so each node is added to its parent
// [ts] * before any children are added.
// [ts] *
// [ts] * In other browsers, doing so is slower or neutral compared to the other order
// [ts] * (in Firefox, twice as slow) so we only do this inversion in IE.
// [ts] *
// [ts] * See https://github.com/spicyj/innerhtml-vs-createelement-vs-clonenode.
// [ts] */
// [ts] let enableLazy = @typeName(document) != "undefined" && @typeName(document.documentMode) == "number" || @typeName(navigator) != "undefined" && @typeName(navigator.userAgent) == "string" && "\\bEdge\\/\\d".test(navigator.userAgent);
// [ts] fn insertTreeChildren(tree: auto) {
// [ts] if (!enableLazy) {
// [ts] return;
// [ts] }
// [ts] let node = tree.node;
// [ts] let children = tree.children;
// [ts] if (children.length) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] insertTreeBefore(node, children[i], null);
// [ts] }
// [ts] } else if (tree.html != null) {
// [ts] setInnerHTML(node, tree.html);
// [ts] } else if (tree.text != null) {
// [ts] setTextContent(node, tree.text);
// [ts] }
// [ts] }
// [ts] let insertTreeBefore = createMicrosoftUnsafeLocalFunction((parentNode, tree, referenceNode) => {
// DocumentFragments aren't actually part of the DOM after insertion so
// appending children won't update the DOM. We need to ensure the fragment
// is properly populated first, breaking out of our lazy approach for just
// this level. Also, some <object> plugins (like Flash Player) will read
// <param> nodes immediately upon insertion into the DOM, so <object>
// must also be populated prior to insertion into the DOM.
// [ts] if (tree.node.nodeType == DOCUMENT_FRAGMENT_NODE_TYPE || tree.node.nodeType == ELEMENT_NODE_TYPE && tree.node.nodeName.toLowerCase() == "object" && (tree.node.namespaceURI == null || tree.node.namespaceURI == DOMNamespaces.html)) {
// [ts] insertTreeChildren(tree);
// [ts] parentNode.insertBefore(tree.node, referenceNode);
// [ts] } else {
// [ts] parentNode.insertBefore(tree.node, referenceNode);
// [ts] insertTreeChildren(tree);
// [ts] }
// [ts] });
// [ts] fn replaceChildWithTree(oldNode: auto, newTree: auto) {
// [ts] oldNode.parentNode.replaceChild(newTree.node, oldNode);
// [ts] insertTreeChildren(newTree);
// [ts] }
// [ts] fn queueChild(parentTree: auto, childTree: auto) {
// [ts] if (enableLazy) {
// [ts] parentTree.children.push(childTree);
// [ts] } else {
// [ts] parentTree.node.appendChild(childTree.node);
// [ts] }
// [ts] }
// [ts] fn queueHTML(tree: auto, html: auto) {
// [ts] if (enableLazy) {
// [ts] tree.html = html;
// [ts] } else {
// [ts] setInnerHTML(tree.node, html);
// [ts] }
// [ts] }
// [ts] fn queueText(tree: auto, text: auto) {
// [ts] if (enableLazy) {
// [ts] tree.text = text;
// [ts] } else {
// [ts] setTextContent(tree.node, text);
// [ts] }
// [ts] }
// [ts] fn toString() {
// [ts] return _.node.nodeName;
// [ts] }
// [ts] fn DOMLazyTree(node: auto) {
// [ts] return {
// [ts] node: node,
// [ts] children: [],
// [ts] html: null,
// [ts] text: null,
// [ts] toString: toString
// [ts] };
// [ts] }
// [ts] DOMLazyTree.insertTreeBefore = insertTreeBefore;
// [ts] DOMLazyTree.replaceChildWithTree = replaceChildWithTree;
// [ts] DOMLazyTree.queueChild = queueChild;
// [ts] DOMLazyTree.queueHTML = queueHTML;
// [ts] DOMLazyTree.queueText = queueText;
// [ts] module.exports = DOMLazyTree;
// [ts] },{"10":10,"105":105,"128":128,"129":129}],10:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMNamespaces = {
// [ts] html: "http://www.w3.org/1999/xhtml",
// [ts] mathml: "http://www.w3.org/1998/Math/MathML",
// [ts] svg: "http://www.w3.org/2000/svg"
// [ts] };
// [ts] module.exports = DOMNamespaces;
// [ts] },{}],11:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] fn checkMask(value: auto, bitmask: auto) {
// [ts] return (value & bitmask) == bitmask;
// [ts] }
// [ts] let DOMPropertyInjection = {
// [ts] /**
// [ts] * Mapping from normalized, camelcased property names to a configuration that
// [ts] * specifies how the associated DOM property should be accessed or rendered.
// [ts] */
// [ts] MUST_USE_PROPERTY: 0x1,
// [ts] HAS_BOOLEAN_VALUE: 0x4,
// [ts] HAS_NUMERIC_VALUE: 0x8,
// [ts] HAS_POSITIVE_NUMERIC_VALUE: 0x10 | 0x8,
// [ts] HAS_OVERLOADED_BOOLEAN_VALUE: 0x20,
// [ts] /**
// [ts] * Inject some specialized knowledge about the DOM. This takes a config object
// [ts] * with the following properties:
// [ts] *
// [ts] * isCustomAttribute: function that given an attribute name will return true
// [ts] * if it can be inserted into the DOM verbatim. Useful for data-* or aria-*
// [ts] * attributes where it's impossible to enumerate all of the possible
// [ts] * attribute names,
// [ts] *
// [ts] * Properties: object mapping DOM property name to one of the
// [ts] * DOMPropertyInjection constants or null. If your attribute isn't in here,
// [ts] * it won't get written to the DOM.
// [ts] *
// [ts] * DOMAttributeNames: object mapping React attribute name to the DOM
// [ts] * attribute name. Attribute names not specified use the **lowercase**
// [ts] * normalized name.
// [ts] *
// [ts] * DOMAttributeNamespaces: object mapping React attribute name to the DOM
// [ts] * attribute namespace URL. (Attribute names not specified use no namespace.)
// [ts] *
// [ts] * DOMPropertyNames: similar to DOMAttributeNames but for DOM properties.
// [ts] * Property names not specified use the normalized name.
// [ts] *
// [ts] * DOMMutationMethods: Properties that require special mutation methods. If
// [ts] * `value` is undefined, the mutation method should unset the property.
// [ts] *
// [ts] * @param {object} domPropertyConfig the config as described above.
// [ts] */
// [ts] injectDOMPropertyConfig: (domPropertyConfig) => {
// [ts] let Injection = DOMPropertyInjection;
// [ts] let Properties = domPropertyConfig.Properties || {};
// [ts] let DOMAttributeNamespaces = domPropertyConfig.DOMAttributeNamespaces || {};
// [ts] let DOMAttributeNames = domPropertyConfig.DOMAttributeNames || {};
// [ts] let DOMPropertyNames = domPropertyConfig.DOMPropertyNames || {};
// [ts] let DOMMutationMethods = domPropertyConfig.DOMMutationMethods || {};
// [ts] if (domPropertyConfig.isCustomAttribute) {
// [ts] DOMProperty._isCustomAttributeFunctions.push(domPropertyConfig.isCustomAttribute);
// [ts] }
// [ts] for (let propName in Properties) {
// [ts] if (!!DOMProperty.properties.hasOwnProperty(propName)) if ("development" != "production") invariant(false, "injectDOMPropertyConfig(...): You're trying to inject DOM property '%s' which has already been injected. You may be accidentally injecting the same DOM property config twice, or you may be injecting two configs that have conflicting property names.", propName) else _prodInvariant("48", propName) else void 0;
// [ts] let lowerCased = propName.toLowerCase();
// [ts] let propConfig = Properties[propName];
// [ts] let propertyInfo = {
// [ts] attributeName: lowerCased,
// [ts] attributeNamespace: null,
// [ts] propertyName: propName,
// [ts] mutationMethod: null,
// [ts] mustUseProperty: checkMask(propConfig, Injection.MUST_USE_PROPERTY),
// [ts] hasBooleanValue: checkMask(propConfig, Injection.HAS_BOOLEAN_VALUE),
// [ts] hasNumericValue: checkMask(propConfig, Injection.HAS_NUMERIC_VALUE),
// [ts] hasPositiveNumericValue: checkMask(propConfig, Injection.HAS_POSITIVE_NUMERIC_VALUE),
// [ts] hasOverloadedBooleanValue: checkMask(propConfig, Injection.HAS_OVERLOADED_BOOLEAN_VALUE)
// [ts] };
// [ts] if (!(propertyInfo.hasBooleanValue + propertyInfo.hasNumericValue + propertyInfo.hasOverloadedBooleanValue <= 1)) if ("development" != "production") invariant(false, "DOMProperty: Value can be one of boolean, overloaded boolean, or numeric value, but not a combination: %s", propName) else _prodInvariant("50", propName) else void 0;
// [ts] if ("development" != "production") {
// [ts] DOMProperty.getPossibleStandardName[lowerCased] = propName;
// [ts] }
// [ts] if (DOMAttributeNames.hasOwnProperty(propName)) {
// [ts] let attributeName = DOMAttributeNames[propName];
// [ts] propertyInfo.attributeName = attributeName;
// [ts] if ("development" != "production") {
// [ts] DOMProperty.getPossibleStandardName[attributeName] = propName;
// [ts] }
// [ts] }
// [ts] if (DOMAttributeNamespaces.hasOwnProperty(propName)) {
// [ts] propertyInfo.attributeNamespace = DOMAttributeNamespaces[propName];
// [ts] }
// [ts] if (DOMPropertyNames.hasOwnProperty(propName)) {
// [ts] propertyInfo.propertyName = DOMPropertyNames[propName];
// [ts] }
// [ts] if (DOMMutationMethods.hasOwnProperty(propName)) {
// [ts] propertyInfo.mutationMethod = DOMMutationMethods[propName];
// [ts] }
// [ts] DOMProperty.properties[propName] = propertyInfo;
// [ts] }
// [ts] }
// [ts] };
// [ts] /* eslint-disable max-len */
// [ts] let ATTRIBUTE_NAME_START_CHAR = ":A-Z_a-z\\u00C0-\\u00D6\\u00D8-\\u00F6\\u00F8-\\u02FF\\u0370-\\u037D\\u037F-\\u1FFF\\u200C-\\u200D\\u2070-\\u218F\\u2C00-\\u2FEF\\u3001-\\uD7FF\\uF900-\\uFDCF\\uFDF0-\\uFFFD";
// [ts] /* eslint-enable max-len */
// [ts] /**
// [ts] * DOMProperty exports lookup objects that can be used like functions:
// [ts] *
// [ts] *   > DOMProperty.isValid['id']
// [ts] *   true
// [ts] *   > DOMProperty.isValid['foobar']
// [ts] *   undefined
// [ts] *
// [ts] * Although this may be confusing, it performs better in general.
// [ts] *
// [ts] * @see http://jsperf.com/key-exists
// [ts] * @see http://jsperf.com/key-missing
// [ts] */
// [ts] let DOMProperty = {
// [ts] ID_ATTRIBUTE_NAME: "data-reactid",
// [ts] ROOT_ATTRIBUTE_NAME: "data-reactroot",
// [ts] ATTRIBUTE_NAME_START_CHAR: ATTRIBUTE_NAME_START_CHAR,
// [ts] ATTRIBUTE_NAME_CHAR: ATTRIBUTE_NAME_START_CHAR + "\\-.0-9\\u00B7\\u0300-\\u036F\\u203F-\\u2040",
// [ts] /**
// [ts] * Map from property "standard name" to an object with info about how to set
// [ts] * the property in the DOM. Each object contains:
// [ts] *
// [ts] * attributeName:
// [ts] *   Used when rendering markup or with `*Attribute()`.
// [ts] * attributeNamespace
// [ts] * propertyName:
// [ts] *   Used on DOM node instances. (This includes properties that mutate due to
// [ts] *   external factors.)
// [ts] * mutationMethod:
// [ts] *   If non-null, used instead of the property or `setAttribute()` after
// [ts] *   initial render.
// [ts] * mustUseProperty:
// [ts] *   Whether the property must be accessed and mutated as an object property.
// [ts] * hasBooleanValue:
// [ts] *   Whether the property should be removed when set to a falsey value.
// [ts] * hasNumericValue:
// [ts] *   Whether the property must be numeric or parse as a numeric and should be
// [ts] *   removed when set to a falsey value.
// [ts] * hasPositiveNumericValue:
// [ts] *   Whether the property must be positive numeric or parse as a positive
// [ts] *   numeric and should be removed when set to a falsey value.
// [ts] * hasOverloadedBooleanValue:
// [ts] *   Whether the property can be used as a flag as well as with a value.
// [ts] *   Removed when strictly equal to false; present without a value when
// [ts] *   strictly equal to true; present with a value otherwise.
// [ts] */
// [ts] properties: {},
// [ts] /**
// [ts] * Mapping from lowercase property names to the properly cased version, used
// [ts] * to warn in the case of missing properties. Available only in __DEV__.
// [ts] *
// [ts] * autofocus is predefined, because adding it to the property whitelist
// [ts] * causes unintended side effects.
// [ts] *
// [ts] * @type {Object}
// [ts] */
// [ts] getPossibleStandardName: if ("development" != "production") { autofocus: "autoFocus" } else null,
// [ts] /**
// [ts] * All of the isCustomAttribute() functions that have been injected.
// [ts] */
// [ts] _isCustomAttributeFunctions: [],
// [ts] /**
// [ts] * Checks whether a property name is a custom attribute.
// [ts] * @method
// [ts] */
// [ts] isCustomAttribute: (attributeName) => {
// [ts] let i = 0;
// [ts] while (i < DOMProperty._isCustomAttributeFunctions.length) { // [cap-migrate] for-step: i++
// [ts] let isCustomAttributeFn = DOMProperty._isCustomAttributeFunctions[i];
// [ts] if (isCustomAttributeFn(attributeName)) {
// [ts] return true;
// [ts] }
// [ts] }
// [ts] return false;
// [ts] },
// [ts] injection: DOMPropertyInjection
// [ts] };
// [ts] module.exports = DOMProperty;
// [ts] },{"126":126,"152":152}],12:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMProperty = _dereq_(11);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let quoteAttributeValueForBrowser = _dereq_(125);
// [ts] let warning = _dereq_(159);
// [ts] let VALID_ATTRIBUTE_NAME_REGEX = RegExp.init("^[" + DOMProperty.ATTRIBUTE_NAME_START_CHAR + "][" + DOMProperty.ATTRIBUTE_NAME_CHAR + "]*$");
// [ts] let illegalAttributeNameCache = {};
// [ts] let validatedAttributeNameCache = {};
// [ts] fn isAttributeNameSafe(attributeName: auto) {
// [ts] if (validatedAttributeNameCache.hasOwnProperty(attributeName)) {
// [ts] return true;
// [ts] }
// [ts] if (illegalAttributeNameCache.hasOwnProperty(attributeName)) {
// [ts] return false;
// [ts] }
// [ts] if (VALID_ATTRIBUTE_NAME_REGEX.test(attributeName)) {
// [ts] validatedAttributeNameCache[attributeName] = true;
// [ts] return true;
// [ts] }
// [ts] illegalAttributeNameCache[attributeName] = true;
// [ts] if ("development" != "production") warning(false, "Invalid attribute name: `%s`", attributeName) else void 0;
// [ts] return false;
// [ts] }
// [ts] fn shouldIgnoreValue(propertyInfo: auto, value: auto) {
// [ts] return value == null || propertyInfo.hasBooleanValue && !value || propertyInfo.hasNumericValue && isNaN(value) || propertyInfo.hasPositiveNumericValue && value < 1 || propertyInfo.hasOverloadedBooleanValue && value == false;
// [ts] }
// [ts] /**
// [ts] * Operations for dealing with DOM properties.
// [ts] */
// [ts] let DOMPropertyOperations = {
// [ts] /**
// [ts] * Creates markup for the ID property.
// [ts] *
// [ts] * @param {string} id Unescaped ID.
// [ts] * @return {string} Markup string.
// [ts] */
// [ts] createMarkupForID: (id) => {
// [ts] return DOMProperty.ID_ATTRIBUTE_NAME + "=" + quoteAttributeValueForBrowser(id);
// [ts] },
// [ts] setAttributeForID: (node, id) => {
// [ts] node.setAttribute(DOMProperty.ID_ATTRIBUTE_NAME, id);
// [ts] },
// [ts] createMarkupForRoot: () => {
// [ts] return DOMProperty.ROOT_ATTRIBUTE_NAME + "=\"\"";
// [ts] },
// [ts] setAttributeForRoot: (node) => {
// [ts] node.setAttribute(DOMProperty.ROOT_ATTRIBUTE_NAME, "");
// [ts] },
// [ts] /**
// [ts] * Creates markup for a property.
// [ts] *
// [ts] * @param {string} name
// [ts] * @param {*} value
// [ts] * @return {?string} Markup string, or null if the property was invalid.
// [ts] */
// [ts] createMarkupForProperty: (name, value) => {
// [ts] let propertyInfo = if (DOMProperty.properties.hasOwnProperty(name)) DOMProperty.properties[name] else null;
// [ts] if (propertyInfo) {
// [ts] if (shouldIgnoreValue(propertyInfo, value)) {
// [ts] return "";
// [ts] }
// [ts] let attributeName = propertyInfo.attributeName;
// [ts] if (propertyInfo.hasBooleanValue || propertyInfo.hasOverloadedBooleanValue && value == true) {
// [ts] return attributeName + "=\"\"";
// [ts] }
// [ts] return attributeName + "=" + quoteAttributeValueForBrowser(value);
// [ts] } else if (DOMProperty.isCustomAttribute(name)) {
// [ts] if (value == null) {
// [ts] return "";
// [ts] }
// [ts] return name + "=" + quoteAttributeValueForBrowser(value);
// [ts] }
// [ts] return null;
// [ts] },
// [ts] /**
// [ts] * Creates markup for a custom property.
// [ts] *
// [ts] * @param {string} name
// [ts] * @param {*} value
// [ts] * @return {string} Markup string, or empty string if the property was invalid.
// [ts] */
// [ts] createMarkupForCustomAttribute: (name, value) => {
// [ts] if (!isAttributeNameSafe(name) || value == null) {
// [ts] return "";
// [ts] }
// [ts] return name + "=" + quoteAttributeValueForBrowser(value);
// [ts] },
// [ts] /**
// [ts] * Sets the value for a property on a node.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {string} name
// [ts] * @param {*} value
// [ts] */
// [ts] setValueForProperty: (node, name, value) => {
// [ts] let propertyInfo = if (DOMProperty.properties.hasOwnProperty(name)) DOMProperty.properties[name] else null;
// [ts] if (propertyInfo) {
// [ts] let mutationMethod = propertyInfo.mutationMethod;
// [ts] if (mutationMethod) {
// [ts] mutationMethod(node, value);
// [ts] } else if (shouldIgnoreValue(propertyInfo, value)) {
// [ts] _.deleteValueForProperty(node, name);
// [ts] return;
// [ts] } else if (propertyInfo.mustUseProperty) {
// Contrary to `setAttribute`, object properties are properly
// `toString`ed by IE8/9.
// [ts] node[propertyInfo.propertyName] = value;
// [ts] } else {
// [ts] let attributeName = propertyInfo.attributeName;
// [ts] let namespace = propertyInfo.attributeNamespace;
// `setAttribute` with objects becomes only `[object]` in IE8/9,
// ('' + value) makes it output the correct toString()-value.
// [ts] if (namespace) {
// [ts] node.setAttributeNS(namespace, attributeName, "" + value);
// [ts] } else if (propertyInfo.hasBooleanValue || propertyInfo.hasOverloadedBooleanValue && value == true) {
// [ts] node.setAttribute(attributeName, "");
// [ts] } else {
// [ts] node.setAttribute(attributeName, "" + value);
// [ts] }
// [ts] }
// [ts] } else if (DOMProperty.isCustomAttribute(name)) {
// [ts] DOMPropertyOperations.setValueForAttribute(node, name, value);
// [ts] return;
// [ts] }
// [ts] if ("development" != "production") {
// [ts] let payload = {};
// [ts] payload[name] = value;
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: ReactDOMComponentTree.getInstanceFromNode(node)._debugID,
// [ts] type: "update attribute",
// [ts] payload: payload
// [ts] });
// [ts] }
// [ts] },
// [ts] setValueForAttribute: (node, name, value) => {
// [ts] if (!isAttributeNameSafe(name)) {
// [ts] return;
// [ts] }
// [ts] if (value == null) {
// [ts] node.removeAttribute(name);
// [ts] } else {
// [ts] node.setAttribute(name, "" + value);
// [ts] }
// [ts] if ("development" != "production") {
// [ts] let payload = {};
// [ts] payload[name] = value;
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: ReactDOMComponentTree.getInstanceFromNode(node)._debugID,
// [ts] type: "update attribute",
// [ts] payload: payload
// [ts] });
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Deletes an attributes from a node.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {string} name
// [ts] */
// [ts] deleteValueForAttribute: (node, name) => {
// [ts] node.removeAttribute(name);
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: ReactDOMComponentTree.getInstanceFromNode(node)._debugID,
// [ts] type: "remove attribute",
// [ts] payload: name
// [ts] });
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Deletes the value for a property on a node.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {string} name
// [ts] */
// [ts] deleteValueForProperty: (node, name) => {
// [ts] let propertyInfo = if (DOMProperty.properties.hasOwnProperty(name)) DOMProperty.properties[name] else null;
// [ts] if (propertyInfo) {
// [ts] let mutationMethod = propertyInfo.mutationMethod;
// [ts] if (mutationMethod) {
// [ts] mutationMethod(node, null);
// [ts] } else if (propertyInfo.mustUseProperty) {
// [ts] let propName = propertyInfo.propertyName;
// [ts] if (propertyInfo.hasBooleanValue) {
// [ts] node[propName] = false;
// [ts] } else {
// [ts] node[propName] = "";
// [ts] }
// [ts] } else {
// [ts] node.removeAttribute(propertyInfo.attributeName);
// [ts] }
// [ts] } else if (DOMProperty.isCustomAttribute(name)) {
// [ts] node.removeAttribute(name);
// [ts] }
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: ReactDOMComponentTree.getInstanceFromNode(node)._debugID,
// [ts] type: "remove attribute",
// [ts] payload: name
// [ts] });
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = DOMPropertyOperations;
// [ts] },{"11":11,"125":125,"159":159,"34":34,"64":64}],13:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let DOMLazyTree = _dereq_(9);
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let createNodesFromMarkup = _dereq_(143);
// [ts] let emptyFunction = _dereq_(144);
// [ts] let invariant = _dereq_(152);
// [ts] let Danger = {
// [ts] /**
// [ts] * Replaces a node with a string of markup at its current position within its
// [ts] * parent. The markup must render into a single root node.
// [ts] *
// [ts] * @param {DOMElement} oldChild Child node to replace.
// [ts] * @param {string} markup Markup to render in place of the child node.
// [ts] * @internal
// [ts] */
// [ts] dangerouslyReplaceNodeWithMarkup: (oldChild, markup) => {
// [ts] if (!ExecutionEnvironment.canUseDOM) if ("development" != "production") invariant(false, "dangerouslyReplaceNodeWithMarkup(...): Cannot render markup in a worker thread. Make sure `window` and `document` are available globally before requiring React when unit testing or use ReactDOMServer.renderToString() for server rendering.") else _prodInvariant("56") else void 0;
// [ts] if (!markup) if ("development" != "production") invariant(false, "dangerouslyReplaceNodeWithMarkup(...): Missing markup.") else _prodInvariant("57") else void 0;
// [ts] if (!(oldChild.nodeName != "HTML")) if ("development" != "production") invariant(false, "dangerouslyReplaceNodeWithMarkup(...): Cannot replace markup of the <html> node. This is because browser quirks make this unreliable and/or slow. If you want to render to the root you must use server rendering. See ReactDOMServer.renderToString().") else _prodInvariant("58") else void 0;
// [ts] if (@typeName(markup) == "string") {
// [ts] let newChild = createNodesFromMarkup(markup, emptyFunction)[0];
// [ts] oldChild.parentNode.replaceChild(newChild, oldChild);
// [ts] } else {
// [ts] DOMLazyTree.replaceChildWithTree(oldChild, markup);
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = Danger;
// [ts] },{"126":126,"138":138,"143":143,"144":144,"152":152,"9":9}],14:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Module that is injectable into `EventPluginHub`, that specifies a
// [ts] * deterministic ordering of `EventPlugin`s. A convenient way to reason about
// [ts] * plugins, without having to package every one of them. This is better than
// [ts] * having plugins be ordered in the same order that they are injected because
// [ts] * that ordering would be influenced by the packaging order.
// [ts] * `ResponderEventPlugin` must occur before `SimpleEventPlugin` so that
// [ts] * preventing default on events is convenient in `SimpleEventPlugin` handlers.
// [ts] */
// [ts] let DefaultEventPluginOrder = ["ResponderEventPlugin", "SimpleEventPlugin", "TapEventPlugin", "EnterLeaveEventPlugin", "ChangeEventPlugin", "SelectEventPlugin", "BeforeInputEventPlugin"];
// [ts] module.exports = DefaultEventPluginOrder;
// [ts] },{}],15:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let EventPropagators = _dereq_(20);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let SyntheticMouseEvent = _dereq_(95);
// [ts] let eventTypes = {
// [ts] mouseEnter: {
// [ts] registrationName: "onMouseEnter",
// [ts] dependencies: ["topMouseOut", "topMouseOver"]
// [ts] },
// [ts] mouseLeave: {
// [ts] registrationName: "onMouseLeave",
// [ts] dependencies: ["topMouseOut", "topMouseOver"]
// [ts] }
// [ts] };
// [ts] let EnterLeaveEventPlugin = {
// [ts] eventTypes: eventTypes,
// [ts] /**
// [ts] * For almost every interaction we care about, there will be both a top-level
// [ts] * `mouseover` and `mouseout` event that occurs. Only use `mouseout` so that
// [ts] * we do not extract duplicate events. However, moving the mouse into the
// [ts] * browser from outside will not fire a `mouseout` event. In this case, we use
// [ts] * the `mouseover` top-level event.
// [ts] */
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget) => {
// [ts] if (topLevelType == "topMouseOver" && (nativeEvent.relatedTarget || nativeEvent.fromElement)) {
// [ts] return null;
// [ts] }
// [ts] if (topLevelType != "topMouseOut" && topLevelType != "topMouseOver") {
// Must not be a mouse in or mouse out - ignoring.
// [ts] return null;
// [ts] }
// [ts] let win;
// [ts] if (nativeEventTarget.window == nativeEventTarget) {
// `nativeEventTarget` is probably a window object.
// [ts] win = nativeEventTarget;
// [ts] } else {
// TODO: Figure out why `ownerDocument` is sometimes undefined in IE8.
// [ts] let doc = nativeEventTarget.ownerDocument;
// [ts] if (doc) {
// [ts] win = doc.defaultView || doc.parentWindow;
// [ts] } else {
// [ts] win = window;
// [ts] }
// [ts] }
// [ts] let from;
// [ts] let to;
// [ts] if (topLevelType == "topMouseOut") {
// [ts] from = targetInst;
// [ts] let related = nativeEvent.relatedTarget || nativeEvent.toElement;
// [ts] to = if (related) ReactDOMComponentTree.getClosestInstanceFromNode(related) else null;
// [ts] } else {
// Moving to a node from outside the window.
// [ts] from = null;
// [ts] to = targetInst;
// [ts] }
// [ts] if (from == to) {
// Nothing pertains to our managed components.
// [ts] return null;
// [ts] }
// [ts] let fromNode = if (from == null) win else ReactDOMComponentTree.getNodeFromInstance(from);
// [ts] let toNode = if (to == null) win else ReactDOMComponentTree.getNodeFromInstance(to);
// [ts] let leave = SyntheticMouseEvent.getPooled(eventTypes.mouseLeave, from, nativeEvent, nativeEventTarget);
// [ts] leave.type = "mouseleave";
// [ts] leave.target = fromNode;
// [ts] leave.relatedTarget = toNode;
// [ts] let enter = SyntheticMouseEvent.getPooled(eventTypes.mouseEnter, to, nativeEvent, nativeEventTarget);
// [ts] enter.type = "mouseenter";
// [ts] enter.target = toNode;
// [ts] enter.relatedTarget = fromNode;
// [ts] EventPropagators.accumulateEnterLeaveDispatches(leave, enter, from, to);
// [ts] return [leave, enter];
// [ts] }
// [ts] };
// [ts] module.exports = EnterLeaveEventPlugin;
// [ts] },{"20":20,"34":34,"95":95}],16:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Types of raw signals from the browser caught at the top level.
// [ts] */
// [ts] let topLevelTypes = {
// [ts] topAbort: null,
// [ts] topAnimationEnd: null,
// [ts] topAnimationIteration: null,
// [ts] topAnimationStart: null,
// [ts] topBlur: null,
// [ts] topCanPlay: null,
// [ts] topCanPlayThrough: null,
// [ts] topChange: null,
// [ts] topClick: null,
// [ts] topCompositionEnd: null,
// [ts] topCompositionStart: null,
// [ts] topCompositionUpdate: null,
// [ts] topContextMenu: null,
// [ts] topCopy: null,
// [ts] topCut: null,
// [ts] topDoubleClick: null,
// [ts] topDrag: null,
// [ts] topDragEnd: null,
// [ts] topDragEnter: null,
// [ts] topDragExit: null,
// [ts] topDragLeave: null,
// [ts] topDragOver: null,
// [ts] topDragStart: null,
// [ts] topDrop: null,
// [ts] topDurationChange: null,
// [ts] topEmptied: null,
// [ts] topEncrypted: null,
// [ts] topEnded: null,
// [ts] topError: null,
// [ts] topFocus: null,
// [ts] topInput: null,
// [ts] topInvalid: null,
// [ts] topKeyDown: null,
// [ts] topKeyPress: null,
// [ts] topKeyUp: null,
// [ts] topLoad: null,
// [ts] topLoadedData: null,
// [ts] topLoadedMetadata: null,
// [ts] topLoadStart: null,
// [ts] topMouseDown: null,
// [ts] topMouseMove: null,
// [ts] topMouseOut: null,
// [ts] topMouseOver: null,
// [ts] topMouseUp: null,
// [ts] topPaste: null,
// [ts] topPause: null,
// [ts] topPlay: null,
// [ts] topPlaying: null,
// [ts] topProgress: null,
// [ts] topRateChange: null,
// [ts] topReset: null,
// [ts] topScroll: null,
// [ts] topSeeked: null,
// [ts] topSeeking: null,
// [ts] topSelectionChange: null,
// [ts] topStalled: null,
// [ts] topSubmit: null,
// [ts] topSuspend: null,
// [ts] topTextInput: null,
// [ts] topTimeUpdate: null,
// [ts] topTouchCancel: null,
// [ts] topTouchEnd: null,
// [ts] topTouchMove: null,
// [ts] topTouchStart: null,
// [ts] topTransitionEnd: null,
// [ts] topVolumeChange: null,
// [ts] topWaiting: null,
// [ts] topWheel: null
// [ts] };
// [ts] let EventConstants = {
// [ts] topLevelTypes: topLevelTypes
// [ts] };
// [ts] module.exports = EventConstants;
// [ts] },{}],17:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let EventPluginRegistry = _dereq_(18);
// [ts] let EventPluginUtils = _dereq_(19);
// [ts] let ReactErrorUtils = _dereq_(55);
// [ts] let accumulateInto = _dereq_(102);
// [ts] let forEachAccumulated = _dereq_(110);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Internal store for event listeners
// [ts] */
// [ts] let listenerBank = {};
// [ts] /**
// [ts] * Internal queue of events that have accumulated their dispatches and are
// [ts] * waiting to have their dispatches executed.
// [ts] */
// [ts] let eventQueue = null;
// [ts] /**
// [ts] * Dispatches an event and releases it back into the pool, unless persistent.
// [ts] *
// [ts] * @param {?object} event Synthetic event to be dispatched.
// [ts] * @param {boolean} simulated If the event is simulated (changes exn behavior)
// [ts] * @private
// [ts] */
// [ts] let executeDispatchesAndRelease = (event, simulated) => {
// [ts] if (event) {
// [ts] EventPluginUtils.executeDispatchesInOrder(event, simulated);
// [ts] if (!event.isPersistent()) {
// [ts] event.constructor.release(event);
// [ts] }
// [ts] }
// [ts] };
// [ts] let executeDispatchesAndReleaseSimulated = (e) => {
// [ts] return executeDispatchesAndRelease(e, true);
// [ts] };
// [ts] let executeDispatchesAndReleaseTopLevel = (e) => {
// [ts] return executeDispatchesAndRelease(e, false);
// [ts] };
// [ts] let getDictionaryKey = (inst) => {
// Prevents V8 performance issue:
// https://github.com/facebook/react/pull/7232
// [ts] return "." + inst._rootNodeID;
// [ts] };
// [ts] fn isInteractive(tag: auto) {
// [ts] return tag == "button" || tag == "input" || tag == "select" || tag == "textarea";
// [ts] }
// [ts] fn shouldPreventMouseEvent(name: auto, type: auto, props: auto) {
// [ts] switch (name) {
// [ts] "onClick", "onClickCapture", "onDoubleClick", "onDoubleClickCapture", "onMouseDown", "onMouseDownCapture", "onMouseMove", "onMouseMoveCapture", "onMouseUp", "onMouseUpCapture" => {
// [ts] return !!(props.disabled && isInteractive(type));
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * This is a unified interface for event plugins to be installed and configured.
// [ts] *
// [ts] * Event plugins can implement the following properties:
// [ts] *
// [ts] *   `extractEvents` {function(string, DOMEventTarget, string, object): *}
// [ts] *     Required. When a top-level event is fired, this method is expected to
// [ts] *     extract synthetic events that will in turn be queued and dispatched.
// [ts] *
// [ts] *   `eventTypes` {object}
// [ts] *     Optional, plugins that fire events must publish a mapping of registration
// [ts] *     names that are used to register listeners. Values of this mapping must
// [ts] *     be objects that contain `registrationName` or `phasedRegistrationNames`.
// [ts] *
// [ts] *   `executeDispatch` {function(object, function, string)}
// [ts] *     Optional, allows plugins to override how an event gets dispatched. By
// [ts] *     default, the listener is simply invoked.
// [ts] *
// [ts] * Each plugin that is injected into `EventsPluginHub` is immediately operable.
// [ts] *
// [ts] * @public
// [ts] */
// [ts] let EventPluginHub = {
// [ts] /**
// [ts] * Methods for injecting dependencies.
// [ts] */
// [ts] injection: {
// [ts] /**
// [ts] * @param {array} InjectedEventPluginOrder
// [ts] * @public
// [ts] */
// [ts] injectEventPluginOrder: EventPluginRegistry.injectEventPluginOrder,
// [ts] /**
// [ts] * @param {object} injectedNamesToPlugins Map from names to plugin modules.
// [ts] */
// [ts] injectEventPluginsByName: EventPluginRegistry.injectEventPluginsByName
// [ts] },
// [ts] /**
// [ts] * Stores `listener` at `listenerBank[registrationName][key]`. Is idempotent.
// [ts] *
// [ts] * @param {object} inst The instance, which is the source of events.
// [ts] * @param {string} registrationName Name of listener (e.g. `onClick`).
// [ts] * @param {function} listener The callback to store.
// [ts] */
// [ts] putListener: (inst, registrationName, listener) => {
// [ts] if (!(@typeName(listener) == "function")) if ("development" != "production") invariant(false, "Expected %s listener to be a function, instead got type %s", registrationName, @typeName(listener)) else _prodInvariant("94", registrationName, @typeName(listener)) else void 0;
// [ts] let key = getDictionaryKey(inst);
// [ts] let bankForRegistrationName = listenerBank[registrationName] || (listenerBank[registrationName] = {});
// [ts] bankForRegistrationName[key] = listener;
// [ts] let PluginModule = EventPluginRegistry.registrationNameModules[registrationName];
// [ts] if (PluginModule && PluginModule.didPutListener) {
// [ts] PluginModule.didPutListener(inst, registrationName, listener);
// [ts] }
// [ts] },
// [ts] /**
// [ts] * @param {object} inst The instance, which is the source of events.
// [ts] * @param {string} registrationName Name of listener (e.g. `onClick`).
// [ts] * @return {?function} The stored callback.
// [ts] */
// [ts] getListener: (inst, registrationName) => {
// TODO: shouldPreventMouseEvent is DOM-specific and definitely should not
// live here; needs to be moved to a better place soon
// [ts] let bankForRegistrationName = listenerBank[registrationName];
// [ts] if (shouldPreventMouseEvent(registrationName, inst._currentElement.type, inst._currentElement.props)) {
// [ts] return null;
// [ts] }
// [ts] let key = getDictionaryKey(inst);
// [ts] return bankForRegistrationName && bankForRegistrationName[key];
// [ts] },
// [ts] /**
// [ts] * Deletes a listener from the registration bank.
// [ts] *
// [ts] * @param {object} inst The instance, which is the source of events.
// [ts] * @param {string} registrationName Name of listener (e.g. `onClick`).
// [ts] */
// [ts] deleteListener: (inst, registrationName) => {
// [ts] let PluginModule = EventPluginRegistry.registrationNameModules[registrationName];
// [ts] if (PluginModule && PluginModule.willDeleteListener) {
// [ts] PluginModule.willDeleteListener(inst, registrationName);
// [ts] }
// [ts] let bankForRegistrationName = listenerBank[registrationName];
// TODO: This should never be null -- when is it?
// [ts] if (bankForRegistrationName) {
// [ts] let key = getDictionaryKey(inst);
// [ts] delete bankForRegistrationName[key];
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Deletes all listeners for the DOM element with the supplied ID.
// [ts] *
// [ts] * @param {object} inst The instance, which is the source of events.
// [ts] */
// [ts] deleteAllListeners: (inst) => {
// [ts] let key = getDictionaryKey(inst);
// [ts] for (let registrationName in listenerBank) {
// [ts] if (!listenerBank.hasOwnProperty(registrationName)) {
// [ts] continue;
// [ts] }
// [ts] if (!listenerBank[registrationName][key]) {
// [ts] continue;
// [ts] }
// [ts] let PluginModule = EventPluginRegistry.registrationNameModules[registrationName];
// [ts] if (PluginModule && PluginModule.willDeleteListener) {
// [ts] PluginModule.willDeleteListener(inst, registrationName);
// [ts] }
// [ts] delete listenerBank[registrationName][key];
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Allows registered plugins an opportunity to extract events from top-level
// [ts] * native browser events.
// [ts] *
// [ts] * @return {*} An accumulation of synthetic events.
// [ts] * @internal
// [ts] */
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget) => {
// [ts] let events;
// [ts] let plugins = EventPluginRegistry.plugins;
// [ts] let i = 0;
// [ts] while (i < plugins.length) { // [cap-migrate] for-step: i++
// Not every plugin in the ordering may be loaded at runtime.
// [ts] let possiblePlugin = plugins[i];
// [ts] if (possiblePlugin) {
// [ts] let extractedEvents = possiblePlugin.extractEvents(topLevelType, targetInst, nativeEvent, nativeEventTarget);
// [ts] if (extractedEvents) {
// [ts] events = accumulateInto(events, extractedEvents);
// [ts] }
// [ts] }
// [ts] }
// [ts] return events;
// [ts] },
// [ts] /**
// [ts] * Enqueues a synthetic event that should be dispatched when
// [ts] * `processEventQueue` is invoked.
// [ts] *
// [ts] * @param {*} events An accumulation of synthetic events.
// [ts] * @internal
// [ts] */
// [ts] enqueueEvents: (events) => {
// [ts] if (events) {
// [ts] eventQueue = accumulateInto(eventQueue, events);
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Dispatches all synthetic events on the event queue.
// [ts] *
// [ts] * @internal
// [ts] */
// [ts] processEventQueue: (simulated) => {
// Set `eventQueue` to null before processing it so that we can tell if more
// events get enqueued while processing.
// [ts] let processingEventQueue = eventQueue;
// [ts] eventQueue = null;
// [ts] if (simulated) {
// [ts] forEachAccumulated(processingEventQueue, executeDispatchesAndReleaseSimulated);
// [ts] } else {
// [ts] forEachAccumulated(processingEventQueue, executeDispatchesAndReleaseTopLevel);
// [ts] }
// [ts] if (!!eventQueue) if ("development" != "production") invariant(false, "processEventQueue(): Additional events were enqueued while processing an event queue. Support for this has not yet been implemented.") else _prodInvariant("95") else void 0;
// This would be a good time to rethrow if any of the event handlers threw.
// [ts] ReactErrorUtils.rethrowCaughtError();
// [ts] },
// [ts] /**
// [ts] * These are needed for tests only. Do not use!
// [ts] */
// [ts] __purge: () => {
// [ts] listenerBank = {};
// [ts] },
// [ts] __getListenerBank: () => {
// [ts] return listenerBank;
// [ts] }
// [ts] };
// [ts] module.exports = EventPluginHub;
// [ts] },{"102":102,"110":110,"126":126,"152":152,"18":18,"19":19,"55":55}],18:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Injectable ordering of event plugins.
// [ts] */
// [ts] let eventPluginOrder = null;
// [ts] /**
// [ts] * Injectable mapping from names to event plugin modules.
// [ts] */
// [ts] let namesToPlugins = {};
// [ts] /**
// [ts] * Recomputes the plugin list using the injected plugins and plugin ordering.
// [ts] *
// [ts] * @private
// [ts] */
// [ts] fn recomputePluginOrdering() {
// [ts] if (!eventPluginOrder) {
// Wait until an `eventPluginOrder` is injected.
// [ts] return;
// [ts] }
// [ts] for (let pluginName in namesToPlugins) {
// [ts] let pluginModule = namesToPlugins[pluginName];
// [ts] let pluginIndex = eventPluginOrder.indexOf(pluginName);
// [ts] if (!(pluginIndex > -1)) if ("development" != "production") invariant(false, "EventPluginRegistry: Cannot inject event plugins that do not exist in the plugin ordering, `%s`.", pluginName) else _prodInvariant("96", pluginName) else void 0;
// [ts] if (EventPluginRegistry.plugins[pluginIndex]) {
// [ts] continue;
// [ts] }
// [ts] if (!pluginModule.extractEvents) if ("development" != "production") invariant(false, "EventPluginRegistry: Event plugins must implement an `extractEvents` method, but `%s` does not.", pluginName) else _prodInvariant("97", pluginName) else void 0;
// [ts] EventPluginRegistry.plugins[pluginIndex] = pluginModule;
// [ts] let publishedEvents = pluginModule.eventTypes;
// [ts] for (let eventName in publishedEvents) {
// [ts] if (!publishEventForPlugin(publishedEvents[eventName], pluginModule, eventName)) if ("development" != "production") invariant(false, "EventPluginRegistry: Failed to publish event `%s` for plugin `%s`.", eventName, pluginName) else _prodInvariant("98", eventName, pluginName) else void 0;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Publishes an event so that it can be dispatched by the supplied plugin.
// [ts] *
// [ts] * @param {object} dispatchConfig Dispatch configuration for the event.
// [ts] * @param {object} PluginModule Plugin publishing the event.
// [ts] * @return {boolean} True if the event was successfully published.
// [ts] * @private
// [ts] */
// [ts] fn publishEventForPlugin(dispatchConfig: auto, pluginModule: auto, eventName: auto) {
// [ts] if (!!EventPluginRegistry.eventNameDispatchConfigs.hasOwnProperty(eventName)) if ("development" != "production") invariant(false, "EventPluginHub: More than one plugin attempted to publish the same event name, `%s`.", eventName) else _prodInvariant("99", eventName) else void 0;
// [ts] EventPluginRegistry.eventNameDispatchConfigs[eventName] = dispatchConfig;
// [ts] let phasedRegistrationNames = dispatchConfig.phasedRegistrationNames;
// [ts] if (phasedRegistrationNames) {
// [ts] for (let phaseName in phasedRegistrationNames) {
// [ts] if (phasedRegistrationNames.hasOwnProperty(phaseName)) {
// [ts] let phasedRegistrationName = phasedRegistrationNames[phaseName];
// [ts] publishRegistrationName(phasedRegistrationName, pluginModule, eventName);
// [ts] }
// [ts] }
// [ts] return true;
// [ts] } else if (dispatchConfig.registrationName) {
// [ts] publishRegistrationName(dispatchConfig.registrationName, pluginModule, eventName);
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] /**
// [ts] * Publishes a registration name that is used to identify dispatched events and
// [ts] * can be used with `EventPluginHub.putListener` to register listeners.
// [ts] *
// [ts] * @param {string} registrationName Registration name to add.
// [ts] * @param {object} PluginModule Plugin publishing the event.
// [ts] * @private
// [ts] */
// [ts] fn publishRegistrationName(registrationName: auto, pluginModule: auto, eventName: auto) {
// [ts] if (!!EventPluginRegistry.registrationNameModules[registrationName]) if ("development" != "production") invariant(false, "EventPluginHub: More than one plugin attempted to publish the same registration name, `%s`.", registrationName) else _prodInvariant("100", registrationName) else void 0;
// [ts] EventPluginRegistry.registrationNameModules[registrationName] = pluginModule;
// [ts] EventPluginRegistry.registrationNameDependencies[registrationName] = pluginModule.eventTypes[eventName].dependencies;
// [ts] if ("development" != "production") {
// [ts] let lowerCasedName = registrationName.toLowerCase();
// [ts] EventPluginRegistry.possibleRegistrationNames[lowerCasedName] = registrationName;
// [ts] if (registrationName == "onDoubleClick") {
// [ts] EventPluginRegistry.possibleRegistrationNames.ondblclick = registrationName;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Registers plugins so that they can extract and dispatch events.
// [ts] *
// [ts] * @see {EventPluginHub}
// [ts] */
// [ts] let EventPluginRegistry = {
// [ts] /**
// [ts] * Ordered list of injected plugins.
// [ts] */
// [ts] plugins: [],
// [ts] /**
// [ts] * Mapping from event name to dispatch config
// [ts] */
// [ts] eventNameDispatchConfigs: {},
// [ts] /**
// [ts] * Mapping from registration name to plugin module
// [ts] */
// [ts] registrationNameModules: {},
// [ts] /**
// [ts] * Mapping from registration name to event name
// [ts] */
// [ts] registrationNameDependencies: {},
// [ts] /**
// [ts] * Mapping from lowercase registration names to the properly cased version,
// [ts] * used to warn in the case of missing event handlers. Available
// [ts] * only in __DEV__.
// [ts] * @type {Object}
// [ts] */
// [ts] possibleRegistrationNames: if ("development" != "production") {} else null,
// Trust the developer to only use possibleRegistrationNames in __DEV__
// [ts] /**
// [ts] * Injects an ordering of plugins (by plugin name). This allows the ordering
// [ts] * to be decoupled from injection of the actual plugins so that ordering is
// [ts] * always deterministic regardless of packaging, on-the-fly injection, etc.
// [ts] *
// [ts] * @param {array} InjectedEventPluginOrder
// [ts] * @internal
// [ts] * @see {EventPluginHub.injection.injectEventPluginOrder}
// [ts] */
// [ts] injectEventPluginOrder: (injectedEventPluginOrder) => {
// [ts] if (!!eventPluginOrder) if ("development" != "production") invariant(false, "EventPluginRegistry: Cannot inject event plugin ordering more than once. You are likely trying to load more than one copy of React.") else _prodInvariant("101") else void 0;
// Clone the ordering so it cannot be dynamically mutated.
// [ts] eventPluginOrder = Array.prototype.slice.call(injectedEventPluginOrder);
// [ts] recomputePluginOrdering();
// [ts] },
// [ts] /**
// [ts] * Injects plugins to be used by `EventPluginHub`. The plugin names must be
// [ts] * in the ordering injected by `injectEventPluginOrder`.
// [ts] *
// [ts] * Plugins can be injected as part of page initialization or on-the-fly.
// [ts] *
// [ts] * @param {object} injectedNamesToPlugins Map from names to plugin modules.
// [ts] * @internal
// [ts] * @see {EventPluginHub.injection.injectEventPluginsByName}
// [ts] */
// [ts] injectEventPluginsByName: (injectedNamesToPlugins) => {
// [ts] let isOrderingDirty = false;
// [ts] for (let pluginName in injectedNamesToPlugins) {
// [ts] if (!injectedNamesToPlugins.hasOwnProperty(pluginName)) {
// [ts] continue;
// [ts] }
// [ts] let pluginModule = injectedNamesToPlugins[pluginName];
// [ts] if (!namesToPlugins.hasOwnProperty(pluginName) || namesToPlugins[pluginName] != pluginModule) {
// [ts] if (!!namesToPlugins[pluginName]) if ("development" != "production") invariant(false, "EventPluginRegistry: Cannot inject two different event plugins using the same name, `%s`.", pluginName) else _prodInvariant("102", pluginName) else void 0;
// [ts] namesToPlugins[pluginName] = pluginModule;
// [ts] isOrderingDirty = true;
// [ts] }
// [ts] }
// [ts] if (isOrderingDirty) {
// [ts] recomputePluginOrdering();
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Looks up the plugin for the supplied event.
// [ts] *
// [ts] * @param {object} event A synthetic event.
// [ts] * @return {?object} The plugin that created the supplied event.
// [ts] * @internal
// [ts] */
// [ts] getPluginModuleForEvent: (event) => {
// [ts] let dispatchConfig = event.dispatchConfig;
// [ts] if (dispatchConfig.registrationName) {
// [ts] return EventPluginRegistry.registrationNameModules[dispatchConfig.registrationName] || null;
// [ts] }
// [ts] if (dispatchConfig.phasedRegistrationNames != null) {
// pulling phasedRegistrationNames out of dispatchConfig helps Flow see
// that it is not undefined.
// [ts] let phasedRegistrationNames = dispatchConfig.phasedRegistrationNames;
// [ts] for (let phase in phasedRegistrationNames) {
// [ts] if (!phasedRegistrationNames.hasOwnProperty(phase)) {
// [ts] continue;
// [ts] }
// [ts] let pluginModule = EventPluginRegistry.registrationNameModules[phasedRegistrationNames[phase]];
// [ts] if (pluginModule) {
// [ts] return pluginModule;
// [ts] }
// [ts] }
// [ts] }
// [ts] return null;
// [ts] },
// [ts] /**
// [ts] * Exposed for unit testing.
// [ts] * @private
// [ts] */
// [ts] _resetEventPlugins: () => {
// [ts] eventPluginOrder = null;
// [ts] for (let pluginName in namesToPlugins) {
// [ts] if (namesToPlugins.hasOwnProperty(pluginName)) {
// [ts] delete namesToPlugins[pluginName];
// [ts] }
// [ts] }
// [ts] EventPluginRegistry.plugins.length = 0;
// [ts] let eventNameDispatchConfigs = EventPluginRegistry.eventNameDispatchConfigs;
// [ts] for (let eventName in eventNameDispatchConfigs) {
// [ts] if (eventNameDispatchConfigs.hasOwnProperty(eventName)) {
// [ts] delete eventNameDispatchConfigs[eventName];
// [ts] }
// [ts] }
// [ts] let registrationNameModules = EventPluginRegistry.registrationNameModules;
// [ts] for (let registrationName in registrationNameModules) {
// [ts] if (registrationNameModules.hasOwnProperty(registrationName)) {
// [ts] delete registrationNameModules[registrationName];
// [ts] }
// [ts] }
// [ts] if ("development" != "production") {
// [ts] let possibleRegistrationNames = EventPluginRegistry.possibleRegistrationNames;
// [ts] for (let lowerCasedName in possibleRegistrationNames) {
// [ts] if (possibleRegistrationNames.hasOwnProperty(lowerCasedName)) {
// [ts] delete possibleRegistrationNames[lowerCasedName];
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = EventPluginRegistry;
// [ts] },{"126":126,"152":152}],19:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let ReactErrorUtils = _dereq_(55);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] /**
// [ts] * Injected dependencies:
// [ts] */
// [ts] /**
// [ts] * - `ComponentTree`: [required] Module that can convert between React instances
// [ts] *   and actual node references.
// [ts] */
// [ts] let ComponentTree;
// [ts] let TreeTraversal;
// [ts] let injection = {
// [ts] injectComponentTree: (Injected) => {
// [ts] ComponentTree = Injected;
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(Injected && Injected.getNodeFromInstance && Injected.getInstanceFromNode, "EventPluginUtils.injection.injectComponentTree(...): Injected " + "module is missing getNodeFromInstance or getInstanceFromNode.") else void 0;
// [ts] }
// [ts] },
// [ts] injectTreeTraversal: (Injected) => {
// [ts] TreeTraversal = Injected;
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(Injected && Injected.isAncestor && Injected.getLowestCommonAncestor, "EventPluginUtils.injection.injectTreeTraversal(...): Injected " + "module is missing isAncestor or getLowestCommonAncestor.") else void 0;
// [ts] }
// [ts] }
// [ts] };
// [ts] fn isEndish(topLevelType: auto) {
// [ts] return topLevelType == "topMouseUp" || topLevelType == "topTouchEnd" || topLevelType == "topTouchCancel";
// [ts] }
// [ts] fn isMoveish(topLevelType: auto) {
// [ts] return topLevelType == "topMouseMove" || topLevelType == "topTouchMove";
// [ts] }
// [ts] fn isStartish(topLevelType: auto) {
// [ts] return topLevelType == "topMouseDown" || topLevelType == "topTouchStart";
// [ts] }
// [ts] let validateEventDispatches;
// [ts] if ("development" != "production") {
// [ts] validateEventDispatches = (event) => {
// [ts] let dispatchListeners = event._dispatchListeners;
// [ts] let dispatchInstances = event._dispatchInstances;
// [ts] let listenersIsArr = Array.isArray(dispatchListeners);
// [ts] let listenersLen = if (listenersIsArr) dispatchListeners.length else if (dispatchListeners) 1 else 0;
// [ts] let instancesIsArr = Array.isArray(dispatchInstances);
// [ts] let instancesLen = if (instancesIsArr) dispatchInstances.length else if (dispatchInstances) 1 else 0;
// [ts] if ("development" != "production") warning(instancesIsArr == listenersIsArr && instancesLen == listenersLen, "EventPluginUtils: Invalid `event`.") else void 0;
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Dispatch the event to the listener.
// [ts] * @param {SyntheticEvent} event SyntheticEvent to handle
// [ts] * @param {boolean} simulated If the event is simulated (changes exn behavior)
// [ts] * @param {function} listener Application-level callback
// [ts] * @param {*} inst Internal component instance
// [ts] */
// [ts] fn executeDispatch(event: auto, simulated: auto, listener: auto, inst: auto) {
// [ts] let type = event.type || "unknown-event";
// [ts] event.currentTarget = EventPluginUtils.getNodeFromInstance(inst);
// [ts] if (simulated) {
// [ts] ReactErrorUtils.invokeGuardedCallbackWithCatch(type, listener, event);
// [ts] } else {
// [ts] ReactErrorUtils.invokeGuardedCallback(type, listener, event);
// [ts] }
// [ts] event.currentTarget = null;
// [ts] }
// [ts] /**
// [ts] * Standard/simple iteration through an event's collected dispatches.
// [ts] */
// [ts] fn executeDispatchesInOrder(event: auto, simulated: auto) {
// [ts] let dispatchListeners = event._dispatchListeners;
// [ts] let dispatchInstances = event._dispatchInstances;
// [ts] if ("development" != "production") {
// [ts] validateEventDispatches(event);
// [ts] }
// [ts] if (Array.isArray(dispatchListeners)) {
// [ts] let i = 0;
// [ts] while (i < dispatchListeners.length) { // [cap-migrate] for-step: i++
// [ts] if (event.isPropagationStopped()) {
// [ts] break;
// [ts] }
// Listeners and Instances are two parallel arrays that are always in sync.
// [ts] executeDispatch(event, simulated, dispatchListeners[i], dispatchInstances[i]);
// [ts] }
// [ts] } else if (dispatchListeners) {
// [ts] executeDispatch(event, simulated, dispatchListeners, dispatchInstances);
// [ts] }
// [ts] event._dispatchListeners = null;
// [ts] event._dispatchInstances = null;
// [ts] }
// [ts] /**
// [ts] * Standard/simple iteration through an event's collected dispatches, but stops
// [ts] * at the first dispatch execution returning true, and returns that id.
// [ts] *
// [ts] * @return {?string} id of the first dispatch execution who's listener returns
// [ts] * true, or null if no listener returned true.
// [ts] */
// [ts] fn executeDispatchesInOrderStopAtTrueImpl(event: auto) {
// [ts] let dispatchListeners = event._dispatchListeners;
// [ts] let dispatchInstances = event._dispatchInstances;
// [ts] if ("development" != "production") {
// [ts] validateEventDispatches(event);
// [ts] }
// [ts] if (Array.isArray(dispatchListeners)) {
// [ts] let i = 0;
// [ts] while (i < dispatchListeners.length) { // [cap-migrate] for-step: i++
// [ts] if (event.isPropagationStopped()) {
// [ts] break;
// [ts] }
// Listeners and Instances are two parallel arrays that are always in sync.
// [ts] if (dispatchListeners[i](event, dispatchInstances[i])) {
// [ts] return dispatchInstances[i];
// [ts] }
// [ts] }
// [ts] } else if (dispatchListeners) {
// [ts] if (dispatchListeners(event, dispatchInstances)) {
// [ts] return dispatchInstances;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * @see executeDispatchesInOrderStopAtTrueImpl
// [ts] */
// [ts] fn executeDispatchesInOrderStopAtTrue(event: auto) {
// [ts] let ret = executeDispatchesInOrderStopAtTrueImpl(event);
// [ts] event._dispatchInstances = null;
// [ts] event._dispatchListeners = null;
// [ts] return ret;
// [ts] }
// [ts] /**
// [ts] * Execution of a "direct" dispatch - there must be at most one dispatch
// [ts] * accumulated on the event or it is considered an error. It doesn't really make
// [ts] * sense for an event with multiple dispatches (bubbled) to keep track of the
// [ts] * return values at each dispatch execution, but it does tend to make sense when
// [ts] * dealing with "direct" dispatches.
// [ts] *
// [ts] * @return {*} The return value of executing the single dispatch.
// [ts] */
// [ts] fn executeDirectDispatch(event: auto) {
// [ts] if ("development" != "production") {
// [ts] validateEventDispatches(event);
// [ts] }
// [ts] let dispatchListener = event._dispatchListeners;
// [ts] let dispatchInstance = event._dispatchInstances;
// [ts] if (!!Array.isArray(dispatchListener)) if ("development" != "production") invariant(false, "executeDirectDispatch(...): Invalid `event`.") else _prodInvariant("103") else void 0;
// [ts] event.currentTarget = if (dispatchListener) EventPluginUtils.getNodeFromInstance(dispatchInstance) else null;
// [ts] let res = if (dispatchListener) dispatchListener(event) else null;
// [ts] event.currentTarget = null;
// [ts] event._dispatchListeners = null;
// [ts] event._dispatchInstances = null;
// [ts] return res;
// [ts] }
// [ts] /**
// [ts] * @param {SyntheticEvent} event
// [ts] * @return {boolean} True iff number of dispatches accumulated is greater than 0.
// [ts] */
// [ts] fn hasDispatches(event: auto) {
// [ts] return !!event._dispatchListeners;
// [ts] }
// [ts] /**
// [ts] * General utilities that are useful in creating custom Event Plugins.
// [ts] */
// [ts] let EventPluginUtils = {
// [ts] isEndish: isEndish,
// [ts] isMoveish: isMoveish,
// [ts] isStartish: isStartish,
// [ts] executeDirectDispatch: executeDirectDispatch,
// [ts] executeDispatchesInOrder: executeDispatchesInOrder,
// [ts] executeDispatchesInOrderStopAtTrue: executeDispatchesInOrderStopAtTrue,
// [ts] hasDispatches: hasDispatches,
// [ts] getInstanceFromNode: (node) => {
// [ts] return ComponentTree.getInstanceFromNode(node);
// [ts] },
// [ts] getNodeFromInstance: (node) => {
// [ts] return ComponentTree.getNodeFromInstance(node);
// [ts] },
// [ts] isAncestor: (a, b) => {
// [ts] return TreeTraversal.isAncestor(a, b);
// [ts] },
// [ts] getLowestCommonAncestor: (a, b) => {
// [ts] return TreeTraversal.getLowestCommonAncestor(a, b);
// [ts] },
// [ts] getParentInstance: (inst) => {
// [ts] return TreeTraversal.getParentInstance(inst);
// [ts] },
// [ts] traverseTwoPhase: (target, fn, arg) => {
// [ts] return TreeTraversal.traverseTwoPhase(target, fn, arg);
// [ts] },
// [ts] traverseEnterLeave: (from, to, fn, argFrom, argTo) => {
// [ts] return TreeTraversal.traverseEnterLeave(from, to, fn, argFrom, argTo);
// [ts] },
// [ts] injection: injection
// [ts] };
// [ts] module.exports = EventPluginUtils;
// [ts] },{"126":126,"152":152,"159":159,"55":55}],20:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let EventPluginHub = _dereq_(17);
// [ts] let EventPluginUtils = _dereq_(19);
// [ts] let accumulateInto = _dereq_(102);
// [ts] let forEachAccumulated = _dereq_(110);
// [ts] let warning = _dereq_(159);
// [ts] let getListener = EventPluginHub.getListener;
// [ts] /**
// [ts] * Some event types have a notion of different registration names for different
// [ts] * "phases" of propagation. This finds listeners by a given phase.
// [ts] */
// [ts] fn listenerAtPhase(inst: auto, event: auto, propagationPhase: auto) {
// [ts] let registrationName = event.dispatchConfig.phasedRegistrationNames[propagationPhase];
// [ts] return getListener(inst, registrationName);
// [ts] }
// [ts] /**
// [ts] * Tags a `SyntheticEvent` with dispatched listeners. Creating this function
// [ts] * here, allows us to not have to bind or create functions for each event.
// [ts] * Mutating the event's members allows us to not have to create a wrapping
// [ts] * "dispatch" object that pairs the event with the listener.
// [ts] */
// [ts] fn accumulateDirectionalDispatches(inst: auto, phase: auto, event: auto) {
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(inst, "Dispatching inst must not be null") else void 0;
// [ts] }
// [ts] let listener = listenerAtPhase(inst, event, phase);
// [ts] if (listener) {
// [ts] event._dispatchListeners = accumulateInto(event._dispatchListeners, listener);
// [ts] event._dispatchInstances = accumulateInto(event._dispatchInstances, inst);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Collect dispatches (must be entirely collected before dispatching - see unit
// [ts] * tests). Lazily allocate the array to conserve memory.  We must loop through
// [ts] * each event and perform the traversal for each one. We cannot perform a
// [ts] * single traversal for the entire collection of events because each event may
// [ts] * have a different target.
// [ts] */
// [ts] fn accumulateTwoPhaseDispatchesSingle(event: auto) {
// [ts] if (event && event.dispatchConfig.phasedRegistrationNames) {
// [ts] EventPluginUtils.traverseTwoPhase(event._targetInst, accumulateDirectionalDispatches, event);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Same as `accumulateTwoPhaseDispatchesSingle`, but skips over the targetID.
// [ts] */
// [ts] fn accumulateTwoPhaseDispatchesSingleSkipTarget(event: auto) {
// [ts] if (event && event.dispatchConfig.phasedRegistrationNames) {
// [ts] let targetInst = event._targetInst;
// [ts] let parentInst = if (targetInst) EventPluginUtils.getParentInstance(targetInst) else null;
// [ts] EventPluginUtils.traverseTwoPhase(parentInst, accumulateDirectionalDispatches, event);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Accumulates without regard to direction, does not look for phased
// [ts] * registration names. Same as `accumulateDirectDispatchesSingle` but without
// [ts] * requiring that the `dispatchMarker` be the same as the dispatched ID.
// [ts] */
// [ts] fn accumulateDispatches(inst: auto, ignoredDirection: auto, event: auto) {
// [ts] if (event && event.dispatchConfig.registrationName) {
// [ts] let registrationName = event.dispatchConfig.registrationName;
// [ts] let listener = getListener(inst, registrationName);
// [ts] if (listener) {
// [ts] event._dispatchListeners = accumulateInto(event._dispatchListeners, listener);
// [ts] event._dispatchInstances = accumulateInto(event._dispatchInstances, inst);
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Accumulates dispatches on an `SyntheticEvent`, but only for the
// [ts] * `dispatchMarker`.
// [ts] * @param {SyntheticEvent} event
// [ts] */
// [ts] fn accumulateDirectDispatchesSingle(event: auto) {
// [ts] if (event && event.dispatchConfig.registrationName) {
// [ts] accumulateDispatches(event._targetInst, null, event);
// [ts] }
// [ts] }
// [ts] fn accumulateTwoPhaseDispatches(events: auto) {
// [ts] forEachAccumulated(events, accumulateTwoPhaseDispatchesSingle);
// [ts] }
// [ts] fn accumulateTwoPhaseDispatchesSkipTarget(events: auto) {
// [ts] forEachAccumulated(events, accumulateTwoPhaseDispatchesSingleSkipTarget);
// [ts] }
// [ts] fn accumulateEnterLeaveDispatches(leave: auto, enter: auto, from: auto, to: auto) {
// [ts] EventPluginUtils.traverseEnterLeave(from, to, accumulateDispatches, leave, enter);
// [ts] }
// [ts] fn accumulateDirectDispatches(events: auto) {
// [ts] forEachAccumulated(events, accumulateDirectDispatchesSingle);
// [ts] }
// [ts] /**
// [ts] * A small set of propagation patterns, each of which will accept a small amount
// [ts] * of information, and generate a set of "dispatch ready event objects" - which
// [ts] * are sets of events that have already been annotated with a set of dispatched
// [ts] * listener functions/ids. The API is designed this way to discourage these
// [ts] * propagation strategies from actually executing the dispatches, since we
// [ts] * always want to collect the entire set of dispatches before executing event a
// [ts] * single one.
// [ts] *
// [ts] * @constructor EventPropagators
// [ts] */
// [ts] let EventPropagators = {
// [ts] accumulateTwoPhaseDispatches: accumulateTwoPhaseDispatches,
// [ts] accumulateTwoPhaseDispatchesSkipTarget: accumulateTwoPhaseDispatchesSkipTarget,
// [ts] accumulateDirectDispatches: accumulateDirectDispatches,
// [ts] accumulateEnterLeaveDispatches: accumulateEnterLeaveDispatches
// [ts] };
// [ts] module.exports = EventPropagators;
// [ts] },{"102":102,"110":110,"159":159,"17":17,"19":19}],21:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let PooledClass = _dereq_(25);
// [ts] let getTextContentAccessor = _dereq_(118);
// [ts] /**
// [ts] * This helper class stores information about text content of a target node,
// [ts] * allowing comparison of content before and after a given event.
// [ts] *
// [ts] * Identify the node where selection currently begins, then observe
// [ts] * both its text content and its current position in the DOM. Since the
// [ts] * browser may natively replace the target node during composition, we can
// [ts] * use its position to find its replacement.
// [ts] *
// [ts] * @param {DOMEventTarget} root
// [ts] */
// [ts] fn FallbackCompositionState(root: auto) {
// [ts] _._root = root;
// [ts] _._startText = _.getText();
// [ts] _._fallbackText = null;
// [ts] }
// [ts] _assign(FallbackCompositionState.prototype, {
// [ts] destructor: () => {
// [ts] _._root = null;
// [ts] _._startText = null;
// [ts] _._fallbackText = null;
// [ts] },
// [ts] /**
// [ts] * Get current text of input.
// [ts] *
// [ts] * @return {string}
// [ts] */
// [ts] getText: () => {
// [ts] if ("value" == _._root) {
// [ts] return _._root.value;
// [ts] }
// [ts] return _._root[getTextContentAccessor()];
// [ts] },
// [ts] /**
// [ts] * Determine the differing substring between the initially stored
// [ts] * text content and the current content.
// [ts] *
// [ts] * @return {string}
// [ts] */
// [ts] getData: () => {
// [ts] if (_._fallbackText) {
// [ts] return _._fallbackText;
// [ts] }
// [ts] let start;
// [ts] let startValue = _._startText;
// [ts] let startLength = startValue.length;
// [ts] let end;
// [ts] let endValue = _.getText();
// [ts] let endLength = endValue.length;
// [ts] start = 0;
// [ts] while (start < startLength) { // [cap-migrate] for-step: start++
// [ts] if (startValue[start] != endValue[start]) {
// [ts] break;
// [ts] }
// [ts] }
// [ts] let minEnd = startLength - start;
// [ts] end = 1;
// [ts] while (end <= minEnd) { // [cap-migrate] for-step: end++
// [ts] if (startValue[startLength - end] != endValue[endLength - end]) {
// [ts] break;
// [ts] }
// [ts] }
// [ts] let sliceTail = if (end > 1) 1 - end else null;
// [ts] _._fallbackText = endValue.slice(start, sliceTail);
// [ts] return _._fallbackText;
// [ts] }
// [ts] });
// [ts] PooledClass.addPoolingTo(FallbackCompositionState);
// [ts] module.exports = FallbackCompositionState;
// [ts] },{"118":118,"160":160,"25":25}],22:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMProperty = _dereq_(11);
// [ts] let MUST_USE_PROPERTY = DOMProperty.injection.MUST_USE_PROPERTY;
// [ts] let HAS_BOOLEAN_VALUE = DOMProperty.injection.HAS_BOOLEAN_VALUE;
// [ts] let HAS_NUMERIC_VALUE = DOMProperty.injection.HAS_NUMERIC_VALUE;
// [ts] let HAS_POSITIVE_NUMERIC_VALUE = DOMProperty.injection.HAS_POSITIVE_NUMERIC_VALUE;
// [ts] let HAS_OVERLOADED_BOOLEAN_VALUE = DOMProperty.injection.HAS_OVERLOADED_BOOLEAN_VALUE;
// [ts] let HTMLDOMPropertyConfig = {
// [ts] isCustomAttribute: RegExp.prototype.test.bind(RegExp.init("^(data|aria)-[" + DOMProperty.ATTRIBUTE_NAME_CHAR + "]*$")),
// [ts] Properties: {
// [ts] /**
// [ts] * Standard Properties
// [ts] */
// [ts] accept: 0,
// [ts] acceptCharset: 0,
// [ts] accessKey: 0,
// [ts] action: 0,
// [ts] allowFullScreen: HAS_BOOLEAN_VALUE,
// [ts] allowTransparency: 0,
// [ts] alt: 0,
// specifies target context for links with `preload` type
// [ts] as: 0,
// [ts] async: HAS_BOOLEAN_VALUE,
// [ts] autoComplete: 0,
// autoFocus is polyfilled/normalized by AutoFocusUtils
// autoFocus: HAS_BOOLEAN_VALUE,
// [ts] autoPlay: HAS_BOOLEAN_VALUE,
// [ts] capture: HAS_BOOLEAN_VALUE,
// [ts] cellPadding: 0,
// [ts] cellSpacing: 0,
// [ts] charSet: 0,
// [ts] challenge: 0,
// [ts] checked: MUST_USE_PROPERTY | HAS_BOOLEAN_VALUE,
// [ts] cite: 0,
// [ts] classID: 0,
// [ts] className: 0,
// [ts] cols: HAS_POSITIVE_NUMERIC_VALUE,
// [ts] colSpan: 0,
// [ts] content: 0,
// [ts] contentEditable: 0,
// [ts] contextMenu: 0,
// [ts] controls: HAS_BOOLEAN_VALUE,
// [ts] controlsList: 0,
// [ts] coords: 0,
// [ts] crossOrigin: 0,
// [ts] data: 0, // For `<object />` acts as `src`.
// [ts] dateTime: 0,
// [ts] "default": HAS_BOOLEAN_VALUE,
// [ts] defer: HAS_BOOLEAN_VALUE,
// [ts] dir: 0,
// [ts] disabled: HAS_BOOLEAN_VALUE,
// [ts] download: HAS_OVERLOADED_BOOLEAN_VALUE,
// [ts] draggable: 0,
// [ts] encType: 0,
// [ts] form: 0,
// [ts] formAction: 0,
// [ts] formEncType: 0,
// [ts] formMethod: 0,
// [ts] formNoValidate: HAS_BOOLEAN_VALUE,
// [ts] formTarget: 0,
// [ts] frameBorder: 0,
// [ts] headers: 0,
// [ts] height: 0,
// [ts] hidden: HAS_BOOLEAN_VALUE,
// [ts] high: 0,
// [ts] href: 0,
// [ts] hrefLang: 0,
// [ts] htmlFor: 0,
// [ts] httpEquiv: 0,
// [ts] icon: 0,
// [ts] id: 0,
// [ts] inputMode: 0,
// [ts] integrity: 0,
// [ts] is: 0,
// [ts] keyParams: 0,
// [ts] keyType: 0,
// [ts] kind: 0,
// [ts] label: 0,
// [ts] lang: 0,
// [ts] list: 0,
// [ts] loop: HAS_BOOLEAN_VALUE,
// [ts] low: 0,
// [ts] manifest: 0,
// [ts] marginHeight: 0,
// [ts] marginWidth: 0,
// [ts] max: 0,
// [ts] maxLength: 0,
// [ts] media: 0,
// [ts] mediaGroup: 0,
// [ts] method: 0,
// [ts] min: 0,
// [ts] minLength: 0,
// Caution; `option.selected` is not updated if `select.multiple` is
// disabled with `removeAttribute`.
// [ts] multiple: MUST_USE_PROPERTY | HAS_BOOLEAN_VALUE,
// [ts] muted: MUST_USE_PROPERTY | HAS_BOOLEAN_VALUE,
// [ts] name: 0,
// [ts] nonce: 0,
// [ts] noValidate: HAS_BOOLEAN_VALUE,
// [ts] open: HAS_BOOLEAN_VALUE,
// [ts] optimum: 0,
// [ts] pattern: 0,
// [ts] placeholder: 0,
// [ts] playsInline: HAS_BOOLEAN_VALUE,
// [ts] poster: 0,
// [ts] preload: 0,
// [ts] profile: 0,
// [ts] radioGroup: 0,
// [ts] readOnly: HAS_BOOLEAN_VALUE,
// [ts] referrerPolicy: 0,
// [ts] rel: 0,
// [ts] required: HAS_BOOLEAN_VALUE,
// [ts] reversed: HAS_BOOLEAN_VALUE,
// [ts] role: 0,
// [ts] rows: HAS_POSITIVE_NUMERIC_VALUE,
// [ts] rowSpan: HAS_NUMERIC_VALUE,
// [ts] sandbox: 0,
// [ts] scope: 0,
// [ts] scoped: HAS_BOOLEAN_VALUE,
// [ts] scrolling: 0,
// [ts] seamless: HAS_BOOLEAN_VALUE,
// [ts] selected: MUST_USE_PROPERTY | HAS_BOOLEAN_VALUE,
// [ts] shape: 0,
// [ts] size: HAS_POSITIVE_NUMERIC_VALUE,
// [ts] sizes: 0,
// [ts] span: HAS_POSITIVE_NUMERIC_VALUE,
// [ts] spellCheck: 0,
// [ts] src: 0,
// [ts] srcDoc: 0,
// [ts] srcLang: 0,
// [ts] srcSet: 0,
// [ts] start: HAS_NUMERIC_VALUE,
// [ts] step: 0,
// [ts] style: 0,
// [ts] summary: 0,
// [ts] tabIndex: 0,
// [ts] target: 0,
// [ts] title: 0,
// Setting .type throws on non-<input> tags
// [ts] type: 0,
// [ts] useMap: 0,
// [ts] value: 0,
// [ts] width: 0,
// [ts] wmode: 0,
// [ts] wrap: 0,
// [ts] /**
// [ts] * RDFa Properties
// [ts] */
// [ts] about: 0,
// [ts] datatype: 0,
// [ts] inlist: 0,
// [ts] prefix: 0,
// property is also supported for OpenGraph in meta tags.
// [ts] property: 0,
// [ts] resource: 0,
// [ts] "typeof": 0,
// [ts] vocab: 0,
// [ts] /**
// [ts] * Non-standard Properties
// [ts] */
// autoCapitalize and autoCorrect are supported in Mobile Safari for
// keyboard hints.
// [ts] autoCapitalize: 0,
// [ts] autoCorrect: 0,
// autoSave allows WebKit/Blink to persist values of input fields on page reloads
// [ts] autoSave: 0,
// color is for Safari mask-icon link
// [ts] color: 0,
// itemProp, itemScope, itemType are for
// Microdata support. See http://schema.org/docs/gs.html
// [ts] itemProp: 0,
// [ts] itemScope: HAS_BOOLEAN_VALUE,
// [ts] itemType: 0,
// itemID and itemRef are for Microdata support as well but
// only specified in the WHATWG spec document. See
// https://html.spec.whatwg.org/multipage/microdata.html#microdata-dom-api
// [ts] itemID: 0,
// [ts] itemRef: 0,
// results show looking glass icon and recent searches on input
// search fields in WebKit/Blink
// [ts] results: 0,
// IE-only attribute that specifies security restrictions on an iframe
// as an alternative to the sandbox attribute on IE<10
// [ts] security: 0,
// IE-only attribute that controls focus behavior
// [ts] unselectable: 0
// [ts] },
// [ts] DOMAttributeNames: {
// [ts] acceptCharset: "accept-charset",
// [ts] className: "class",
// [ts] htmlFor: "for",
// [ts] httpEquiv: "http-equiv"
// [ts] },
// [ts] DOMPropertyNames: {},
// [ts] DOMMutationMethods: {
// [ts] value: (node, value) => {
// [ts] if (value == null) {
// [ts] return node.removeAttribute("value");
// [ts] }
// Number inputs get special treatment due to some edge cases in
// Chrome. Let everything else assign the value attribute as normal.
// https://github.com/facebook/react/issues/7253#issuecomment-236074326
// [ts] if (node.type != "number" || node.hasAttribute("value") == false) {
// [ts] node.setAttribute("value", "" + value);
// [ts] } else if (node.validity && !node.validity.badInput && node.ownerDocument.activeElement != node) {
// Don't assign an attribute if validation reports bad
// input. Chrome will clear the value. Additionally, don't
// operate on inputs that have focus, otherwise Chrome might
// strip off trailing decimal places and cause the user's
// cursor position to jump to the beginning of the input.
//
// In ReactDOMInput, we have an onBlur event that will trigger
// this function again when focus is lost.
// [ts] node.setAttribute("value", "" + value);
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = HTMLDOMPropertyConfig;
// [ts] },{"11":11}],23:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Escape and wrap key so it is safe to use as a reactid
// [ts] *
// [ts] * @param {string} key to be escaped.
// [ts] * @return {string} the escaped key.
// [ts] */
// [ts] fn escape(key: auto) {
// [ts] let escapeRegex = "[=:]";
// [ts] let escaperLookup = {
// [ts] "=": "=0",
// [ts] ":": "=2"
// [ts] };
// [ts] let escapedString = ("" + key).replace(escapeRegex, (match_val) => {
// [ts] return escaperLookup[match_val];
// [ts] });
// [ts] return "$" + escapedString;
// [ts] }
// [ts] /**
// [ts] * Unescape and unwrap key for human-readable display
// [ts] *
// [ts] * @param {string} key to unescape.
// [ts] * @return {string} the unescaped key.
// [ts] */
// [ts] fn unescape(key: auto) {
// [ts] let unescapeRegex = "(=0|=2)";
// [ts] let unescaperLookup = {
// [ts] "=0": "=",
// [ts] "=2": ":"
// [ts] };
// [ts] let keySubstring = if (key[0] == "." && key[1] == "$") key.substring(2) else key.substring(1);
// [ts] return ("" + keySubstring).replace(unescapeRegex, (match_val) => {
// [ts] return unescaperLookup[match_val];
// [ts] });
// [ts] }
// [ts] let KeyEscapeUtils = {
// [ts] escape: escape,
// [ts] unescape: unescape
// [ts] };
// [ts] module.exports = KeyEscapeUtils;
// [ts] },{}],24:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let ReactPropTypesSecret = _dereq_(73);
// [ts] let propTypesFactory = _dereq_(162);
// [ts] let React = _dereq_(135);
// [ts] let PropTypes = propTypesFactory(React.isValidElement);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] let hasReadOnlyValue = {
// [ts] button: true,
// [ts] checkbox: true,
// [ts] image: true,
// [ts] hidden: true,
// [ts] radio: true,
// [ts] reset: true,
// [ts] submit: true
// [ts] };
// [ts] fn _assertSingleLink(inputProps: auto) {
// [ts] if (!(inputProps.checkedLink == null || inputProps.valueLink == null)) if ("development" != "production") invariant(false, "Cannot provide a checkedLink and a valueLink. If you want to use checkedLink, you probably don't want to use valueLink and vice versa.") else _prodInvariant("87") else void 0;
// [ts] }
// [ts] fn _assertValueLink(inputProps: auto) {
// [ts] _assertSingleLink(inputProps);
// [ts] if (!(inputProps.value == null && inputProps.onChange == null)) if ("development" != "production") invariant(false, "Cannot provide a valueLink and a value or onChange event. If you want to use value or onChange, you probably don't want to use valueLink.") else _prodInvariant("88") else void 0;
// [ts] }
// [ts] fn _assertCheckedLink(inputProps: auto) {
// [ts] _assertSingleLink(inputProps);
// [ts] if (!(inputProps.checked == null && inputProps.onChange == null)) if ("development" != "production") invariant(false, "Cannot provide a checkedLink and a checked property or onChange event. If you want to use checked or onChange, you probably don't want to use checkedLink") else _prodInvariant("89") else void 0;
// [ts] }
// [ts] let propTypes = {
// [ts] value: (props, propName, componentName) => {
// [ts] if (!props[propName] || hasReadOnlyValue[props.type] || props.onChange || props.readOnly || props.disabled) {
// [ts] return null;
// [ts] }
// [ts] return Error.init("You provided a `value` prop to a form field without an " + "`onChange` handler. This will render a read-only field. If " + "the field should be mutable use `defaultValue`. Otherwise, " + "set either `onChange` or `readOnly`.");
// [ts] },
// [ts] checked: (props, propName, componentName) => {
// [ts] if (!props[propName] || props.onChange || props.readOnly || props.disabled) {
// [ts] return null;
// [ts] }
// [ts] return Error.init("You provided a `checked` prop to a form field without an " + "`onChange` handler. This will render a read-only field. If " + "the field should be mutable use `defaultChecked`. Otherwise, " + "set either `onChange` or `readOnly`.");
// [ts] },
// [ts] onChange: PropTypes.func
// [ts] };
// [ts] let loggedTypeFailures = {};
// [ts] fn getDeclarationErrorAddendum(owner: auto) {
// [ts] if (owner) {
// [ts] let name = owner.getName();
// [ts] if (name) {
// [ts] return " Check the render method of `" + name + "`.";
// [ts] }
// [ts] }
// [ts] return "";
// [ts] }
// [ts] /**
// [ts] * Provide a linked `value` attribute for controlled forms. You should not use
// [ts] * this outside of the ReactDOM controlled form components.
// [ts] */
// [ts] let LinkedValueUtils = {
// [ts] checkPropTypes: (tagName, props, owner) => {
// [ts] for (let propName in propTypes) {
// [ts] if (propTypes.hasOwnProperty(propName)) {
// [ts] let error = propTypes[propName](props, propName, tagName, "prop", null, ReactPropTypesSecret);
// [ts] }
// [ts] if (error == Error && !(error.message == loggedTypeFailures)) {
// Only monitor this failure once because there tends to be a lot of the
// same error.
// [ts] loggedTypeFailures[error.message] = true;
// [ts] let addendum = getDeclarationErrorAddendum(owner);
// [ts] if ("development" != "production") warning(false, "Failed form propType: %s%s", error.message, addendum) else void 0;
// [ts] }
// [ts] }
// [ts] },
// [ts] /**
// [ts] * @param {object} inputProps Props for form component
// [ts] * @return {*} current value of the input either from value prop or link.
// [ts] */
// [ts] getValue: (inputProps) => {
// [ts] if (inputProps.valueLink) {
// [ts] _assertValueLink(inputProps);
// [ts] return inputProps.valueLink.value;
// [ts] }
// [ts] return inputProps.value;
// [ts] },
// [ts] /**
// [ts] * @param {object} inputProps Props for form component
// [ts] * @return {*} current checked status of the input either from checked prop
// [ts] *             or link.
// [ts] */
// [ts] getChecked: (inputProps) => {
// [ts] if (inputProps.checkedLink) {
// [ts] _assertCheckedLink(inputProps);
// [ts] return inputProps.checkedLink.value;
// [ts] }
// [ts] return inputProps.checked;
// [ts] },
// [ts] /**
// [ts] * @param {object} inputProps Props for form component
// [ts] * @param {SyntheticEvent} event change event to handle
// [ts] */
// [ts] executeOnChange: (inputProps, event) => {
// [ts] if (inputProps.valueLink) {
// [ts] _assertValueLink(inputProps);
// [ts] return inputProps.valueLink.requestChange(event.target.value);
// [ts] } else if (inputProps.checkedLink) {
// [ts] _assertCheckedLink(inputProps);
// [ts] return inputProps.checkedLink.requestChange(event.target.checked);
// [ts] } else if (inputProps.onChange) {
// [ts] return inputProps.onChange.call(null, event);
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = LinkedValueUtils;
// [ts] },{"126":126,"135":135,"152":152,"159":159,"162":162,"73":73}],25:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Static poolers. Several custom versions for each potential number of
// [ts] * arguments. A completely generic pooler is easy to implement, but would
// [ts] * require accessing the `arguments` object. In each of these, `this` refers to
// [ts] * the Class itself, not an instance. If any others are needed, simply add them
// [ts] * here, or in their own files.
// [ts] */
// [ts] let oneArgumentPooler = (copyFieldsFrom) => {
// [ts] let Klass = this;
// [ts] if (Klass.instancePool.length) {
// [ts] let instance = Klass.instancePool.pop();
// [ts] Klass.call(instance, copyFieldsFrom);
// [ts] return instance;
// [ts] } else {
// [ts] return Klass.init(copyFieldsFrom);
// [ts] }
// [ts] };
// [ts] let twoArgumentPooler = (a1, a2) => {
// [ts] let Klass = this;
// [ts] if (Klass.instancePool.length) {
// [ts] let instance = Klass.instancePool.pop();
// [ts] Klass.call(instance, a1, a2);
// [ts] return instance;
// [ts] } else {
// [ts] return Klass.init(a1, a2);
// [ts] }
// [ts] };
// [ts] let threeArgumentPooler = (a1, a2, a3) => {
// [ts] let Klass = this;
// [ts] if (Klass.instancePool.length) {
// [ts] let instance = Klass.instancePool.pop();
// [ts] Klass.call(instance, a1, a2, a3);
// [ts] return instance;
// [ts] } else {
// [ts] return Klass.init(a1, a2, a3);
// [ts] }
// [ts] };
// [ts] let fourArgumentPooler = (a1, a2, a3, a4) => {
// [ts] let Klass = this;
// [ts] if (Klass.instancePool.length) {
// [ts] let instance = Klass.instancePool.pop();
// [ts] Klass.call(instance, a1, a2, a3, a4);
// [ts] return instance;
// [ts] } else {
// [ts] return Klass.init(a1, a2, a3, a4);
// [ts] }
// [ts] };
// [ts] let standardReleaser = (instance) => {
// [ts] let Klass = this;
// [ts] if (!(instance == Klass)) if ("development" != "production") invariant(false, "Trying to release an instance into a pool of a different type.") else _prodInvariant("25") else void 0;
// [ts] instance.destructor();
// [ts] if (Klass.instancePool.length < Klass.poolSize) {
// [ts] Klass.instancePool.push(instance);
// [ts] }
// [ts] };
// [ts] let DEFAULT_POOL_SIZE = 10;
// [ts] let DEFAULT_POOLER = oneArgumentPooler;
// [ts] /**
// [ts] * Augments `CopyConstructor` to be a poolable class, augmenting only the class
// [ts] * itself (statically) not adding any prototypical fields. Any CopyConstructor
// [ts] * you give this may have a `poolSize` property, and will look for a
// [ts] * prototypical `destructor` on instances.
// [ts] *
// [ts] * @param {Function} CopyConstructor Constructor that can be used to reset.
// [ts] * @param {Function} pooler Customizable pooler.
// [ts] */
// [ts] let addPoolingTo = (CopyConstructor, pooler) => {
// Casting as any so that flow ignores the actual implementation and trusts
// it to match the type we declared
// [ts] let NewKlass = CopyConstructor;
// [ts] NewKlass.instancePool = [];
// [ts] NewKlass.getPooled = pooler || DEFAULT_POOLER;
// [ts] if (!NewKlass.poolSize) {
// [ts] NewKlass.poolSize = DEFAULT_POOL_SIZE;
// [ts] }
// [ts] NewKlass.release = standardReleaser;
// [ts] return NewKlass;
// [ts] };
// [ts] let PooledClass = {
// [ts] addPoolingTo: addPoolingTo,
// [ts] oneArgumentPooler: oneArgumentPooler,
// [ts] twoArgumentPooler: twoArgumentPooler,
// [ts] threeArgumentPooler: threeArgumentPooler,
// [ts] fourArgumentPooler: fourArgumentPooler
// [ts] };
// [ts] module.exports = PooledClass;
// [ts] },{"126":126,"152":152}],26:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let EventPluginRegistry = _dereq_(18);
// [ts] let ReactEventEmitterMixin = _dereq_(56);
// [ts] let ViewportMetrics = _dereq_(101);
// [ts] let getVendorPrefixedEventName = _dereq_(119);
// [ts] let isEventSupported = _dereq_(122);
// [ts] /**
// [ts] * Summary of `ReactBrowserEventEmitter` event handling:
// [ts] *
// [ts] *  - Top-level delegation is used to trap most native browser events. This
// [ts] *    may only occur in the main thread and is the responsibility of
// [ts] *    ReactEventListener, which is injected and can therefore support pluggable
// [ts] *    event sources. This is the only work that occurs in the main thread.
// [ts] *
// [ts] *  - We normalize and de-duplicate events to account for browser quirks. This
// [ts] *    may be done in the worker thread.
// [ts] *
// [ts] *  - Forward these native events (with the associated top-level type used to
// [ts] *    trap it) to `EventPluginHub`, which in turn will ask plugins if they want
// [ts] *    to extract any synthetic events.
// [ts] *
// [ts] *  - The `EventPluginHub` will then process each event by annotating them with
// [ts] *    "dispatches", a sequence of listeners and IDs that care about that event.
// [ts] *
// [ts] *  - The `EventPluginHub` then dispatches the events.
// [ts] *
// [ts] * Overview of React and the event system:
// [ts] *
// [ts] * +------------+    .
// [ts] * |    DOM     |    .
// [ts] * +------------+    .
// [ts] *       |           .
// [ts] *       v           .
// [ts] * +------------+    .
// [ts] * | ReactEvent |    .
// [ts] * |  Listener  |    .
// [ts] * +------------+    .                         +-----------+
// [ts] *       |           .               +--------+|SimpleEvent|
// [ts] *       |           .               |         |Plugin     |
// [ts] * +-----|------+    .               v         +-----------+
// [ts] * |     |      |    .    +--------------+                    +------------+
// [ts] * |     +-----------.--->|EventPluginHub|                    |    Event   |
// [ts] * |            |    .    |              |     +-----------+  | Propagators|
// [ts] * | ReactEvent |    .    |              |     |TapEvent   |  |------------|
// [ts] * |  Emitter   |    .    |              |<---+|Plugin     |  |other plugin|
// [ts] * |            |    .    |              |     +-----------+  |  utilities |
// [ts] * |     +-----------.--->|              |                    +------------+
// [ts] * |     |      |    .    +--------------+
// [ts] * +-----|------+    .                ^        +-----------+
// [ts] *       |           .                |        |Enter/Leave|
// [ts] *       +           .                +-------+|Plugin     |
// [ts] * +-------------+   .                         +-----------+
// [ts] * | application |   .
// [ts] * |-------------|   .
// [ts] * |             |   .
// [ts] * |             |   .
// [ts] * +-------------+   .
// [ts] *                   .
// [ts] *    React Core     .  General Purpose Event Plugin System
// [ts] */
// [ts] let hasEventPageXY;
// [ts] let alreadyListeningTo = {};
// [ts] let isMonitoringScrollValue = false;
// [ts] let reactTopListenersCounter = 0;
// For events like 'submit' which don't consistently bubble (which we trap at a
// lower node than `document`), binding at `document` would cause duplicate
// events so we don't include them here
// [ts] let topEventMapping = {
// [ts] topAbort: "abort",
// [ts] topAnimationEnd: getVendorPrefixedEventName("animationend") || "animationend",
// [ts] topAnimationIteration: getVendorPrefixedEventName("animationiteration") || "animationiteration",
// [ts] topAnimationStart: getVendorPrefixedEventName("animationstart") || "animationstart",
// [ts] topBlur: "blur",
// [ts] topCanPlay: "canplay",
// [ts] topCanPlayThrough: "canplaythrough",
// [ts] topChange: "change",
// [ts] topClick: "click",
// [ts] topCompositionEnd: "compositionend",
// [ts] topCompositionStart: "compositionstart",
// [ts] topCompositionUpdate: "compositionupdate",
// [ts] topContextMenu: "contextmenu",
// [ts] topCopy: "copy",
// [ts] topCut: "cut",
// [ts] topDoubleClick: "dblclick",
// [ts] topDrag: "drag",
// [ts] topDragEnd: "dragend",
// [ts] topDragEnter: "dragenter",
// [ts] topDragExit: "dragexit",
// [ts] topDragLeave: "dragleave",
// [ts] topDragOver: "dragover",
// [ts] topDragStart: "dragstart",
// [ts] topDrop: "drop",
// [ts] topDurationChange: "durationchange",
// [ts] topEmptied: "emptied",
// [ts] topEncrypted: "encrypted",
// [ts] topEnded: "ended",
// [ts] topError: "error",
// [ts] topFocus: "focus",
// [ts] topInput: "input",
// [ts] topKeyDown: "keydown",
// [ts] topKeyPress: "keypress",
// [ts] topKeyUp: "keyup",
// [ts] topLoadedData: "loadeddata",
// [ts] topLoadedMetadata: "loadedmetadata",
// [ts] topLoadStart: "loadstart",
// [ts] topMouseDown: "mousedown",
// [ts] topMouseMove: "mousemove",
// [ts] topMouseOut: "mouseout",
// [ts] topMouseOver: "mouseover",
// [ts] topMouseUp: "mouseup",
// [ts] topPaste: "paste",
// [ts] topPause: "pause",
// [ts] topPlay: "play",
// [ts] topPlaying: "playing",
// [ts] topProgress: "progress",
// [ts] topRateChange: "ratechange",
// [ts] topScroll: "scroll",
// [ts] topSeeked: "seeked",
// [ts] topSeeking: "seeking",
// [ts] topSelectionChange: "selectionchange",
// [ts] topStalled: "stalled",
// [ts] topSuspend: "suspend",
// [ts] topTextInput: "textInput",
// [ts] topTimeUpdate: "timeupdate",
// [ts] topTouchCancel: "touchcancel",
// [ts] topTouchEnd: "touchend",
// [ts] topTouchMove: "touchmove",
// [ts] topTouchStart: "touchstart",
// [ts] topTransitionEnd: getVendorPrefixedEventName("transitionend") || "transitionend",
// [ts] topVolumeChange: "volumechange",
// [ts] topWaiting: "waiting",
// [ts] topWheel: "wheel"
// [ts] };
// [ts] /**
// [ts] * To ensure no conflicts with other potential React instances on the page
// [ts] */
// [ts] let topListenersIDKey = "_reactListenersID" + String(Math.random()).slice(2);
// [ts] fn getListeningForDocument(mountAt: auto) {
// In IE8, `mountAt` is a host object and doesn't have `hasOwnProperty`
// directly.
// [ts] if (!Object.prototype.hasOwnProperty.call(mountAt, topListenersIDKey)) {
// [ts] mountAt[topListenersIDKey] = reactTopListenersCounter++;
// [ts] alreadyListeningTo[mountAt[topListenersIDKey]] = {};
// [ts] }
// [ts] return alreadyListeningTo[mountAt[topListenersIDKey]];
// [ts] }
// [ts] /**
// [ts] * `ReactBrowserEventEmitter` is used to attach top-level event listeners. For
// [ts] * example:
// [ts] *
// [ts] *   EventPluginHub.putListener('myID', 'onClick', myFunction);
// [ts] *
// [ts] * This would allocate a "registration" of `('onClick', myFunction)` on 'myID'.
// [ts] *
// [ts] * @internal
// [ts] */
// [ts] let ReactBrowserEventEmitter = _assign({}, ReactEventEmitterMixin, {
// [ts] /**
// [ts] * Injectable event backend
// [ts] */
// [ts] ReactEventListener: null,
// [ts] injection: {
// [ts] /**
// [ts] * @param {object} ReactEventListener
// [ts] */
// [ts] injectReactEventListener: (ReactEventListener) => {
// [ts] ReactEventListener.setHandleTopLevel(ReactBrowserEventEmitter.handleTopLevel);
// [ts] ReactBrowserEventEmitter.ReactEventListener = ReactEventListener;
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Sets whether or not any created callbacks should be enabled.
// [ts] *
// [ts] * @param {boolean} enabled True if callbacks should be enabled.
// [ts] */
// [ts] setEnabled: (enabled) => {
// [ts] if (ReactBrowserEventEmitter.ReactEventListener) {
// [ts] ReactBrowserEventEmitter.ReactEventListener.setEnabled(enabled);
// [ts] }
// [ts] },
// [ts] /**
// [ts] * @return {boolean} True if callbacks are enabled.
// [ts] */
// [ts] isEnabled: () => {
// [ts] return !!(ReactBrowserEventEmitter.ReactEventListener && ReactBrowserEventEmitter.ReactEventListener.isEnabled());
// [ts] },
// [ts] /**
// [ts] * We listen for bubbled touch events on the document object.
// [ts] *
// [ts] * Firefox v8.01 (and possibly others) exhibited strange behavior when
// [ts] * mounting `onmousemove` events at some node that was not the document
// [ts] * element. The symptoms were that if your mouse is not moving over something
// [ts] * contained within that mount point (for example on the background) the
// [ts] * top-level listeners for `onmousemove` won't be called. However, if you
// [ts] * register the `mousemove` on the document object, then it will of course
// [ts] * catch all `mousemove`s. This along with iOS quirks, justifies restricting
// [ts] * top-level listeners to the document object only, at least for these
// [ts] * movement types of events and possibly all events.
// [ts] *
// [ts] * @see http://www.quirksmode.org/blog/archives/2010/09/click_event_del.html
// [ts] *
// [ts] * Also, `keyup`/`keypress`/`keydown` do not bubble to the window on IE, but
// [ts] * they bubble to document.
// [ts] *
// [ts] * @param {string} registrationName Name of listener (e.g. `onClick`).
// [ts] * @param {object} contentDocumentHandle Document which owns the container
// [ts] */
// [ts] listenTo: (registrationName, contentDocumentHandle) => {
// [ts] let mountAt = contentDocumentHandle;
// [ts] let isListening = getListeningForDocument(mountAt);
// [ts] let dependencies = EventPluginRegistry.registrationNameDependencies[registrationName];
// [ts] let i = 0;
// [ts] while (i < dependencies.length) { // [cap-migrate] for-step: i++
// [ts] let dependency = dependencies[i];
// [ts] if (!(isListening.hasOwnProperty(dependency) && isListening[dependency])) {
// [ts] if (dependency == "topWheel") {
// [ts] if (isEventSupported("wheel")) {
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapBubbledEvent("topWheel", "wheel", mountAt);
// [ts] } else if (isEventSupported("mousewheel")) {
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapBubbledEvent("topWheel", "mousewheel", mountAt);
// [ts] } else {
// Firefox needs to capture a different mouse scroll event.
// @see http://www.quirksmode.org/dom/events/tests/scroll.html
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapBubbledEvent("topWheel", "DOMMouseScroll", mountAt);
// [ts] }
// [ts] } else if (dependency == "topScroll") {
// [ts] if (isEventSupported("scroll", true)) {
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapCapturedEvent("topScroll", "scroll", mountAt);
// [ts] } else {
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapBubbledEvent("topScroll", "scroll", ReactBrowserEventEmitter.ReactEventListener.WINDOW_HANDLE);
// [ts] }
// [ts] } else if (dependency == "topFocus" || dependency == "topBlur") {
// [ts] if (isEventSupported("focus", true)) {
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapCapturedEvent("topFocus", "focus", mountAt);
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapCapturedEvent("topBlur", "blur", mountAt);
// [ts] } else if (isEventSupported("focusin")) {
// IE has `focusin` and `focusout` events which bubble.
// @see http://www.quirksmode.org/blog/archives/2008/04/delegating_the.html
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapBubbledEvent("topFocus", "focusin", mountAt);
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapBubbledEvent("topBlur", "focusout", mountAt);
// [ts] }
// to make sure blur and focus event listeners are only attached once
// [ts] isListening.topBlur = true;
// [ts] isListening.topFocus = true;
// [ts] } else if (topEventMapping.hasOwnProperty(dependency)) {
// [ts] ReactBrowserEventEmitter.ReactEventListener.trapBubbledEvent(dependency, topEventMapping[dependency], mountAt);
// [ts] }
// [ts] isListening[dependency] = true;
// [ts] }
// [ts] }
// [ts] },
// [ts] trapBubbledEvent: (topLevelType, handlerBaseName, handle) => {
// [ts] return ReactBrowserEventEmitter.ReactEventListener.trapBubbledEvent(topLevelType, handlerBaseName, handle);
// [ts] },
// [ts] trapCapturedEvent: (topLevelType, handlerBaseName, handle) => {
// [ts] return ReactBrowserEventEmitter.ReactEventListener.trapCapturedEvent(topLevelType, handlerBaseName, handle);
// [ts] },
// [ts] /**
// [ts] * Protect against document.createEvent() returning null
// [ts] * Some popup blocker extensions appear to do this:
// [ts] * https://github.com/facebook/react/issues/6887
// [ts] */
// [ts] supportsEventPageXY: () => {
// [ts] if (!document.createEvent) {
// [ts] return false;
// [ts] }
// [ts] let ev = document.createEvent("MouseEvent");
// [ts] return ev != null && "pageX" == ev;
// [ts] },
// [ts] /**
// [ts] * Listens to window scroll and resize events. We cache scroll values so that
// [ts] * application code can access them without triggering reflows.
// [ts] *
// [ts] * ViewportMetrics is only used by SyntheticMouse/TouchEvent and only when
// [ts] * pageX/pageY isn't supported (legacy browsers).
// [ts] *
// [ts] * NOTE: Scroll events do not bubble.
// [ts] *
// [ts] * @see http://www.quirksmode.org/dom/events/scroll.html
// [ts] */
// [ts] ensureScrollValueMonitoring: () => {
// [ts] if (hasEventPageXY == null) {
// [ts] hasEventPageXY = ReactBrowserEventEmitter.supportsEventPageXY();
// [ts] }
// [ts] if (!hasEventPageXY && !isMonitoringScrollValue) {
// [ts] let refresh = ViewportMetrics.refreshScrollValues;
// [ts] ReactBrowserEventEmitter.ReactEventListener.monitorScrollValue(refresh);
// [ts] isMonitoringScrollValue = true;
// [ts] }
// [ts] }
// [ts] });
// [ts] module.exports = ReactBrowserEventEmitter;
// [ts] },{"101":101,"119":119,"122":122,"160":160,"18":18,"56":56}],27:[(_dereq_, module, exports) => {
// [ts] ((process) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactReconciler = _dereq_(75);
// [ts] let instantiateReactComponent = _dereq_(121);
// [ts] let KeyEscapeUtils = _dereq_(23);
// [ts] let shouldUpdateReactComponent = _dereq_(130);
// [ts] let traverseAllChildren = _dereq_(131);
// [ts] let warning = _dereq_(159);
// [ts] let ReactComponentTreeHook;
// [ts] if (@typeName(process) != "undefined" && process.env && "development" == "test") {
// Temporary hack.
// Inline requires don't work well with Jest:
// https://github.com/facebook/react/issues/7240
// Remove the inline requires when we don't need them anymore:
// https://github.com/facebook/react/pull/7178
// [ts] ReactComponentTreeHook = _dereq_(133);
// [ts] }
// [ts] fn instantiateChild(childInstances: auto, child: auto, name: auto, selfDebugID: auto) {
// We found a component instance.
// [ts] let keyUnique = childInstances[name] == null;
// [ts] if ("development" != "production") {
// [ts] if (!ReactComponentTreeHook) {
// [ts] ReactComponentTreeHook = _dereq_(133);
// [ts] }
// [ts] if (!keyUnique) {
// [ts] if ("development" != "production") warning(false, "flattenChildren(...): Encountered two children with the same key, " + "`%s`. Child keys must be unique; when two children share a key, only " + "the first child will be used.%s", KeyEscapeUtils.unescape(name), ReactComponentTreeHook.getStackAddendumByID(selfDebugID)) else void 0;
// [ts] }
// [ts] }
// [ts] if (child != null && keyUnique) {
// [ts] childInstances[name] = instantiateReactComponent(child, true);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * ReactChildReconciler provides helpers for initializing or updating a set of
// [ts] * children. Its output is suitable for passing it onto ReactMultiChild which
// [ts] * does diffed reordering and insertion.
// [ts] */
// [ts] let ReactChildReconciler = {
// [ts] /**
// [ts] * Generates a "mount image" for each of the supplied children. In the case
// [ts] * of `ReactDOMComponent`, a mount image is a string of markup.
// [ts] *
// [ts] * @param {?object} nestedChildNodes Nested child maps.
// [ts] * @return {?object} A set of child instances.
// [ts] * @internal
// [ts] */
// [ts] instantiateChildren: function (nestedChildNodes, transaction, context, selfDebugID) // 0 in production and for roots
// [ts] {
// [ts] if (nestedChildNodes == null) {
// [ts] return null;
// [ts] }
// [ts] let childInstances = {};
// [ts] if ("development" != "production") {
// [ts] traverseAllChildren(nestedChildNodes, (childInsts, child, name) => {
// [ts] return instantiateChild(childInsts, child, name, selfDebugID);
// [ts] }, childInstances);
// [ts] } else {
// [ts] traverseAllChildren(nestedChildNodes, instantiateChild, childInstances);
// [ts] }
// [ts] return childInstances;
// [ts] },
// [ts] /**
// [ts] * Updates the rendered children and returns a new set of children.
// [ts] *
// [ts] * @param {?object} prevChildren Previously initialized set of children.
// [ts] * @param {?object} nextChildren Flat child element maps.
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @param {object} context
// [ts] * @return {?object} A new set of child instances.
// [ts] * @internal
// [ts] */
// [ts] updateChildren: function (prevChildren, nextChildren, mountImages, removedNodes, transaction, hostParent, hostContainerInfo, context, selfDebugID) // 0 in production and for roots
// [ts] {
// We currently don't have a way to track moves here but if we use iterators
// instead of for..in we can zip the iterators and check if an item has
// moved.
// TODO: If nothing has changed, return the prevChildren object so that we
// can quickly bailout if nothing has changed.
// [ts] if (!nextChildren && !prevChildren) {
// [ts] return;
// [ts] }
// [ts] let name;
// [ts] let prevChild;
// [ts] for (name in nextChildren) {
// [ts] if (!nextChildren.hasOwnProperty(name)) {
// [ts] continue;
// [ts] }
// [ts] prevChild = prevChildren && prevChildren[name];
// [ts] let prevElement = prevChild && prevChild._currentElement;
// [ts] let nextElement = nextChildren[name];
// [ts] if (prevChild != null && shouldUpdateReactComponent(prevElement, nextElement)) {
// [ts] ReactReconciler.receiveComponent(prevChild, nextElement, transaction, context);
// [ts] nextChildren[name] = prevChild;
// [ts] } else {
// [ts] if (prevChild) {
// [ts] removedNodes[name] = ReactReconciler.getHostNode(prevChild);
// [ts] ReactReconciler.unmountComponent(prevChild, false);
// [ts] }
// The child must be instantiated before it's mounted.
// [ts] let nextChildInstance = instantiateReactComponent(nextElement, true);
// [ts] nextChildren[name] = nextChildInstance;
// Creating mount image now ensures refs are resolved in right order
// (see https://github.com/facebook/react/pull/7101 for explanation).
// [ts] let nextChildMountImage = ReactReconciler.mountComponent(nextChildInstance, transaction, hostParent, hostContainerInfo, context, selfDebugID);
// [ts] mountImages.push(nextChildMountImage);
// [ts] }
// [ts] }
// Unmount children that are no longer present.
// [ts] for (name in prevChildren) {
// [ts] if (prevChildren.hasOwnProperty(name) && !(nextChildren && nextChildren.hasOwnProperty(name))) {
// [ts] prevChild = prevChildren[name];
// [ts] removedNodes[name] = ReactReconciler.getHostNode(prevChild);
// [ts] ReactReconciler.unmountComponent(prevChild, false);
// [ts] }
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Unmounts all rendered children. This should be used to clean up children
// [ts] * when this component is unmounted.
// [ts] *
// [ts] * @param {?object} renderedChildren Previously initialized set of children.
// [ts] * @internal
// [ts] */
// [ts] unmountChildren: (renderedChildren, safely) => {
// [ts] for (let name in renderedChildren) {
// [ts] if (renderedChildren.hasOwnProperty(name)) {
// [ts] let renderedChild = renderedChildren[name];
// [ts] ReactReconciler.unmountComponent(renderedChild, safely);
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactChildReconciler;
// [ts] }).call(this,null)
// [ts] },{"121":121,"130":130,"131":131,"133":133,"159":159,"23":23,"75":75}],28:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMChildrenOperations = _dereq_(8);
// [ts] let ReactDOMIDOperations = _dereq_(38);
// [ts] /**
// [ts] * Abstracts away all functionality of the reconciler that requires knowledge of
// [ts] * the browser context. TODO: These callers should be refactored to avoid the
// [ts] * need for this injection.
// [ts] */
// [ts] let ReactComponentBrowserEnvironment = {
// [ts] processChildrenUpdates: ReactDOMIDOperations.dangerouslyProcessChildrenUpdates,
// [ts] replaceNodeWithMarkup: DOMChildrenOperations.dangerouslyReplaceNodeWithMarkup
// [ts] };
// [ts] module.exports = ReactComponentBrowserEnvironment;
// [ts] },{"38":38,"8":8}],29:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] let injected = false;
// [ts] let ReactComponentEnvironment = {
// [ts] /**
// [ts] * Optionally injectable hook for swapping out mount images in the middle of
// [ts] * the tree.
// [ts] */
// [ts] replaceNodeWithMarkup: null,
// [ts] /**
// [ts] * Optionally injectable hook for processing a queue of child updates. Will
// [ts] * later move into MultiChildComponents.
// [ts] */
// [ts] processChildrenUpdates: null,
// [ts] injection: {
// [ts] injectEnvironment: (environment) => {
// [ts] if (!!injected) if ("development" != "production") invariant(false, "ReactCompositeComponent: injectEnvironment() can only be called once.") else _prodInvariant("104") else void 0;
// [ts] ReactComponentEnvironment.replaceNodeWithMarkup = environment.replaceNodeWithMarkup;
// [ts] ReactComponentEnvironment.processChildrenUpdates = environment.processChildrenUpdates;
// [ts] injected = true;
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactComponentEnvironment;
// [ts] },{"126":126,"152":152}],30:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] let React = _dereq_(135);
// [ts] let ReactComponentEnvironment = _dereq_(29);
// [ts] let ReactCurrentOwner = _dereq_(134);
// [ts] let ReactErrorUtils = _dereq_(55);
// [ts] let ReactInstanceMap = _dereq_(63);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let ReactNodeTypes = _dereq_(69);
// [ts] let ReactReconciler = _dereq_(75);
// [ts] if ("development" != "production") {
// [ts] let checkReactTypeSpec = _dereq_(104);
// [ts] }
// [ts] let emptyObject = _dereq_(145);
// [ts] let invariant = _dereq_(152);
// [ts] let shallowEqual = _dereq_(158);
// [ts] let shouldUpdateReactComponent = _dereq_(130);
// [ts] let warning = _dereq_(159);
// [ts] let CompositeTypes = {
// [ts] ImpureClass: 0,
// [ts] PureClass: 1,
// [ts] StatelessFunctional: 2
// [ts] };
// [ts] fn StatelessComponent(Component: auto) {}
// [ts] StatelessComponent.prototype.render = () => {
// [ts] let Component = ReactInstanceMap.get(this)._currentElement.type;
// [ts] let element = Component(_.props, _.context, _.updater);
// [ts] warnIfInvalidElement(Component, element);
// [ts] return element;
// [ts] };
// [ts] fn warnIfInvalidElement(Component: auto, element: auto) {
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(element == null || element == false || React.isValidElement(element), "%s(...): A valid React element (or null) must be returned. You may have " + "returned undefined, an array or some other invalid object.", Component.displayName || Component.name || "Component") else void 0;
// [ts] if ("development" != "production") warning(!Component.childContextTypes, "%s(...): childContextTypes cannot be defined on a functional component.", Component.displayName || Component.name || "Component") else void 0;
// [ts] }
// [ts] }
// [ts] fn shouldConstruct(Component: auto) {
// [ts] return !!(Component.prototype && Component.prototype.isReactComponent);
// [ts] }
// [ts] fn isPureComponent(Component: auto) {
// [ts] return !!(Component.prototype && Component.prototype.isPureReactComponent);
// [ts] }
// Separated into a function to contain deoptimizations caused by try/finally.
// [ts] fn measureLifeCyclePerf(fn: auto, debugID: auto, timerType: auto) {
// [ts] if (debugID == 0) {
// Top-level wrappers (see ReactMount) and empty components (see
// ReactDOMEmptyComponent) are invisible to hooks and devtools.
// Both are implementation details that should go away in the future.
// [ts] return fn();
// [ts] }
// [ts] ReactInstrumentation.debugTool.onBeginLifeCycleTimer(debugID, timerType);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return fn();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactInstrumentation.debugTool.onEndLifeCycleTimer(debugID, timerType);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * ------------------ The Life-Cycle of a Composite Component ------------------
// [ts] *
// [ts] * - constructor: Initialization of state. The instance is now retained.
// [ts] *   - componentWillMount
// [ts] *   - render
// [ts] *   - [children's constructors]
// [ts] *     - [children's componentWillMount and render]
// [ts] *     - [children's componentDidMount]
// [ts] *     - componentDidMount
// [ts] *
// [ts] *       Update Phases:
// [ts] *       - componentWillReceiveProps (only called if parent updated)
// [ts] *       - shouldComponentUpdate
// [ts] *         - componentWillUpdate
// [ts] *           - render
// [ts] *           - [children's constructors or receive props phases]
// [ts] *         - componentDidUpdate
// [ts] *
// [ts] *     - componentWillUnmount
// [ts] *     - [children's componentWillUnmount]
// [ts] *   - [children destroyed]
// [ts] * - (destroyed): The instance is now blank, released by React and ready for GC.
// [ts] *
// [ts] * -----------------------------------------------------------------------------
// [ts] */
// [ts] /**
// [ts] * An incrementing ID assigned to each component when it is mounted. This is
// [ts] * used to enforce the order in which `ReactUpdates` updates dirty components.
// [ts] *
// [ts] * @private
// [ts] */
// [ts] let nextMountID = 1;
// [ts] /**
// [ts] * @lends {ReactCompositeComponent.prototype}
// [ts] */
// [ts] let ReactCompositeComponent = {
// [ts] /**
// [ts] * Base constructor for all composite component.
// [ts] *
// [ts] * @param {ReactElement} element
// [ts] * @final
// [ts] * @internal
// [ts] */
// [ts] construct: (element) => {
// [ts] _._currentElement = element;
// [ts] _._rootNodeID = 0;
// [ts] _._compositeType = null;
// [ts] _._instance = null;
// [ts] _._hostParent = null;
// [ts] _._hostContainerInfo = null;
// See ReactUpdateQueue
// [ts] _._updateBatchNumber = null;
// [ts] _._pendingElement = null;
// [ts] _._pendingStateQueue = null;
// [ts] _._pendingReplaceState = false;
// [ts] _._pendingForceUpdate = false;
// [ts] _._renderedNodeType = null;
// [ts] _._renderedComponent = null;
// [ts] _._context = null;
// [ts] _._mountOrder = 0;
// [ts] _._topLevelWrapper = null;
// See ReactUpdates and ReactUpdateQueue.
// [ts] _._pendingCallbacks = null;
// ComponentWillUnmount shall only be called once
// [ts] _._calledComponentWillUnmount = false;
// [ts] if ("development" != "production") {
// [ts] _._warnedAboutRefsInRender = false;
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Initializes the component, renders markup, and registers event listeners.
// [ts] *
// [ts] * @param {ReactReconcileTransaction|ReactServerRenderingTransaction} transaction
// [ts] * @param {?object} hostParent
// [ts] * @param {?object} hostContainerInfo
// [ts] * @param {?object} context
// [ts] * @return {?string} Rendered markup to be inserted into the DOM.
// [ts] * @final
// [ts] * @internal
// [ts] */
// [ts] mountComponent: (transaction, hostParent, hostContainerInfo, context) => {
// [ts] let _this = this;
// [ts] _._context = context;
// [ts] _._mountOrder = nextMountID++;
// [ts] _._hostParent = hostParent;
// [ts] _._hostContainerInfo = hostContainerInfo;
// [ts] let publicProps = _._currentElement.props;
// [ts] let publicContext = _._processContext(context);
// [ts] let Component = _._currentElement.type;
// [ts] let updateQueue = transaction.getUpdateQueue();
// Initialize the public class
// [ts] let doConstruct = shouldConstruct(Component);
// [ts] let inst = _._constructComponent(doConstruct, publicProps, publicContext, updateQueue);
// [ts] let renderedElement;
// Support functional components
// [ts] if (!doConstruct && (inst == null || inst.render == null)) {
// [ts] renderedElement = inst;
// [ts] warnIfInvalidElement(Component, renderedElement);
// [ts] if (!(inst == null || inst == false || React.isValidElement(inst))) if ("development" != "production") invariant(false, "%s(...): A valid React element (or null) must be returned. You may have returned undefined, an array or some other invalid object.", Component.displayName || Component.name || "Component") else _prodInvariant("105", Component.displayName || Component.name || "Component") else void 0;
// [ts] inst = StatelessComponent.init(Component);
// [ts] _._compositeType = CompositeTypes.StatelessFunctional;
// [ts] } else {
// [ts] if (isPureComponent(Component)) {
// [ts] _._compositeType = CompositeTypes.PureClass;
// [ts] } else {
// [ts] _._compositeType = CompositeTypes.ImpureClass;
// [ts] }
// [ts] }
// [ts] if ("development" != "production") {
// This will throw later in _renderValidatedComponent, but add an early
// warning now to help debugging
// [ts] if (inst.render == null) {
// [ts] if ("development" != "production") warning(false, "%s(...): No `render` method found on the returned component " + "instance: you may have forgotten to define `render`.", Component.displayName || Component.name || "Component") else void 0;
// [ts] }
// [ts] let propsMutated = inst.props != publicProps;
// [ts] let componentName = Component.displayName || Component.name || "Component";
// [ts] if ("development" != "production") warning(inst.props == null || !propsMutated, "%s(...): When calling super() in `%s`, make sure to pass " + "up the same props that your component's constructor was passed.", componentName, componentName) else void 0;
// [ts] }
// These should be set up in the constructor, but as a convenience for
// simpler class abstractions, we set them up after the fact.
// [ts] inst.props = publicProps;
// [ts] inst.context = publicContext;
// [ts] inst.refs = emptyObject;
// [ts] inst.updater = updateQueue;
// [ts] _._instance = inst;
// Store a reference from the instance back to the internal representation
// [ts] ReactInstanceMap.set(inst, this);
// [ts] if ("development" != "production") {
// Since plain JS classes are defined without any special initialization
// logic, we can not catch common errors early. Therefore, we have to
// catch them here, at initialization time, instead.
// [ts] if ("development" != "production") warning(!inst.getInitialState || inst.getInitialState.isReactClassApproved || inst.state, "getInitialState was defined on %s, a plain JavaScript class. " + "This is only supported for classes created using React.createClass. " + "Did you mean to define a state property instead?", _.getName() || "a component") else void 0;
// [ts] if ("development" != "production") warning(!inst.getDefaultProps || inst.getDefaultProps.isReactClassApproved, "getDefaultProps was defined on %s, a plain JavaScript class. " + "This is only supported for classes created using React.createClass. " + "Use a static property to define defaultProps instead.", _.getName() || "a component") else void 0;
// [ts] if ("development" != "production") warning(!inst.propTypes, "propTypes was defined as an instance property on %s. Use a static " + "property to define propTypes instead.", _.getName() || "a component") else void 0;
// [ts] if ("development" != "production") warning(!inst.contextTypes, "contextTypes was defined as an instance property on %s. Use a " + "static property to define contextTypes instead.", _.getName() || "a component") else void 0;
// [ts] if ("development" != "production") warning(@typeName(inst.componentShouldUpdate) != "function", "%s has a method called " + "componentShouldUpdate(). Did you mean shouldComponentUpdate()? " + "The name is phrased as a question because the function is " + "expected to return a value.", _.getName() || "A component") else void 0;
// [ts] if ("development" != "production") warning(@typeName(inst.componentDidUnmount) != "function", "%s has a method called " + "componentDidUnmount(). But there is no such lifecycle method. " + "Did you mean componentWillUnmount()?", _.getName() || "A component") else void 0;
// [ts] if ("development" != "production") warning(@typeName(inst.componentWillRecieveProps) != "function", "%s has a method called " + "componentWillRecieveProps(). Did you mean componentWillReceiveProps()?", _.getName() || "A component") else void 0;
// [ts] }
// [ts] let initialState = inst.state;
// [ts] if (initialState == null) {
// [ts] inst.state = initialState = null;
// [ts] }
// [ts] if (!(@typeName(initialState) == "object" && !Array.isArray(initialState))) if ("development" != "production") invariant(false, "%s.state: must be set to an object or null", _.getName() || "ReactCompositeComponent") else _prodInvariant("106", _.getName() || "ReactCompositeComponent") else void 0;
// [ts] _._pendingStateQueue = null;
// [ts] _._pendingReplaceState = false;
// [ts] _._pendingForceUpdate = false;
// [ts] let markup;
// [ts] if (inst.unstable_handleError) {
// [ts] markup = _.performInitialMountWithErrorHandling(renderedElement, hostParent, hostContainerInfo, transaction, context);
// [ts] } else {
// [ts] markup = _.performInitialMount(renderedElement, hostParent, hostContainerInfo, transaction, context);
// [ts] }
// [ts] if (inst.componentDidMount) {
// [ts] if ("development" != "production") {
// [ts] transaction.getReactMountReady().enqueue(() => {
// [ts] measureLifeCyclePerf(() => {
// [ts] return inst.componentDidMount();
// [ts] }, __._debugID, "componentDidMount");
// [ts] });
// [ts] } else {
// [ts] transaction.getReactMountReady().enqueue(inst.componentDidMount, inst);
// [ts] }
// [ts] }
// [ts] return markup;
// [ts] },
// [ts] _constructComponent: (doConstruct, publicProps, publicContext, updateQueue) => {
// [ts] if ("development" != "production" && !doConstruct) {
// [ts] ReactCurrentOwner.current = this;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return _._constructComponentWithoutOwner(doConstruct, publicProps, publicContext, updateQueue);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentOwner.current = null;
// [ts] }
// [ts] } else {
// [ts] return _._constructComponentWithoutOwner(doConstruct, publicProps, publicContext, updateQueue);
// [ts] }
// [ts] },
// [ts] _constructComponentWithoutOwner: (doConstruct, publicProps, publicContext, updateQueue) => {
// [ts] let Component = _._currentElement.type;
// [ts] if (doConstruct) {
// [ts] if ("development" != "production") {
// [ts] return measureLifeCyclePerf(() => {
// [ts] return Component.init(publicProps, publicContext, updateQueue);
// [ts] }, _._debugID, "ctor");
// [ts] } else {
// [ts] return Component.init(publicProps, publicContext, updateQueue);
// [ts] }
// [ts] }
// This can still be an instance in case of factory components
// but we'll count this as time spent rendering as the more common case.
// [ts] if ("development" != "production") {
// [ts] return measureLifeCyclePerf(() => {
// [ts] return Component(publicProps, publicContext, updateQueue);
// [ts] }, _._debugID, "render");
// [ts] } else {
// [ts] return Component(publicProps, publicContext, updateQueue);
// [ts] }
// [ts] },
// [ts] performInitialMountWithErrorHandling: (renderedElement, hostParent, hostContainerInfo, transaction, context) => {
// [ts] let markup;
// [ts] let checkpoint = transaction.checkpoint();
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] markup = _.performInitialMount(renderedElement, hostParent, hostContainerInfo, transaction, context);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// Roll back to checkpoint, handle error (which may add items to the transaction), and take a new checkpoint
// [ts] transaction.rollback(checkpoint);
// [ts] _._instance.unstable_handleError(e);
// [ts] if (_._pendingStateQueue) {
// [ts] _._instance.state = _._processPendingState(_._instance.props, _._instance.context);
// [ts] }
// [ts] checkpoint = transaction.checkpoint();
// [ts] _._renderedComponent.unmountComponent(true);
// [ts] transaction.rollback(checkpoint);
// Try again - we've informed the component about the error, so they can render an error message this time.
// If this throws again, the error will bubble up (and can be caught by a higher error boundary).
// [ts] markup = _.performInitialMount(renderedElement, hostParent, hostContainerInfo, transaction, context);
// [ts] }
// [ts] return markup;
// [ts] },
// [ts] performInitialMount: (renderedElement, hostParent, hostContainerInfo, transaction, context) => {
// [ts] let inst = _._instance;
// [ts] let debugID = 0;
// [ts] if ("development" != "production") {
// [ts] debugID = _._debugID;
// [ts] }
// [ts] if (inst.componentWillMount) {
// [ts] if ("development" != "production") {
// [ts] measureLifeCyclePerf(() => {
// [ts] return inst.componentWillMount();
// [ts] }, debugID, "componentWillMount");
// [ts] } else {
// [ts] inst.componentWillMount();
// [ts] }
// When mounting, calls to `setState` by `componentWillMount` will set
// `this._pendingStateQueue` without triggering a re-render.
// [ts] if (_._pendingStateQueue) {
// [ts] inst.state = _._processPendingState(inst.props, inst.context);
// [ts] }
// [ts] }
// If not a stateless component, we now render
// [ts] if (renderedElement == null) {
// [ts] renderedElement = _._renderValidatedComponent();
// [ts] }
// [ts] let nodeType = ReactNodeTypes.getType(renderedElement);
// [ts] _._renderedNodeType = nodeType;
// [ts] let child = _._instantiateReactComponent(renderedElement, nodeType != ReactNodeTypes.EMPTY /* shouldHaveDebugID */
// [ts] );
// [ts] _._renderedComponent = child;
// [ts] let markup = ReactReconciler.mountComponent(child, transaction, hostParent, hostContainerInfo, _._processChildContext(context), debugID);
// [ts] if ("development" != "production") {
// [ts] if (debugID != 0) {
// [ts] let childDebugIDs = if (child._debugID != 0) [child._debugID] else [];
// [ts] ReactInstrumentation.debugTool.onSetChildren(debugID, childDebugIDs);
// [ts] }
// [ts] }
// [ts] return markup;
// [ts] },
// [ts] getHostNode: () => {
// [ts] return ReactReconciler.getHostNode(_._renderedComponent);
// [ts] },
// [ts] /**
// [ts] * Releases any resources allocated by `mountComponent`.
// [ts] *
// [ts] * @final
// [ts] * @internal
// [ts] */
// [ts] unmountComponent: (safely) => {
// [ts] if (!_._renderedComponent) {
// [ts] return;
// [ts] }
// [ts] let inst = _._instance;
// [ts] if (inst.componentWillUnmount && !inst._calledComponentWillUnmount) {
// [ts] inst._calledComponentWillUnmount = true;
// [ts] if (safely) {
// [ts] let name = _.getName() + ".componentWillUnmount()";
// [ts] ReactErrorUtils.invokeGuardedCallback(name, inst.componentWillUnmount.bind(inst));
// [ts] } else {
// [ts] if ("development" != "production") {
// [ts] measureLifeCyclePerf(() => {
// [ts] return inst.componentWillUnmount();
// [ts] }, _._debugID, "componentWillUnmount");
// [ts] } else {
// [ts] inst.componentWillUnmount();
// [ts] }
// [ts] }
// [ts] }
// [ts] if (_._renderedComponent) {
// [ts] ReactReconciler.unmountComponent(_._renderedComponent, safely);
// [ts] _._renderedNodeType = null;
// [ts] _._renderedComponent = null;
// [ts] _._instance = null;
// [ts] }
// Reset pending fields
// Even if this component is scheduled for another update in ReactUpdates,
// it would still be ignored because these fields are reset.
// [ts] _._pendingStateQueue = null;
// [ts] _._pendingReplaceState = false;
// [ts] _._pendingForceUpdate = false;
// [ts] _._pendingCallbacks = null;
// [ts] _._pendingElement = null;
// These fields do not really need to be reset since this object is no
// longer accessible.
// [ts] _._context = null;
// [ts] _._rootNodeID = 0;
// [ts] _._topLevelWrapper = null;
// Delete the reference from the instance to this internal representation
// which allow the internals to be properly cleaned up even if the user
// leaks a reference to the public instance.
// [ts] ReactInstanceMap.remove(inst);
// Some existing components rely on inst.props even after they've been
// destroyed (in event handlers).
// TODO: inst.props = null;
// TODO: inst.state = null;
// TODO: inst.context = null;
// [ts] },
// [ts] /**
// [ts] * Filters the context object to only contain keys specified in
// [ts] * `contextTypes`
// [ts] *
// [ts] * @param {object} context
// [ts] * @return {?object}
// [ts] * @private
// [ts] */
// [ts] _maskContext: (context) => {
// [ts] let Component = _._currentElement.type;
// [ts] let contextTypes = Component.contextTypes;
// [ts] if (!contextTypes) {
// [ts] return emptyObject;
// [ts] }
// [ts] let maskedContext = {};
// [ts] for (let contextName in contextTypes) {
// [ts] maskedContext[contextName] = context[contextName];
// [ts] }
// [ts] return maskedContext;
// [ts] },
// [ts] /**
// [ts] * Filters the context object to only contain keys specified in
// [ts] * `contextTypes`, and asserts that they are valid.
// [ts] *
// [ts] * @param {object} context
// [ts] * @return {?object}
// [ts] * @private
// [ts] */
// [ts] _processContext: (context) => {
// [ts] let maskedContext = _._maskContext(context);
// [ts] if ("development" != "production") {
// [ts] let Component = _._currentElement.type;
// [ts] if (Component.contextTypes) {
// [ts] _._checkContextTypes(Component.contextTypes, maskedContext, "context");
// [ts] }
// [ts] }
// [ts] return maskedContext;
// [ts] },
// [ts] /**
// [ts] * @param {object} currentContext
// [ts] * @return {object}
// [ts] * @private
// [ts] */
// [ts] _processChildContext: (currentContext) => {
// [ts] let Component = _._currentElement.type;
// [ts] let inst = _._instance;
// [ts] let childContext;
// [ts] if (inst.getChildContext) {
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onBeginProcessingChildContext();
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] childContext = inst.getChildContext();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactInstrumentation.debugTool.onEndProcessingChildContext();
// [ts] }
// [ts] } else {
// [ts] childContext = inst.getChildContext();
// [ts] }
// [ts] }
// [ts] if (childContext) {
// [ts] if (!(@typeName(Component.childContextTypes) == "object")) if ("development" != "production") invariant(false, "%s.getChildContext(): childContextTypes must be defined in order to use getChildContext().", _.getName() || "ReactCompositeComponent") else _prodInvariant("107", _.getName() || "ReactCompositeComponent") else void 0;
// [ts] if ("development" != "production") {
// [ts] _._checkContextTypes(Component.childContextTypes, childContext, "child context");
// [ts] }
// [ts] for (let name in childContext) {
// [ts] if (!(name == Component.childContextTypes)) if ("development" != "production") invariant(false, "%s.getChildContext(): key \"%s\" is not defined in childContextTypes.", _.getName() || "ReactCompositeComponent", name) else _prodInvariant("108", _.getName() || "ReactCompositeComponent", name) else void 0;
// [ts] }
// [ts] return _assign({}, currentContext, childContext);
// [ts] }
// [ts] return currentContext;
// [ts] },
// [ts] /**
// [ts] * Assert that the context types are valid
// [ts] *
// [ts] * @param {object} typeSpecs Map of context field to a ReactPropType
// [ts] * @param {object} values Runtime values that need to be type-checked
// [ts] * @param {string} location e.g. "prop", "context", "child context"
// [ts] * @private
// [ts] */
// [ts] _checkContextTypes: (typeSpecs, values, location) => {
// [ts] if ("development" != "production") {
// [ts] checkReactTypeSpec(typeSpecs, values, location, _.getName(), null, _._debugID);
// [ts] }
// [ts] },
// [ts] receiveComponent: (nextElement, transaction, nextContext) => {
// [ts] let prevElement = _._currentElement;
// [ts] let prevContext = _._context;
// [ts] _._pendingElement = null;
// [ts] _.updateComponent(transaction, prevElement, nextElement, prevContext, nextContext);
// [ts] },
// [ts] /**
// [ts] * If any of `_pendingElement`, `_pendingStateQueue`, or `_pendingForceUpdate`
// [ts] * is set, update the component.
// [ts] *
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @internal
// [ts] */
// [ts] performUpdateIfNecessary: (transaction) => {
// [ts] if (_._pendingElement != null) {
// [ts] ReactReconciler.receiveComponent(this, _._pendingElement, transaction, _._context);
// [ts] } else if (_._pendingStateQueue != null || _._pendingForceUpdate) {
// [ts] _.updateComponent(transaction, _._currentElement, _._currentElement, _._context, _._context);
// [ts] } else {
// [ts] _._updateBatchNumber = null;
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Perform an update to a mounted component. The componentWillReceiveProps and
// [ts] * shouldComponentUpdate methods are called, then (assuming the update isn't
// [ts] * skipped) the remaining update lifecycle methods are called and the DOM
// [ts] * representation is updated.
// [ts] *
// [ts] * By default, this implements React's rendering and reconciliation algorithm.
// [ts] * Sophisticated clients may wish to override this.
// [ts] *
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @param {ReactElement} prevParentElement
// [ts] * @param {ReactElement} nextParentElement
// [ts] * @internal
// [ts] * @overridable
// [ts] */
// [ts] updateComponent: (transaction, prevParentElement, nextParentElement, prevUnmaskedContext, nextUnmaskedContext) => {
// [ts] let inst = _._instance;
// [ts] if (!(inst != null)) if ("development" != "production") invariant(false, "Attempted to update component `%s` that has already been unmounted (or failed to mount).", _.getName() || "ReactCompositeComponent") else _prodInvariant("136", _.getName() || "ReactCompositeComponent") else void 0;
// [ts] let willReceive = false;
// [ts] let nextContext;
// Determine if the context has changed or not
// [ts] if (_._context == nextUnmaskedContext) {
// [ts] nextContext = inst.context;
// [ts] } else {
// [ts] nextContext = _._processContext(nextUnmaskedContext);
// [ts] willReceive = true;
// [ts] }
// [ts] let prevProps = prevParentElement.props;
// [ts] let nextProps = nextParentElement.props;
// Not a simple state update but a props update
// [ts] if (prevParentElement != nextParentElement) {
// [ts] willReceive = true;
// [ts] }
// An update here will schedule an update but immediately set
// _pendingStateQueue which will ensure that any state updates gets
// immediately reconciled instead of waiting for the next batch.
// [ts] if (willReceive && inst.componentWillReceiveProps) {
// [ts] if ("development" != "production") {
// [ts] measureLifeCyclePerf(() => {
// [ts] return inst.componentWillReceiveProps(nextProps, nextContext);
// [ts] }, _._debugID, "componentWillReceiveProps");
// [ts] } else {
// [ts] inst.componentWillReceiveProps(nextProps, nextContext);
// [ts] }
// [ts] }
// [ts] let nextState = _._processPendingState(nextProps, nextContext);
// [ts] let shouldUpdate = true;
// [ts] if (!_._pendingForceUpdate) {
// [ts] if (inst.shouldComponentUpdate) {
// [ts] if ("development" != "production") {
// [ts] shouldUpdate = measureLifeCyclePerf(() => {
// [ts] return inst.shouldComponentUpdate(nextProps, nextState, nextContext);
// [ts] }, _._debugID, "shouldComponentUpdate");
// [ts] } else {
// [ts] shouldUpdate = inst.shouldComponentUpdate(nextProps, nextState, nextContext);
// [ts] }
// [ts] } else {
// [ts] if (_._compositeType == CompositeTypes.PureClass) {
// [ts] shouldUpdate = !shallowEqual(prevProps, nextProps) || !shallowEqual(inst.state, nextState);
// [ts] }
// [ts] }
// [ts] }
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(shouldUpdate != null, "%s.shouldComponentUpdate(): Returned undefined instead of a " + "boolean value. Make sure to return true or false.", _.getName() || "ReactCompositeComponent") else void 0;
// [ts] }
// [ts] _._updateBatchNumber = null;
// [ts] if (shouldUpdate) {
// [ts] _._pendingForceUpdate = false;
// Will set `this.props`, `this.state` and `this.context`.
// [ts] _._performComponentUpdate(nextParentElement, nextProps, nextState, nextContext, transaction, nextUnmaskedContext);
// [ts] } else {
// If it's determined that a component should not update, we still want
// to set props and state but we shortcut the rest of the update.
// [ts] _._currentElement = nextParentElement;
// [ts] _._context = nextUnmaskedContext;
// [ts] inst.props = nextProps;
// [ts] inst.state = nextState;
// [ts] inst.context = nextContext;
// [ts] }
// [ts] },
// [ts] _processPendingState: (props, context) => {
// [ts] let inst = _._instance;
// [ts] let queue = _._pendingStateQueue;
// [ts] let replace = _._pendingReplaceState;
// [ts] _._pendingReplaceState = false;
// [ts] _._pendingStateQueue = null;
// [ts] if (!queue) {
// [ts] return inst.state;
// [ts] }
// [ts] if (replace && queue.length == 1) {
// [ts] return queue[0];
// [ts] }
// [ts] let nextState = _assign(if ({}, replace) queue[0] else inst.state);
// [ts] let i = if (replace) 1 else 0;
// [ts] while (i < queue.length) { // [cap-migrate] for-step: i++
// [ts] let partial = queue[i];
// [ts] _assign(if (nextState, @typeName(partial) == "function") partial.call(inst, nextState, props, context) else partial);
// [ts] }
// [ts] return nextState;
// [ts] },
// [ts] /**
// [ts] * Merges new props and state, notifies delegate methods of update and
// [ts] * performs update.
// [ts] *
// [ts] * @param {ReactElement} nextElement Next element
// [ts] * @param {object} nextProps Next public object to set as properties.
// [ts] * @param {?object} nextState Next object to set as state.
// [ts] * @param {?object} nextContext Next public object to set as context.
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @param {?object} unmaskedContext
// [ts] * @private
// [ts] */
// [ts] _performComponentUpdate: (nextElement, nextProps, nextState, nextContext, transaction, unmaskedContext) => {
// [ts] let _this2 = this;
// [ts] let inst = _._instance;
// [ts] let hasComponentDidUpdate = Boolean(inst.componentDidUpdate);
// [ts] let prevProps;
// [ts] let prevState;
// [ts] let prevContext;
// [ts] if (hasComponentDidUpdate) {
// [ts] prevProps = inst.props;
// [ts] prevState = inst.state;
// [ts] prevContext = inst.context;
// [ts] }
// [ts] if (inst.componentWillUpdate) {
// [ts] if ("development" != "production") {
// [ts] measureLifeCyclePerf(() => {
// [ts] return inst.componentWillUpdate(nextProps, nextState, nextContext);
// [ts] }, _._debugID, "componentWillUpdate");
// [ts] } else {
// [ts] inst.componentWillUpdate(nextProps, nextState, nextContext);
// [ts] }
// [ts] }
// [ts] _._currentElement = nextElement;
// [ts] _._context = unmaskedContext;
// [ts] inst.props = nextProps;
// [ts] inst.state = nextState;
// [ts] inst.context = nextContext;
// [ts] _._updateRenderedComponent(transaction, unmaskedContext);
// [ts] if (hasComponentDidUpdate) {
// [ts] if ("development" != "production") {
// [ts] transaction.getReactMountReady().enqueue(() => {
// [ts] measureLifeCyclePerf(inst.componentDidUpdate.bind(inst, prevProps, prevState, prevContext), _this2._debugID, "componentDidUpdate");
// [ts] });
// [ts] } else {
// [ts] transaction.getReactMountReady().enqueue(inst.componentDidUpdate.bind(inst, prevProps, prevState, prevContext), inst);
// [ts] }
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Call the component's `render` method and update the DOM accordingly.
// [ts] *
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @internal
// [ts] */
// [ts] _updateRenderedComponent: (transaction, context) => {
// [ts] let prevComponentInstance = _._renderedComponent;
// [ts] let prevRenderedElement = prevComponentInstance._currentElement;
// [ts] let nextRenderedElement = _._renderValidatedComponent();
// [ts] let debugID = 0;
// [ts] if ("development" != "production") {
// [ts] debugID = _._debugID;
// [ts] }
// [ts] if (shouldUpdateReactComponent(prevRenderedElement, nextRenderedElement)) {
// [ts] ReactReconciler.receiveComponent(prevComponentInstance, nextRenderedElement, transaction, _._processChildContext(context));
// [ts] } else {
// [ts] let oldHostNode = ReactReconciler.getHostNode(prevComponentInstance);
// [ts] ReactReconciler.unmountComponent(prevComponentInstance, false);
// [ts] let nodeType = ReactNodeTypes.getType(nextRenderedElement);
// [ts] _._renderedNodeType = nodeType;
// [ts] let child = _._instantiateReactComponent(nextRenderedElement, nodeType != ReactNodeTypes.EMPTY /* shouldHaveDebugID */
// [ts] );
// [ts] _._renderedComponent = child;
// [ts] let nextMarkup = ReactReconciler.mountComponent(child, transaction, _._hostParent, _._hostContainerInfo, _._processChildContext(context), debugID);
// [ts] if ("development" != "production") {
// [ts] if (debugID != 0) {
// [ts] let childDebugIDs = if (child._debugID != 0) [child._debugID] else [];
// [ts] ReactInstrumentation.debugTool.onSetChildren(debugID, childDebugIDs);
// [ts] }
// [ts] }
// [ts] _._replaceNodeWithMarkup(oldHostNode, nextMarkup, prevComponentInstance);
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Overridden in shallow rendering.
// [ts] *
// [ts] * @protected
// [ts] */
// [ts] _replaceNodeWithMarkup: (oldHostNode, nextMarkup, prevInstance) => {
// [ts] ReactComponentEnvironment.replaceNodeWithMarkup(oldHostNode, nextMarkup, prevInstance);
// [ts] },
// [ts] /**
// [ts] * @protected
// [ts] */
// [ts] _renderValidatedComponentWithoutOwnerOrContext: () => {
// [ts] let inst = _._instance;
// [ts] let renderedElement;
// [ts] if ("development" != "production") {
// [ts] renderedElement = measureLifeCyclePerf(() => {
// [ts] return inst.render();
// [ts] }, _._debugID, "render");
// [ts] } else {
// [ts] renderedElement = inst.render();
// [ts] }
// [ts] if ("development" != "production") {
// We allow auto-mocks to proceed as if they're returning null.
// [ts] if (renderedElement == null && inst.render._isMockFunction) {
// This is probably bad practice. Consider warning here and
// deprecating this convenience.
// [ts] renderedElement = null;
// [ts] }
// [ts] }
// [ts] return renderedElement;
// [ts] },
// [ts] /**
// [ts] * @private
// [ts] */
// [ts] _renderValidatedComponent: () => {
// [ts] let renderedElement;
// [ts] if ("development" != "production" || _._compositeType != CompositeTypes.StatelessFunctional) {
// [ts] ReactCurrentOwner.current = this;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] renderedElement = _._renderValidatedComponentWithoutOwnerOrContext();
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentOwner.current = null;
// [ts] }
// [ts] } else {
// [ts] renderedElement = _._renderValidatedComponentWithoutOwnerOrContext();
// [ts] }
// [ts] !(
// TODO: An `isValidNode` function would probably be more appropriate
// [ts] renderedElement == null || renderedElement == false || React.isValidElement(renderedElement)) ? "development" != "production" ? invariant(false, "%s.render(): A valid React element (or null) must be returned. You may have returned undefined, an array or some other invalid object.", _.getName() || "ReactCompositeComponent") : _prodInvariant("109", _.getName() || "ReactCompositeComponent") : void 0;
// [ts] return renderedElement;
// [ts] },
// [ts] /**
// [ts] * Lazily allocates the refs object and stores `component` as `ref`.
// [ts] *
// [ts] * @param {string} ref Reference name.
// [ts] * @param {component} component Component to store as `ref`.
// [ts] * @final
// [ts] * @private
// [ts] */
// [ts] attachRef: (ref, component) => {
// [ts] let inst = _.getPublicInstance();
// [ts] if (!(inst != null)) if ("development" != "production") invariant(false, "Stateless function components cannot have refs.") else _prodInvariant("110") else void 0;
// [ts] let publicComponentInstance = component.getPublicInstance();
// [ts] if ("development" != "production") {
// [ts] let componentName = if (component && component.getName) component.getName() else "a component";
// [ts] if ("development" != "production") warning(publicComponentInstance != null || component._compositeType != CompositeTypes.StatelessFunctional, "Stateless function components cannot be given refs " + "(See ref \"%s\" in %s created by %s). " + "Attempts to access this ref will fail.", ref, componentName, _.getName()) else void 0;
// [ts] }
// [ts] let refs = if (inst.refs == emptyObject) inst.refs = {} else inst.refs;
// [ts] refs[ref] = publicComponentInstance;
// [ts] },
// [ts] /**
// [ts] * Detaches a reference name.
// [ts] *
// [ts] * @param {string} ref Name to dereference.
// [ts] * @final
// [ts] * @private
// [ts] */
// [ts] detachRef: (ref) => {
// [ts] let refs = _.getPublicInstance().refs;
// [ts] delete refs[ref];
// [ts] },
// [ts] /**
// [ts] * Get a text description of the component that can be used to identify it
// [ts] * in error messages.
// [ts] * @return {string} The name or null.
// [ts] * @internal
// [ts] */
// [ts] getName: () => {
// [ts] let type = _._currentElement.type;
// [ts] let constructor = _._instance && _._instance.constructor;
// [ts] return type.displayName || constructor && constructor.displayName || type.name || constructor && constructor.name || null;
// [ts] },
// [ts] /**
// [ts] * Get the publicly accessible representation of this component - i.e. what
// [ts] * is exposed by refs and returned by render. Can be null for stateless
// [ts] * components.
// [ts] *
// [ts] * @return {ReactComponent} the public component instance.
// [ts] * @internal
// [ts] */
// [ts] getPublicInstance: () => {
// [ts] let inst = _._instance;
// [ts] if (_._compositeType == CompositeTypes.StatelessFunctional) {
// [ts] return null;
// [ts] }
// [ts] return inst;
// [ts] },
// Stub
// [ts] _instantiateReactComponent: null
// [ts] };
// [ts] module.exports = ReactCompositeComponent;
// [ts] },{"104":104,"126":126,"130":130,"134":134,"135":135,"145":145,"152":152,"158":158,"159":159,"160":160,"29":29,"55":55,"63":63,"64":64,"69":69,"75":75}],31:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] /* globals __REACT_DEVTOOLS_GLOBAL_HOOK__*/
// [ts] "use strict";
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactDefaultInjection = _dereq_(52);
// [ts] let ReactMount = _dereq_(67);
// [ts] let ReactReconciler = _dereq_(75);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let ReactVersion = _dereq_(83);
// [ts] let findDOMNode = _dereq_(108);
// [ts] let getHostComponentFromComposite = _dereq_(115);
// [ts] let renderSubtreeIntoContainer = _dereq_(127);
// [ts] let warning = _dereq_(159);
// [ts] ReactDefaultInjection.inject();
// [ts] let ReactDOM = {
// [ts] findDOMNode: findDOMNode,
// [ts] render: ReactMount.render,
// [ts] unmountComponentAtNode: ReactMount.unmountComponentAtNode,
// [ts] version: ReactVersion,
// [ts] /* eslint-disable camelcase */
// [ts] unstable_batchedUpdates: ReactUpdates.batchedUpdates,
// [ts] unstable_renderSubtreeIntoContainer: renderSubtreeIntoContainer
// [ts] /* eslint-enable camelcase */
// [ts] };
// Inject the runtime into a devtools global hook regardless of browser.
// Allows for debugging when the hook is injected on the page.
// [ts] if (@typeName(__REACT_DEVTOOLS_GLOBAL_HOOK__) != "undefined" && @typeName(__REACT_DEVTOOLS_GLOBAL_HOOK__.inject) == "function") {
// [ts] __REACT_DEVTOOLS_GLOBAL_HOOK__.inject({
// [ts] ComponentTree: {
// [ts] getClosestInstanceFromNode: ReactDOMComponentTree.getClosestInstanceFromNode,
// [ts] getNodeFromInstance: (inst) => {
// inst is an internal instance (but could be a composite)
// [ts] if (inst._renderedComponent) {
// [ts] inst = getHostComponentFromComposite(inst);
// [ts] }
// [ts] if (inst) {
// [ts] return ReactDOMComponentTree.getNodeFromInstance(inst);
// [ts] } else {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] },
// [ts] Mount: ReactMount,
// [ts] Reconciler: ReactReconciler
// [ts] });
// [ts] }
// [ts] if ("development" != "production") {
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] if (ExecutionEnvironment.canUseDOM && window.top == window.self) {
// First check if devtools is not installed
// [ts] if (@typeName(__REACT_DEVTOOLS_GLOBAL_HOOK__) == "undefined") {
// If we're in Chrome or Firefox, provide a download link if not installed.
// [ts] if (navigator.userAgent.indexOf("Chrome") > -1 && navigator.userAgent.indexOf("Edge") == -1 || navigator.userAgent.indexOf("Firefox") > -1) {
// Firefox does not have the issue with devtools loaded over file://
// [ts] let showFileUrlMessage = window.location.protocol.indexOf("http") == -1 && navigator.userAgent.indexOf("Firefox") == -1;
// [ts] console.debug("Download the React DevTools " + (if (showFileUrlMessage) "and use an HTTP server (instead of a file: URL) " else "") + "for a better development experience: " + "https://fb.me/react-devtools");
// [ts] }
// [ts] }
// [ts] let testFunc = () => {};
// [ts] if ("development" != "production") warning((testFunc.name || testFunc.toString()).indexOf("testFn") != -1, "It looks like you're using a minified copy of the development build " + "of React. When deploying React apps to production, make sure to use " + "the production build which skips development warnings and is faster. " + "See https://fb.me/react-minification for more details.") else void 0;
// If we're in IE8, check to see if we are in compatibility mode and provide
// information on preventing compatibility mode
// [ts] let ieCompatibilityMode = document.documentMode && document.documentMode < 8;
// [ts] if ("development" != "production") warning(!ieCompatibilityMode, "Internet Explorer is running in compatibility mode; please add the " + "following tag to your HTML to prevent this from happening: " + "<meta http-equiv=\"X-UA-Compatible\" content=\"IE=edge\" />") else void 0;
// [ts] let expectedFeatures = [
// shims
// [ts] Array.isArray, Array.prototype.every, Array.prototype.forEach, Array.prototype.indexOf, Array.prototype.map, Date.now, Function.prototype.bind, Object.keys, String.prototype.trim];
// [ts] let i = 0;
// [ts] while (i < expectedFeatures.length) { // [cap-migrate] for-step: i++
// [ts] if (!expectedFeatures[i]) {
// [ts] if ("development" != "production") warning(false, "One or more ES5 shims expected by React are not available: " + "https://fb.me/react-warning-polyfills") else void 0;
// [ts] break;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] if ("development" != "production") {
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let ReactDOMUnknownPropertyHook = _dereq_(49);
// [ts] let ReactDOMNullInputValuePropHook = _dereq_(41);
// [ts] let ReactDOMInvalidARIAHook = _dereq_(40);
// [ts] ReactInstrumentation.debugTool.addHook(ReactDOMUnknownPropertyHook);
// [ts] ReactInstrumentation.debugTool.addHook(ReactDOMNullInputValuePropHook);
// [ts] ReactInstrumentation.debugTool.addHook(ReactDOMInvalidARIAHook);
// [ts] }
// [ts] module.exports = ReactDOM;
// [ts] },{"108":108,"115":115,"127":127,"138":138,"159":159,"34":34,"40":40,"41":41,"49":49,"52":52,"64":64,"67":67,"75":75,"82":82,"83":83}],32:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] /* global hasOwnProperty:true */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] let AutoFocusUtils = _dereq_(2);
// [ts] let CSSPropertyOperations = _dereq_(5);
// [ts] let DOMLazyTree = _dereq_(9);
// [ts] let DOMNamespaces = _dereq_(10);
// [ts] let DOMProperty = _dereq_(11);
// [ts] let DOMPropertyOperations = _dereq_(12);
// [ts] let EventPluginHub = _dereq_(17);
// [ts] let EventPluginRegistry = _dereq_(18);
// [ts] let ReactBrowserEventEmitter = _dereq_(26);
// [ts] let ReactDOMComponentFlags = _dereq_(33);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactDOMInput = _dereq_(39);
// [ts] let ReactDOMOption = _dereq_(42);
// [ts] let ReactDOMSelect = _dereq_(43);
// [ts] let ReactDOMTextarea = _dereq_(46);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let ReactMultiChild = _dereq_(68);
// [ts] let ReactServerRenderingTransaction = _dereq_(77);
// [ts] let emptyFunction = _dereq_(144);
// [ts] let escapeTextContentForBrowser = _dereq_(107);
// [ts] let invariant = _dereq_(152);
// [ts] let isEventSupported = _dereq_(122);
// [ts] let shallowEqual = _dereq_(158);
// [ts] let inputValueTracking = _dereq_(120);
// [ts] let validateDOMNesting = _dereq_(132);
// [ts] let warning = _dereq_(159);
// [ts] let Flags = ReactDOMComponentFlags;
// [ts] let deleteListener = EventPluginHub.deleteListener;
// [ts] let getNode = ReactDOMComponentTree.getNodeFromInstance;
// [ts] let listenTo = ReactBrowserEventEmitter.listenTo;
// [ts] let registrationNameModules = EventPluginRegistry.registrationNameModules;
// For quickly matching children type, to test if can be treated as content.
// [ts] let CONTENT_TYPES = { string: true, number: true };
// [ts] let STYLE = "style";
// [ts] let HTML = "__html";
// [ts] let RESERVED_PROPS = {
// [ts] children: null,
// [ts] dangerouslySetInnerHTML: null,
// [ts] suppressContentEditableWarning: null
// [ts] };
// Node type for document fragments (Node.DOCUMENT_FRAGMENT_NODE).
// [ts] let DOC_FRAGMENT_TYPE = 11;
// [ts] fn getDeclarationErrorAddendum(internalInstance: auto) {
// [ts] if (internalInstance) {
// [ts] let owner = internalInstance._currentElement._owner || null;
// [ts] if (owner) {
// [ts] let name = owner.getName();
// [ts] if (name) {
// [ts] return " This DOM node was rendered by `" + name + "`.";
// [ts] }
// [ts] }
// [ts] }
// [ts] return "";
// [ts] }
// [ts] fn friendlyStringify(obj: auto) {
// [ts] if (@typeName(obj) == "object") {
// [ts] if (Array.isArray(obj)) {
// [ts] return "[" + obj.map(friendlyStringify).join(", ") + "]";
// [ts] } else {
// [ts] let pairs = [];
// [ts] for (let key in obj) {
// [ts] if (Object.prototype.hasOwnProperty.call(obj, key)) {
// [ts] let keyEscaped = if ("^[a-z$_][\\w$_]*$".test(key)) key else JSON.stringify(key);
// [ts] pairs.push(keyEscaped + ": " + friendlyStringify(obj[key]));
// [ts] }
// [ts] }
// [ts] return "{" + pairs.join(", ") + "}";
// [ts] }
// [ts] } else if (@typeName(obj) == "string") {
// [ts] return JSON.stringify(obj);
// [ts] } else if (@typeName(obj) == "function") {
// [ts] return "[function object]";
// [ts] }
// Differs from JSON.stringify in that undefined because undefined and that
// inf and nan don't become null
// [ts] return String(obj);
// [ts] }
// [ts] let styleMutationWarning = {};
// [ts] fn checkAndWarnForMutatedStyle(style1: auto, style2: auto, component: auto) {
// [ts] if (style1 == null || style2 == null) {
// [ts] return;
// [ts] }
// [ts] if (shallowEqual(style1, style2)) {
// [ts] return;
// [ts] }
// [ts] let componentName = component._tag;
// [ts] let owner = component._currentElement._owner;
// [ts] let ownerName;
// [ts] if (owner) {
// [ts] ownerName = owner.getName();
// [ts] }
// [ts] let hash = ownerName + "|" + componentName;
// [ts] if (styleMutationWarning.hasOwnProperty(hash)) {
// [ts] return;
// [ts] }
// [ts] styleMutationWarning[hash] = true;
// [ts] if ("development" != "production") warning(if (false, "`%s` was passed a style object that has previously been mutated. " + "Mutating `style` is deprecated. Consider cloning it beforehand. Check " + "the `render` %s. Previous style: %s. Mutated style: %s.", componentName, owner) "of `" + ownerName + "`" else "using <" + componentName + ">", friendlyStringify(style1), friendlyStringify(style2)) else void 0;
// [ts] }
// [ts] /**
// [ts] * @param {object} component
// [ts] * @param {?object} props
// [ts] */
// [ts] fn assertValidProps(component: auto, props: auto) {
// [ts] if (!props) {
// [ts] return;
// [ts] }
// Note the use of `==` which checks for null or undefined.
// [ts] if (voidElementTags[component._tag]) {
// [ts] if (!(props.children == null && props.dangerouslySetInnerHTML == null)) if ("development" != "production") invariant(if (false, "%s is a void element tag and must neither have `children` nor use `dangerouslySetInnerHTML`.%s", component._tag, component._currentElement._owner) " Check the render method of " + component._currentElement._owner.getName() + "." else "") else _prodInvariant(if ("137", component._tag, component._currentElement._owner) " Check the render method of " + component._currentElement._owner.getName() + "." else "") else void 0;
// [ts] }
// [ts] if (props.dangerouslySetInnerHTML != null) {
// [ts] if (!(props.children == null)) if ("development" != "production") invariant(false, "Can only set one of `children` or `props.dangerouslySetInnerHTML`.") else _prodInvariant("60") else void 0;
// [ts] if (!(@typeName(props.dangerouslySetInnerHTML) == "object" && HTML == props.dangerouslySetInnerHTML)) if ("development" != "production") invariant(false, "`props.dangerouslySetInnerHTML` must be in the form `{__html: ...}`. Please visit https://fb.me/react-invariant-dangerously-set-inner-html for more information.") else _prodInvariant("61") else void 0;
// [ts] }
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(props.innerHTML == null, "Directly setting property `innerHTML` is not permitted. " + "For more information, lookup documentation on `dangerouslySetInnerHTML`.") else void 0;
// [ts] if ("development" != "production") warning(props.suppressContentEditableWarning || !props.contentEditable || props.children == null, "A component is `contentEditable` and contains `children` managed by " + "React. It is now your responsibility to guarantee that none of " + "those nodes are unexpectedly modified or duplicated. This is " + "probably not intentional.") else void 0;
// [ts] if ("development" != "production") warning(props.onFocusIn == null && props.onFocusOut == null, "React uses onFocus and onBlur instead of onFocusIn and onFocusOut. " + "All React events are normalized to bubble, so onFocusIn and onFocusOut " + "are not needed/supported by React.") else void 0;
// [ts] }
// [ts] if (!(props.style == null || @typeName(props.style) == "object")) if ("development" != "production") invariant(false, "The `style` prop expects a mapping from style properties to values, not a string. For example, style={{marginRight: spacing + 'em'}} when using JSX.%s", getDeclarationErrorAddendum(component)) else _prodInvariant("62", getDeclarationErrorAddendum(component)) else void 0;
// [ts] }
// [ts] fn enqueuePutListener(inst: auto, registrationName: auto, listener: auto, transaction: auto) {
// [ts] if (transaction == ReactServerRenderingTransaction) {
// [ts] return;
// [ts] }
// [ts] if ("development" != "production") {
// IE8 has no API for event capturing and the `onScroll` event doesn't
// bubble.
// [ts] if ("development" != "production") warning(registrationName != "onScroll" || isEventSupported("scroll", true), "This browser doesn't support the `onScroll` event") else void 0;
// [ts] }
// [ts] let containerInfo = inst._hostContainerInfo;
// [ts] let isDocumentFragment = containerInfo._node && containerInfo._node.nodeType == DOC_FRAGMENT_TYPE;
// [ts] let doc = if (isDocumentFragment) containerInfo._node else containerInfo._ownerDocument;
// [ts] listenTo(registrationName, doc);
// [ts] transaction.getReactMountReady().enqueue(putListener, {
// [ts] inst: inst,
// [ts] registrationName: registrationName,
// [ts] listener: listener
// [ts] });
// [ts] }
// [ts] fn putListener() {
// [ts] let listenerToPut = this;
// [ts] EventPluginHub.putListener(listenerToPut.inst, listenerToPut.registrationName, listenerToPut.listener);
// [ts] }
// [ts] fn inputPostMount() {
// [ts] let inst = this;
// [ts] ReactDOMInput.postMountWrapper(inst);
// [ts] }
// [ts] fn textareaPostMount() {
// [ts] let inst = this;
// [ts] ReactDOMTextarea.postMountWrapper(inst);
// [ts] }
// [ts] fn optionPostMount() {
// [ts] let inst = this;
// [ts] ReactDOMOption.postMountWrapper(inst);
// [ts] }
// [ts] let setAndValidateContentChildDev = emptyFunction;
// [ts] if ("development" != "production") {
// [ts] setAndValidateContentChildDev = (content) => {
// [ts] let hasExistingContent = _._contentDebugID != null;
// [ts] let debugID = _._debugID;
// This ID represents the inlined child that has no backing instance:
// [ts] let contentDebugID = -debugID;
// [ts] if (content == null) {
// [ts] if (hasExistingContent) {
// [ts] ReactInstrumentation.debugTool.onUnmountComponent(_._contentDebugID);
// [ts] }
// [ts] _._contentDebugID = null;
// [ts] return;
// [ts] }
// [ts] validateDOMNesting(null, String(content), this, _._ancestorInfo);
// [ts] _._contentDebugID = contentDebugID;
// [ts] if (hasExistingContent) {
// [ts] ReactInstrumentation.debugTool.onBeforeUpdateComponent(contentDebugID, content);
// [ts] ReactInstrumentation.debugTool.onUpdateComponent(contentDebugID);
// [ts] } else {
// [ts] ReactInstrumentation.debugTool.onBeforeMountComponent(contentDebugID, content, debugID);
// [ts] ReactInstrumentation.debugTool.onMountComponent(contentDebugID);
// [ts] ReactInstrumentation.debugTool.onSetChildren(debugID, [contentDebugID]);
// [ts] }
// [ts] };
// [ts] }
// There are so many media events, it makes sense to just
// maintain a list rather than create a `trapBubbledEvent` for each
// [ts] let mediaEvents = {
// [ts] topAbort: "abort",
// [ts] topCanPlay: "canplay",
// [ts] topCanPlayThrough: "canplaythrough",
// [ts] topDurationChange: "durationchange",
// [ts] topEmptied: "emptied",
// [ts] topEncrypted: "encrypted",
// [ts] topEnded: "ended",
// [ts] topError: "error",
// [ts] topLoadedData: "loadeddata",
// [ts] topLoadedMetadata: "loadedmetadata",
// [ts] topLoadStart: "loadstart",
// [ts] topPause: "pause",
// [ts] topPlay: "play",
// [ts] topPlaying: "playing",
// [ts] topProgress: "progress",
// [ts] topRateChange: "ratechange",
// [ts] topSeeked: "seeked",
// [ts] topSeeking: "seeking",
// [ts] topStalled: "stalled",
// [ts] topSuspend: "suspend",
// [ts] topTimeUpdate: "timeupdate",
// [ts] topVolumeChange: "volumechange",
// [ts] topWaiting: "waiting"
// [ts] };
// [ts] fn trackInputValue() {
// [ts] inputValueTracking.track(this);
// [ts] }
// [ts] fn trapBubbledEventsLocal() {
// [ts] let inst = this;
// If a component renders to null or if another component fatals and causes
// the state of the tree to be corrupted, `node` here can be null.
// [ts] if (!inst._rootNodeID) if ("development" != "production") invariant(false, "Must be mounted to trap events") else _prodInvariant("63") else void 0;
// [ts] let node = getNode(inst);
// [ts] if (!node) if ("development" != "production") invariant(false, "trapBubbledEvent(...): Requires node to be rendered.") else _prodInvariant("64") else void 0;
// [ts] switch (inst._tag) {
// [ts] "iframe", "object" => {
// [ts] inst._wrapperState.listeners = [ReactBrowserEventEmitter.trapBubbledEvent("topLoad", "load", node)];
// break;
// [ts] },
// [ts] "video", "audio" => {
// [ts] inst._wrapperState.listeners = [];
// Create listener for each media event
// [ts] for (let event in mediaEvents) {
// [ts] if (mediaEvents.hasOwnProperty(event)) {
// [ts] inst._wrapperState.listeners.push(ReactBrowserEventEmitter.trapBubbledEvent(event, mediaEvents[event], node));
// [ts] }
// [ts] }
// break;
// [ts] },
// [ts] "source" => {
// [ts] inst._wrapperState.listeners = [ReactBrowserEventEmitter.trapBubbledEvent("topError", "error", node)];
// break;
// [ts] },
// [ts] "img" => {
// [ts] inst._wrapperState.listeners = [ReactBrowserEventEmitter.trapBubbledEvent("topError", "error", node), ReactBrowserEventEmitter.trapBubbledEvent("topLoad", "load", node)];
// break;
// [ts] },
// [ts] "form" => {
// [ts] inst._wrapperState.listeners = [ReactBrowserEventEmitter.trapBubbledEvent("topReset", "reset", node), ReactBrowserEventEmitter.trapBubbledEvent("topSubmit", "submit", node)];
// break;
// [ts] },
// [ts] "input", "select", "textarea" => {
// [ts] inst._wrapperState.listeners = [ReactBrowserEventEmitter.trapBubbledEvent("topInvalid", "invalid", node)];
// break;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn postUpdateSelectWrapper() {
// [ts] ReactDOMSelect.postUpdateWrapper(this);
// [ts] }
// For HTML, certain tags should omit their close tag. We keep a whitelist for
// those special-case tags.
// [ts] let omittedCloseTags = {
// [ts] area: true,
// [ts] base: true,
// [ts] br: true,
// [ts] col: true,
// [ts] embed: true,
// [ts] hr: true,
// [ts] img: true,
// [ts] input: true,
// [ts] keygen: true,
// [ts] link: true,
// [ts] meta: true,
// [ts] param: true,
// [ts] source: true,
// [ts] track: true,
// [ts] wbr: true
// NOTE: menuitem's close tag should be omitted, but that causes problems.
// [ts] };
// [ts] let newlineEatingTags = {
// [ts] listing: true,
// [ts] pre: true,
// [ts] textarea: true
// [ts] };
// For HTML, certain tags cannot have children. This has the same purpose as
// `omittedCloseTags` except that `menuitem` should still have its closing tag.
// [ts] let voidElementTags = _assign({
// [ts] menuitem: true
// [ts] }, omittedCloseTags);
// We accept any tag to be rendered but since this gets injected into arbitrary
// HTML, we want to make sure that it's a safe tag.
// http://www.w3.org/TR/REC-xml/#NT-Name
// [ts] let VALID_TAG_REGEX = "^[a-zA-Z][a-zA-Z:_\\.\\-\\d]*$"; // Simplified subset
// [ts] let validatedTagCache = {};
// [ts] let hasOwnProperty = {}.hasOwnProperty;
// [ts] fn validateDangerousTag(tag: auto) {
// [ts] if (!hasOwnProperty.call(validatedTagCache, tag)) {
// [ts] if (!VALID_TAG_REGEX.test(tag)) if ("development" != "production") invariant(false, "Invalid tag: %s", tag) else _prodInvariant("65", tag) else void 0;
// [ts] validatedTagCache[tag] = true;
// [ts] }
// [ts] }
// [ts] fn isCustomComponent(tagName: auto, props: auto) {
// [ts] return tagName.indexOf("-") >= 0 || props.is != null;
// [ts] }
// [ts] let globalIdCounter = 1;
// [ts] /**
// [ts] * Creates a new React class that is idempotent and capable of containing other
// [ts] * React components. It accepts event listeners and DOM properties that are
// [ts] * valid according to `DOMProperty`.
// [ts] *
// [ts] *  - Event listeners: `onClick`, `onMouseDown`, etc.
// [ts] *  - DOM properties: `className`, `name`, `title`, etc.
// [ts] *
// [ts] * The `style` property functions differently from the DOM API. It accepts an
// [ts] * object mapping of style properties to values.
// [ts] *
// [ts] * @constructor ReactDOMComponent
// [ts] * @extends ReactMultiChild
// [ts] */
// [ts] fn ReactDOMComponent(element: auto) {
// [ts] let tag = element.type;
// [ts] validateDangerousTag(tag);
// [ts] _._currentElement = element;
// [ts] _._tag = tag.toLowerCase();
// [ts] _._namespaceURI = null;
// [ts] _._renderedChildren = null;
// [ts] _._previousStyle = null;
// [ts] _._previousStyleCopy = null;
// [ts] _._hostNode = null;
// [ts] _._hostParent = null;
// [ts] _._rootNodeID = 0;
// [ts] _._domID = 0;
// [ts] _._hostContainerInfo = null;
// [ts] _._wrapperState = null;
// [ts] _._topLevelWrapper = null;
// [ts] _._flags = 0;
// [ts] if ("development" != "production") {
// [ts] _._ancestorInfo = null;
// [ts] setAndValidateContentChildDev.call(this, null);
// [ts] }
// [ts] }
// [ts] ReactDOMComponent.displayName = "ReactDOMComponent";
// [ts] ReactDOMComponent.Mixin = {
// [ts] /**
// [ts] * Generates root tag markup then recurses. This method has side effects and
// [ts] * is not idempotent.
// [ts] *
// [ts] * @internal
// [ts] * @param {ReactReconcileTransaction|ReactServerRenderingTransaction} transaction
// [ts] * @param {?ReactDOMComponent} the parent component instance
// [ts] * @param {?object} info about the host container
// [ts] * @param {object} context
// [ts] * @return {string} The computed markup.
// [ts] */
// [ts] mountComponent: (transaction, hostParent, hostContainerInfo, context) => {
// [ts] _._rootNodeID = globalIdCounter++;
// [ts] _._domID = hostContainerInfo._idCounter++;
// [ts] _._hostParent = hostParent;
// [ts] _._hostContainerInfo = hostContainerInfo;
// [ts] let props = _._currentElement.props;
// [ts] switch (_._tag) {
// [ts] "audio", "form", "iframe", "img", "link", "object", "source", "video" => {
// [ts] _._wrapperState = {
// [ts] listeners: null
// [ts] };
// [ts] transaction.getReactMountReady().enqueue(trapBubbledEventsLocal, this);
// break;
// [ts] },
// [ts] "input" => {
// [ts] ReactDOMInput.mountWrapper(this, props, hostParent);
// [ts] props = ReactDOMInput.getHostProps(this, props);
// [ts] transaction.getReactMountReady().enqueue(trackInputValue, this);
// [ts] transaction.getReactMountReady().enqueue(trapBubbledEventsLocal, this);
// break;
// [ts] },
// [ts] "option" => {
// [ts] ReactDOMOption.mountWrapper(this, props, hostParent);
// [ts] props = ReactDOMOption.getHostProps(this, props);
// break;
// [ts] },
// [ts] "select" => {
// [ts] ReactDOMSelect.mountWrapper(this, props, hostParent);
// [ts] props = ReactDOMSelect.getHostProps(this, props);
// [ts] transaction.getReactMountReady().enqueue(trapBubbledEventsLocal, this);
// break;
// [ts] },
// [ts] "textarea" => {
// [ts] ReactDOMTextarea.mountWrapper(this, props, hostParent);
// [ts] props = ReactDOMTextarea.getHostProps(this, props);
// [ts] transaction.getReactMountReady().enqueue(trackInputValue, this);
// [ts] transaction.getReactMountReady().enqueue(trapBubbledEventsLocal, this);
// break;
// [ts] }
// [ts] }
// [ts] assertValidProps(this, props);
// We create tags in the namespace of their parent container, except HTML
// tags get no namespace.
// [ts] let namespaceURI;
// [ts] let parentTag;
// [ts] if (hostParent != null) {
// [ts] namespaceURI = hostParent._namespaceURI;
// [ts] parentTag = hostParent._tag;
// [ts] } else if (hostContainerInfo._tag) {
// [ts] namespaceURI = hostContainerInfo._namespaceURI;
// [ts] parentTag = hostContainerInfo._tag;
// [ts] }
// [ts] if (namespaceURI == null || namespaceURI == DOMNamespaces.svg && parentTag == "foreignobject") {
// [ts] namespaceURI = DOMNamespaces.html;
// [ts] }
// [ts] if (namespaceURI == DOMNamespaces.html) {
// [ts] if (_._tag == "svg") {
// [ts] namespaceURI = DOMNamespaces.svg;
// [ts] } else if (_._tag == "math") {
// [ts] namespaceURI = DOMNamespaces.mathml;
// [ts] }
// [ts] }
// [ts] _._namespaceURI = namespaceURI;
// [ts] if ("development" != "production") {
// [ts] let parentInfo;
// [ts] if (hostParent != null) {
// [ts] parentInfo = hostParent._ancestorInfo;
// [ts] } else if (hostContainerInfo._tag) {
// [ts] parentInfo = hostContainerInfo._ancestorInfo;
// [ts] }
// [ts] if (parentInfo) {
// parentInfo should always be present except for the top-level
// component when server rendering
// [ts] validateDOMNesting(_._tag, null, this, parentInfo);
// [ts] }
// [ts] _._ancestorInfo = validateDOMNesting.updatedAncestorInfo(parentInfo, _._tag, this);
// [ts] }
// [ts] let mountImage;
// [ts] if (transaction.useCreateElement) {
// [ts] let ownerDocument = hostContainerInfo._ownerDocument;
// [ts] let el;
// [ts] if (namespaceURI == DOMNamespaces.html) {
// [ts] if (_._tag == "script") {
// Create the script via .innerHTML so its "parser-inserted" flag is
// set to true and it does not execute
// [ts] let div = ownerDocument.createElement("div");
// [ts] let type = _._currentElement.type;
// [ts] div.innerHTML = "<" + type + "></" + type + ">";
// [ts] el = div.removeChild(div.firstChild);
// [ts] } else if (props.is) {
// [ts] el = ownerDocument.createElement(_._currentElement.type, props.is);
// [ts] } else {
// Separate else branch instead of using `props.is || undefined` above becuase of a Firefox bug.
// See discussion in https://github.com/facebook/react/pull/6896
// and discussion in https://bugzilla.mozilla.org/show_bug.cgi?id=1276240
// [ts] el = ownerDocument.createElement(_._currentElement.type);
// [ts] }
// [ts] } else {
// [ts] el = ownerDocument.createElementNS(namespaceURI, _._currentElement.type);
// [ts] }
// [ts] ReactDOMComponentTree.precacheNode(this, el);
// [ts] _._flags |= Flags.hasCachedChildNodes;
// [ts] if (!_._hostParent) {
// [ts] DOMPropertyOperations.setAttributeForRoot(el);
// [ts] }
// [ts] _._updateDOMProperties(null, props, transaction);
// [ts] let lazyTree = DOMLazyTree(el);
// [ts] _._createInitialChildren(transaction, props, context, lazyTree);
// [ts] mountImage = lazyTree;
// [ts] } else {
// [ts] let tagOpen = _._createOpenTagMarkupAndPutListeners(transaction, props);
// [ts] let tagContent = _._createContentMarkup(transaction, props, context);
// [ts] if (!tagContent && omittedCloseTags[_._tag]) {
// [ts] mountImage = tagOpen + "/>";
// [ts] } else {
// [ts] mountImage = tagOpen + ">" + tagContent + "</" + _._currentElement.type + ">";
// [ts] }
// [ts] }
// [ts] switch (_._tag) {
// [ts] "input" => {
// [ts] transaction.getReactMountReady().enqueue(inputPostMount, this);
// [ts] if (props.autoFocus) {
// [ts] transaction.getReactMountReady().enqueue(AutoFocusUtils.focusDOMComponent, this);
// [ts] }
// break;
// [ts] },
// [ts] "textarea" => {
// [ts] transaction.getReactMountReady().enqueue(textareaPostMount, this);
// [ts] if (props.autoFocus) {
// [ts] transaction.getReactMountReady().enqueue(AutoFocusUtils.focusDOMComponent, this);
// [ts] }
// break;
// [ts] },
// [ts] "select" => {
// [ts] if (props.autoFocus) {
// [ts] transaction.getReactMountReady().enqueue(AutoFocusUtils.focusDOMComponent, this);
// [ts] }
// break;
// [ts] },
// [ts] "button" => {
// [ts] if (props.autoFocus) {
// [ts] transaction.getReactMountReady().enqueue(AutoFocusUtils.focusDOMComponent, this);
// [ts] }
// break;
// [ts] },
// [ts] "option" => {
// [ts] transaction.getReactMountReady().enqueue(optionPostMount, this);
// break;
// [ts] }
// [ts] }
// [ts] return mountImage;
// [ts] },
// [ts] /**
// [ts] * Creates markup for the open tag and all attributes.
// [ts] *
// [ts] * This method has side effects because events get registered.
// [ts] *
// [ts] * Iterating over object properties is faster than iterating over arrays.
// [ts] * @see http://jsperf.com/obj-vs-arr-iteration
// [ts] *
// [ts] * @private
// [ts] * @param {ReactReconcileTransaction|ReactServerRenderingTransaction} transaction
// [ts] * @param {object} props
// [ts] * @return {string} Markup of opening tag.
// [ts] */
// [ts] _createOpenTagMarkupAndPutListeners: (transaction, props) => {
// [ts] let ret = "<" + _._currentElement.type;
// [ts] for (let propKey in props) {
// [ts] if (!props.hasOwnProperty(propKey)) {
// [ts] continue;
// [ts] }
// [ts] let propValue = props[propKey];
// [ts] if (propValue == null) {
// [ts] continue;
// [ts] }
// [ts] if (registrationNameModules.hasOwnProperty(propKey)) {
// [ts] if (propValue) {
// [ts] enqueuePutListener(this, propKey, propValue, transaction);
// [ts] }
// [ts] } else {
// [ts] if (propKey == STYLE) {
// [ts] if (propValue) {
// [ts] if ("development" != "production") {
// See `_updateDOMProperties`. style block
// [ts] _._previousStyle = propValue;
// [ts] }
// [ts] propValue = _._previousStyleCopy = _assign({}, props.style);
// [ts] }
// [ts] propValue = CSSPropertyOperations.createMarkupForStyles(propValue, this);
// [ts] }
// [ts] let markup = null;
// [ts] if (_._tag != null && isCustomComponent(_._tag, props)) {
// [ts] if (!RESERVED_PROPS.hasOwnProperty(propKey)) {
// [ts] markup = DOMPropertyOperations.createMarkupForCustomAttribute(propKey, propValue);
// [ts] }
// [ts] } else {
// [ts] markup = DOMPropertyOperations.createMarkupForProperty(propKey, propValue);
// [ts] }
// [ts] if (markup) {
// [ts] ret += " " + markup;
// [ts] }
// [ts] }
// [ts] }
// For static pages, no need to put React ID and checksum. Saves lots of
// bytes.
// [ts] if (transaction.renderToStaticMarkup) {
// [ts] return ret;
// [ts] }
// [ts] if (!_._hostParent) {
// [ts] ret += " " + DOMPropertyOperations.createMarkupForRoot();
// [ts] }
// [ts] ret += " " + DOMPropertyOperations.createMarkupForID(_._domID);
// [ts] return ret;
// [ts] },
// [ts] /**
// [ts] * Creates markup for the content between the tags.
// [ts] *
// [ts] * @private
// [ts] * @param {ReactReconcileTransaction|ReactServerRenderingTransaction} transaction
// [ts] * @param {object} props
// [ts] * @param {object} context
// [ts] * @return {string} Content markup.
// [ts] */
// [ts] _createContentMarkup: (transaction, props, context) => {
// [ts] let ret = "";
// Intentional use of != to avoid catching zero/false.
// [ts] let innerHTML = props.dangerouslySetInnerHTML;
// [ts] if (innerHTML != null) {
// [ts] if (innerHTML.__html != null) {
// [ts] ret = innerHTML.__html;
// [ts] }
// [ts] } else {
// [ts] let contentToUse = if (CONTENT_TYPES[@typeName(props.children)]) props.children else null;
// [ts] let childrenToUse = if (contentToUse != null) null else props.children;
// [ts] if (contentToUse != null) {
// TODO: Validate that text is allowed as a child of this node
// [ts] ret = escapeTextContentForBrowser(contentToUse);
// [ts] if ("development" != "production") {
// [ts] setAndValidateContentChildDev.call(this, contentToUse);
// [ts] }
// [ts] } else if (childrenToUse != null) {
// [ts] let mountImages = _.mountChildren(childrenToUse, transaction, context);
// [ts] ret = mountImages.join("");
// [ts] }
// [ts] }
// [ts] if (newlineEatingTags[_._tag] && ret.charAt(0) == "\n") {
// text/html ignores the first character in these tags if it's a newline
// Prefer to break application/xml over text/html (for now) by adding
// a newline specifically to get eaten by the parser. (Alternately for
// textareas, replacing "^\n" with "\r\n" doesn't get eaten, and the first
// \r is normalized out by HTMLTextAreaElement#value.)
// See: <http://www.w3.org/TR/html-polyglot/#newlines-in-textarea-and-pre>
// See: <http://www.w3.org/TR/html5/syntax.html#element-restrictions>
// See: <http://www.w3.org/TR/html5/syntax.html#newlines>
// See: Parsing of "textarea" "listing" and "pre" elements
//  from <http://www.w3.org/TR/html5/syntax.html#parsing-main-inbody>
// [ts] return "\n" + ret;
// [ts] } else {
// [ts] return ret;
// [ts] }
// [ts] },
// [ts] _createInitialChildren: (transaction, props, context, lazyTree) => {
// Intentional use of != to avoid catching zero/false.
// [ts] let innerHTML = props.dangerouslySetInnerHTML;
// [ts] if (innerHTML != null) {
// [ts] if (innerHTML.__html != null) {
// [ts] DOMLazyTree.queueHTML(lazyTree, innerHTML.__html);
// [ts] }
// [ts] } else {
// [ts] let contentToUse = if (CONTENT_TYPES[@typeName(props.children)]) props.children else null;
// [ts] let childrenToUse = if (contentToUse != null) null else props.children;
// TODO: Validate that text is allowed as a child of this node
// [ts] if (contentToUse != null) {
// Avoid setting textContent when the text is empty. In IE11 setting
// textContent on a text area will cause the placeholder to not
// show within the textarea until it has been focused and blurred again.
// https://github.com/facebook/react/issues/6731#issuecomment-254874553
// [ts] if (contentToUse != "") {
// [ts] if ("development" != "production") {
// [ts] setAndValidateContentChildDev.call(this, contentToUse);
// [ts] }
// [ts] DOMLazyTree.queueText(lazyTree, contentToUse);
// [ts] }
// [ts] } else if (childrenToUse != null) {
// [ts] let mountImages = _.mountChildren(childrenToUse, transaction, context);
// [ts] let i = 0;
// [ts] while (i < mountImages.length) { // [cap-migrate] for-step: i++
// [ts] DOMLazyTree.queueChild(lazyTree, mountImages[i]);
// [ts] }
// [ts] }
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Receives a next element and updates the component.
// [ts] *
// [ts] * @internal
// [ts] * @param {ReactElement} nextElement
// [ts] * @param {ReactReconcileTransaction|ReactServerRenderingTransaction} transaction
// [ts] * @param {object} context
// [ts] */
// [ts] receiveComponent: (nextElement, transaction, context) => {
// [ts] let prevElement = _._currentElement;
// [ts] _._currentElement = nextElement;
// [ts] _.updateComponent(transaction, prevElement, nextElement, context);
// [ts] },
// [ts] /**
// [ts] * Updates a DOM component after it has already been allocated and
// [ts] * attached to the DOM. Reconciles the root DOM node, then recurses.
// [ts] *
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @param {ReactElement} prevElement
// [ts] * @param {ReactElement} nextElement
// [ts] * @internal
// [ts] * @overridable
// [ts] */
// [ts] updateComponent: (transaction, prevElement, nextElement, context) => {
// [ts] let lastProps = prevElement.props;
// [ts] let nextProps = _._currentElement.props;
// [ts] switch (_._tag) {
// [ts] "input" => {
// [ts] lastProps = ReactDOMInput.getHostProps(this, lastProps);
// [ts] nextProps = ReactDOMInput.getHostProps(this, nextProps);
// break;
// [ts] },
// [ts] "option" => {
// [ts] lastProps = ReactDOMOption.getHostProps(this, lastProps);
// [ts] nextProps = ReactDOMOption.getHostProps(this, nextProps);
// break;
// [ts] },
// [ts] "select" => {
// [ts] lastProps = ReactDOMSelect.getHostProps(this, lastProps);
// [ts] nextProps = ReactDOMSelect.getHostProps(this, nextProps);
// break;
// [ts] },
// [ts] "textarea" => {
// [ts] lastProps = ReactDOMTextarea.getHostProps(this, lastProps);
// [ts] nextProps = ReactDOMTextarea.getHostProps(this, nextProps);
// break;
// [ts] }
// [ts] }
// [ts] assertValidProps(this, nextProps);
// [ts] _._updateDOMProperties(lastProps, nextProps, transaction);
// [ts] _._updateDOMChildren(lastProps, nextProps, transaction, context);
// [ts] switch (_._tag) {
// Update the wrapper around inputs *after* updating props. This has to
// happen after `_updateDOMProperties`. Otherwise HTML5 input validations
// raise warnings and prevent the new value from being assigned.
// [ts] "input" => {
// [ts] ReactDOMInput.updateWrapper(this);
// We also check that we haven't missed a value update, such as a
// Radio group shifting the checked value to another named radio input.
// [ts] inputValueTracking.updateValueIfChanged(this);
// break;
// [ts] },
// [ts] "textarea" => {
// [ts] ReactDOMTextarea.updateWrapper(this);
// break;
// [ts] },
// <select> value update needs to occur after <option> children
// reconciliation
// [ts] "select" => {
// [ts] transaction.getReactMountReady().enqueue(postUpdateSelectWrapper, this);
// break;
// [ts] }
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Reconciles the properties by detecting differences in property values and
// [ts] * updating the DOM as necessary. This function is probably the single most
// [ts] * critical path for performance optimization.
// [ts] *
// [ts] * TODO: Benchmark whether checking for changed values in memory actually
// [ts] *       improves performance (especially statically positioned elements).
// [ts] * TODO: Benchmark the effects of putting this at the top since 99% of props
// [ts] *       do not change for a given reconciliation.
// [ts] * TODO: Benchmark areas that can be improved with caching.
// [ts] *
// [ts] * @private
// [ts] * @param {object} lastProps
// [ts] * @param {object} nextProps
// [ts] * @param {?DOMElement} node
// [ts] */
// [ts] _updateDOMProperties: (lastProps, nextProps, transaction) => {
// [ts] let propKey;
// [ts] let styleName;
// [ts] let styleUpdates;
// [ts] for (propKey in lastProps) {
// [ts] if (nextProps.hasOwnProperty(propKey) || !lastProps.hasOwnProperty(propKey) || lastProps[propKey] == null) {
// [ts] continue;
// [ts] }
// [ts] if (propKey == STYLE) {
// [ts] let lastStyle = _._previousStyleCopy;
// [ts] for (styleName in lastStyle) {
// [ts] if (lastStyle.hasOwnProperty(styleName)) {
// [ts] styleUpdates = styleUpdates || {};
// [ts] styleUpdates[styleName] = "";
// [ts] }
// [ts] }
// [ts] _._previousStyleCopy = null;
// [ts] } else if (registrationNameModules.hasOwnProperty(propKey)) {
// [ts] if (lastProps[propKey]) {
// Only call deleteListener if there was a listener previously or
// else willDeleteListener gets called when there wasn't actually a
// listener (e.g., onClick={null})
// [ts] deleteListener(this, propKey);
// [ts] }
// [ts] } else if (isCustomComponent(_._tag, lastProps)) {
// [ts] if (!RESERVED_PROPS.hasOwnProperty(propKey)) {
// [ts] DOMPropertyOperations.deleteValueForAttribute(getNode(this), propKey);
// [ts] }
// [ts] } else if (DOMProperty.properties[propKey] || DOMProperty.isCustomAttribute(propKey)) {
// [ts] DOMPropertyOperations.deleteValueForProperty(getNode(this), propKey);
// [ts] }
// [ts] }
// [ts] for (propKey in nextProps) {
// [ts] let nextProp = nextProps[propKey];
// [ts] let lastProp = if (propKey == STYLE) _._previousStyleCopy else if (lastProps != null) lastProps[propKey] else null;
// [ts] if (!nextProps.hasOwnProperty(propKey) || nextProp == lastProp || nextProp == null && lastProp == null) {
// [ts] continue;
// [ts] }
// [ts] if (propKey == STYLE) {
// [ts] if (nextProp) {
// [ts] if ("development" != "production") {
// [ts] checkAndWarnForMutatedStyle(_._previousStyleCopy, _._previousStyle, this);
// [ts] _._previousStyle = nextProp;
// [ts] }
// [ts] nextProp = _._previousStyleCopy = _assign({}, nextProp);
// [ts] } else {
// [ts] _._previousStyleCopy = null;
// [ts] }
// [ts] if (lastProp) {
// Unset styles on `lastProp` but not on `nextProp`.
// [ts] for (styleName in lastProp) {
// [ts] if (lastProp.hasOwnProperty(styleName) && (!nextProp || !nextProp.hasOwnProperty(styleName))) {
// [ts] styleUpdates = styleUpdates || {};
// [ts] styleUpdates[styleName] = "";
// [ts] }
// [ts] }
// Update styles that changed since `lastProp`.
// [ts] for (styleName in nextProp) {
// [ts] if (nextProp.hasOwnProperty(styleName) && lastProp[styleName] != nextProp[styleName]) {
// [ts] styleUpdates = styleUpdates || {};
// [ts] styleUpdates[styleName] = nextProp[styleName];
// [ts] }
// [ts] }
// [ts] } else {
// Relies on `updateStylesByID` not mutating `styleUpdates`.
// [ts] styleUpdates = nextProp;
// [ts] }
// [ts] } else if (registrationNameModules.hasOwnProperty(propKey)) {
// [ts] if (nextProp) {
// [ts] enqueuePutListener(this, propKey, nextProp, transaction);
// [ts] } else if (lastProp) {
// [ts] deleteListener(this, propKey);
// [ts] }
// [ts] } else if (isCustomComponent(_._tag, nextProps)) {
// [ts] if (!RESERVED_PROPS.hasOwnProperty(propKey)) {
// [ts] DOMPropertyOperations.setValueForAttribute(getNode(this), propKey, nextProp);
// [ts] }
// [ts] } else if (DOMProperty.properties[propKey] || DOMProperty.isCustomAttribute(propKey)) {
// [ts] let node = getNode(this);
// If we're updating to null or undefined, we should remove the property
// from the DOM node instead of inadvertently setting to a string. This
// brings us in line with the same behavior we have on initial render.
// [ts] if (nextProp != null) {
// [ts] DOMPropertyOperations.setValueForProperty(node, propKey, nextProp);
// [ts] } else {
// [ts] DOMPropertyOperations.deleteValueForProperty(node, propKey);
// [ts] }
// [ts] }
// [ts] }
// [ts] if (styleUpdates) {
// [ts] CSSPropertyOperations.setValueForStyles(getNode(this), styleUpdates, this);
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Reconciles the children with the various properties that affect the
// [ts] * children content.
// [ts] *
// [ts] * @param {object} lastProps
// [ts] * @param {object} nextProps
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @param {object} context
// [ts] */
// [ts] _updateDOMChildren: (lastProps, nextProps, transaction, context) => {
// [ts] let lastContent = if (CONTENT_TYPES[@typeName(lastProps.children)]) lastProps.children else null;
// [ts] let nextContent = if (CONTENT_TYPES[@typeName(nextProps.children)]) nextProps.children else null;
// [ts] let lastHtml = lastProps.dangerouslySetInnerHTML && lastProps.dangerouslySetInnerHTML.__html;
// [ts] let nextHtml = nextProps.dangerouslySetInnerHTML && nextProps.dangerouslySetInnerHTML.__html;
// Note the use of `!=` which checks for null or undefined.
// [ts] let lastChildren = if (lastContent != null) null else lastProps.children;
// [ts] let nextChildren = if (nextContent != null) null else nextProps.children;
// If we're switching from children to content/html or vice versa, remove
// the old content
// [ts] let lastHasContentOrHtml = lastContent != null || lastHtml != null;
// [ts] let nextHasContentOrHtml = nextContent != null || nextHtml != null;
// [ts] if (lastChildren != null && nextChildren == null) {
// [ts] _.updateChildren(null, transaction, context);
// [ts] } else if (lastHasContentOrHtml && !nextHasContentOrHtml) {
// [ts] _.updateTextContent("");
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onSetChildren(_._debugID, []);
// [ts] }
// [ts] }
// [ts] if (nextContent != null) {
// [ts] if (lastContent != nextContent) {
// [ts] _.updateTextContent("" + nextContent);
// [ts] if ("development" != "production") {
// [ts] setAndValidateContentChildDev.call(this, nextContent);
// [ts] }
// [ts] }
// [ts] } else if (nextHtml != null) {
// [ts] if (lastHtml != nextHtml) {
// [ts] _.updateMarkup("" + nextHtml);
// [ts] }
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onSetChildren(_._debugID, []);
// [ts] }
// [ts] } else if (nextChildren != null) {
// [ts] if ("development" != "production") {
// [ts] setAndValidateContentChildDev.call(this, null);
// [ts] }
// [ts] _.updateChildren(nextChildren, transaction, context);
// [ts] }
// [ts] },
// [ts] getHostNode: () => {
// [ts] return getNode(this);
// [ts] },
// [ts] /**
// [ts] * Destroys all event registrations for this instance. Does not remove from
// [ts] * the DOM. That must be done by the parent.
// [ts] *
// [ts] * @internal
// [ts] */
// [ts] unmountComponent: (safely) => {
// [ts] switch (_._tag) {
// [ts] "audio", "form", "iframe", "img", "link", "object", "source", "video" => {
// [ts] let listeners = _._wrapperState.listeners;
// [ts] if (listeners) {
// [ts] let i = 0;
// [ts] while (i < listeners.length) { // [cap-migrate] for-step: i++
// [ts] listeners[i].remove();
// [ts] }
// [ts] }
// break;
// [ts] },
// [ts] "input", "textarea" => {
// [ts] inputValueTracking.stopTracking(this);
// break;
// [ts] },
// [ts] /**
// [ts] * Components like <html> <head> and <body> can't be removed or added
// [ts] * easily in a cross-browser way, however it's valuable to be able to
// [ts] * take advantage of React's reconciliation for styling and <title>
// [ts] * management. So we just document it and throw in dangerous cases.
// [ts] */
// [ts] "html", "head", "body" => {
// [ts] if (!false) if ("development" != "production") invariant(false, "<%s> tried to unmount. Because of cross-browser quirks it is impossible to unmount some top-level components (eg <html>, <head>, and <body>) reliably and efficiently. To fix this, have a single top-level component that never unmounts render these elements.", _._tag) else _prodInvariant("66", _._tag) else void 0;
// break;
// [ts] }
// [ts] }
// [ts] _.unmountChildren(safely);
// [ts] ReactDOMComponentTree.uncacheNode(this);
// [ts] EventPluginHub.deleteAllListeners(this);
// [ts] _._rootNodeID = 0;
// [ts] _._domID = 0;
// [ts] _._wrapperState = null;
// [ts] if ("development" != "production") {
// [ts] setAndValidateContentChildDev.call(this, null);
// [ts] }
// [ts] },
// [ts] getPublicInstance: () => {
// [ts] return getNode(this);
// [ts] }
// [ts] };
// [ts] _assign(ReactDOMComponent.prototype, ReactDOMComponent.Mixin, ReactMultiChild.Mixin);
// [ts] module.exports = ReactDOMComponent;
// [ts] },{"10":10,"107":107,"11":11,"12":12,"120":120,"122":122,"126":126,"132":132,"144":144,"152":152,"158":158,"159":159,"160":160,"17":17,"18":18,"2":2,"26":26,"33":33,"34":34,"39":39,"42":42,"43":43,"46":46,"5":5,"64":64,"68":68,"77":77,"9":9}],33:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2015-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactDOMComponentFlags = {
// [ts] hasCachedChildNodes: 1 << 0
// [ts] };
// [ts] module.exports = ReactDOMComponentFlags;
// [ts] },{}],34:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let DOMProperty = _dereq_(11);
// [ts] let ReactDOMComponentFlags = _dereq_(33);
// [ts] let invariant = _dereq_(152);
// [ts] let ATTR_NAME = DOMProperty.ID_ATTRIBUTE_NAME;
// [ts] let Flags = ReactDOMComponentFlags;
// [ts] let internalInstanceKey = "__reactInternalInstance$" + Math.random().toString(36).slice(2);
// [ts] /**
// [ts] * Check if a given node should be cached.
// [ts] */
// [ts] fn shouldPrecacheNode(node: auto, nodeID: auto) {
// [ts] return node.nodeType == 1 && node.getAttribute(ATTR_NAME) == String(nodeID) || node.nodeType == 8 && node.nodeValue == " react-text: " + nodeID + " " || node.nodeType == 8 && node.nodeValue == " react-empty: " + nodeID + " ";
// [ts] }
// [ts] /**
// [ts] * Drill down (through composites and empty components) until we get a host or
// [ts] * host text component.
// [ts] *
// [ts] * This is pretty polymorphic but unavoidable with the current structure we have
// [ts] * for `_renderedChildren`.
// [ts] */
// [ts] fn getRenderedHostOrTextFromComponent(component: auto) {
// [ts] let rendered;
// [ts] while (rendered = component._renderedComponent) {
// [ts] component = rendered;
// [ts] }
// [ts] return component;
// [ts] }
// [ts] /**
// [ts] * Populate `_hostNode` on the rendered host/text component with the given
// [ts] * DOM node. The passed `inst` can be a composite.
// [ts] */
// [ts] fn precacheNode(inst: auto, node: auto) {
// [ts] let hostInst = getRenderedHostOrTextFromComponent(inst);
// [ts] hostInst._hostNode = node;
// [ts] node[internalInstanceKey] = hostInst;
// [ts] }
// [ts] fn uncacheNode(inst: auto) {
// [ts] let node = inst._hostNode;
// [ts] if (node) {
// [ts] delete node[internalInstanceKey];
// [ts] inst._hostNode = null;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Populate `_hostNode` on each child of `inst`, assuming that the children
// [ts] * match up with the DOM (element) children of `node`.
// [ts] *
// [ts] * We cache entire levels at once to avoid an n^2 problem where we access the
// [ts] * children of a node sequentially and have to walk from the start to our target
// [ts] * node every time.
// [ts] *
// [ts] * Since we update `_renderedChildren` and the actual DOM at (slightly)
// [ts] * different times, we could race here and see a newer `_renderedChildren` than
// [ts] * the DOM nodes we see. To avoid this, ReactMultiChild calls
// [ts] * `prepareToManageChildren` before we change `_renderedChildren`, at which
// [ts] * time the container's child nodes are always cached (until it unmounts).
// [ts] */
// [ts] fn precacheChildNodes(inst: auto, node: auto) {
// [ts] if (inst._flags & Flags.hasCachedChildNodes) {
// [ts] return;
// [ts] }
// [ts] let children = inst._renderedChildren;
// [ts] let childNode = node.firstChild;
// [ts] outer: for (let name == children) {
// [ts] if (!children.hasOwnProperty(name)) {
// [ts] continue;
// [ts] }
// [ts] let childInst = children[name];
// [ts] let childID = getRenderedHostOrTextFromComponent(childInst)._domID;
// [ts] if (childID == 0) {
// We're currently unmounting this child in ReactMultiChild; skip it.
// [ts] continue;
// [ts] }
// We assume the child nodes are in the same order as the child instances.
// [ts] while (childNode != null) { // [cap-migrate] for-step: childNode = childNode.nextSibling
// [ts] if (shouldPrecacheNode(childNode, childID)) {
// [ts] precacheNode(childInst, childNode);
// [ts] continue outer;
// [ts] }
// [ts] }
// We reached the end of the DOM children without finding an ID match.
// [ts] if (!false) if ("development" != "production") invariant(false, "Unable to find element with ID %s.", childID) else _prodInvariant("32", childID) else void 0;
// [ts] }
// [ts] inst._flags |= Flags.hasCachedChildNodes;
// [ts] }
// [ts] /**
// [ts] * Given a DOM node, return the closest ReactDOMComponent or
// [ts] * ReactDOMTextComponent instance ancestor.
// [ts] */
// [ts] fn getClosestInstanceFromNode(node: auto) {
// [ts] if (node[internalInstanceKey]) {
// [ts] return node[internalInstanceKey];
// [ts] }
// Walk up the tree until we find an ancestor whose instance we have cached.
// [ts] let parents = [];
// [ts] while (!node[internalInstanceKey]) {
// [ts] parents.push(node);
// [ts] if (node.parentNode) {
// [ts] node = node.parentNode;
// [ts] } else {
// Top of the tree. This node must not be part of a React tree (or is
// unmounted, potentially).
// [ts] return null;
// [ts] }
// [ts] }
// [ts] let closest;
// [ts] let inst;
// [ts] while (node && (inst = node[internalInstanceKey])) { // [cap-migrate] for-step: node = parents.pop()
// [ts] closest = inst;
// [ts] if (parents.length) {
// [ts] precacheChildNodes(inst, node);
// [ts] }
// [ts] }
// [ts] return closest;
// [ts] }
// [ts] /**
// [ts] * Given a DOM node, return the ReactDOMComponent or ReactDOMTextComponent
// [ts] * instance, or null if the node was not rendered by this React.
// [ts] */
// [ts] fn getInstanceFromNode(node: auto) {
// [ts] let inst = getClosestInstanceFromNode(node);
// [ts] if (inst != null && inst._hostNode == node) {
// [ts] return inst;
// [ts] } else {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Given a ReactDOMComponent or ReactDOMTextComponent, return the corresponding
// [ts] * DOM node.
// [ts] */
// [ts] fn getNodeFromInstance(inst: auto) {
// Without this first invariant, passing a non-DOM-component triggers the next
// invariant for a missing parent, which is super confusing.
// [ts] if (!(inst._hostNode != null)) if ("development" != "production") invariant(false, "getNodeFromInstance: Invalid argument.") else _prodInvariant("33") else void 0;
// [ts] if (inst._hostNode) {
// [ts] return inst._hostNode;
// [ts] }
// Walk up the tree until we find an ancestor whose DOM node we have cached.
// [ts] let parents = [];
// [ts] while (!inst._hostNode) {
// [ts] parents.push(inst);
// [ts] if (!inst._hostParent) if ("development" != "production") invariant(false, "React DOM tree root should always have a node reference.") else _prodInvariant("34") else void 0;
// [ts] inst = inst._hostParent;
// [ts] }
// Now parents contains each ancestor that does *not* have a cached native
// node, and `inst` is the deepest ancestor that does.
// [ts] while (parents.length) { // [cap-migrate] for-step: inst = parents.pop()
// [ts] precacheChildNodes(inst, inst._hostNode);
// [ts] }
// [ts] return inst._hostNode;
// [ts] }
// [ts] let ReactDOMComponentTree = {
// [ts] getClosestInstanceFromNode: getClosestInstanceFromNode,
// [ts] getInstanceFromNode: getInstanceFromNode,
// [ts] getNodeFromInstance: getNodeFromInstance,
// [ts] precacheChildNodes: precacheChildNodes,
// [ts] precacheNode: precacheNode,
// [ts] uncacheNode: uncacheNode
// [ts] };
// [ts] module.exports = ReactDOMComponentTree;
// [ts] },{"11":11,"126":126,"152":152,"33":33}],35:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let validateDOMNesting = _dereq_(132);
// [ts] let DOC_NODE_TYPE = 9;
// [ts] fn ReactDOMContainerInfo(topLevelWrapper: auto, node: auto) {
// [ts] let info = {
// [ts] _topLevelWrapper: topLevelWrapper,
// [ts] _idCounter: 1,
// [ts] _ownerDocument: if (node) if (node.nodeType == DOC_NODE_TYPE) node else node.ownerDocument else null,
// [ts] _node: node,
// [ts] _tag: if (node) node.nodeName.toLowerCase() else null,
// [ts] _namespaceURI: if (node) node.namespaceURI else null
// [ts] };
// [ts] if ("development" != "production") {
// [ts] info._ancestorInfo = if (node) validateDOMNesting.updatedAncestorInfo(null, info._tag, null) else null;
// [ts] }
// [ts] return info;
// [ts] }
// [ts] module.exports = ReactDOMContainerInfo;
// [ts] },{"132":132}],36:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let DOMLazyTree = _dereq_(9);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactDOMEmptyComponent = (instantiate) => {
// ReactCompositeComponent uses this:
// [ts] _._currentElement = null;
// ReactDOMComponentTree uses these:
// [ts] _._hostNode = null;
// [ts] _._hostParent = null;
// [ts] _._hostContainerInfo = null;
// [ts] _._domID = 0;
// [ts] };
// [ts] _assign(ReactDOMEmptyComponent.prototype, {
// [ts] mountComponent: (transaction, hostParent, hostContainerInfo, context) => {
// [ts] let domID = hostContainerInfo._idCounter++;
// [ts] _._domID = domID;
// [ts] _._hostParent = hostParent;
// [ts] _._hostContainerInfo = hostContainerInfo;
// [ts] let nodeValue = " react-empty: " + _._domID + " ";
// [ts] if (transaction.useCreateElement) {
// [ts] let ownerDocument = hostContainerInfo._ownerDocument;
// [ts] let node = ownerDocument.createComment(nodeValue);
// [ts] ReactDOMComponentTree.precacheNode(this, node);
// [ts] return DOMLazyTree(node);
// [ts] } else {
// [ts] if (transaction.renderToStaticMarkup) {
// Normally we'd insert a comment node, but since this is a situation
// where React won't take over (static pages), we can simply return
// nothing.
// [ts] return "";
// [ts] }
// [ts] return "<!--" + nodeValue + "-->";
// [ts] }
// [ts] },
// [ts] receiveComponent: () => {},
// [ts] getHostNode: () => {
// [ts] return ReactDOMComponentTree.getNodeFromInstance(this);
// [ts] },
// [ts] unmountComponent: () => {
// [ts] ReactDOMComponentTree.uncacheNode(this);
// [ts] }
// [ts] });
// [ts] module.exports = ReactDOMEmptyComponent;
// [ts] },{"160":160,"34":34,"9":9}],37:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactDOMFeatureFlags = {
// [ts] useCreateElement: true,
// [ts] useFiber: false
// [ts] };
// [ts] module.exports = ReactDOMFeatureFlags;
// [ts] },{}],38:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMChildrenOperations = _dereq_(8);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] /**
// [ts] * Operations used to process updates to DOM nodes.
// [ts] */
// [ts] let ReactDOMIDOperations = {
// [ts] /**
// [ts] * Updates a component's children by processing a series of updates.
// [ts] *
// [ts] * @param {array<object>} updates List of update configurations.
// [ts] * @internal
// [ts] */
// [ts] dangerouslyProcessChildrenUpdates: (parentInst, updates) => {
// [ts] let node = ReactDOMComponentTree.getNodeFromInstance(parentInst);
// [ts] DOMChildrenOperations.processUpdates(node, updates);
// [ts] }
// [ts] };
// [ts] module.exports = ReactDOMIDOperations;
// [ts] },{"34":34,"8":8}],39:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] let DOMPropertyOperations = _dereq_(12);
// [ts] let LinkedValueUtils = _dereq_(24);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] let didWarnValueLink = false;
// [ts] let didWarnCheckedLink = false;
// [ts] let didWarnValueDefaultValue = false;
// [ts] let didWarnCheckedDefaultChecked = false;
// [ts] let didWarnControlledToUncontrolled = false;
// [ts] let didWarnUncontrolledToControlled = false;
// [ts] fn forceUpdateIfMounted() {
// [ts] if (_._rootNodeID) {
// DOM component is still mounted; update
// [ts] ReactDOMInput.updateWrapper(this);
// [ts] }
// [ts] }
// [ts] fn isControlled(props: auto) {
// [ts] let usesChecked = props.type == "checkbox" || props.type == "radio";
// [ts] return if (usesChecked) props.checked != null else props.value != null;
// [ts] }
// [ts] /**
// [ts] * Implements an <input> host component that allows setting these optional
// [ts] * props: `checked`, `value`, `defaultChecked`, and `defaultValue`.
// [ts] *
// [ts] * If `checked` or `value` are not supplied (or null/undefined), user actions
// [ts] * that affect the checked state or value will trigger updates to the element.
// [ts] *
// [ts] * If they are supplied (and not null/undefined), the rendered element will not
// [ts] * trigger updates to the element. Instead, the props must change in order for
// [ts] * the rendered element to be updated.
// [ts] *
// [ts] * The rendered element will be initialized as unchecked (or `defaultChecked`)
// [ts] * with an empty value (or `defaultValue`).
// [ts] *
// [ts] * @see http://www.w3.org/TR/2012/WD-html5-20121025/the-input-element.html
// [ts] */
// [ts] let ReactDOMInput = {
// [ts] getHostProps: (inst, props) => {
// [ts] let value = LinkedValueUtils.getValue(props);
// [ts] let checked = LinkedValueUtils.getChecked(props);
// [ts] let hostProps = _assign({
// Make sure we set .type before any other properties (setting .value
// before .type means .value is lost in IE11 and below)
// [ts] type: null,
// Make sure we set .step before .value (setting .value before .step
// means .value is rounded on mount, based upon step precision)
// [ts] step: null,
// Make sure we set .min & .max before .value (to ensure proper order
// in corner cases such as min or max deriving from value, e.g. Issue #7170)
// [ts] min: null,
// [ts] max: null
// [ts] }, props, {
// [ts] defaultChecked: null,
// [ts] defaultValue: null,
// [ts] value: if (value != null) value else inst._wrapperState.initialValue,
// [ts] checked: if (checked != null) checked else inst._wrapperState.initialChecked,
// [ts] onChange: inst._wrapperState.onChange
// [ts] });
// [ts] return hostProps;
// [ts] },
// [ts] mountWrapper: (inst, props) => {
// [ts] if ("development" != "production") {
// [ts] LinkedValueUtils.checkPropTypes("input", props, inst._currentElement._owner);
// [ts] let owner = inst._currentElement._owner;
// [ts] if (props.valueLink != null && !didWarnValueLink) {
// [ts] if ("development" != "production") warning(false, "`valueLink` prop on `input` is deprecated; set `value` and `onChange` instead.") else void 0;
// [ts] didWarnValueLink = true;
// [ts] }
// [ts] if (props.checkedLink != null && !didWarnCheckedLink) {
// [ts] if ("development" != "production") warning(false, "`checkedLink` prop on `input` is deprecated; set `value` and `onChange` instead.") else void 0;
// [ts] didWarnCheckedLink = true;
// [ts] }
// [ts] if (props.checked != null && props.defaultChecked != null && !didWarnCheckedDefaultChecked) {
// [ts] if ("development" != "production") warning(false, "%s contains an input of type %s with both checked and defaultChecked props. " + "Input elements must be either controlled or uncontrolled " + "(specify either the checked prop, or the defaultChecked prop, but not " + "both). Decide between using a controlled or uncontrolled input " + "element and remove one of these props. More info: " + "https://fb.me/react-controlled-components", owner && owner.getName() || "A component", props.type) else void 0;
// [ts] didWarnCheckedDefaultChecked = true;
// [ts] }
// [ts] if (props.value != null && props.defaultValue != null && !didWarnValueDefaultValue) {
// [ts] if ("development" != "production") warning(false, "%s contains an input of type %s with both value and defaultValue props. " + "Input elements must be either controlled or uncontrolled " + "(specify either the value prop, or the defaultValue prop, but not " + "both). Decide between using a controlled or uncontrolled input " + "element and remove one of these props. More info: " + "https://fb.me/react-controlled-components", owner && owner.getName() || "A component", props.type) else void 0;
// [ts] didWarnValueDefaultValue = true;
// [ts] }
// [ts] }
// [ts] let defaultValue = props.defaultValue;
// [ts] inst._wrapperState = {
// [ts] initialChecked: if (props.checked != null) props.checked else props.defaultChecked,
// [ts] initialValue: if (props.value != null) props.value else defaultValue,
// [ts] listeners: null,
// [ts] onChange: _handleChange.bind(inst),
// [ts] controlled: isControlled(props)
// [ts] };
// [ts] },
// [ts] updateWrapper: (inst) => {
// [ts] let props = inst._currentElement.props;
// [ts] if ("development" != "production") {
// [ts] let controlled = isControlled(props);
// [ts] let owner = inst._currentElement._owner;
// [ts] if (!inst._wrapperState.controlled && controlled && !didWarnUncontrolledToControlled) {
// [ts] if ("development" != "production") warning(false, "%s is changing an uncontrolled input of type %s to be controlled. " + "Input elements should not switch from uncontrolled to controlled (or vice versa). " + "Decide between using a controlled or uncontrolled input " + "element for the lifetime of the component. More info: https://fb.me/react-controlled-components", owner && owner.getName() || "A component", props.type) else void 0;
// [ts] didWarnUncontrolledToControlled = true;
// [ts] }
// [ts] if (inst._wrapperState.controlled && !controlled && !didWarnControlledToUncontrolled) {
// [ts] if ("development" != "production") warning(false, "%s is changing a controlled input of type %s to be uncontrolled. " + "Input elements should not switch from controlled to uncontrolled (or vice versa). " + "Decide between using a controlled or uncontrolled input " + "element for the lifetime of the component. More info: https://fb.me/react-controlled-components", owner && owner.getName() || "A component", props.type) else void 0;
// [ts] didWarnControlledToUncontrolled = true;
// [ts] }
// [ts] }
// TODO: Shouldn't this be getChecked(props)?
// [ts] let checked = props.checked;
// [ts] if (checked != null) {
// [ts] DOMPropertyOperations.setValueForProperty(ReactDOMComponentTree.getNodeFromInstance(inst), "checked", checked || false);
// [ts] }
// [ts] let node = ReactDOMComponentTree.getNodeFromInstance(inst);
// [ts] let value = LinkedValueUtils.getValue(props);
// [ts] if (value != null) {
// [ts] if (value == 0 && node.value == "") {
// [ts] node.value = "0";
// Note: IE9 reports a number inputs as 'text', so check props instead.
// [ts] } else if (props.type == "number") {
// Simulate `input.valueAsNumber`. IE9 does not support it
// [ts] let valueAsNumber = parseFloat(node.value, 10) || 0;
// [ts] if (
// eslint-disable-next-line
// [ts] value != valueAsNumber ||
// eslint-disable-next-line
// [ts] value == valueAsNumber && node.value != value) {
// Cast `value` to a string to ensure the value is set correctly. While
// browsers typically do this as necessary, jsdom doesn't.
// [ts] node.value = "" + value;
// [ts] }
// [ts] } else if (node.value != "" + value) {
// Cast `value` to a string to ensure the value is set correctly. While
// browsers typically do this as necessary, jsdom doesn't.
// [ts] node.value = "" + value;
// [ts] }
// [ts] } else {
// [ts] if (props.value == null && props.defaultValue != null) {
// In Chrome, assigning defaultValue to certain input types triggers input validation.
// For number inputs, the display value loses trailing decimal points. For email inputs,
// Chrome raises "The specified value <x> is not a valid email address".
//
// Here we check to see if the defaultValue has actually changed, avoiding these problems
// when the user is inputting text
//
// https://github.com/facebook/react/issues/7253
// [ts] if (node.defaultValue != "" + props.defaultValue) {
// [ts] node.defaultValue = "" + props.defaultValue;
// [ts] }
// [ts] }
// [ts] if (props.checked == null && props.defaultChecked != null) {
// [ts] node.defaultChecked = !!props.defaultChecked;
// [ts] }
// [ts] }
// [ts] },
// [ts] postMountWrapper: (inst) => {
// [ts] let props = inst._currentElement.props;
// This is in postMount because we need access to the DOM node, which is not
// available until after the component has mounted.
// [ts] let node = ReactDOMComponentTree.getNodeFromInstance(inst);
// Detach value from defaultValue. We won't do anything if we're working on
// submit or reset inputs as those values & defaultValues are linked. They
// are not resetable nodes so this operation doesn't matter and actually
// removes browser-default values (eg "Submit Query") when no value is
// provided.
// [ts] switch (props.type) {
// [ts] "submit", "reset" => {
// [ts] break;
// [ts] },
// This fixes the no-show issue on iOS Safari and Android Chrome:
// https://github.com/facebook/react/issues/7233
// [ts] "color", "date", "datetime", "datetime-local", "month", "time", "week" => {
// [ts] node.value = "";
// [ts] node.value = node.defaultValue;
// break;
// [ts] },
// [ts] else => {
// [ts] node.value = node.value;
// break;
// [ts] }
// [ts] }
// Normally, we'd just do `node.checked = node.checked` upon initial mount, less this bug
// this is needed to work around a chrome bug where setting defaultChecked
// will sometimes influence the value of checked (even after detachment).
// Reference: https://bugs.chromium.org/p/chromium/issues/detail?id=608416
// We need to temporarily unset name to avoid disrupting radio button groups.
// [ts] let name = node.name;
// [ts] if (name != "") {
// [ts] node.name = "";
// [ts] }
// [ts] node.defaultChecked = !node.defaultChecked;
// [ts] node.defaultChecked = !node.defaultChecked;
// [ts] if (name != "") {
// [ts] node.name = name;
// [ts] }
// [ts] }
// [ts] };
// [ts] fn _handleChange(event: auto) {
// [ts] let props = _._currentElement.props;
// [ts] let returnValue = LinkedValueUtils.executeOnChange(props, event);
// Here we use asap to wait until all updates have propagated, which
// is important when using controlled components within layers:
// https://github.com/facebook/react/issues/1698
// [ts] ReactUpdates.asap(forceUpdateIfMounted, this);
// [ts] let name = props.name;
// [ts] if (props.type == "radio" && name != null) {
// [ts] let rootNode = ReactDOMComponentTree.getNodeFromInstance(this);
// [ts] let queryRoot = rootNode;
// [ts] while (queryRoot.parentNode) {
// [ts] queryRoot = queryRoot.parentNode;
// [ts] }
// If `rootNode.form` was non-null, then we could try `form.elements`,
// but that sometimes behaves strangely in IE8. We could also try using
// `form.getElementsByName`, but that will only return direct children
// and won't include inputs that use the HTML5 `form=` attribute. Since
// the input might not even be in a form, let's just use the global
// `querySelectorAll` to ensure we don't miss anything.
// [ts] let group = queryRoot.querySelectorAll("input[name=" + JSON.stringify("" + name) + "][type=\"radio\"]");
// [ts] let i = 0;
// [ts] while (i < group.length) { // [cap-migrate] for-step: i++
// [ts] let otherNode = group[i];
// [ts] if (otherNode == rootNode || otherNode.form != rootNode.form) {
// [ts] continue;
// [ts] }
// This will throw if radio buttons rendered by different copies of React
// and the same name are rendered into the same form (same as #1939).
// That's probably okay; we don't support it just as we don't support
// mixing React radio buttons with non-React ones.
// [ts] let otherInstance = ReactDOMComponentTree.getInstanceFromNode(otherNode);
// [ts] if (!otherInstance) if ("development" != "production") invariant(false, "ReactDOMInput: Mixing React and non-React radio inputs with the same `name` is not supported.") else _prodInvariant("90") else void 0;
// If this is a controlled radio button group, forcing the input that
// was previously checked to update will cause it to be come re-checked
// as appropriate.
// [ts] ReactUpdates.asap(forceUpdateIfMounted, otherInstance);
// [ts] }
// [ts] }
// [ts] return returnValue;
// [ts] }
// [ts] module.exports = ReactDOMInput;
// [ts] },{"12":12,"126":126,"152":152,"159":159,"160":160,"24":24,"34":34,"82":82}],40:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMProperty = _dereq_(11);
// [ts] let ReactComponentTreeHook = _dereq_(133);
// [ts] let warning = _dereq_(159);
// [ts] let warnedProperties = {};
// [ts] let rARIA = RegExp.init("^(aria)-[" + DOMProperty.ATTRIBUTE_NAME_CHAR + "]*$");
// [ts] fn validateProperty(tagName: auto, name: auto, debugID: auto) {
// [ts] if (warnedProperties.hasOwnProperty(name) && warnedProperties[name]) {
// [ts] return true;
// [ts] }
// [ts] if (rARIA.test(name)) {
// [ts] let lowerCasedName = name.toLowerCase();
// [ts] let standardName = if (DOMProperty.getPossibleStandardName.hasOwnProperty(lowerCasedName)) DOMProperty.getPossibleStandardName[lowerCasedName] else null;
// If this is an aria-* attribute, but is not listed in the known DOM
// DOM properties, then it is an invalid aria-* attribute.
// [ts] if (standardName == null) {
// [ts] warnedProperties[name] = true;
// [ts] return false;
// [ts] }
// aria-* attributes should be lowercase; suggest the lowercase version.
// [ts] if (name != standardName) {
// [ts] if ("development" != "production") warning(false, "Unknown ARIA attribute %s. Did you mean %s?%s", name, standardName, ReactComponentTreeHook.getStackAddendumByID(debugID)) else void 0;
// [ts] warnedProperties[name] = true;
// [ts] return true;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn warnInvalidARIAProps(debugID: auto, element: auto) {
// [ts] let invalidProps = [];
// [ts] for (let key in element.props) {
// [ts] let isValid = validateProperty(element.type, key, debugID);
// [ts] if (!isValid) {
// [ts] invalidProps.push(key);
// [ts] }
// [ts] }
// [ts] let unknownPropString = invalidProps.map((prop) => {
// [ts] return "`" + prop + "`";
// [ts] }).join(", ");
// [ts] if (invalidProps.length == 1) {
// [ts] if ("development" != "production") warning(false, "Invalid aria prop %s on <%s> tag. " + "For details, see https://fb.me/invalid-aria-prop%s", unknownPropString, element.type, ReactComponentTreeHook.getStackAddendumByID(debugID)) else void 0;
// [ts] } else if (invalidProps.length > 1) {
// [ts] if ("development" != "production") warning(false, "Invalid aria props %s on <%s> tag. " + "For details, see https://fb.me/invalid-aria-prop%s", unknownPropString, element.type, ReactComponentTreeHook.getStackAddendumByID(debugID)) else void 0;
// [ts] }
// [ts] }
// [ts] fn handleElement(debugID: auto, element: auto) {
// [ts] if (element == null || @typeName(element.type) != "string") {
// [ts] return;
// [ts] }
// [ts] if (element.type.indexOf("-") >= 0 || element.props.is) {
// [ts] return;
// [ts] }
// [ts] warnInvalidARIAProps(debugID, element);
// [ts] }
// [ts] let ReactDOMInvalidARIAHook = {
// [ts] onBeforeMountComponent: (debugID, element) => {
// [ts] if ("development" != "production") {
// [ts] handleElement(debugID, element);
// [ts] }
// [ts] },
// [ts] onBeforeUpdateComponent: (debugID, element) => {
// [ts] if ("development" != "production") {
// [ts] handleElement(debugID, element);
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactDOMInvalidARIAHook;
// [ts] },{"11":11,"133":133,"159":159}],41:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactComponentTreeHook = _dereq_(133);
// [ts] let warning = _dereq_(159);
// [ts] let didWarnValueNull = false;
// [ts] fn handleElement(debugID: auto, element: auto) {
// [ts] if (element == null) {
// [ts] return;
// [ts] }
// [ts] if (element.type != "input" && element.type != "textarea" && element.type != "select") {
// [ts] return;
// [ts] }
// [ts] if (element.props != null && element.props.value == null && !didWarnValueNull) {
// [ts] if ("development" != "production") warning(false, "`value` prop on `%s` should not be null. " + "Consider using the empty string to clear the component or `undefined` " + "for uncontrolled components.%s", element.type, ReactComponentTreeHook.getStackAddendumByID(debugID)) else void 0;
// [ts] didWarnValueNull = true;
// [ts] }
// [ts] }
// [ts] let ReactDOMNullInputValuePropHook = {
// [ts] onBeforeMountComponent: (debugID, element) => {
// [ts] handleElement(debugID, element);
// [ts] },
// [ts] onBeforeUpdateComponent: (debugID, element) => {
// [ts] handleElement(debugID, element);
// [ts] }
// [ts] };
// [ts] module.exports = ReactDOMNullInputValuePropHook;
// [ts] },{"133":133,"159":159}],42:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let React = _dereq_(135);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactDOMSelect = _dereq_(43);
// [ts] let warning = _dereq_(159);
// [ts] let didWarnInvalidOptionChildren = false;
// [ts] fn flattenChildren(children: auto) {
// [ts] let content = "";
// Flatten children and warn if they aren't strings or numbers;
// invalid types are ignored.
// [ts] React.Children.forEach(children, (child) => {
// [ts] if (child == null) {
// [ts] return;
// [ts] }
// [ts] if (@typeName(child) == "string" || @typeName(child) == "number") {
// [ts] content += child;
// [ts] } else if (!didWarnInvalidOptionChildren) {
// [ts] didWarnInvalidOptionChildren = true;
// [ts] if ("development" != "production") warning(false, "Only strings and numbers are supported as <option> children.") else void 0;
// [ts] }
// [ts] });
// [ts] return content;
// [ts] }
// [ts] /**
// [ts] * Implements an <option> host component that warns when `selected` is set.
// [ts] */
// [ts] let ReactDOMOption = {
// [ts] mountWrapper: (inst, props, hostParent) => {
// TODO (yungsters): Remove support for `selected` in <option>.
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(props.selected == null, "Use the `defaultValue` or `value` props on <select> instead of " + "setting `selected` on <option>.") else void 0;
// [ts] }
// Look up whether this option is 'selected'
// [ts] let selectValue = null;
// [ts] if (hostParent != null) {
// [ts] let selectParent = hostParent;
// [ts] if (selectParent._tag == "optgroup") {
// [ts] selectParent = selectParent._hostParent;
// [ts] }
// [ts] if (selectParent != null && selectParent._tag == "select") {
// [ts] selectValue = ReactDOMSelect.getSelectValueContext(selectParent);
// [ts] }
// [ts] }
// If the value is null (e.g., no specified value or after initial mount)
// or missing (e.g., for <datalist>), we don't change props.selected
// [ts] let selected = null;
// [ts] if (selectValue != null) {
// [ts] let value;
// [ts] if (props.value != null) {
// [ts] value = props.value + "";
// [ts] } else {
// [ts] value = flattenChildren(props.children);
// [ts] }
// [ts] selected = false;
// [ts] if (Array.isArray(selectValue)) {
// multiple
// [ts] let i = 0;
// [ts] while (i < selectValue.length) { // [cap-migrate] for-step: i++
// [ts] if ("" + selectValue[i] == value) {
// [ts] selected = true;
// [ts] break;
// [ts] }
// [ts] }
// [ts] } else {
// [ts] selected = "" + selectValue == value;
// [ts] }
// [ts] }
// [ts] inst._wrapperState = { selected: selected };
// [ts] },
// [ts] postMountWrapper: (inst) => {
// value="" should make a value attribute (#6219)
// [ts] let props = inst._currentElement.props;
// [ts] if (props.value != null) {
// [ts] let node = ReactDOMComponentTree.getNodeFromInstance(inst);
// [ts] node.setAttribute("value", props.value);
// [ts] }
// [ts] },
// [ts] getHostProps: (inst, props) => {
// [ts] let hostProps = _assign({ selected: null, children: null }, props);
// Read state only from initial mount because <select> updates value
// manually; we need the initial state only for server rendering
// [ts] if (inst._wrapperState.selected != null) {
// [ts] hostProps.selected = inst._wrapperState.selected;
// [ts] }
// [ts] let content = flattenChildren(props.children);
// [ts] if (content) {
// [ts] hostProps.children = content;
// [ts] }
// [ts] return hostProps;
// [ts] }
// [ts] };
// [ts] module.exports = ReactDOMOption;
// [ts] },{"135":135,"159":159,"160":160,"34":34,"43":43}],43:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let LinkedValueUtils = _dereq_(24);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let warning = _dereq_(159);
// [ts] let didWarnValueLink = false;
// [ts] let didWarnValueDefaultValue = false;
// [ts] fn updateOptionsIfPendingUpdateAndMounted() {
// [ts] if (_._rootNodeID && _._wrapperState.pendingUpdate) {
// [ts] _._wrapperState.pendingUpdate = false;
// [ts] let props = _._currentElement.props;
// [ts] let value = LinkedValueUtils.getValue(props);
// [ts] if (value != null) {
// [ts] updateOptions(this, Boolean(props.multiple), value);
// [ts] }
// [ts] }
// [ts] }
// [ts] fn getDeclarationErrorAddendum(owner: auto) {
// [ts] if (owner) {
// [ts] let name = owner.getName();
// [ts] if (name) {
// [ts] return " Check the render method of `" + name + "`.";
// [ts] }
// [ts] }
// [ts] return "";
// [ts] }
// [ts] let valuePropNames = ["value", "defaultValue"];
// [ts] /**
// [ts] * Validation function for `value` and `defaultValue`.
// [ts] * @private
// [ts] */
// [ts] fn checkSelectPropTypes(inst: auto, props: auto) {
// [ts] let owner = inst._currentElement._owner;
// [ts] LinkedValueUtils.checkPropTypes("select", props, owner);
// [ts] if (props.valueLink != null && !didWarnValueLink) {
// [ts] if ("development" != "production") warning(false, "`valueLink` prop on `select` is deprecated; set `value` and `onChange` instead.") else void 0;
// [ts] didWarnValueLink = true;
// [ts] }
// [ts] let i = 0;
// [ts] while (i < valuePropNames.length) { // [cap-migrate] for-step: i++
// [ts] let propName = valuePropNames[i];
// [ts] if (props[propName] == null) {
// [ts] continue;
// [ts] }
// [ts] let isArray = Array.isArray(props[propName]);
// [ts] if (props.multiple && !isArray) {
// [ts] if ("development" != "production") warning(false, "The `%s` prop supplied to <select> must be an array if " + "`multiple` is true.%s", propName, getDeclarationErrorAddendum(owner)) else void 0;
// [ts] } else if (!props.multiple && isArray) {
// [ts] if ("development" != "production") warning(false, "The `%s` prop supplied to <select> must be a scalar " + "value if `multiple` is false.%s", propName, getDeclarationErrorAddendum(owner)) else void 0;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * @param {ReactDOMComponent} inst
// [ts] * @param {boolean} multiple
// [ts] * @param {*} propValue A stringable (with `multiple`, a list of stringables).
// [ts] * @private
// [ts] */
// [ts] fn updateOptions(inst: auto, multiple: auto, propValue: auto) {
// [ts] let selectedValue, i;
// [ts] let options = ReactDOMComponentTree.getNodeFromInstance(inst).options;
// [ts] if (multiple) {
// [ts] selectedValue = {};
// [ts] i = 0;
// [ts] while (i < propValue.length) { // [cap-migrate] for-step: i++
// [ts] selectedValue["" + propValue[i]] = true;
// [ts] }
// [ts] i = 0;
// [ts] while (i < options.length) { // [cap-migrate] for-step: i++
// [ts] let selected = selectedValue.hasOwnProperty(options[i].value);
// [ts] if (options[i].selected != selected) {
// [ts] options[i].selected = selected;
// [ts] }
// [ts] }
// [ts] } else {
// Do not set `select.value` as exact behavior isn't consistent across all
// browsers for all cases.
// [ts] selectedValue = "" + propValue;
// [ts] i = 0;
// [ts] while (i < options.length) { // [cap-migrate] for-step: i++
// [ts] if (options[i].value == selectedValue) {
// [ts] options[i].selected = true;
// [ts] return;
// [ts] }
// [ts] }
// [ts] if (options.length) {
// [ts] options[0].selected = true;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Implements a <select> host component that allows optionally setting the
// [ts] * props `value` and `defaultValue`. If `multiple` is false, the prop must be a
// [ts] * stringable. If `multiple` is true, the prop must be an array of stringables.
// [ts] *
// [ts] * If `value` is not supplied (or null/undefined), user actions that change the
// [ts] * selected option will trigger updates to the rendered options.
// [ts] *
// [ts] * If it is supplied (and not null/undefined), the rendered options will not
// [ts] * update in response to user actions. Instead, the `value` prop must change in
// [ts] * order for the rendered options to update.
// [ts] *
// [ts] * If `defaultValue` is provided, any options with the supplied values will be
// [ts] * selected.
// [ts] */
// [ts] let ReactDOMSelect = {
// [ts] getHostProps: (inst, props) => {
// [ts] return _assign({}, props, {
// [ts] onChange: inst._wrapperState.onChange,
// [ts] value: null
// [ts] });
// [ts] },
// [ts] mountWrapper: (inst, props) => {
// [ts] if ("development" != "production") {
// [ts] checkSelectPropTypes(inst, props);
// [ts] }
// [ts] let value = LinkedValueUtils.getValue(props);
// [ts] inst._wrapperState = {
// [ts] pendingUpdate: false,
// [ts] initialValue: if (value != null) value else props.defaultValue,
// [ts] listeners: null,
// [ts] onChange: _handleChange.bind(inst),
// [ts] wasMultiple: Boolean(props.multiple)
// [ts] };
// [ts] if (props.value != null && props.defaultValue != null && !didWarnValueDefaultValue) {
// [ts] if ("development" != "production") warning(false, "Select elements must be either controlled or uncontrolled " + "(specify either the value prop, or the defaultValue prop, but not " + "both). Decide between using a controlled or uncontrolled select " + "element and remove one of these props. More info: " + "https://fb.me/react-controlled-components") else void 0;
// [ts] didWarnValueDefaultValue = true;
// [ts] }
// [ts] },
// [ts] getSelectValueContext: (inst) => {
// ReactDOMOption looks at this initial value so the initial generated
// markup has correct `selected` attributes
// [ts] return inst._wrapperState.initialValue;
// [ts] },
// [ts] postUpdateWrapper: (inst) => {
// [ts] let props = inst._currentElement.props;
// After the initial mount, we control selected-ness manually so don't pass
// this value down
// [ts] inst._wrapperState.initialValue = null;
// [ts] let wasMultiple = inst._wrapperState.wasMultiple;
// [ts] inst._wrapperState.wasMultiple = Boolean(props.multiple);
// [ts] let value = LinkedValueUtils.getValue(props);
// [ts] if (value != null) {
// [ts] inst._wrapperState.pendingUpdate = false;
// [ts] updateOptions(inst, Boolean(props.multiple), value);
// [ts] } else if (wasMultiple != Boolean(props.multiple)) {
// For simplicity, reapply `defaultValue` if `multiple` is toggled.
// [ts] if (props.defaultValue != null) {
// [ts] updateOptions(inst, Boolean(props.multiple), props.defaultValue);
// [ts] } else {
// Revert the select back to its default unselected state.
// [ts] updateOptions(if (inst, Boolean(props.multiple), props.multiple) [] else "");
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] fn _handleChange(event: auto) {
// [ts] let props = _._currentElement.props;
// [ts] let returnValue = LinkedValueUtils.executeOnChange(props, event);
// [ts] if (_._rootNodeID) {
// [ts] _._wrapperState.pendingUpdate = true;
// [ts] }
// [ts] ReactUpdates.asap(updateOptionsIfPendingUpdateAndMounted, this);
// [ts] return returnValue;
// [ts] }
// [ts] module.exports = ReactDOMSelect;
// [ts] },{"159":159,"160":160,"24":24,"34":34,"82":82}],44:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let getNodeForCharacterOffset = _dereq_(117);
// [ts] let getTextContentAccessor = _dereq_(118);
// [ts] /**
// [ts] * While `isCollapsed` is available on the Selection object and `collapsed`
// [ts] * is available on the Range object, IE11 sometimes gets them wrong.
// [ts] * If the anchor/focus nodes and offsets are the same, the range is collapsed.
// [ts] */
// [ts] fn isCollapsed(anchorNode: auto, anchorOffset: auto, focusNode: auto, focusOffset: auto) {
// [ts] return anchorNode == focusNode && anchorOffset == focusOffset;
// [ts] }
// [ts] /**
// [ts] * Get the appropriate anchor and focus node/offset pairs for IE.
// [ts] *
// [ts] * The catch here is that IE's selection API doesn't provide information
// [ts] * about whether the selection is forward or backward, so we have to
// [ts] * behave as though it's always forward.
// [ts] *
// [ts] * IE text differs from modern selection in that it behaves as though
// [ts] * block elements end with a new line. This means character offsets will
// [ts] * differ between the two APIs.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @return {object}
// [ts] */
// [ts] fn getIEOffsets(node: auto) {
// [ts] let selection = document.selection;
// [ts] let selectedRange = selection.createRange();
// [ts] let selectedLength = selectedRange.text.length;
// Duplicate selection so we can move range without breaking user selection.
// [ts] let fromStart = selectedRange.duplicate();
// [ts] fromStart.moveToElementText(node);
// [ts] fromStart.setEndPoint("EndToStart", selectedRange);
// [ts] let startOffset = fromStart.text.length;
// [ts] let endOffset = startOffset + selectedLength;
// [ts] return {
// [ts] start: startOffset,
// [ts] end: endOffset
// [ts] };
// [ts] }
// [ts] /**
// [ts] * @param {DOMElement} node
// [ts] * @return {?object}
// [ts] */
// [ts] fn getModernOffsets(node: auto) {
// [ts] let selection = window.getSelection && window.getSelection();
// [ts] if (!selection || selection.rangeCount == 0) {
// [ts] return null;
// [ts] }
// [ts] let anchorNode = selection.anchorNode;
// [ts] let anchorOffset = selection.anchorOffset;
// [ts] let focusNode = selection.focusNode;
// [ts] let focusOffset = selection.focusOffset;
// [ts] let currentRange = selection.getRangeAt(0);
// In Firefox, range.startContainer and range.endContainer can be "anonymous
// divs", e.g. the up/down buttons on an <input type="number">. Anonymous
// divs do not seem to expose properties, triggering a "Permission denied
// error" if any of its properties are accessed. The only seemingly possible
// way to avoid erroring is to access a property that typically works for
// non-anonymous divs and catch any error that may otherwise arise. See
// https://bugzilla.mozilla.org/show_bug.cgi?id=208427
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] /* eslint-disable no-unused-expressions */
// [ts] currentRange.startContainer.nodeType;
// [ts] currentRange.endContainer.nodeType;
// [ts] /* eslint-enable no-unused-expressions */
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] return null;
// [ts] }
// If the node and offset values are the same, the selection is collapsed.
// `Selection.isCollapsed` is available natively, but IE sometimes gets
// this value wrong.
// [ts] let isSelectionCollapsed = isCollapsed(selection.anchorNode, selection.anchorOffset, selection.focusNode, selection.focusOffset);
// [ts] let rangeLength = if (isSelectionCollapsed) 0 else currentRange.toString().length;
// [ts] let tempRange = currentRange.cloneRange();
// [ts] tempRange.selectNodeContents(node);
// [ts] tempRange.setEnd(currentRange.startContainer, currentRange.startOffset);
// [ts] let isTempRangeCollapsed = isCollapsed(tempRange.startContainer, tempRange.startOffset, tempRange.endContainer, tempRange.endOffset);
// [ts] let start = if (isTempRangeCollapsed) 0 else tempRange.toString().length;
// [ts] let end = start + rangeLength;
// Detect whether the selection is backward.
// [ts] let detectionRange = document.createRange();
// [ts] detectionRange.setStart(anchorNode, anchorOffset);
// [ts] detectionRange.setEnd(focusNode, focusOffset);
// [ts] let isBackward = detectionRange.collapsed;
// [ts] return {
// [ts] start: if (isBackward) end else start,
// [ts] end: if (isBackward) start else end
// [ts] };
// [ts] }
// [ts] /**
// [ts] * @param {DOMElement|DOMTextNode} node
// [ts] * @param {object} offsets
// [ts] */
// [ts] fn setIEOffsets(node: auto, offsets: auto) {
// [ts] let range = document.selection.createRange().duplicate();
// [ts] let start, end;
// [ts] if (offsets.end == null) {
// [ts] start = offsets.start;
// [ts] end = start;
// [ts] } else if (offsets.start > offsets.end) {
// [ts] start = offsets.end;
// [ts] end = offsets.start;
// [ts] } else {
// [ts] start = offsets.start;
// [ts] end = offsets.end;
// [ts] }
// [ts] range.moveToElementText(node);
// [ts] range.moveStart("character", start);
// [ts] range.setEndPoint("EndToStart", range);
// [ts] range.moveEnd("character", end - start);
// [ts] range.select();
// [ts] }
// [ts] /**
// [ts] * In modern non-IE browsers, we can support both forward and backward
// [ts] * selections.
// [ts] *
// [ts] * Note: IE10+ supports the Selection object, but it does not support
// [ts] * the `extend` method, which means that even in modern IE, it's not possible
// [ts] * to programmatically create a backward selection. Thus, for all IE
// [ts] * versions, we use the old IE API to create our selections.
// [ts] *
// [ts] * @param {DOMElement|DOMTextNode} node
// [ts] * @param {object} offsets
// [ts] */
// [ts] fn setModernOffsets(node: auto, offsets: auto) {
// [ts] if (!window.getSelection) {
// [ts] return;
// [ts] }
// [ts] let selection = window.getSelection();
// [ts] let length = node[getTextContentAccessor()].length;
// [ts] let start = Math.min(offsets.start, length);
// [ts] let end = if (offsets.end == null) start else Math.min(offsets.end, length);
// IE 11 uses modern selection, but doesn't support the extend method.
// Flip backward selections, so we can set with a single range.
// [ts] if (!selection.extend && start > end) {
// [ts] let temp = end;
// [ts] end = start;
// [ts] start = temp;
// [ts] }
// [ts] let startMarker = getNodeForCharacterOffset(node, start);
// [ts] let endMarker = getNodeForCharacterOffset(node, end);
// [ts] if (startMarker && endMarker) {
// [ts] let range = document.createRange();
// [ts] range.setStart(startMarker.node, startMarker.offset);
// [ts] selection.removeAllRanges();
// [ts] if (start > end) {
// [ts] selection.addRange(range);
// [ts] selection.extend(endMarker.node, endMarker.offset);
// [ts] } else {
// [ts] range.setEnd(endMarker.node, endMarker.offset);
// [ts] selection.addRange(range);
// [ts] }
// [ts] }
// [ts] }
// [ts] let useIEOffsets = ExecutionEnvironment.canUseDOM && "selection" == document && !("getSelection" == window);
// [ts] let ReactDOMSelection = {
// [ts] /**
// [ts] * @param {DOMElement} node
// [ts] */
// [ts] getOffsets: if (useIEOffsets) getIEOffsets else getModernOffsets,
// [ts] /**
// [ts] * @param {DOMElement|DOMTextNode} node
// [ts] * @param {object} offsets
// [ts] */
// [ts] setOffsets: if (useIEOffsets) setIEOffsets else setModernOffsets
// [ts] };
// [ts] module.exports = ReactDOMSelection;
// [ts] },{"117":117,"118":118,"138":138}],45:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] let DOMChildrenOperations = _dereq_(8);
// [ts] let DOMLazyTree = _dereq_(9);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let escapeTextContentForBrowser = _dereq_(107);
// [ts] let invariant = _dereq_(152);
// [ts] let validateDOMNesting = _dereq_(132);
// [ts] /**
// [ts] * Text nodes violate a couple assumptions that React makes about components:
// [ts] *
// [ts] *  - When mounting text into the DOM, adjacent text nodes are merged.
// [ts] *  - Text nodes cannot be assigned a React root ID.
// [ts] *
// [ts] * This component is used to wrap strings between comment nodes so that they
// [ts] * can undergo the same reconciliation that is applied to elements.
// [ts] *
// [ts] * TODO: Investigate representing React components in the DOM with text nodes.
// [ts] *
// [ts] * @class ReactDOMTextComponent
// [ts] * @extends ReactComponent
// [ts] * @internal
// [ts] */
// [ts] let ReactDOMTextComponent = (text) => {
// TODO: This is really a ReactText (ReactNode), not a ReactElement
// [ts] _._currentElement = text;
// [ts] _._stringText = "" + text;
// ReactDOMComponentTree uses these:
// [ts] _._hostNode = null;
// [ts] _._hostParent = null;
// Properties
// [ts] _._domID = 0;
// [ts] _._mountIndex = 0;
// [ts] _._closingComment = null;
// [ts] _._commentNodes = null;
// [ts] };
// [ts] _assign(ReactDOMTextComponent.prototype, {
// [ts] /**
// [ts] * Creates the markup for this text node. This node is not intended to have
// [ts] * any features besides containing text content.
// [ts] *
// [ts] * @param {ReactReconcileTransaction|ReactServerRenderingTransaction} transaction
// [ts] * @return {string} Markup for this text node.
// [ts] * @internal
// [ts] */
// [ts] mountComponent: (transaction, hostParent, hostContainerInfo, context) => {
// [ts] if ("development" != "production") {
// [ts] let parentInfo;
// [ts] if (hostParent != null) {
// [ts] parentInfo = hostParent._ancestorInfo;
// [ts] } else if (hostContainerInfo != null) {
// [ts] parentInfo = hostContainerInfo._ancestorInfo;
// [ts] }
// [ts] if (parentInfo) {
// parentInfo should always be present except for the top-level
// component when server rendering
// [ts] validateDOMNesting(null, _._stringText, this, parentInfo);
// [ts] }
// [ts] }
// [ts] let domID = hostContainerInfo._idCounter++;
// [ts] let openingValue = " react-text: " + domID + " ";
// [ts] let closingValue = " /react-text ";
// [ts] _._domID = domID;
// [ts] _._hostParent = hostParent;
// [ts] if (transaction.useCreateElement) {
// [ts] let ownerDocument = hostContainerInfo._ownerDocument;
// [ts] let openingComment = ownerDocument.createComment(openingValue);
// [ts] let closingComment = ownerDocument.createComment(closingValue);
// [ts] let lazyTree = DOMLazyTree(ownerDocument.createDocumentFragment());
// [ts] DOMLazyTree.queueChild(lazyTree, DOMLazyTree(openingComment));
// [ts] if (_._stringText) {
// [ts] DOMLazyTree.queueChild(lazyTree, DOMLazyTree(ownerDocument.createTextNode(_._stringText)));
// [ts] }
// [ts] DOMLazyTree.queueChild(lazyTree, DOMLazyTree(closingComment));
// [ts] ReactDOMComponentTree.precacheNode(this, openingComment);
// [ts] _._closingComment = closingComment;
// [ts] return lazyTree;
// [ts] } else {
// [ts] let escapedText = escapeTextContentForBrowser(_._stringText);
// [ts] if (transaction.renderToStaticMarkup) {
// Normally we'd wrap this between comment nodes for the reasons stated
// above, but since this is a situation where React won't take over
// (static pages), we can simply return the text as it is.
// [ts] return escapedText;
// [ts] }
// [ts] return "<!--" + openingValue + "-->" + escapedText + "<!--" + closingValue + "-->";
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Updates this component by updating the text content.
// [ts] *
// [ts] * @param {ReactText} nextText The next text content
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @internal
// [ts] */
// [ts] receiveComponent: (nextText, transaction) => {
// [ts] if (nextText != _._currentElement) {
// [ts] _._currentElement = nextText;
// [ts] let nextStringText = "" + nextText;
// [ts] if (nextStringText != _._stringText) {
// TODO: Save this as pending props and use performUpdateIfNecessary
// and/or updateComponent to do the actual update for consistency with
// other component types?
// [ts] _._stringText = nextStringText;
// [ts] let commentNodes = _.getHostNode();
// [ts] DOMChildrenOperations.replaceDelimitedText(commentNodes[0], commentNodes[1], nextStringText);
// [ts] }
// [ts] }
// [ts] },
// [ts] getHostNode: () => {
// [ts] let hostNode = _._commentNodes;
// [ts] if (hostNode) {
// [ts] return hostNode;
// [ts] }
// [ts] if (!_._closingComment) {
// [ts] let openingComment = ReactDOMComponentTree.getNodeFromInstance(this);
// [ts] let node = openingComment.nextSibling;
// [ts] while (true) {
// [ts] if (!(node != null)) if ("development" != "production") invariant(false, "Missing closing comment for text component %s", _._domID) else _prodInvariant("67", _._domID) else void 0;
// [ts] if (node.nodeType == 8 && node.nodeValue == " /react-text ") {
// [ts] _._closingComment = node;
// [ts] break;
// [ts] }
// [ts] node = node.nextSibling;
// [ts] }
// [ts] }
// [ts] hostNode = [_._hostNode, _._closingComment];
// [ts] _._commentNodes = hostNode;
// [ts] return hostNode;
// [ts] },
// [ts] unmountComponent: () => {
// [ts] _._closingComment = null;
// [ts] _._commentNodes = null;
// [ts] ReactDOMComponentTree.uncacheNode(this);
// [ts] }
// [ts] });
// [ts] module.exports = ReactDOMTextComponent;
// [ts] },{"107":107,"126":126,"132":132,"152":152,"160":160,"34":34,"8":8,"9":9}],46:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] let LinkedValueUtils = _dereq_(24);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] let didWarnValueLink = false;
// [ts] let didWarnValDefaultVal = false;
// [ts] fn forceUpdateIfMounted() {
// [ts] if (_._rootNodeID) {
// DOM component is still mounted; update
// [ts] ReactDOMTextarea.updateWrapper(this);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Implements a <textarea> host component that allows setting `value`, and
// [ts] * `defaultValue`. This differs from the traditional DOM API because value is
// [ts] * usually set as PCDATA children.
// [ts] *
// [ts] * If `value` is not supplied (or null/undefined), user actions that affect the
// [ts] * value will trigger updates to the element.
// [ts] *
// [ts] * If `value` is supplied (and not null/undefined), the rendered element will
// [ts] * not trigger updates to the element. Instead, the `value` prop must change in
// [ts] * order for the rendered element to be updated.
// [ts] *
// [ts] * The rendered element will be initialized with an empty value, the prop
// [ts] * `defaultValue` if specified, or the children content (deprecated).
// [ts] */
// [ts] let ReactDOMTextarea = {
// [ts] getHostProps: (inst, props) => {
// [ts] if (!(props.dangerouslySetInnerHTML == null)) if ("development" != "production") invariant(false, "`dangerouslySetInnerHTML` does not make sense on <textarea>.") else _prodInvariant("91") else void 0;
// Always set children to the same thing. In IE9, the selection range will
// get reset if `textContent` is mutated.  We could add a check in setTextContent
// to only set the value if/when the value differs from the node value (which would
// completely solve this IE9 bug), but Sebastian+Ben seemed to like this solution.
// The value can be a boolean or object so that's why it's forced to be a string.
// [ts] let hostProps = _assign({}, props, {
// [ts] value: null,
// [ts] defaultValue: null,
// [ts] children: "" + inst._wrapperState.initialValue,
// [ts] onChange: inst._wrapperState.onChange
// [ts] });
// [ts] return hostProps;
// [ts] },
// [ts] mountWrapper: (inst, props) => {
// [ts] if ("development" != "production") {
// [ts] LinkedValueUtils.checkPropTypes("textarea", props, inst._currentElement._owner);
// [ts] if (props.valueLink != null && !didWarnValueLink) {
// [ts] if ("development" != "production") warning(false, "`valueLink` prop on `textarea` is deprecated; set `value` and `onChange` instead.") else void 0;
// [ts] didWarnValueLink = true;
// [ts] }
// [ts] if (props.value != null && props.defaultValue != null && !didWarnValDefaultVal) {
// [ts] if ("development" != "production") warning(false, "Textarea elements must be either controlled or uncontrolled " + "(specify either the value prop, or the defaultValue prop, but not " + "both). Decide between using a controlled or uncontrolled textarea " + "and remove one of these props. More info: " + "https://fb.me/react-controlled-components") else void 0;
// [ts] didWarnValDefaultVal = true;
// [ts] }
// [ts] }
// [ts] let value = LinkedValueUtils.getValue(props);
// [ts] let initialValue = value;
// Only bother fetching default value if we're going to use it
// [ts] if (value == null) {
// [ts] let defaultValue = props.defaultValue;
// TODO (yungsters): Remove support for children content in <textarea>.
// [ts] let children = props.children;
// [ts] if (children != null) {
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(false, "Use the `defaultValue` or `value` props instead of setting " + "children on <textarea>.") else void 0;
// [ts] }
// [ts] if (!(defaultValue == null)) if ("development" != "production") invariant(false, "If you supply `defaultValue` on a <textarea>, do not pass children.") else _prodInvariant("92") else void 0;
// [ts] if (Array.isArray(children)) {
// [ts] if (!(children.length <= 1)) if ("development" != "production") invariant(false, "<textarea> can only have at most one child.") else _prodInvariant("93") else void 0;
// [ts] children = children[0];
// [ts] }
// [ts] defaultValue = "" + children;
// [ts] }
// [ts] if (defaultValue == null) {
// [ts] defaultValue = "";
// [ts] }
// [ts] initialValue = defaultValue;
// [ts] }
// [ts] inst._wrapperState = {
// [ts] initialValue: "" + initialValue,
// [ts] listeners: null,
// [ts] onChange: _handleChange.bind(inst)
// [ts] };
// [ts] },
// [ts] updateWrapper: (inst) => {
// [ts] let props = inst._currentElement.props;
// [ts] let node = ReactDOMComponentTree.getNodeFromInstance(inst);
// [ts] let value = LinkedValueUtils.getValue(props);
// [ts] if (value != null) {
// Cast `value` to a string to ensure the value is set correctly. While
// browsers typically do this as necessary, jsdom doesn't.
// [ts] let newValue = "" + value;
// To avoid side effects (such as losing text selection), only set value if changed
// [ts] if (newValue != node.value) {
// [ts] node.value = newValue;
// [ts] }
// [ts] if (props.defaultValue == null) {
// [ts] node.defaultValue = newValue;
// [ts] }
// [ts] }
// [ts] if (props.defaultValue != null) {
// [ts] node.defaultValue = props.defaultValue;
// [ts] }
// [ts] },
// [ts] postMountWrapper: (inst) => {
// This is in postMount because we need access to the DOM node, which is not
// available until after the component has mounted.
// [ts] let node = ReactDOMComponentTree.getNodeFromInstance(inst);
// [ts] let textContent = node.textContent;
// Only set node.value if textContent is equal to the expected
// initial value. In IE10/IE11 there is a bug where the placeholder attribute
// will populate textContent as well.
// https://developer.microsoft.com/microsoft-edge/platform/issues/101525/
// [ts] if (textContent == inst._wrapperState.initialValue) {
// [ts] node.value = textContent;
// [ts] }
// [ts] }
// [ts] };
// [ts] fn _handleChange(event: auto) {
// [ts] let props = _._currentElement.props;
// [ts] let returnValue = LinkedValueUtils.executeOnChange(props, event);
// [ts] ReactUpdates.asap(forceUpdateIfMounted, this);
// [ts] return returnValue;
// [ts] }
// [ts] module.exports = ReactDOMTextarea;
// [ts] },{"126":126,"152":152,"159":159,"160":160,"24":24,"34":34,"82":82}],47:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2015-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Return the lowest common ancestor of A and B, or null if they are in
// [ts] * different trees.
// [ts] */
// [ts] fn getLowestCommonAncestor(instA: auto, instB: auto) {
// [ts] if (!("_hostNode" == instA)) if ("development" != "production") invariant(false, "getNodeFromInstance: Invalid argument.") else _prodInvariant("33") else void 0;
// [ts] if (!("_hostNode" == instB)) if ("development" != "production") invariant(false, "getNodeFromInstance: Invalid argument.") else _prodInvariant("33") else void 0;
// [ts] let depthA = 0;
// [ts] let tempA = instA;
// [ts] while (tempA) { // [cap-migrate] for-step: tempA = tempA._hostParent
// [ts] depthA++;
// [ts] }
// [ts] let depthB = 0;
// [ts] let tempB = instB;
// [ts] while (tempB) { // [cap-migrate] for-step: tempB = tempB._hostParent
// [ts] depthB++;
// [ts] }
// If A is deeper, crawl up.
// [ts] while (depthA - depthB > 0) {
// [ts] instA = instA._hostParent;
// [ts] depthA--;
// [ts] }
// If B is deeper, crawl up.
// [ts] while (depthB - depthA > 0) {
// [ts] instB = instB._hostParent;
// [ts] depthB--;
// [ts] }
// Walk in lockstep until we find a match.
// [ts] let depth = depthA;
// [ts] while (depth--) {
// [ts] if (instA == instB) {
// [ts] return instA;
// [ts] }
// [ts] instA = instA._hostParent;
// [ts] instB = instB._hostParent;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * Return if A is an ancestor of B.
// [ts] */
// [ts] fn isAncestor(instA: auto, instB: auto) {
// [ts] if (!("_hostNode" == instA)) if ("development" != "production") invariant(false, "isAncestor: Invalid argument.") else _prodInvariant("35") else void 0;
// [ts] if (!("_hostNode" == instB)) if ("development" != "production") invariant(false, "isAncestor: Invalid argument.") else _prodInvariant("35") else void 0;
// [ts] while (instB) {
// [ts] if (instB == instA) {
// [ts] return true;
// [ts] }
// [ts] instB = instB._hostParent;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] /**
// [ts] * Return the parent instance of the passed-in instance.
// [ts] */
// [ts] fn getParentInstance(inst: auto) {
// [ts] if (!("_hostNode" == inst)) if ("development" != "production") invariant(false, "getParentInstance: Invalid argument.") else _prodInvariant("36") else void 0;
// [ts] return inst._hostParent;
// [ts] }
// [ts] /**
// [ts] * Simulates the traversal of a two-phase, capture/bubble event dispatch.
// [ts] */
// [ts] fn traverseTwoPhase(inst: auto, fn: auto, arg: auto) {
// [ts] let path = [];
// [ts] while (inst) {
// [ts] path.push(inst);
// [ts] inst = inst._hostParent;
// [ts] }
// [ts] let i;
// [ts] for (i = path.length; i-- > 0;) {
// [ts] fn(path[i], "captured", arg);
// [ts] }
// [ts] i = 0;
// [ts] while (i < path.length) { // [cap-migrate] for-step: i++
// [ts] fn(path[i], "bubbled", arg);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Traverses the ID hierarchy and invokes the supplied `cb` on any IDs that
// [ts] * should would receive a `mouseEnter` or `mouseLeave` event.
// [ts] *
// [ts] * Does not invoke the callback on the nearest common ancestor because nothing
// [ts] * "entered" or "left" that element.
// [ts] */
// [ts] fn traverseEnterLeave(from: auto, to: auto, fn: auto, argFrom: auto, argTo: auto) {
// [ts] let common = if (from && to) getLowestCommonAncestor(from, to) else null;
// [ts] let pathFrom = [];
// [ts] while (from && from != common) {
// [ts] pathFrom.push(from);
// [ts] from = from._hostParent;
// [ts] }
// [ts] let pathTo = [];
// [ts] while (to && to != common) {
// [ts] pathTo.push(to);
// [ts] to = to._hostParent;
// [ts] }
// [ts] let i;
// [ts] i = 0;
// [ts] while (i < pathFrom.length) { // [cap-migrate] for-step: i++
// [ts] fn(pathFrom[i], "bubbled", argFrom);
// [ts] }
// [ts] for (i = pathTo.length; i-- > 0;) {
// [ts] fn(pathTo[i], "captured", argTo);
// [ts] }
// [ts] }
// [ts] module.exports = {
// [ts] isAncestor: isAncestor,
// [ts] getLowestCommonAncestor: getLowestCommonAncestor,
// [ts] getParentInstance: getParentInstance,
// [ts] traverseTwoPhase: traverseTwoPhase,
// [ts] traverseEnterLeave: traverseEnterLeave
// [ts] };
// [ts] },{"126":126,"152":152}],48:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let React = _dereq_(135);
// [ts] let ReactDOM = _dereq_(31);
// [ts] let ReactDOMUMDEntry = ReactDOM;
// [ts] if ("development" != "production") {
// [ts] ReactDOMUMDEntry.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED = {
// ReactPerf and ReactTestUtils currently only work with the DOM renderer
// so we expose them from here, but only in DEV mode.
// [ts] ReactPerf: _dereq_(71),
// [ts] ReactTestUtils: _dereq_(80)
// [ts] };
// [ts] }
// Inject ReactDOM into React for the addons UMD build that depends on ReactDOM (TransitionGroup).
// We can remove this after we deprecate and remove the addons UMD build.
// [ts] if (React.addons) {
// [ts] React.__SECRET_INJECTED_REACT_DOM_DO_NOT_USE_OR_YOU_WILL_BE_FIRED = ReactDOMUMDEntry;
// [ts] }
// [ts] module.exports = ReactDOMUMDEntry;
// [ts] },{"135":135,"31":31,"71":71,"80":80}],49:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMProperty = _dereq_(11);
// [ts] let EventPluginRegistry = _dereq_(18);
// [ts] let ReactComponentTreeHook = _dereq_(133);
// [ts] let warning = _dereq_(159);
// [ts] if ("development" != "production") {
// [ts] let reactProps = {
// [ts] children: true,
// [ts] dangerouslySetInnerHTML: true,
// [ts] key: true,
// [ts] ref: true,
// [ts] autoFocus: true,
// [ts] defaultValue: true,
// [ts] valueLink: true,
// [ts] defaultChecked: true,
// [ts] checkedLink: true,
// [ts] innerHTML: true,
// [ts] suppressContentEditableWarning: true,
// [ts] onFocusIn: true,
// [ts] onFocusOut: true
// [ts] };
// [ts] let warnedProperties = {};
// [ts] let validateProperty = (tagName, name, debugID) => {
// [ts] if (DOMProperty.properties.hasOwnProperty(name) || DOMProperty.isCustomAttribute(name)) {
// [ts] return true;
// [ts] }
// [ts] if (reactProps.hasOwnProperty(name) && reactProps[name] || warnedProperties.hasOwnProperty(name) && warnedProperties[name]) {
// [ts] return true;
// [ts] }
// [ts] if (EventPluginRegistry.registrationNameModules.hasOwnProperty(name)) {
// [ts] return true;
// [ts] }
// [ts] warnedProperties[name] = true;
// [ts] let lowerCasedName = name.toLowerCase();
// data-* attributes should be lowercase; suggest the lowercase version
// [ts] let standardName = if (DOMProperty.isCustomAttribute(lowerCasedName)) lowerCasedName else if (DOMProperty.getPossibleStandardName.hasOwnProperty(lowerCasedName)) DOMProperty.getPossibleStandardName[lowerCasedName] else null;
// [ts] let registrationName = if (EventPluginRegistry.possibleRegistrationNames.hasOwnProperty(lowerCasedName)) EventPluginRegistry.possibleRegistrationNames[lowerCasedName] else null;
// [ts] if (standardName != null) {
// [ts] if ("development" != "production") warning(false, "Unknown DOM property %s. Did you mean %s?%s", name, standardName, ReactComponentTreeHook.getStackAddendumByID(debugID)) else void 0;
// [ts] return true;
// [ts] } else if (registrationName != null) {
// [ts] if ("development" != "production") warning(false, "Unknown event handler property %s. Did you mean `%s`?%s", name, registrationName, ReactComponentTreeHook.getStackAddendumByID(debugID)) else void 0;
// [ts] return true;
// [ts] } else {
// We were unable to guess which prop the user intended.
// It is likely that the user was just blindly spreading/forwarding props
// Components should be careful to only render valid props/attributes.
// Warning will be invoked in warnUnknownProperties to allow grouping.
// [ts] return false;
// [ts] }
// [ts] };
// [ts] }
// [ts] let warnUnknownProperties = (debugID, element) => {
// [ts] let unknownProps = [];
// [ts] for (let key in element.props) {
// [ts] let isValid = validateProperty(element.type, key, debugID);
// [ts] if (!isValid) {
// [ts] unknownProps.push(key);
// [ts] }
// [ts] }
// [ts] let unknownPropString = unknownProps.map((prop) => {
// [ts] return "`" + prop + "`";
// [ts] }).join(", ");
// [ts] if (unknownProps.length == 1) {
// [ts] if ("development" != "production") warning(false, "Unknown prop %s on <%s> tag. Remove this prop from the element. " + "For details, see https://fb.me/react-unknown-prop%s", unknownPropString, element.type, ReactComponentTreeHook.getStackAddendumByID(debugID)) else void 0;
// [ts] } else if (unknownProps.length > 1) {
// [ts] if ("development" != "production") warning(false, "Unknown props %s on <%s> tag. Remove these props from the element. " + "For details, see https://fb.me/react-unknown-prop%s", unknownPropString, element.type, ReactComponentTreeHook.getStackAddendumByID(debugID)) else void 0;
// [ts] }
// [ts] };
// [ts] fn handleElement(debugID: auto, element: auto) {
// [ts] if (element == null || @typeName(element.type) != "string") {
// [ts] return;
// [ts] }
// [ts] if (element.type.indexOf("-") >= 0 || element.props.is) {
// [ts] return;
// [ts] }
// [ts] warnUnknownProperties(debugID, element);
// [ts] }
// [ts] let ReactDOMUnknownPropertyHook = {
// [ts] onBeforeMountComponent: (debugID, element) => {
// [ts] handleElement(debugID, element);
// [ts] },
// [ts] onBeforeUpdateComponent: (debugID, element) => {
// [ts] handleElement(debugID, element);
// [ts] }
// [ts] };
// [ts] module.exports = ReactDOMUnknownPropertyHook;
// [ts] },{"11":11,"133":133,"159":159,"18":18}],50:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2016-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactInvalidSetStateWarningHook = _dereq_(65);
// [ts] let ReactHostOperationHistoryHook = _dereq_(60);
// [ts] let ReactComponentTreeHook = _dereq_(133);
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let performanceNow = _dereq_(157);
// [ts] let warning = _dereq_(159);
// [ts] let hooks = [];
// [ts] let didHookThrowForEvent = {};
// [ts] fn callHook(event: auto, fn: auto, context: auto, arg1: auto, arg2: auto, arg3: auto, arg4: auto, arg5: auto) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] fn.call(context, arg1, arg2, arg3, arg4, arg5);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] if ("development" != "production") warning(didHookThrowForEvent[event], "Exception thrown by hook while handling %s: %s", event, e + "\n" + e.stack) else void 0;
// [ts] didHookThrowForEvent[event] = true;
// [ts] }
// [ts] }
// [ts] fn emitEvent(event: auto, arg1: auto, arg2: auto, arg3: auto, arg4: auto, arg5: auto) {
// [ts] let i = 0;
// [ts] while (i < hooks.length) { // [cap-migrate] for-step: i++
// [ts] let hook = hooks[i];
// [ts] let fn = hook[event];
// [ts] if (fn) {
// [ts] callHook(event, fn, hook, arg1, arg2, arg3, arg4, arg5);
// [ts] }
// [ts] }
// [ts] }
// [ts] let isProfiling = false;
// [ts] let flushHistory = [];
// [ts] let lifeCycleTimerStack = [];
// [ts] let currentFlushNesting = 0;
// [ts] let currentFlushMeasurements = [];
// [ts] let currentFlushStartTime = 0;
// [ts] let currentTimerDebugID = null;
// [ts] let currentTimerStartTime = 0;
// [ts] let currentTimerNestedFlushDuration = 0;
// [ts] let currentTimerType = null;
// [ts] let lifeCycleTimerHasWarned = false;
// [ts] fn clearHistory() {
// [ts] ReactComponentTreeHook.purgeUnmountedComponents();
// [ts] ReactHostOperationHistoryHook.clearHistory();
// [ts] }
// [ts] fn getTreeSnapshot(registeredIDs: auto) {
// [ts] return registeredIDs.reduce((tree, id) => {
// [ts] let ownerID = ReactComponentTreeHook.getOwnerID(id);
// [ts] let parentID = ReactComponentTreeHook.getParentID(id);
// [ts] tree[id] = {
// [ts] displayName: ReactComponentTreeHook.getDisplayName(id),
// [ts] text: ReactComponentTreeHook.getText(id),
// [ts] updateCount: ReactComponentTreeHook.getUpdateCount(id),
// [ts] childIDs: ReactComponentTreeHook.getChildIDs(id),
// Text nodes don't have owners but this is close enough.
// [ts] ownerID: ownerID || parentID && ReactComponentTreeHook.getOwnerID(parentID) || 0,
// [ts] parentID: parentID
// [ts] };
// [ts] return tree;
// [ts] }, {});
// [ts] }
// [ts] fn resetMeasurements() {
// [ts] let previousStartTime = currentFlushStartTime;
// [ts] let previousMeasurements = currentFlushMeasurements;
// [ts] let previousOperations = ReactHostOperationHistoryHook.getHistory();
// [ts] if (currentFlushNesting == 0) {
// [ts] currentFlushStartTime = 0;
// [ts] currentFlushMeasurements = [];
// [ts] clearHistory();
// [ts] return;
// [ts] }
// [ts] if (previousMeasurements.length || previousOperations.length) {
// [ts] let registeredIDs = ReactComponentTreeHook.getRegisteredIDs();
// [ts] flushHistory.push({
// [ts] duration: performanceNow() - previousStartTime,
// [ts] measurements: previousMeasurements || [],
// [ts] operations: previousOperations || [],
// [ts] treeSnapshot: getTreeSnapshot(registeredIDs)
// [ts] });
// [ts] }
// [ts] clearHistory();
// [ts] currentFlushStartTime = performanceNow();
// [ts] currentFlushMeasurements = [];
// [ts] }
// [ts] fn checkDebugID(debugID: auto) {
// [ts] let allowRoot = if (arguments.length > 1 && arguments[1] != null) arguments[1] else false;
// [ts] if (allowRoot && debugID == 0) {
// [ts] return;
// [ts] }
// [ts] if (!debugID) {
// [ts] if ("development" != "production") warning(false, "ReactDebugTool: debugID may not be empty.") else void 0;
// [ts] }
// [ts] }
// [ts] fn beginLifeCycleTimer(debugID: auto, timerType: auto) {
// [ts] if (currentFlushNesting == 0) {
// [ts] return;
// [ts] }
// [ts] if (currentTimerType && !lifeCycleTimerHasWarned) {
// [ts] if ("development" != "production") warning(if (false, "There is an internal error in the React performance measurement code. " + "Did not expect %s timer to start while %s timer is still in " + "progress for %s instance.", timerType, currentTimerType || "no", debugID == currentTimerDebugID) "the same" else "another") else void 0;
// [ts] lifeCycleTimerHasWarned = true;
// [ts] }
// [ts] currentTimerStartTime = performanceNow();
// [ts] currentTimerNestedFlushDuration = 0;
// [ts] currentTimerDebugID = debugID;
// [ts] currentTimerType = timerType;
// [ts] }
// [ts] fn endLifeCycleTimer(debugID: auto, timerType: auto) {
// [ts] if (currentFlushNesting == 0) {
// [ts] return;
// [ts] }
// [ts] if (currentTimerType != timerType && !lifeCycleTimerHasWarned) {
// [ts] if ("development" != "production") warning(if (false, "There is an internal error in the React performance measurement code. " + "We did not expect %s timer to stop while %s timer is still in " + "progress for %s instance. Please report this as a bug in React.", timerType, currentTimerType || "no", debugID == currentTimerDebugID) "the same" else "another") else void 0;
// [ts] lifeCycleTimerHasWarned = true;
// [ts] }
// [ts] if (isProfiling) {
// [ts] currentFlushMeasurements.push({
// [ts] timerType: timerType,
// [ts] instanceID: debugID,
// [ts] duration: performanceNow() - currentTimerStartTime - currentTimerNestedFlushDuration
// [ts] });
// [ts] }
// [ts] currentTimerStartTime = 0;
// [ts] currentTimerNestedFlushDuration = 0;
// [ts] currentTimerDebugID = null;
// [ts] currentTimerType = null;
// [ts] }
// [ts] fn pauseCurrentLifeCycleTimer() {
// [ts] let currentTimer = {
// [ts] startTime: currentTimerStartTime,
// [ts] nestedFlushStartTime: performanceNow(),
// [ts] debugID: currentTimerDebugID,
// [ts] timerType: currentTimerType
// [ts] };
// [ts] lifeCycleTimerStack.push(currentTimer);
// [ts] currentTimerStartTime = 0;
// [ts] currentTimerNestedFlushDuration = 0;
// [ts] currentTimerDebugID = null;
// [ts] currentTimerType = null;
// [ts] }
// [ts] fn resumeCurrentLifeCycleTimer() {
// [ts] let _lifeCycleTimerStack$ = lifeCycleTimerStack.pop(),
// [ts] startTime = _lifeCycleTimerStack$.startTime,
// [ts] nestedFlushStartTime = _lifeCycleTimerStack$.nestedFlushStartTime,
// [ts] debugID = _lifeCycleTimerStack$.debugID,
// [ts] timerType = _lifeCycleTimerStack$.timerType;
// [ts] let nestedFlushDuration = performanceNow() - nestedFlushStartTime;
// [ts] currentTimerStartTime = startTime;
// [ts] currentTimerNestedFlushDuration += nestedFlushDuration;
// [ts] currentTimerDebugID = debugID;
// [ts] currentTimerType = timerType;
// [ts] }
// [ts] let lastMarkTimeStamp = 0;
// [ts] let canUsePerformanceMeasure = @typeName(performance) != "undefined" && @typeName(performance.mark) == "function" && @typeName(performance.clearMarks) == "function" && @typeName(performance.measure) == "function" && @typeName(performance.clearMeasures) == "function";
// [ts] fn shouldMark(debugID: auto) {
// [ts] if (!isProfiling || !canUsePerformanceMeasure) {
// [ts] return false;
// [ts] }
// [ts] let element = ReactComponentTreeHook.getElement(debugID);
// [ts] if (element == null || @typeName(element) != "object") {
// [ts] return false;
// [ts] }
// [ts] let isHostElement = @typeName(element.type) == "string";
// [ts] if (isHostElement) {
// [ts] return false;
// [ts] }
// [ts] return true;
// [ts] }
// [ts] fn markBegin(debugID: auto, markType: auto) {
// [ts] if (!shouldMark(debugID)) {
// [ts] return;
// [ts] }
// [ts] let markName = debugID + "::" + markType;
// [ts] lastMarkTimeStamp = performanceNow();
// [ts] performance.mark(markName);
// [ts] }
// [ts] fn markEnd(debugID: auto, markType: auto) {
// [ts] if (!shouldMark(debugID)) {
// [ts] return;
// [ts] }
// [ts] let markName = debugID + "::" + markType;
// [ts] let displayName = ReactComponentTreeHook.getDisplayName(debugID) || "Unknown";
// Chrome has an issue of dropping markers recorded too fast:
// https://bugs.chromium.org/p/chromium/issues/detail?id=640652
// To work around this, we will not report very small measurements.
// I determined the magic number by tweaking it back and forth.
// 0.05ms was enough to prevent the issue, but I set it to 0.1ms to be safe.
// When the bug is fixed, we can `measure()` unconditionally if we want to.
// [ts] let timeStamp = performanceNow();
// [ts] if (timeStamp - lastMarkTimeStamp > 0.1) {
// [ts] let measurementName = displayName + " [" + markType + "]";
// [ts] performance.measure(measurementName, markName);
// [ts] }
// [ts] performance.clearMarks(markName);
// [ts] if (measurementName) {
// [ts] performance.clearMeasures(measurementName);
// [ts] }
// [ts] }
// [ts] let ReactDebugTool = {
// [ts] addHook: (hook) => {
// [ts] hooks.push(hook);
// [ts] },
// [ts] removeHook: (hook) => {
// [ts] let i = 0;
// [ts] while (i < hooks.length) { // [cap-migrate] for-step: i++
// [ts] if (hooks[i] == hook) {
// [ts] hooks.splice(i, 1);
// [ts] i--;
// [ts] }
// [ts] }
// [ts] },
// [ts] isProfiling: () => {
// [ts] return isProfiling;
// [ts] },
// [ts] beginProfiling: () => {
// [ts] if (isProfiling) {
// [ts] return;
// [ts] }
// [ts] isProfiling = true;
// [ts] flushHistory.length = 0;
// [ts] resetMeasurements();
// [ts] ReactDebugTool.addHook(ReactHostOperationHistoryHook);
// [ts] },
// [ts] endProfiling: () => {
// [ts] if (!isProfiling) {
// [ts] return;
// [ts] }
// [ts] isProfiling = false;
// [ts] resetMeasurements();
// [ts] ReactDebugTool.removeHook(ReactHostOperationHistoryHook);
// [ts] },
// [ts] getFlushHistory: () => {
// [ts] return flushHistory;
// [ts] },
// [ts] onBeginFlush: () => {
// [ts] currentFlushNesting++;
// [ts] resetMeasurements();
// [ts] pauseCurrentLifeCycleTimer();
// [ts] emitEvent("onBeginFlush");
// [ts] },
// [ts] onEndFlush: () => {
// [ts] resetMeasurements();
// [ts] currentFlushNesting--;
// [ts] resumeCurrentLifeCycleTimer();
// [ts] emitEvent("onEndFlush");
// [ts] },
// [ts] onBeginLifeCycleTimer: (debugID, timerType) => {
// [ts] checkDebugID(debugID);
// [ts] emitEvent("onBeginLifeCycleTimer", debugID, timerType);
// [ts] markBegin(debugID, timerType);
// [ts] beginLifeCycleTimer(debugID, timerType);
// [ts] },
// [ts] onEndLifeCycleTimer: (debugID, timerType) => {
// [ts] checkDebugID(debugID);
// [ts] endLifeCycleTimer(debugID, timerType);
// [ts] markEnd(debugID, timerType);
// [ts] emitEvent("onEndLifeCycleTimer", debugID, timerType);
// [ts] },
// [ts] onBeginProcessingChildContext: () => {
// [ts] emitEvent("onBeginProcessingChildContext");
// [ts] },
// [ts] onEndProcessingChildContext: () => {
// [ts] emitEvent("onEndProcessingChildContext");
// [ts] },
// [ts] onHostOperation: (operation) => {
// [ts] checkDebugID(operation.instanceID);
// [ts] emitEvent("onHostOperation", operation);
// [ts] },
// [ts] onSetState: () => {
// [ts] emitEvent("onSetState");
// [ts] },
// [ts] onSetChildren: (debugID, childDebugIDs) => {
// [ts] checkDebugID(debugID);
// [ts] childDebugIDs.forEach(checkDebugID);
// [ts] emitEvent("onSetChildren", debugID, childDebugIDs);
// [ts] },
// [ts] onBeforeMountComponent: (debugID, element, parentDebugID) => {
// [ts] checkDebugID(debugID);
// [ts] checkDebugID(parentDebugID, true);
// [ts] emitEvent("onBeforeMountComponent", debugID, element, parentDebugID);
// [ts] markBegin(debugID, "mount");
// [ts] },
// [ts] onMountComponent: (debugID) => {
// [ts] checkDebugID(debugID);
// [ts] markEnd(debugID, "mount");
// [ts] emitEvent("onMountComponent", debugID);
// [ts] },
// [ts] onBeforeUpdateComponent: (debugID, element) => {
// [ts] checkDebugID(debugID);
// [ts] emitEvent("onBeforeUpdateComponent", debugID, element);
// [ts] markBegin(debugID, "update");
// [ts] },
// [ts] onUpdateComponent: (debugID) => {
// [ts] checkDebugID(debugID);
// [ts] markEnd(debugID, "update");
// [ts] emitEvent("onUpdateComponent", debugID);
// [ts] },
// [ts] onBeforeUnmountComponent: (debugID) => {
// [ts] checkDebugID(debugID);
// [ts] emitEvent("onBeforeUnmountComponent", debugID);
// [ts] markBegin(debugID, "unmount");
// [ts] },
// [ts] onUnmountComponent: (debugID) => {
// [ts] checkDebugID(debugID);
// [ts] markEnd(debugID, "unmount");
// [ts] emitEvent("onUnmountComponent", debugID);
// [ts] },
// [ts] onTestEvent: () => {
// [ts] emitEvent("onTestEvent");
// [ts] }
// [ts] };
// TODO remove these when RN/www gets updated
// [ts] ReactDebugTool.addDevtool = ReactDebugTool.addHook;
// [ts] ReactDebugTool.removeDevtool = ReactDebugTool.removeHook;
// [ts] ReactDebugTool.addHook(ReactInvalidSetStateWarningHook);
// [ts] ReactDebugTool.addHook(ReactComponentTreeHook);
// [ts] let url = ExecutionEnvironment.canUseDOM && window.location.href || "";
// [ts] if ("[?&]react_perf\\b".test(url)) {
// [ts] ReactDebugTool.beginProfiling();
// [ts] }
// [ts] module.exports = ReactDebugTool;
// [ts] },{"133":133,"138":138,"157":157,"159":159,"60":60,"65":65}],51:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let Transaction = _dereq_(100);
// [ts] let emptyFunction = _dereq_(144);
// [ts] let RESET_BATCHED_UPDATES = {
// [ts] initialize: emptyFunction,
// [ts] close: () => {
// [ts] ReactDefaultBatchingStrategy.isBatchingUpdates = false;
// [ts] }
// [ts] };
// [ts] let FLUSH_BATCHED_UPDATES = {
// [ts] initialize: emptyFunction,
// [ts] close: ReactUpdates.flushBatchedUpdates.bind(ReactUpdates)
// [ts] };
// [ts] let TRANSACTION_WRAPPERS = [FLUSH_BATCHED_UPDATES, RESET_BATCHED_UPDATES];
// [ts] fn ReactDefaultBatchingStrategyTransaction() {
// [ts] _.reinitializeTransaction();
// [ts] }
// [ts] _assign(ReactDefaultBatchingStrategyTransaction.prototype, Transaction, {
// [ts] getTransactionWrappers: () => {
// [ts] return TRANSACTION_WRAPPERS;
// [ts] }
// [ts] });
// [ts] let transaction = ReactDefaultBatchingStrategyTransaction.init();
// [ts] let ReactDefaultBatchingStrategy = {
// [ts] isBatchingUpdates: false,
// [ts] /**
// [ts] * Call the provided function in a context within which calls to `setState`
// [ts] * and friends are batched such that components aren't updated unnecessarily.
// [ts] */
// [ts] batchedUpdates: (callback, a, b, c, d, e) => {
// [ts] let alreadyBatchingUpdates = ReactDefaultBatchingStrategy.isBatchingUpdates;
// [ts] ReactDefaultBatchingStrategy.isBatchingUpdates = true;
// The code is written this way to avoid extra allocations
// [ts] if (alreadyBatchingUpdates) {
// [ts] return callback(a, b, c, d, e);
// [ts] } else {
// [ts] return transaction.perform(callback, null, a, b, c, d, e);
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactDefaultBatchingStrategy;
// [ts] },{"100":100,"144":144,"160":160,"82":82}],52:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ARIADOMPropertyConfig = _dereq_(1);
// [ts] let BeforeInputEventPlugin = _dereq_(3);
// [ts] let ChangeEventPlugin = _dereq_(7);
// [ts] let DefaultEventPluginOrder = _dereq_(14);
// [ts] let EnterLeaveEventPlugin = _dereq_(15);
// [ts] let HTMLDOMPropertyConfig = _dereq_(22);
// [ts] let ReactComponentBrowserEnvironment = _dereq_(28);
// [ts] let ReactDOMComponent = _dereq_(32);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactDOMEmptyComponent = _dereq_(36);
// [ts] let ReactDOMTreeTraversal = _dereq_(47);
// [ts] let ReactDOMTextComponent = _dereq_(45);
// [ts] let ReactDefaultBatchingStrategy = _dereq_(51);
// [ts] let ReactEventListener = _dereq_(57);
// [ts] let ReactInjection = _dereq_(61);
// [ts] let ReactReconcileTransaction = _dereq_(74);
// [ts] let SVGDOMPropertyConfig = _dereq_(84);
// [ts] let SelectEventPlugin = _dereq_(85);
// [ts] let SimpleEventPlugin = _dereq_(86);
// [ts] let alreadyInjected = false;
// [ts] fn inject() {
// [ts] if (alreadyInjected) {
// TODO: This is currently true because these injections are shared between
// the client and the server package. They should be built independently
// and not share any injection state. Then this problem will be solved.
// [ts] return;
// [ts] }
// [ts] alreadyInjected = true;
// [ts] ReactInjection.EventEmitter.injectReactEventListener(ReactEventListener);
// [ts] /**
// [ts] * Inject modules for resolving DOM hierarchy and plugin ordering.
// [ts] */
// [ts] ReactInjection.EventPluginHub.injectEventPluginOrder(DefaultEventPluginOrder);
// [ts] ReactInjection.EventPluginUtils.injectComponentTree(ReactDOMComponentTree);
// [ts] ReactInjection.EventPluginUtils.injectTreeTraversal(ReactDOMTreeTraversal);
// [ts] /**
// [ts] * Some important event plugins included by default (without having to require
// [ts] * them).
// [ts] */
// [ts] ReactInjection.EventPluginHub.injectEventPluginsByName({
// [ts] SimpleEventPlugin: SimpleEventPlugin,
// [ts] EnterLeaveEventPlugin: EnterLeaveEventPlugin,
// [ts] ChangeEventPlugin: ChangeEventPlugin,
// [ts] SelectEventPlugin: SelectEventPlugin,
// [ts] BeforeInputEventPlugin: BeforeInputEventPlugin
// [ts] });
// [ts] ReactInjection.HostComponent.injectGenericComponentClass(ReactDOMComponent);
// [ts] ReactInjection.HostComponent.injectTextComponentClass(ReactDOMTextComponent);
// [ts] ReactInjection.DOMProperty.injectDOMPropertyConfig(ARIADOMPropertyConfig);
// [ts] ReactInjection.DOMProperty.injectDOMPropertyConfig(HTMLDOMPropertyConfig);
// [ts] ReactInjection.DOMProperty.injectDOMPropertyConfig(SVGDOMPropertyConfig);
// [ts] ReactInjection.EmptyComponent.injectEmptyComponentFactory((instantiate) => {
// [ts] return ReactDOMEmptyComponent.init(instantiate);
// [ts] });
// [ts] ReactInjection.Updates.injectReconcileTransaction(ReactReconcileTransaction);
// [ts] ReactInjection.Updates.injectBatchingStrategy(ReactDefaultBatchingStrategy);
// [ts] ReactInjection.Component.injectEnvironment(ReactComponentBrowserEnvironment);
// [ts] }
// [ts] module.exports = {
// [ts] inject: inject
// [ts] };
// [ts] },{"1":1,"14":14,"15":15,"22":22,"28":28,"3":3,"32":32,"34":34,"36":36,"45":45,"47":47,"51":51,"57":57,"61":61,"7":7,"74":74,"84":84,"85":85,"86":86}],53:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// The Symbol used to tag the ReactElement type. If there is no native Symbol
// nor polyfill, then a plain number is used for performance.
// [ts] let REACT_ELEMENT_TYPE = @typeName(Symbol) == "function" && Symbol["for"] && Symbol["for"]("react.element") || 0xeac7;
// [ts] module.exports = REACT_ELEMENT_TYPE;
// [ts] },{}],54:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let emptyComponentFactory;
// [ts] let ReactEmptyComponentInjection = {
// [ts] injectEmptyComponentFactory: (factory) => {
// [ts] emptyComponentFactory = factory;
// [ts] }
// [ts] };
// [ts] let ReactEmptyComponent = {
// [ts] create: (instantiate) => {
// [ts] return emptyComponentFactory(instantiate);
// [ts] }
// [ts] };
// [ts] ReactEmptyComponent.injection = ReactEmptyComponentInjection;
// [ts] module.exports = ReactEmptyComponent;
// [ts] },{}],55:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let caughtError = null;
// [ts] /**
// [ts] * Call a function while guarding against errors that happens within it.
// [ts] *
// [ts] * @param {String} name of the guard to use for logging or debugging
// [ts] * @param {Function} func The function to invoke
// [ts] * @param {*} a First argument
// [ts] * @param {*} b Second argument
// [ts] */
// [ts] fn invokeGuardedCallback(name: auto, func: auto, a: auto) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] func(a);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {)
// [ts] if (caughtError == null) {
// [ts] caughtError = x;
// [ts] }
// [ts] }
// [ts] }
// [ts] let ReactErrorUtils = {
// [ts] invokeGuardedCallback: invokeGuardedCallback,
// [ts] /**
// [ts] * Invoked by ReactTestUtils.Simulate so that any errors thrown by the event
// [ts] * handler are sure to be rethrown by rethrowCaughtError.
// [ts] */
// [ts] invokeGuardedCallbackWithCatch: invokeGuardedCallback,
// [ts] /**
// [ts] * During execution of guarded functions we will capture the first error which
// [ts] * we will rethrow to be handled by the top level error handler.
// [ts] */
// [ts] rethrowCaughtError: () => {
// [ts] if (caughtError) {
// [ts] let error = caughtError;
// [ts] caughtError = null;
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] }
// [ts] }
// [ts] };
// [ts] if ("development" != "production") {
// [ts] /**
// [ts] * To help development we can get better devtools integration by simulating a
// [ts] * real browser event.
// [ts] */
// [ts] if (@typeName(window) != "undefined" && @typeName(window.dispatchEvent) == "function" && @typeName(document) != "undefined" && @typeName(document.createEvent) == "function") {
// [ts] let fakeNode = document.createElement("react");
// [ts] ReactErrorUtils.invokeGuardedCallback = (name, func, a) => {
// [ts] let boundFunc = () => {
// [ts] func(a);
// [ts] };
// [ts] let evtType = "react-" + name;
// [ts] fakeNode.addEventListener(evtType, boundFunc, false);
// [ts] let evt = document.createEvent("Event");
// [ts] evt.initEvent(evtType, false, false);
// [ts] fakeNode.dispatchEvent(evt);
// [ts] fakeNode.removeEventListener(evtType, boundFunc, false);
// [ts] };
// [ts] }
// [ts] }
// [ts] module.exports = ReactErrorUtils;
// [ts] },{}],56:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let EventPluginHub = _dereq_(17);
// [ts] fn runEventQueueInBatch(events: auto) {
// [ts] EventPluginHub.enqueueEvents(events);
// [ts] EventPluginHub.processEventQueue(false);
// [ts] }
// [ts] let ReactEventEmitterMixin = {
// [ts] /**
// [ts] * Streams a fired top-level event to `EventPluginHub` where plugins have the
// [ts] * opportunity to create `ReactEvent`s to be dispatched.
// [ts] */
// [ts] handleTopLevel: (topLevelType, targetInst, nativeEvent, nativeEventTarget) => {
// [ts] let events = EventPluginHub.extractEvents(topLevelType, targetInst, nativeEvent, nativeEventTarget);
// [ts] runEventQueueInBatch(events);
// [ts] }
// [ts] };
// [ts] module.exports = ReactEventEmitterMixin;
// [ts] },{"17":17}],57:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let EventListener = _dereq_(137);
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let PooledClass = _dereq_(25);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let getEventTarget = _dereq_(114);
// [ts] let getUnboundedScrollPosition = _dereq_(149);
// [ts] /**
// [ts] * Find the deepest React component completely containing the root of the
// [ts] * passed-in instance (for use when entire React trees are nested within each
// [ts] * other). If React trees are not nested, returns null.
// [ts] */
// [ts] fn findParent(inst: auto) {
// TODO: It may be a good idea to cache this to prevent unnecessary DOM
// traversal, but caching is difficult to do correctly without using a
// mutation observer to listen for all DOM changes.
// [ts] while (inst._hostParent) {
// [ts] inst = inst._hostParent;
// [ts] }
// [ts] let rootNode = ReactDOMComponentTree.getNodeFromInstance(inst);
// [ts] let container = rootNode.parentNode;
// [ts] return ReactDOMComponentTree.getClosestInstanceFromNode(container);
// [ts] }
// Used to store ancestor hierarchy in top level callback
// [ts] fn TopLevelCallbackBookKeeping(topLevelType: auto, nativeEvent: auto) {
// [ts] _.topLevelType = topLevelType;
// [ts] _.nativeEvent = nativeEvent;
// [ts] _.ancestors = [];
// [ts] }
// [ts] _assign(TopLevelCallbackBookKeeping.prototype, {
// [ts] destructor: () => {
// [ts] _.topLevelType = null;
// [ts] _.nativeEvent = null;
// [ts] _.ancestors.length = 0;
// [ts] }
// [ts] });
// [ts] PooledClass.addPoolingTo(TopLevelCallbackBookKeeping, PooledClass.twoArgumentPooler);
// [ts] fn handleTopLevelImpl(bookKeeping: auto) {
// [ts] let nativeEventTarget = getEventTarget(bookKeeping.nativeEvent);
// [ts] let targetInst = ReactDOMComponentTree.getClosestInstanceFromNode(nativeEventTarget);
// Loop through the hierarchy, in case there's any nested components.
// It's important that we build the array of ancestors before calling any
// event handlers, because event handlers can modify the DOM, leading to
// inconsistencies with ReactMount's node cache. See #1105.
// [ts] let ancestor = targetInst;
// [ts] do {
// [ts] bookKeeping.ancestors.push(ancestor);
// [ts] ancestor = ancestor && findParent(ancestor);
// [ts] } while (ancestor);
// [ts] let i = 0;
// [ts] while (i < bookKeeping.ancestors.length) { // [cap-migrate] for-step: i++
// [ts] targetInst = bookKeeping.ancestors[i];
// [ts] ReactEventListener._handleTopLevel(bookKeeping.topLevelType, targetInst, bookKeeping.nativeEvent, getEventTarget(bookKeeping.nativeEvent));
// [ts] }
// [ts] }
// [ts] fn scrollValueMonitor(cb: auto) {
// [ts] let scrollPosition = getUnboundedScrollPosition(window);
// [ts] cb(scrollPosition);
// [ts] }
// [ts] let ReactEventListener = {
// [ts] _enabled: true,
// [ts] _handleTopLevel: null,
// [ts] WINDOW_HANDLE: if (ExecutionEnvironment.canUseDOM) window else null,
// [ts] setHandleTopLevel: (handleTopLevel) => {
// [ts] ReactEventListener._handleTopLevel = handleTopLevel;
// [ts] },
// [ts] setEnabled: (enabled) => {
// [ts] ReactEventListener._enabled = !!enabled;
// [ts] },
// [ts] isEnabled: () => {
// [ts] return ReactEventListener._enabled;
// [ts] },
// [ts] /**
// [ts] * Traps top-level events by using event bubbling.
// [ts] *
// [ts] * @param {string} topLevelType Record from `EventConstants`.
// [ts] * @param {string} handlerBaseName Event name (e.g. "click").
// [ts] * @param {object} element Element on which to attach listener.
// [ts] * @return {?object} An object with a remove function which will forcefully
// [ts] *                  remove the listener.
// [ts] * @internal
// [ts] */
// [ts] trapBubbledEvent: (topLevelType, handlerBaseName, element) => {
// [ts] if (!element) {
// [ts] return null;
// [ts] }
// [ts] return EventListener.listen(element, handlerBaseName, ReactEventListener.dispatchEvent.bind(null, topLevelType));
// [ts] },
// [ts] /**
// [ts] * Traps a top-level event by using event capturing.
// [ts] *
// [ts] * @param {string} topLevelType Record from `EventConstants`.
// [ts] * @param {string} handlerBaseName Event name (e.g. "click").
// [ts] * @param {object} element Element on which to attach listener.
// [ts] * @return {?object} An object with a remove function which will forcefully
// [ts] *                  remove the listener.
// [ts] * @internal
// [ts] */
// [ts] trapCapturedEvent: (topLevelType, handlerBaseName, element) => {
// [ts] if (!element) {
// [ts] return null;
// [ts] }
// [ts] return EventListener.capture(element, handlerBaseName, ReactEventListener.dispatchEvent.bind(null, topLevelType));
// [ts] },
// [ts] monitorScrollValue: (refresh) => {
// [ts] let callback = scrollValueMonitor.bind(null, refresh);
// [ts] EventListener.listen(window, "scroll", callback);
// [ts] },
// [ts] dispatchEvent: (topLevelType, nativeEvent) => {
// [ts] if (!ReactEventListener._enabled) {
// [ts] return;
// [ts] }
// [ts] let bookKeeping = TopLevelCallbackBookKeeping.getPooled(topLevelType, nativeEvent);
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Event queue being processed in the same cycle allows
// `preventDefault`.
// [ts] ReactUpdates.batchedUpdates(handleTopLevelImpl, bookKeeping);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] TopLevelCallbackBookKeeping.release(bookKeeping);
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactEventListener;
// [ts] },{"114":114,"137":137,"138":138,"149":149,"160":160,"25":25,"34":34,"82":82}],58:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactFeatureFlags = {
// When true, call console.time() before and .timeEnd() after each top-level
// render (both initial renders and updates). Useful when looking at prod-mode
// timeline profiles in Chrome, for example.
// [ts] logTopLevelRenders: false
// [ts] };
// [ts] module.exports = ReactFeatureFlags;
// [ts] },{}],59:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] let genericComponentClass = null;
// [ts] let textComponentClass = null;
// [ts] let ReactHostComponentInjection = {
// This accepts a class that receives the tag string. This is a catch all
// that can render any kind of tag.
// [ts] injectGenericComponentClass: (componentClass) => {
// [ts] genericComponentClass = componentClass;
// [ts] },
// This accepts a text component class that takes the text string to be
// rendered as props.
// [ts] injectTextComponentClass: (componentClass) => {
// [ts] textComponentClass = componentClass;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Get a host internal component class for a specific tag.
// [ts] *
// [ts] * @param {ReactElement} element The element to create.
// [ts] * @return {function} The internal class constructor function.
// [ts] */
// [ts] fn createInternalComponent(element: auto) {
// [ts] if (!genericComponentClass) if ("development" != "production") invariant(false, "There is no registered component for the tag %s", element.type) else _prodInvariant("111", element.type) else void 0;
// [ts] return genericComponentClass.init(element);
// [ts] }
// [ts] /**
// [ts] * @param {ReactText} text
// [ts] * @return {ReactComponent}
// [ts] */
// [ts] fn createInstanceForText(text: auto) {
// [ts] return textComponentClass.init(text);
// [ts] }
// [ts] /**
// [ts] * @param {ReactComponent} component
// [ts] * @return {boolean}
// [ts] */
// [ts] fn isTextComponent(component: auto) {
// [ts] return component == textComponentClass;
// [ts] }
// [ts] let ReactHostComponent = {
// [ts] createInternalComponent: createInternalComponent,
// [ts] createInstanceForText: createInstanceForText,
// [ts] isTextComponent: isTextComponent,
// [ts] injection: ReactHostComponentInjection
// [ts] };
// [ts] module.exports = ReactHostComponent;
// [ts] },{"126":126,"152":152}],60:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2016-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let history = [];
// [ts] let ReactHostOperationHistoryHook = {
// [ts] onHostOperation: (operation) => {
// [ts] history.push(operation);
// [ts] },
// [ts] clearHistory: () => {
// [ts] if (ReactHostOperationHistoryHook._preventClearing) {
// Should only be used for tests.
// [ts] return;
// [ts] }
// [ts] history = [];
// [ts] },
// [ts] getHistory: () => {
// [ts] return history;
// [ts] }
// [ts] };
// [ts] module.exports = ReactHostOperationHistoryHook;
// [ts] },{}],61:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let DOMProperty = _dereq_(11);
// [ts] let EventPluginHub = _dereq_(17);
// [ts] let EventPluginUtils = _dereq_(19);
// [ts] let ReactComponentEnvironment = _dereq_(29);
// [ts] let ReactEmptyComponent = _dereq_(54);
// [ts] let ReactBrowserEventEmitter = _dereq_(26);
// [ts] let ReactHostComponent = _dereq_(59);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let ReactInjection = {
// [ts] Component: ReactComponentEnvironment.injection,
// [ts] DOMProperty: DOMProperty.injection,
// [ts] EmptyComponent: ReactEmptyComponent.injection,
// [ts] EventPluginHub: EventPluginHub.injection,
// [ts] EventPluginUtils: EventPluginUtils.injection,
// [ts] EventEmitter: ReactBrowserEventEmitter.injection,
// [ts] HostComponent: ReactHostComponent.injection,
// [ts] Updates: ReactUpdates.injection
// [ts] };
// [ts] module.exports = ReactInjection;
// [ts] },{"11":11,"17":17,"19":19,"26":26,"29":29,"54":54,"59":59,"82":82}],62:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactDOMSelection = _dereq_(44);
// [ts] let containsNode = _dereq_(141);
// [ts] let focusNode = _dereq_(146);
// [ts] let getActiveElement = _dereq_(147);
// [ts] fn isInDocument(node: auto) {
// [ts] return containsNode(document.documentElement, node);
// [ts] }
// [ts] /**
// [ts] * @ReactInputSelection: React input selection module. Based on Selection.js,
// [ts] * but modified to be suitable for react and has a couple of bug fixes (doesn't
// [ts] * assume buttons have range selections allowed).
// [ts] * Input selection module for React.
// [ts] */
// [ts] let ReactInputSelection = {
// [ts] hasSelectionCapabilities: (elem) => {
// [ts] let nodeName = elem && elem.nodeName && elem.nodeName.toLowerCase();
// [ts] return nodeName && (nodeName == "input" && elem.type == "text" || nodeName == "textarea" || elem.contentEditable == "true");
// [ts] },
// [ts] getSelectionInformation: () => {
// [ts] let focusedElem = getActiveElement();
// [ts] return {
// [ts] focusedElem: focusedElem,
// [ts] selectionRange: if (ReactInputSelection.hasSelectionCapabilities(focusedElem)) ReactInputSelection.getSelection(focusedElem) else null
// [ts] };
// [ts] },
// [ts] /**
// [ts] * @restoreSelection: If any selection information was potentially lost,
// [ts] * restore it. This is useful when performing operations that could remove dom
// [ts] * nodes and place them back in, resulting in focus being lost.
// [ts] */
// [ts] restoreSelection: (priorSelectionInformation) => {
// [ts] let curFocusedElem = getActiveElement();
// [ts] let priorFocusedElem = priorSelectionInformation.focusedElem;
// [ts] let priorSelectionRange = priorSelectionInformation.selectionRange;
// [ts] if (curFocusedElem != priorFocusedElem && isInDocument(priorFocusedElem)) {
// [ts] if (ReactInputSelection.hasSelectionCapabilities(priorFocusedElem)) {
// [ts] ReactInputSelection.setSelection(priorFocusedElem, priorSelectionRange);
// [ts] }
// [ts] focusNode(priorFocusedElem);
// [ts] }
// [ts] },
// [ts] /**
// [ts] * @getSelection: Gets the selection bounds of a focused textarea, input or
// [ts] * contentEditable node.
// [ts] * -@input: Look up selection bounds of this input
// [ts] * -@return {start: selectionStart, end: selectionEnd}
// [ts] */
// [ts] getSelection: (input) => {
// [ts] let selection;
// [ts] if ("selectionStart" == input) {
// Modern browser with input or textarea.
// [ts] selection = {
// [ts] start: input.selectionStart,
// [ts] end: input.selectionEnd
// [ts] };
// [ts] } else if (document.selection && input.nodeName && input.nodeName.toLowerCase() == "input") {
// IE8 input.
// [ts] let range = document.selection.createRange();
// There can only be one selection per document in IE, so it must
// be in our element.
// [ts] if (range.parentElement() == input) {
// [ts] selection = {
// [ts] start: -range.moveStart("character", -input.value.length),
// [ts] end: -range.moveEnd("character", -input.value.length)
// [ts] };
// [ts] }
// [ts] } else {
// Content editable or old IE textarea.
// [ts] selection = ReactDOMSelection.getOffsets(input);
// [ts] }
// [ts] return selection || { start: 0, end: 0 };
// [ts] },
// [ts] /**
// [ts] * @setSelection: Sets the selection bounds of a textarea or input and focuses
// [ts] * the input.
// [ts] * -@input     Set selection bounds of this input or textarea
// [ts] * -@offsets   Object of same form that is returned from get*
// [ts] */
// [ts] setSelection: (input, offsets) => {
// [ts] let start = offsets.start;
// [ts] let end = offsets.end;
// [ts] if (end == null) {
// [ts] end = start;
// [ts] }
// [ts] if ("selectionStart" == input) {
// [ts] input.selectionStart = start;
// [ts] input.selectionEnd = Math.min(end, input.value.length);
// [ts] } else if (document.selection && input.nodeName && input.nodeName.toLowerCase() == "input") {
// [ts] let range = input.createTextRange();
// [ts] range.collapse(true);
// [ts] range.moveStart("character", start);
// [ts] range.moveEnd("character", end - start);
// [ts] range.select();
// [ts] } else {
// [ts] ReactDOMSelection.setOffsets(input, offsets);
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactInputSelection;
// [ts] },{"141":141,"146":146,"147":147,"44":44}],63:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * `ReactInstanceMap` maintains a mapping from a public facing stateful
// [ts] * instance (key) and the internal representation (value). This allows public
// [ts] * methods to accept the user facing instance as an argument and map them back
// [ts] * to internal methods.
// [ts] */
// TODO: Replace this with ES6: var ReactInstanceMap = new Map();
// [ts] let ReactInstanceMap = {
// [ts] /**
// [ts] * This API should be called `delete` but we'd have to make sure to always
// [ts] * transform these to strings for IE support. When this transform is fully
// [ts] * supported we can rename it.
// [ts] */
// [ts] remove: (key) => {
// [ts] key._reactInternalInstance = null;
// [ts] },
// [ts] get: (key) => {
// [ts] return key._reactInternalInstance;
// [ts] },
// [ts] has: (key) => {
// [ts] return key._reactInternalInstance != null;
// [ts] },
// [ts] set: (key, value) => {
// [ts] key._reactInternalInstance = value;
// [ts] }
// [ts] };
// [ts] module.exports = ReactInstanceMap;
// [ts] },{}],64:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2016-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// Trust the developer to only use ReactInstrumentation with a __DEV__ check
// [ts] let debugTool = null;
// [ts] if ("development" != "production") {
// [ts] let ReactDebugTool = _dereq_(50);
// [ts] debugTool = ReactDebugTool;
// [ts] }
// [ts] module.exports = { debugTool: debugTool };
// [ts] },{"50":50}],65:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2016-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let warning = _dereq_(159);
// [ts] if ("development" != "production") {
// [ts] let processingChildContext = false;
// [ts] let warnInvalidSetState = () => {
// [ts] if ("development" != "production") warning(!processingChildContext, "setState(...): Cannot call setState() inside getChildContext()") else void 0;
// [ts] };
// [ts] }
// [ts] let ReactInvalidSetStateWarningHook = {
// [ts] onBeginProcessingChildContext: () => {
// [ts] processingChildContext = true;
// [ts] },
// [ts] onEndProcessingChildContext: () => {
// [ts] processingChildContext = false;
// [ts] },
// [ts] onSetState: () => {
// [ts] warnInvalidSetState();
// [ts] }
// [ts] };
// [ts] module.exports = ReactInvalidSetStateWarningHook;
// [ts] },{"159":159}],66:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let adler32 = _dereq_(103);
// [ts] let TAG_END = "\\/?>";
// [ts] let COMMENT_START = "^<\\!\\-\\-";
// [ts] let ReactMarkupChecksum = {
// [ts] CHECKSUM_ATTR_NAME: "data-react-checksum",
// [ts] /**
// [ts] * @param {string} markup Markup string
// [ts] * @return {string} Markup string with checksum attribute attached
// [ts] */
// [ts] addChecksumToMarkup: (markup) => {
// [ts] let checksum = adler32(markup);
// Add checksum (handle both parent tags, comments and self-closing tags)
// [ts] if (COMMENT_START.test(markup)) {
// [ts] return markup;
// [ts] } else {
// [ts] return markup.replace(TAG_END, " " + ReactMarkupChecksum.CHECKSUM_ATTR_NAME + "=\"" + checksum + "\"$&");
// [ts] }
// [ts] },
// [ts] /**
// [ts] * @param {string} markup to use
// [ts] * @param {DOMElement} element root React element
// [ts] * @returns {boolean} whether or not the markup is the same
// [ts] */
// [ts] canReuseMarkup: (markup, element) => {
// [ts] let existingChecksum = element.getAttribute(ReactMarkupChecksum.CHECKSUM_ATTR_NAME);
// [ts] existingChecksum = existingChecksum && parseInt(existingChecksum, 10);
// [ts] let markupChecksum = adler32(markup);
// [ts] return markupChecksum == existingChecksum;
// [ts] }
// [ts] };
// [ts] module.exports = ReactMarkupChecksum;
// [ts] },{"103":103}],67:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let DOMLazyTree = _dereq_(9);
// [ts] let DOMProperty = _dereq_(11);
// [ts] let React = _dereq_(135);
// [ts] let ReactBrowserEventEmitter = _dereq_(26);
// [ts] let ReactCurrentOwner = _dereq_(134);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactDOMContainerInfo = _dereq_(35);
// [ts] let ReactDOMFeatureFlags = _dereq_(37);
// [ts] let ReactFeatureFlags = _dereq_(58);
// [ts] let ReactInstanceMap = _dereq_(63);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let ReactMarkupChecksum = _dereq_(66);
// [ts] let ReactReconciler = _dereq_(75);
// [ts] let ReactUpdateQueue = _dereq_(81);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let emptyObject = _dereq_(145);
// [ts] let instantiateReactComponent = _dereq_(121);
// [ts] let invariant = _dereq_(152);
// [ts] let setInnerHTML = _dereq_(128);
// [ts] let shouldUpdateReactComponent = _dereq_(130);
// [ts] let warning = _dereq_(159);
// [ts] let ATTR_NAME = DOMProperty.ID_ATTRIBUTE_NAME;
// [ts] let ROOT_ATTR_NAME = DOMProperty.ROOT_ATTRIBUTE_NAME;
// [ts] let ELEMENT_NODE_TYPE = 1;
// [ts] let DOC_NODE_TYPE = 9;
// [ts] let DOCUMENT_FRAGMENT_NODE_TYPE = 11;
// [ts] let instancesByReactRootID = {};
// [ts] /**
// [ts] * Finds the index of the first character
// [ts] * that's not common between the two given strings.
// [ts] *
// [ts] * @return {number} the index of the character where the strings diverge
// [ts] */
// [ts] fn firstDifferenceIndex(string1: auto, string2: auto) {
// [ts] let minLen = Math.min(string1.length, string2.length);
// [ts] let i = 0;
// [ts] while (i < minLen) { // [cap-migrate] for-step: i++
// [ts] if (string1.charAt(i) != string2.charAt(i)) {
// [ts] return i;
// [ts] }
// [ts] }
// [ts] return if (string1.length == string2.length) -1 else minLen;
// [ts] }
// [ts] /**
// [ts] * @param {DOMElement|DOMDocument} container DOM element that may contain
// [ts] * a React component
// [ts] * @return {?*} DOM element that may have the reactRoot ID, or null.
// [ts] */
// [ts] fn getReactRootElementInContainer(container: auto) {
// [ts] if (!container) {
// [ts] return null;
// [ts] }
// [ts] if (container.nodeType == DOC_NODE_TYPE) {
// [ts] return container.documentElement;
// [ts] } else {
// [ts] return container.firstChild;
// [ts] }
// [ts] }
// [ts] fn internalGetID(node: auto) {
// If node is something like a window, document, or text node, none of
// which support attributes or a .getAttribute method, gracefully return
// the empty string, as if the attribute were missing.
// [ts] return node.getAttribute && node.getAttribute(ATTR_NAME) || "";
// [ts] }
// [ts] /**
// [ts] * Mounts this component and inserts it into the DOM.
// [ts] *
// [ts] * @param {ReactComponent} componentInstance The instance to mount.
// [ts] * @param {DOMElement} container DOM element to mount into.
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @param {boolean} shouldReuseMarkup If true, do not insert markup
// [ts] */
// [ts] fn mountComponentIntoNode(wrapperInstance: auto, container: auto, transaction: auto, shouldReuseMarkup: auto, context: auto) {
// [ts] let markerName;
// [ts] if (ReactFeatureFlags.logTopLevelRenders) {
// [ts] let wrappedElement = wrapperInstance._currentElement.props.child;
// [ts] let type = wrappedElement.type;
// [ts] markerName = "React mount: " + (if (@typeName(type) == "string") type else type.displayName || type.name);
// [ts] console.time(markerName);
// [ts] }
// [ts] let markup = ReactReconciler.mountComponent(wrapperInstance, transaction, null, ReactDOMContainerInfo(wrapperInstance, container), context, 0 /* parentDebugID */
// [ts] );
// [ts] if (markerName) {
// [ts] console.timeEnd(markerName);
// [ts] }
// [ts] wrapperInstance._renderedComponent._topLevelWrapper = wrapperInstance;
// [ts] ReactMount._mountImageIntoNode(markup, container, wrapperInstance, shouldReuseMarkup, transaction);
// [ts] }
// [ts] /**
// [ts] * Batched mount.
// [ts] *
// [ts] * @param {ReactComponent} componentInstance The instance to mount.
// [ts] * @param {DOMElement} container DOM element to mount into.
// [ts] * @param {boolean} shouldReuseMarkup If true, do not insert markup
// [ts] */
// [ts] fn batchedMountComponentIntoNode(componentInstance: auto, container: auto, shouldReuseMarkup: auto, context: auto) {
// [ts] let transaction = ReactUpdates.ReactReconcileTransaction.getPooled(
// [ts] /* useCreateElement */
// [ts] !shouldReuseMarkup && ReactDOMFeatureFlags.useCreateElement);
// [ts] transaction.perform(mountComponentIntoNode, null, componentInstance, container, transaction, shouldReuseMarkup, context);
// [ts] ReactUpdates.ReactReconcileTransaction.release(transaction);
// [ts] }
// [ts] /**
// [ts] * Unmounts a component and removes it from the DOM.
// [ts] *
// [ts] * @param {ReactComponent} instance React component instance.
// [ts] * @param {DOMElement} container DOM element to unmount from.
// [ts] * @final
// [ts] * @internal
// [ts] * @see {ReactMount.unmountComponentAtNode}
// [ts] */
// [ts] fn unmountComponentFromNode(instance: auto, container: auto, safely: auto) {
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onBeginFlush();
// [ts] }
// [ts] ReactReconciler.unmountComponent(instance, safely);
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onEndFlush();
// [ts] }
// [ts] if (container.nodeType == DOC_NODE_TYPE) {
// [ts] container = container.documentElement;
// [ts] }
// http://jsperf.com/emptying-a-node
// [ts] while (container.lastChild) {
// [ts] container.removeChild(container.lastChild);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * True if the supplied DOM node has a direct React-rendered child that is
// [ts] * not a React root element. Useful for warning in `render`,
// [ts] * `unmountComponentAtNode`, etc.
// [ts] *
// [ts] * @param {?DOMElement} node The candidate DOM node.
// [ts] * @return {boolean} True if the DOM element contains a direct child that was
// [ts] * rendered by React but is not a root element.
// [ts] * @internal
// [ts] */
// [ts] fn hasNonRootReactChild(container: auto) {
// [ts] let rootEl = getReactRootElementInContainer(container);
// [ts] if (rootEl) {
// [ts] let inst = ReactDOMComponentTree.getInstanceFromNode(rootEl);
// [ts] return !!(inst && inst._hostParent);
// [ts] }
// [ts] }
// [ts] /**
// [ts] * True if the supplied DOM node is a React DOM element and
// [ts] * it has been rendered by another copy of React.
// [ts] *
// [ts] * @param {?DOMElement} node The candidate DOM node.
// [ts] * @return {boolean} True if the DOM has been rendered by another copy of React
// [ts] * @internal
// [ts] */
// [ts] fn nodeIsRenderedByOtherInstance(container: auto) {
// [ts] let rootEl = getReactRootElementInContainer(container);
// [ts] return !!(rootEl && isReactNode(rootEl) && !ReactDOMComponentTree.getInstanceFromNode(rootEl));
// [ts] }
// [ts] /**
// [ts] * True if the supplied DOM node is a valid node element.
// [ts] *
// [ts] * @param {?DOMElement} node The candidate DOM node.
// [ts] * @return {boolean} True if the DOM is a valid DOM node.
// [ts] * @internal
// [ts] */
// [ts] fn isValidContainer(node: auto) {
// [ts] return !!(node && (node.nodeType == ELEMENT_NODE_TYPE || node.nodeType == DOC_NODE_TYPE || node.nodeType == DOCUMENT_FRAGMENT_NODE_TYPE));
// [ts] }
// [ts] /**
// [ts] * True if the supplied DOM node is a valid React node element.
// [ts] *
// [ts] * @param {?DOMElement} node The candidate DOM node.
// [ts] * @return {boolean} True if the DOM is a valid React DOM node.
// [ts] * @internal
// [ts] */
// [ts] fn isReactNode(node: auto) {
// [ts] return isValidContainer(node) && (node.hasAttribute(ROOT_ATTR_NAME) || node.hasAttribute(ATTR_NAME));
// [ts] }
// [ts] fn getHostRootInstanceInContainer(container: auto) {
// [ts] let rootEl = getReactRootElementInContainer(container);
// [ts] let prevHostInstance = rootEl && ReactDOMComponentTree.getInstanceFromNode(rootEl);
// [ts] return if (prevHostInstance && !prevHostInstance._hostParent) prevHostInstance else null;
// [ts] }
// [ts] fn getTopLevelWrapperInContainer(container: auto) {
// [ts] let root = getHostRootInstanceInContainer(container);
// [ts] return if (root) root._hostContainerInfo._topLevelWrapper else null;
// [ts] }
// [ts] /**
// [ts] * Temporary (?) hack so that we can store all top-level pending updates on
// [ts] * composites instead of having to worry about different types of components
// [ts] * here.
// [ts] */
// [ts] let topLevelRootCounter = 1;
// [ts] let TopLevelWrapper = () => {
// [ts] _.rootID = topLevelRootCounter++;
// [ts] };
// [ts] TopLevelWrapper.prototype.isReactComponent = {};
// [ts] if ("development" != "production") {
// [ts] TopLevelWrapper.displayName = "TopLevelWrapper";
// [ts] }
// [ts] TopLevelWrapper.prototype.render = () => {
// [ts] return _.props.child;
// [ts] };
// [ts] TopLevelWrapper.isReactTopLevelWrapper = true;
// [ts] /**
// [ts] * Mounting is the process of initializing a React component by creating its
// [ts] * representative DOM elements and inserting them into a supplied `container`.
// [ts] * Any prior content inside `container` is destroyed in the process.
// [ts] *
// [ts] *   ReactMount.render(
// [ts] *     component,
// [ts] *     document.getElementById('container')
// [ts] *   );
// [ts] *
// [ts] *   <div id="container">                   <-- Supplied `container`.
// [ts] *     <div data-reactid=".3">              <-- Rendered reactRoot of React
// [ts] *       // ...                                 component.
// [ts] *     </div>
// [ts] *   </div>
// [ts] *
// [ts] * Inside of `container`, the first element rendered is the "reactRoot".
// [ts] */
// [ts] let ReactMount = {
// [ts] TopLevelWrapper: TopLevelWrapper,
// [ts] /**
// [ts] * Used by devtools. The keys are not important.
// [ts] */
// [ts] _instancesByReactRootID: instancesByReactRootID,
// [ts] /**
// [ts] * This is a hook provided to support rendering React components while
// [ts] * ensuring that the apparent scroll position of its `container` does not
// [ts] * change.
// [ts] *
// [ts] * @param {DOMElement} container The `container` being rendered into.
// [ts] * @param {function} renderCallback This must be called once to do the render.
// [ts] */
// [ts] scrollMonitor: (container, renderCallback) => {
// [ts] renderCallback();
// [ts] },
// [ts] /**
// [ts] * Take a component that's already mounted into the DOM and replace its props
// [ts] * @param {ReactComponent} prevComponent component instance already in the DOM
// [ts] * @param {ReactElement} nextElement component instance to render
// [ts] * @param {DOMElement} container container to render into
// [ts] * @param {?function} callback function triggered on completion
// [ts] */
// [ts] _updateRootComponent: (prevComponent, nextElement, nextContext, container, callback) => {
// [ts] ReactMount.scrollMonitor(container, () => {
// [ts] ReactUpdateQueue.enqueueElementInternal(prevComponent, nextElement, nextContext);
// [ts] if (callback) {
// [ts] ReactUpdateQueue.enqueueCallbackInternal(prevComponent, callback);
// [ts] }
// [ts] });
// [ts] return prevComponent;
// [ts] },
// [ts] /**
// [ts] * Render a new component into the DOM. Hooked by hooks!
// [ts] *
// [ts] * @param {ReactElement} nextElement element to render
// [ts] * @param {DOMElement} container container to render into
// [ts] * @param {boolean} shouldReuseMarkup if we should skip the markup insertion
// [ts] * @return {ReactComponent} nextComponent
// [ts] */
// [ts] _renderNewRootComponent: (nextElement, container, shouldReuseMarkup, context) => {
// Various parts of our code (such as ReactCompositeComponent's
// _renderValidatedComponent) assume that calls to render aren't nested;
// verify that that's the case.
// [ts] if ("development" != "production") warning(ReactCurrentOwner.current == null, "_renderNewRootComponent(): Render methods should be a pure function " + "of props and state; triggering nested component updates from " + "render is not allowed. If necessary, trigger nested updates in " + "componentDidUpdate. Check the render method of %s.", ReactCurrentOwner.current && ReactCurrentOwner.current.getName() || "ReactCompositeComponent") else void 0;
// [ts] if (!isValidContainer(container)) if ("development" != "production") invariant(false, "_registerComponent(...): Target container is not a DOM element.") else _prodInvariant("37") else void 0;
// [ts] ReactBrowserEventEmitter.ensureScrollValueMonitoring();
// [ts] let componentInstance = instantiateReactComponent(nextElement, false);
// The initial render is synchronous but any updates that happen during
// rendering, in componentWillMount or componentDidMount, will be batched
// according to the current batching strategy.
// [ts] ReactUpdates.batchedUpdates(batchedMountComponentIntoNode, componentInstance, container, shouldReuseMarkup, context);
// [ts] let wrapperID = componentInstance._instance.rootID;
// [ts] instancesByReactRootID[wrapperID] = componentInstance;
// [ts] return componentInstance;
// [ts] },
// [ts] /**
// [ts] * Renders a React component into the DOM in the supplied `container`.
// [ts] *
// [ts] * If the React component was previously rendered into `container`, this will
// [ts] * perform an update on it and only mutate the DOM as necessary to reflect the
// [ts] * latest React component.
// [ts] *
// [ts] * @param {ReactComponent} parentComponent The conceptual parent of this render tree.
// [ts] * @param {ReactElement} nextElement Component element to render.
// [ts] * @param {DOMElement} container DOM element to render into.
// [ts] * @param {?function} callback function triggered on completion
// [ts] * @return {ReactComponent} Component instance rendered in `container`.
// [ts] */
// [ts] renderSubtreeIntoContainer: (parentComponent, nextElement, container, callback) => {
// [ts] if (!(parentComponent != null && ReactInstanceMap.has(parentComponent))) if ("development" != "production") invariant(false, "parentComponent must be a valid React Component") else _prodInvariant("38") else void 0;
// [ts] return ReactMount._renderSubtreeIntoContainer(parentComponent, nextElement, container, callback);
// [ts] },
// [ts] _renderSubtreeIntoContainer: (parentComponent, nextElement, container, callback) => {
// [ts] ReactUpdateQueue.validateCallback(callback, "ReactDOM.render");
// [ts] !React.isValidElement(nextElement) ? "development" != "production" ? invariant(false, "ReactDOM.render(): Invalid component element.%s", @typeName(nextElement) == "string" ? " Instead of passing a string like 'div', pass " + "React.createElement('div') or <div />." : @typeName(nextElement) == "function" ? " Instead of passing a class like Foo, pass " + "React.createElement(Foo) or <Foo />." : // Check if it quacks like an element
// [ts] if (nextElement != null && nextElement.props != null) " This may be caused by unintentionally loading two independent " + "copies of React." else "") : _prodInvariant(if ("39", @typeName(nextElement) == "string") " Instead of passing a string like 'div', pass " + "React.createElement('div') or <div />." else if (@typeName(nextElement) == "function") " Instead of passing a class like Foo, pass " + "React.createElement(Foo) or <Foo />." else if (nextElement != null && nextElement.props != null) " This may be caused by unintentionally loading two independent " + "copies of React." else "") : void 0;
// [ts] if ("development" != "production") warning(!container || !container.tagName || container.tagName.toUpperCase() != "BODY", "render(): Rendering components directly into document.body is " + "discouraged, since its children are often manipulated by third-party " + "scripts and browser extensions. This may lead to subtle " + "reconciliation issues. Try rendering into a container element created " + "for your app.") else void 0;
// [ts] let nextWrappedElement = React.createElement(TopLevelWrapper, {
// [ts] child: nextElement
// [ts] });
// [ts] let nextContext;
// [ts] if (parentComponent) {
// [ts] let parentInst = ReactInstanceMap.get(parentComponent);
// [ts] nextContext = parentInst._processChildContext(parentInst._context);
// [ts] } else {
// [ts] nextContext = emptyObject;
// [ts] }
// [ts] let prevComponent = getTopLevelWrapperInContainer(container);
// [ts] if (prevComponent) {
// [ts] let prevWrappedElement = prevComponent._currentElement;
// [ts] let prevElement = prevWrappedElement.props.child;
// [ts] if (shouldUpdateReactComponent(prevElement, nextElement)) {
// [ts] let publicInst = prevComponent._renderedComponent.getPublicInstance();
// [ts] let updatedCallback = callback && () => {
// [ts] callback.call(publicInst);
// [ts] };
// [ts] ReactMount._updateRootComponent(prevComponent, nextWrappedElement, nextContext, container, updatedCallback);
// [ts] return publicInst;
// [ts] } else {
// [ts] ReactMount.unmountComponentAtNode(container);
// [ts] }
// [ts] }
// [ts] let reactRootElement = getReactRootElementInContainer(container);
// [ts] let containerHasReactMarkup = reactRootElement && !!internalGetID(reactRootElement);
// [ts] let containerHasNonRootReactChild = hasNonRootReactChild(container);
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(!containerHasNonRootReactChild, "render(...): Replacing React-rendered children with a new root " + "component. If you intended to update the children of this node, " + "you should instead have the existing children update their state " + "and render the new components instead of calling ReactDOM.render.") else void 0;
// [ts] if (!containerHasReactMarkup || reactRootElement.nextSibling) {
// [ts] let rootElementSibling = reactRootElement;
// [ts] while (rootElementSibling) {
// [ts] if (internalGetID(rootElementSibling)) {
// [ts] if ("development" != "production") warning(false, "render(): Target node has markup rendered by React, but there " + "are unrelated nodes as well. This is most commonly caused by " + "white-space inserted around server-rendered markup.") else void 0;
// [ts] break;
// [ts] }
// [ts] rootElementSibling = rootElementSibling.nextSibling;
// [ts] }
// [ts] }
// [ts] }
// [ts] let shouldReuseMarkup = containerHasReactMarkup && !prevComponent && !containerHasNonRootReactChild;
// [ts] let component = ReactMount._renderNewRootComponent(nextWrappedElement, container, shouldReuseMarkup, nextContext)._renderedComponent.getPublicInstance();
// [ts] if (callback) {
// [ts] callback.call(component);
// [ts] }
// [ts] return component;
// [ts] },
// [ts] /**
// [ts] * Renders a React component into the DOM in the supplied `container`.
// [ts] * See https://facebook.github.io/react/docs/top-level-api.html#reactdom.render
// [ts] *
// [ts] * If the React component was previously rendered into `container`, this will
// [ts] * perform an update on it and only mutate the DOM as necessary to reflect the
// [ts] * latest React component.
// [ts] *
// [ts] * @param {ReactElement} nextElement Component element to render.
// [ts] * @param {DOMElement} container DOM element to render into.
// [ts] * @param {?function} callback function triggered on completion
// [ts] * @return {ReactComponent} Component instance rendered in `container`.
// [ts] */
// [ts] render: (nextElement, container, callback) => {
// [ts] return ReactMount._renderSubtreeIntoContainer(null, nextElement, container, callback);
// [ts] },
// [ts] /**
// [ts] * Unmounts and destroys the React component rendered in the `container`.
// [ts] * See https://facebook.github.io/react/docs/top-level-api.html#reactdom.unmountcomponentatnode
// [ts] *
// [ts] * @param {DOMElement} container DOM element containing a React component.
// [ts] * @return {boolean} True if a component was found in and unmounted from
// [ts] *                   `container`
// [ts] */
// [ts] unmountComponentAtNode: (container) => {
// Various parts of our code (such as ReactCompositeComponent's
// _renderValidatedComponent) assume that calls to render aren't nested;
// verify that that's the case. (Strictly speaking, unmounting won't cause a
// render but we still don't expect to be in a render call here.)
// [ts] if ("development" != "production") warning(ReactCurrentOwner.current == null, "unmountComponentAtNode(): Render methods should be a pure function " + "of props and state; triggering nested component updates from render " + "is not allowed. If necessary, trigger nested updates in " + "componentDidUpdate. Check the render method of %s.", ReactCurrentOwner.current && ReactCurrentOwner.current.getName() || "ReactCompositeComponent") else void 0;
// [ts] if (!isValidContainer(container)) if ("development" != "production") invariant(false, "unmountComponentAtNode(...): Target container is not a DOM element.") else _prodInvariant("40") else void 0;
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(!nodeIsRenderedByOtherInstance(container), "unmountComponentAtNode(): The node you're attempting to unmount " + "was rendered by another copy of React.") else void 0;
// [ts] }
// [ts] let prevComponent = getTopLevelWrapperInContainer(container);
// [ts] if (!prevComponent) {
// Check if the node being unmounted was rendered by React, but isn't a
// root node.
// [ts] let containerHasNonRootReactChild = hasNonRootReactChild(container);
// Check if the container itself is a React root node.
// [ts] let isContainerReactRoot = container.nodeType == 1 && container.hasAttribute(ROOT_ATTR_NAME);
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(if (!containerHasNonRootReactChild, "unmountComponentAtNode(): The node you're attempting to unmount " + "was rendered by React and is not a top-level container. %s", isContainerReactRoot) "You may have accidentally passed in a React root node instead " + "of its container." else "Instead, have the parent component update its state and " + "rerender in order to remove this component.") else void 0;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] delete instancesByReactRootID[prevComponent._instance.rootID];
// [ts] ReactUpdates.batchedUpdates(unmountComponentFromNode, prevComponent, container, false);
// [ts] return true;
// [ts] },
// [ts] _mountImageIntoNode: (markup, container, instance, shouldReuseMarkup, transaction) => {
// [ts] if (!isValidContainer(container)) if ("development" != "production") invariant(false, "mountComponentIntoNode(...): Target container is not valid.") else _prodInvariant("41") else void 0;
// [ts] if (shouldReuseMarkup) {
// [ts] let rootElement = getReactRootElementInContainer(container);
// [ts] if (ReactMarkupChecksum.canReuseMarkup(markup, rootElement)) {
// [ts] ReactDOMComponentTree.precacheNode(instance, rootElement);
// [ts] return;
// [ts] } else {
// [ts] let checksum = rootElement.getAttribute(ReactMarkupChecksum.CHECKSUM_ATTR_NAME);
// [ts] rootElement.removeAttribute(ReactMarkupChecksum.CHECKSUM_ATTR_NAME);
// [ts] let rootMarkup = rootElement.outerHTML;
// [ts] rootElement.setAttribute(ReactMarkupChecksum.CHECKSUM_ATTR_NAME, checksum);
// [ts] let normalizedMarkup = markup;
// [ts] if ("development" != "production") {
// because rootMarkup is retrieved from the DOM, various normalizations
// will have occurred which will not be present in `markup`. Here,
// insert markup into a <div> or <iframe> depending on the container
// type to perform the same normalizations before comparing.
// [ts] let normalizer;
// [ts] if (container.nodeType == ELEMENT_NODE_TYPE) {
// [ts] normalizer = document.createElement("div");
// [ts] normalizer.innerHTML = markup;
// [ts] normalizedMarkup = normalizer.innerHTML;
// [ts] } else {
// [ts] normalizer = document.createElement("iframe");
// [ts] document.body.appendChild(normalizer);
// [ts] normalizer.contentDocument.write(markup);
// [ts] normalizedMarkup = normalizer.contentDocument.documentElement.outerHTML;
// [ts] document.body.removeChild(normalizer);
// [ts] }
// [ts] }
// [ts] let diffIndex = firstDifferenceIndex(normalizedMarkup, rootMarkup);
// [ts] let difference = " (client) " + normalizedMarkup.substring(diffIndex - 20, diffIndex + 20) + "\n (server) " + rootMarkup.substring(diffIndex - 20, diffIndex + 20);
// [ts] if (!(container.nodeType != DOC_NODE_TYPE)) if ("development" != "production") invariant(false, "You're trying to render a component to the document using server rendering but the checksum was invalid. This usually means you rendered a different component type or props on the client from the one on the server, or your render() methods are impure. React cannot handle this case due to cross-browser quirks by rendering at the document root. You should look for environment dependent code in your components and ensure the props are the same client and server side:\n%s", difference) else _prodInvariant("42", difference) else void 0;
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(false, "React attempted to reuse markup in a container but the " + "checksum was invalid. This generally means that you are " + "using server rendering and the markup generated on the " + "server was not what the client was expecting. React injected " + "new markup to compensate which works but you have lost many " + "of the benefits of server rendering. Instead, figure out " + "why the markup being generated is different on the client " + "or server:\n%s", difference) else void 0;
// [ts] }
// [ts] }
// [ts] }
// [ts] if (!(container.nodeType != DOC_NODE_TYPE)) if ("development" != "production") invariant(false, "You're trying to render a component to the document but you didn't use server rendering. We can't do this without using server rendering due to cross-browser quirks. See ReactDOMServer.renderToString() for server rendering.") else _prodInvariant("43") else void 0;
// [ts] if (transaction.useCreateElement) {
// [ts] while (container.lastChild) {
// [ts] container.removeChild(container.lastChild);
// [ts] }
// [ts] DOMLazyTree.insertTreeBefore(container, markup, null);
// [ts] } else {
// [ts] setInnerHTML(container, markup);
// [ts] ReactDOMComponentTree.precacheNode(instance, container.firstChild);
// [ts] }
// [ts] if ("development" != "production") {
// [ts] let hostNode = ReactDOMComponentTree.getInstanceFromNode(container.firstChild);
// [ts] if (hostNode._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onHostOperation({
// [ts] instanceID: hostNode._debugID,
// [ts] type: "mount",
// [ts] payload: markup.toString()
// [ts] });
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactMount;
// [ts] },{"11":11,"121":121,"126":126,"128":128,"130":130,"134":134,"135":135,"145":145,"152":152,"159":159,"26":26,"34":34,"35":35,"37":37,"58":58,"63":63,"64":64,"66":66,"75":75,"81":81,"82":82,"9":9}],68:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let ReactComponentEnvironment = _dereq_(29);
// [ts] let ReactInstanceMap = _dereq_(63);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let ReactCurrentOwner = _dereq_(134);
// [ts] let ReactReconciler = _dereq_(75);
// [ts] let ReactChildReconciler = _dereq_(27);
// [ts] let emptyFunction = _dereq_(144);
// [ts] let flattenChildren = _dereq_(109);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Make an update for markup to be rendered and inserted at a supplied index.
// [ts] *
// [ts] * @param {string} markup Markup that renders into an element.
// [ts] * @param {number} toIndex Destination index.
// [ts] * @private
// [ts] */
// [ts] fn makeInsertMarkup(markup: auto, afterNode: auto, toIndex: auto) {
// NOTE: Null values reduce hidden classes.
// [ts] return {
// [ts] type: "INSERT_MARKUP",
// [ts] content: markup,
// [ts] fromIndex: null,
// [ts] fromNode: null,
// [ts] toIndex: toIndex,
// [ts] afterNode: afterNode
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Make an update for moving an existing element to another index.
// [ts] *
// [ts] * @param {number} fromIndex Source index of the existing element.
// [ts] * @param {number} toIndex Destination index of the element.
// [ts] * @private
// [ts] */
// [ts] fn makeMove(child: auto, afterNode: auto, toIndex: auto) {
// NOTE: Null values reduce hidden classes.
// [ts] return {
// [ts] type: "MOVE_EXISTING",
// [ts] content: null,
// [ts] fromIndex: child._mountIndex,
// [ts] fromNode: ReactReconciler.getHostNode(child),
// [ts] toIndex: toIndex,
// [ts] afterNode: afterNode
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Make an update for removing an element at an index.
// [ts] *
// [ts] * @param {number} fromIndex Index of the element to remove.
// [ts] * @private
// [ts] */
// [ts] fn makeRemove(child: auto, node: auto) {
// NOTE: Null values reduce hidden classes.
// [ts] return {
// [ts] type: "REMOVE_NODE",
// [ts] content: null,
// [ts] fromIndex: child._mountIndex,
// [ts] fromNode: node,
// [ts] toIndex: null,
// [ts] afterNode: null
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Make an update for setting the markup of a node.
// [ts] *
// [ts] * @param {string} markup Markup that renders into an element.
// [ts] * @private
// [ts] */
// [ts] fn makeSetMarkup(markup: auto) {
// NOTE: Null values reduce hidden classes.
// [ts] return {
// [ts] type: "SET_MARKUP",
// [ts] content: markup,
// [ts] fromIndex: null,
// [ts] fromNode: null,
// [ts] toIndex: null,
// [ts] afterNode: null
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Make an update for setting the text content.
// [ts] *
// [ts] * @param {string} textContent Text content to set.
// [ts] * @private
// [ts] */
// [ts] fn makeTextContent(textContent: auto) {
// NOTE: Null values reduce hidden classes.
// [ts] return {
// [ts] type: "TEXT_CONTENT",
// [ts] content: textContent,
// [ts] fromIndex: null,
// [ts] fromNode: null,
// [ts] toIndex: null,
// [ts] afterNode: null
// [ts] };
// [ts] }
// [ts] /**
// [ts] * Push an update, if any, onto the queue. Creates a new queue if none is
// [ts] * passed and always returns the queue. Mutative.
// [ts] */
// [ts] fn enqueue(queue: auto, update: auto) {
// [ts] if (update) {
// [ts] queue = queue || [];
// [ts] queue.push(update);
// [ts] }
// [ts] return queue;
// [ts] }
// [ts] /**
// [ts] * Processes any enqueued updates.
// [ts] *
// [ts] * @private
// [ts] */
// [ts] fn processQueue(inst: auto, updateQueue: auto) {
// [ts] ReactComponentEnvironment.processChildrenUpdates(inst, updateQueue);
// [ts] }
// [ts] let setChildrenForInstrumentation = emptyFunction;
// [ts] if ("development" != "production") {
// [ts] let getDebugID = (inst) => {
// [ts] if (!inst._debugID) {
// Check for ART-like instances. TODO: This is silly/gross.
// [ts] let internal;
// [ts] if (internal = ReactInstanceMap.get(inst)) {
// [ts] inst = internal;
// [ts] }
// [ts] }
// [ts] return inst._debugID;
// [ts] };
// [ts] setChildrenForInstrumentation = (children) => {
// [ts] let debugID = getDebugID(this);
// TODO: React Native empty components are also multichild.
// This means they still get into this method but don't have _debugID.
// [ts] if (debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onSetChildren(debugID, children ? Object.keys(children).map((key) => {
// [ts] return children[key]._debugID;
// [ts] }) : []);
// [ts] }
// [ts] };
// [ts] }
// [ts] /**
// [ts] * ReactMultiChild are capable of reconciling multiple children.
// [ts] *
// [ts] * @class ReactMultiChild
// [ts] * @internal
// [ts] */
// [ts] let ReactMultiChild = {
// [ts] /**
// [ts] * Provides common functionality for components that must reconcile multiple
// [ts] * children. This is used by `ReactDOMComponent` to mount, update, and
// [ts] * unmount child components.
// [ts] *
// [ts] * @lends {ReactMultiChild.prototype}
// [ts] */
// [ts] Mixin: {
// [ts] _reconcilerInstantiateChildren: (nestedChildren, transaction, context) => {
// [ts] if ("development" != "production") {
// [ts] let selfDebugID = getDebugID(this);
// [ts] if (_._currentElement) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] ReactCurrentOwner.current = _._currentElement._owner;
// [ts] return ReactChildReconciler.instantiateChildren(nestedChildren, transaction, context, selfDebugID);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentOwner.current = null;
// [ts] }
// [ts] }
// [ts] }
// [ts] return ReactChildReconciler.instantiateChildren(nestedChildren, transaction, context);
// [ts] },
// [ts] _reconcilerUpdateChildren: (prevChildren, nextNestedChildrenElements, mountImages, removedNodes, transaction, context) => {
// [ts] let nextChildren;
// [ts] let selfDebugID = 0;
// [ts] if ("development" != "production") {
// [ts] selfDebugID = getDebugID(this);
// [ts] if (_._currentElement) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] ReactCurrentOwner.current = _._currentElement._owner;
// [ts] nextChildren = flattenChildren(nextNestedChildrenElements, selfDebugID);
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] ReactCurrentOwner.current = null;
// [ts] }
// [ts] ReactChildReconciler.updateChildren(prevChildren, nextChildren, mountImages, removedNodes, transaction, this, _._hostContainerInfo, context, selfDebugID);
// [ts] return nextChildren;
// [ts] }
// [ts] }
// [ts] nextChildren = flattenChildren(nextNestedChildrenElements, selfDebugID);
// [ts] ReactChildReconciler.updateChildren(prevChildren, nextChildren, mountImages, removedNodes, transaction, this, _._hostContainerInfo, context, selfDebugID);
// [ts] return nextChildren;
// [ts] },
// [ts] /**
// [ts] * Generates a "mount image" for each of the supplied children. In the case
// [ts] * of `ReactDOMComponent`, a mount image is a string of markup.
// [ts] *
// [ts] * @param {?object} nestedChildren Nested child maps.
// [ts] * @return {array} An array of mounted representations.
// [ts] * @internal
// [ts] */
// [ts] mountChildren: (nestedChildren, transaction, context) => {
// [ts] let children = _._reconcilerInstantiateChildren(nestedChildren, transaction, context);
// [ts] _._renderedChildren = children;
// [ts] let mountImages = [];
// [ts] let index = 0;
// [ts] for (let name in children) {
// [ts] if (children.hasOwnProperty(name)) {
// [ts] let child = children[name];
// [ts] let selfDebugID = 0;
// [ts] if ("development" != "production") {
// [ts] selfDebugID = getDebugID(this);
// [ts] }
// [ts] let mountImage = ReactReconciler.mountComponent(child, transaction, this, _._hostContainerInfo, context, selfDebugID);
// [ts] child._mountIndex = index++;
// [ts] mountImages.push(mountImage);
// [ts] }
// [ts] }
// [ts] if ("development" != "production") {
// [ts] setChildrenForInstrumentation.call(this, children);
// [ts] }
// [ts] return mountImages;
// [ts] },
// [ts] /**
// [ts] * Replaces any rendered children with a text content string.
// [ts] *
// [ts] * @param {string} nextContent String of content.
// [ts] * @internal
// [ts] */
// [ts] updateTextContent: (nextContent) => {
// [ts] let prevChildren = _._renderedChildren;
// Remove any rendered children.
// [ts] ReactChildReconciler.unmountChildren(prevChildren, false);
// [ts] for (let name in prevChildren) {
// [ts] if (prevChildren.hasOwnProperty(name)) {
// [ts] if (!false) if ("development" != "production") invariant(false, "updateTextContent called on non-empty component.") else _prodInvariant("118") else void 0;
// [ts] }
// [ts] }
// Set new text content.
// [ts] let updates = [makeTextContent(nextContent)];
// [ts] processQueue(this, updates);
// [ts] },
// [ts] /**
// [ts] * Replaces any rendered children with a markup string.
// [ts] *
// [ts] * @param {string} nextMarkup String of markup.
// [ts] * @internal
// [ts] */
// [ts] updateMarkup: (nextMarkup) => {
// [ts] let prevChildren = _._renderedChildren;
// Remove any rendered children.
// [ts] ReactChildReconciler.unmountChildren(prevChildren, false);
// [ts] for (let name in prevChildren) {
// [ts] if (prevChildren.hasOwnProperty(name)) {
// [ts] if (!false) if ("development" != "production") invariant(false, "updateTextContent called on non-empty component.") else _prodInvariant("118") else void 0;
// [ts] }
// [ts] }
// [ts] let updates = [makeSetMarkup(nextMarkup)];
// [ts] processQueue(this, updates);
// [ts] },
// [ts] /**
// [ts] * Updates the rendered children with new children.
// [ts] *
// [ts] * @param {?object} nextNestedChildrenElements Nested child element maps.
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @internal
// [ts] */
// [ts] updateChildren: (nextNestedChildrenElements, transaction, context) => {
// Hook used by React ART
// [ts] _._updateChildren(nextNestedChildrenElements, transaction, context);
// [ts] },
// [ts] /**
// [ts] * @param {?object} nextNestedChildrenElements Nested child element maps.
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @final
// [ts] * @protected
// [ts] */
// [ts] _updateChildren: (nextNestedChildrenElements, transaction, context) => {
// [ts] let prevChildren = _._renderedChildren;
// [ts] let removedNodes = {};
// [ts] let mountImages = [];
// [ts] let nextChildren = _._reconcilerUpdateChildren(prevChildren, nextNestedChildrenElements, mountImages, removedNodes, transaction, context);
// [ts] if (!nextChildren && !prevChildren) {
// [ts] return;
// [ts] }
// [ts] let updates = null;
// [ts] let name;
// `nextIndex` will increment for each child in `nextChildren`, but
// `lastIndex` will be the last index visited in `prevChildren`.
// [ts] let nextIndex = 0;
// [ts] let lastIndex = 0;
// `nextMountIndex` will increment for each newly mounted child.
// [ts] let nextMountIndex = 0;
// [ts] let lastPlacedNode = null;
// [ts] for (name in nextChildren) {
// [ts] if (!nextChildren.hasOwnProperty(name)) {
// [ts] continue;
// [ts] }
// [ts] let prevChild = prevChildren && prevChildren[name];
// [ts] let nextChild = nextChildren[name];
// [ts] if (prevChild == nextChild) {
// [ts] updates = enqueue(updates, _.moveChild(prevChild, lastPlacedNode, nextIndex, lastIndex));
// [ts] lastIndex = Math.max(prevChild._mountIndex, lastIndex);
// [ts] prevChild._mountIndex = nextIndex;
// [ts] } else {
// [ts] if (prevChild) {
// Update `lastIndex` before `_mountIndex` gets unset by unmounting.
// [ts] lastIndex = Math.max(prevChild._mountIndex, lastIndex);
// The `removedNodes` loop below will actually remove the child.
// [ts] }
// The child must be instantiated before it's mounted.
// [ts] updates = enqueue(updates, _._mountChildAtIndex(nextChild, mountImages[nextMountIndex], lastPlacedNode, nextIndex, transaction, context));
// [ts] nextMountIndex++;
// [ts] }
// [ts] nextIndex++;
// [ts] lastPlacedNode = ReactReconciler.getHostNode(nextChild);
// [ts] }
// Remove children that are no longer present.
// [ts] for (name in removedNodes) {
// [ts] if (removedNodes.hasOwnProperty(name)) {
// [ts] updates = enqueue(updates, _._unmountChild(prevChildren[name], removedNodes[name]));
// [ts] }
// [ts] }
// [ts] if (updates) {
// [ts] processQueue(this, updates);
// [ts] }
// [ts] _._renderedChildren = nextChildren;
// [ts] if ("development" != "production") {
// [ts] setChildrenForInstrumentation.call(this, nextChildren);
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Unmounts all rendered children. This should be used to clean up children
// [ts] * when this component is unmounted. It does not actually perform any
// [ts] * backend operations.
// [ts] *
// [ts] * @internal
// [ts] */
// [ts] unmountChildren: (safely) => {
// [ts] let renderedChildren = _._renderedChildren;
// [ts] ReactChildReconciler.unmountChildren(renderedChildren, safely);
// [ts] _._renderedChildren = null;
// [ts] },
// [ts] /**
// [ts] * Moves a child component to the supplied index.
// [ts] *
// [ts] * @param {ReactComponent} child Component to move.
// [ts] * @param {number} toIndex Destination index of the element.
// [ts] * @param {number} lastIndex Last index visited of the siblings of `child`.
// [ts] * @protected
// [ts] */
// [ts] moveChild: (child, afterNode, toIndex, lastIndex) => {
// If the index of `child` is less than `lastIndex`, then it needs to
// be moved. Otherwise, we do not need to move it because a child will be
// inserted or moved before `child`.
// [ts] if (child._mountIndex < lastIndex) {
// [ts] return makeMove(child, afterNode, toIndex);
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Creates a child component.
// [ts] *
// [ts] * @param {ReactComponent} child Component to create.
// [ts] * @param {string} mountImage Markup to insert.
// [ts] * @protected
// [ts] */
// [ts] createChild: (child, afterNode, mountImage) => {
// [ts] return makeInsertMarkup(mountImage, afterNode, child._mountIndex);
// [ts] },
// [ts] /**
// [ts] * Removes a child component.
// [ts] *
// [ts] * @param {ReactComponent} child Child to remove.
// [ts] * @protected
// [ts] */
// [ts] removeChild: (child, node) => {
// [ts] return makeRemove(child, node);
// [ts] },
// [ts] /**
// [ts] * Mounts a child with the supplied name.
// [ts] *
// [ts] * NOTE: This is part of `updateChildren` and is here for readability.
// [ts] *
// [ts] * @param {ReactComponent} child Component to mount.
// [ts] * @param {string} name Name of the child.
// [ts] * @param {number} index Index at which to insert the child.
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @private
// [ts] */
// [ts] _mountChildAtIndex: (child, mountImage, afterNode, index, transaction, context) => {
// [ts] child._mountIndex = index;
// [ts] return _.createChild(child, afterNode, mountImage);
// [ts] },
// [ts] /**
// [ts] * Unmounts a rendered child.
// [ts] *
// [ts] * NOTE: This is part of `updateChildren` and is here for readability.
// [ts] *
// [ts] * @param {ReactComponent} child Component to unmount.
// [ts] * @private
// [ts] */
// [ts] _unmountChild: (child, node) => {
// [ts] let update = _.removeChild(child, node);
// [ts] child._mountIndex = null;
// [ts] return update;
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactMultiChild;
// [ts] },{"109":109,"126":126,"134":134,"144":144,"152":152,"27":27,"29":29,"63":63,"64":64,"75":75}],69:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let React = _dereq_(135);
// [ts] let invariant = _dereq_(152);
// [ts] let ReactNodeTypes = {
// [ts] HOST: 0,
// [ts] COMPOSITE: 1,
// [ts] EMPTY: 2,
// [ts] getType: (node) => {
// [ts] if (node == null || node == false) {
// [ts] return ReactNodeTypes.EMPTY;
// [ts] } else if (React.isValidElement(node)) {
// [ts] if (@typeName(node.type) == "function") {
// [ts] return ReactNodeTypes.COMPOSITE;
// [ts] } else {
// [ts] return ReactNodeTypes.HOST;
// [ts] }
// [ts] }
// [ts] if (!false) if ("development" != "production") invariant(false, "Unexpected node: %s", node) else _prodInvariant("26", node) else void 0;
// [ts] }
// [ts] };
// [ts] module.exports = ReactNodeTypes;
// [ts] },{"126":126,"135":135,"152":152}],70:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * @param {?object} object
// [ts] * @return {boolean} True if `object` is a valid owner.
// [ts] * @final
// [ts] */
// [ts] fn isValidOwner(object: auto) {
// [ts] return !!(object && @typeName(object.attachRef) == "function" && @typeName(object.detachRef) == "function");
// [ts] }
// [ts] /**
// [ts] * ReactOwners are capable of storing references to owned components.
// [ts] *
// [ts] * All components are capable of //being// referenced by owner components, but
// [ts] * only ReactOwner components are capable of //referencing// owned components.
// [ts] * The named reference is known as a "ref".
// [ts] *
// [ts] * Refs are available when mounted and updated during reconciliation.
// [ts] *
// [ts] *   var MyComponent = React.createClass({
// [ts] *     render: function() {
// [ts] *       return (
// [ts] *         <div onClick={this.handleClick}>
// [ts] *           <CustomComponent ref="custom" />
// [ts] *         </div>
// [ts] *       );
// [ts] *     },
// [ts] *     handleClick: function() {
// [ts] *       this.refs.custom.handleClick();
// [ts] *     },
// [ts] *     componentDidMount: function() {
// [ts] *       this.refs.custom.initialize();
// [ts] *     }
// [ts] *   });
// [ts] *
// [ts] * Refs should rarely be used. When refs are used, they should only be done to
// [ts] * control data that is not handled by React's data flow.
// [ts] *
// [ts] * @class ReactOwner
// [ts] */
// [ts] let ReactOwner = {
// [ts] /**
// [ts] * Adds a component by ref to an owner component.
// [ts] *
// [ts] * @param {ReactComponent} component Component to reference.
// [ts] * @param {string} ref Name by which to refer to the component.
// [ts] * @param {ReactOwner} owner Component on which to record the ref.
// [ts] * @final
// [ts] * @internal
// [ts] */
// [ts] addComponentAsRefTo: (component, ref, owner) => {
// [ts] if (!isValidOwner(owner)) if ("development" != "production") invariant(false, "addComponentAsRefTo(...): Only a ReactOwner can have refs. You might be adding a ref to a component that was not created inside a component's `render` method, or you have multiple copies of React loaded (details: https://fb.me/react-refs-must-have-owner).") else _prodInvariant("119") else void 0;
// [ts] owner.attachRef(ref, component);
// [ts] },
// [ts] /**
// [ts] * Removes a component by ref from an owner component.
// [ts] *
// [ts] * @param {ReactComponent} component Component to dereference.
// [ts] * @param {string} ref Name of the ref to remove.
// [ts] * @param {ReactOwner} owner Component on which the ref is recorded.
// [ts] * @final
// [ts] * @internal
// [ts] */
// [ts] removeComponentAsRefFrom: (component, ref, owner) => {
// [ts] if (!isValidOwner(owner)) if ("development" != "production") invariant(false, "removeComponentAsRefFrom(...): Only a ReactOwner can have refs. You might be removing a ref to a component that was not created inside a component's `render` method, or you have multiple copies of React loaded (details: https://fb.me/react-refs-must-have-owner).") else _prodInvariant("120") else void 0;
// [ts] let ownerPublicInstance = owner.getPublicInstance();
// Check that `component`'s owner is still alive and that `component` is still the current ref
// because we do not want to detach the ref if another component stole it.
// [ts] if (ownerPublicInstance && ownerPublicInstance.refs[ref] == component.getPublicInstance()) {
// [ts] owner.detachRef(ref);
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactOwner;
// [ts] },{"126":126,"152":152}],71:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2016-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let _extends = _assign || (target) => { for (let i = 1; i < arguments.length; i++) { let source = arguments[i]; for (let key == source) { if (Object.prototype.hasOwnProperty.call(source, key)) { target[key] = source[key]; } } } return target; };
// [ts] let ReactDebugTool = _dereq_(50);
// [ts] let lowPriorityWarning = _dereq_(124);
// [ts] let alreadyWarned = false;
// [ts] fn roundFloat(val: auto) {
// [ts] let base = if (arguments.length > 1 && arguments[1] != null) arguments[1] else 2;
// [ts] let n = Math.pow(10, base);
// [ts] return Math.floor(val * n) / n;
// [ts] }
// Flow type definition of console.table is too strict right now, see
// https://github.com/facebook/flow/pull/2353 for updates
// [ts] fn consoleTable(table: auto) {
// [ts] console.table(table);
// [ts] }
// [ts] fn warnInProduction() {
// [ts] if (alreadyWarned) {
// [ts] return;
// [ts] }
// [ts] alreadyWarned = true;
// [ts] if (@typeName(console) != "undefined") {
// [ts] @print("ReactPerf is not supported in the production builds of React. " + "To collect measurements, please use the development build of React instead.");
// [ts] }
// [ts] }
// [ts] fn getLastMeasurements() {
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return [];
// [ts] }
// [ts] return ReactDebugTool.getFlushHistory();
// [ts] }
// [ts] fn getExclusive() {
// [ts] let flushHistory = if (arguments.length > 0 && arguments[0] != null) arguments[0] else getLastMeasurements();
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return [];
// [ts] }
// [ts] let aggregatedStats = {};
// [ts] let affectedIDs = {};
// [ts] fn updateAggregatedStats(treeSnapshot: auto, instanceID: auto, timerType: auto, applyUpdate: auto) {
// [ts] let displayName = treeSnapshot[instanceID].displayName;
// [ts] let key = displayName;
// [ts] let stats = aggregatedStats[key];
// [ts] if (!stats) {
// [ts] affectedIDs[key] = {};
// [ts] stats = aggregatedStats[key] = {
// [ts] key: key,
// [ts] instanceCount: 0,
// [ts] counts: {},
// [ts] durations: {},
// [ts] totalDuration: 0
// [ts] };
// [ts] }
// [ts] if (!stats.durations[timerType]) {
// [ts] stats.durations[timerType] = 0;
// [ts] }
// [ts] if (!stats.counts[timerType]) {
// [ts] stats.counts[timerType] = 0;
// [ts] }
// [ts] affectedIDs[key][instanceID] = true;
// [ts] applyUpdate(stats);
// [ts] }
// [ts] flushHistory.forEach((flush) => {
// [ts] let measurements = flush.measurements,
// [ts] treeSnapshot = flush.treeSnapshot;
// [ts] measurements.forEach((measurement) => {
// [ts] let duration = measurement.duration,
// [ts] instanceID = measurement.instanceID,
// [ts] timerType = measurement.timerType;
// [ts] updateAggregatedStats(treeSnapshot, instanceID, timerType, (stats) => {
// [ts] stats.totalDuration += duration;
// [ts] stats.durations[timerType] += duration;
// [ts] stats.counts[timerType]++;
// [ts] });
// [ts] });
// [ts] });
// [ts] return Object.keys(aggregatedStats).map((key) => {
// [ts] return _extends({}, aggregatedStats[key], {
// [ts] instanceCount: Object.keys(affectedIDs[key]).length
// [ts] });
// [ts] }).sort((a, b) => {
// [ts] return b.totalDuration - a.totalDuration;
// [ts] });
// [ts] }
// [ts] fn getInclusive() {
// [ts] let flushHistory = if (arguments.length > 0 && arguments[0] != null) arguments[0] else getLastMeasurements();
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return [];
// [ts] }
// [ts] let aggregatedStats = {};
// [ts] let affectedIDs = {};
// [ts] fn updateAggregatedStats(treeSnapshot: auto, instanceID: auto, applyUpdate: auto) {
// [ts] let _treeSnapshot$instanc = treeSnapshot[instanceID],
// [ts] displayName = _treeSnapshot$instanc.displayName,
// [ts] ownerID = _treeSnapshot$instanc.ownerID;
// [ts] let owner = treeSnapshot[ownerID];
// [ts] let key = (if (owner) owner.displayName + " > " else "") + displayName;
// [ts] let stats = aggregatedStats[key];
// [ts] if (!stats) {
// [ts] affectedIDs[key] = {};
// [ts] stats = aggregatedStats[key] = {
// [ts] key: key,
// [ts] instanceCount: 0,
// [ts] inclusiveRenderDuration: 0,
// [ts] renderCount: 0
// [ts] };
// [ts] }
// [ts] affectedIDs[key][instanceID] = true;
// [ts] applyUpdate(stats);
// [ts] }
// [ts] let isCompositeByID = {};
// [ts] flushHistory.forEach((flush) => {
// [ts] let measurements = flush.measurements;
// [ts] measurements.forEach((measurement) => {
// [ts] let instanceID = measurement.instanceID,
// [ts] timerType = measurement.timerType;
// [ts] if (timerType != "render") {
// [ts] return;
// [ts] }
// [ts] isCompositeByID[instanceID] = true;
// [ts] });
// [ts] });
// [ts] flushHistory.forEach((flush) => {
// [ts] let measurements = flush.measurements,
// [ts] treeSnapshot = flush.treeSnapshot;
// [ts] measurements.forEach((measurement) => {
// [ts] let duration = measurement.duration,
// [ts] instanceID = measurement.instanceID,
// [ts] timerType = measurement.timerType;
// [ts] if (timerType != "render") {
// [ts] return;
// [ts] }
// [ts] updateAggregatedStats(treeSnapshot, instanceID, (stats) => {
// [ts] stats.renderCount++;
// [ts] });
// [ts] let nextParentID = instanceID;
// [ts] while (nextParentID) {
// As we traverse parents, only count inclusive time towards composites.
// We know something is a composite if its render() was called.
// [ts] if (isCompositeByID[nextParentID]) {
// [ts] updateAggregatedStats(treeSnapshot, nextParentID, (stats) => {
// [ts] stats.inclusiveRenderDuration += duration;
// [ts] });
// [ts] }
// [ts] nextParentID = treeSnapshot[nextParentID].parentID;
// [ts] }
// [ts] });
// [ts] });
// [ts] return Object.keys(aggregatedStats).map((key) => {
// [ts] return _extends({}, aggregatedStats[key], {
// [ts] instanceCount: Object.keys(affectedIDs[key]).length
// [ts] });
// [ts] }).sort((a, b) => {
// [ts] return b.inclusiveRenderDuration - a.inclusiveRenderDuration;
// [ts] });
// [ts] }
// [ts] fn getWasted() {
// [ts] let flushHistory = if (arguments.length > 0 && arguments[0] != null) arguments[0] else getLastMeasurements();
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return [];
// [ts] }
// [ts] let aggregatedStats = {};
// [ts] let affectedIDs = {};
// [ts] fn updateAggregatedStats(treeSnapshot: auto, instanceID: auto, applyUpdate: auto) {
// [ts] let _treeSnapshot$instanc2 = treeSnapshot[instanceID],
// [ts] displayName = _treeSnapshot$instanc2.displayName,
// [ts] ownerID = _treeSnapshot$instanc2.ownerID;
// [ts] let owner = treeSnapshot[ownerID];
// [ts] let key = (if (owner) owner.displayName + " > " else "") + displayName;
// [ts] let stats = aggregatedStats[key];
// [ts] if (!stats) {
// [ts] affectedIDs[key] = {};
// [ts] stats = aggregatedStats[key] = {
// [ts] key: key,
// [ts] instanceCount: 0,
// [ts] inclusiveRenderDuration: 0,
// [ts] renderCount: 0
// [ts] };
// [ts] }
// [ts] affectedIDs[key][instanceID] = true;
// [ts] applyUpdate(stats);
// [ts] }
// [ts] flushHistory.forEach((flush) => {
// [ts] let measurements = flush.measurements,
// [ts] treeSnapshot = flush.treeSnapshot,
// [ts] operations = flush.operations;
// [ts] let isDefinitelyNotWastedByID = {};
// Find host components associated with an operation in this batch.
// Mark all components in their parent tree as definitely not wasted.
// [ts] operations.forEach((operation) => {
// [ts] let instanceID = operation.instanceID;
// [ts] let nextParentID = instanceID;
// [ts] while (nextParentID) {
// [ts] isDefinitelyNotWastedByID[nextParentID] = true;
// [ts] nextParentID = treeSnapshot[nextParentID].parentID;
// [ts] }
// [ts] });
// Find composite components that rendered in this batch.
// These are potential candidates for being wasted renders.
// [ts] let renderedCompositeIDs = {};
// [ts] measurements.forEach((measurement) => {
// [ts] let instanceID = measurement.instanceID,
// [ts] timerType = measurement.timerType;
// [ts] if (timerType != "render") {
// [ts] return;
// [ts] }
// [ts] renderedCompositeIDs[instanceID] = true;
// [ts] });
// [ts] measurements.forEach((measurement) => {
// [ts] let duration = measurement.duration,
// [ts] instanceID = measurement.instanceID,
// [ts] timerType = measurement.timerType;
// [ts] if (timerType != "render") {
// [ts] return;
// [ts] }
// If there was a DOM update below this component, or it has just been
// mounted, its render() is not considered wasted.
// [ts] let updateCount = treeSnapshot[instanceID].updateCount;
// [ts] if (isDefinitelyNotWastedByID[instanceID] || updateCount == 0) {
// [ts] return;
// [ts] }
// We consider this render() wasted.
// [ts] updateAggregatedStats(treeSnapshot, instanceID, (stats) => {
// [ts] stats.renderCount++;
// [ts] });
// [ts] let nextParentID = instanceID;
// [ts] while (nextParentID) {
// Any parents rendered during this batch are considered wasted
// unless we previously marked them as dirty.
// [ts] let isWasted = renderedCompositeIDs[nextParentID] && !isDefinitelyNotWastedByID[nextParentID];
// [ts] if (isWasted) {
// [ts] updateAggregatedStats(treeSnapshot, nextParentID, (stats) => {
// [ts] stats.inclusiveRenderDuration += duration;
// [ts] });
// [ts] }
// [ts] nextParentID = treeSnapshot[nextParentID].parentID;
// [ts] }
// [ts] });
// [ts] });
// [ts] return Object.keys(aggregatedStats).map((key) => {
// [ts] return _extends({}, aggregatedStats[key], {
// [ts] instanceCount: Object.keys(affectedIDs[key]).length
// [ts] });
// [ts] }).sort((a, b) => {
// [ts] return b.inclusiveRenderDuration - a.inclusiveRenderDuration;
// [ts] });
// [ts] }
// [ts] fn getOperations() {
// [ts] let flushHistory = if (arguments.length > 0 && arguments[0] != null) arguments[0] else getLastMeasurements();
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return [];
// [ts] }
// [ts] let stats = [];
// [ts] flushHistory.forEach((flush, flushIndex) => {
// [ts] let operations = flush.operations,
// [ts] treeSnapshot = flush.treeSnapshot;
// [ts] operations.forEach((operation) => {
// [ts] let instanceID = operation.instanceID,
// [ts] type = operation.type,
// [ts] payload = operation.payload;
// [ts] let _treeSnapshot$instanc3 = treeSnapshot[instanceID],
// [ts] displayName = _treeSnapshot$instanc3.displayName,
// [ts] ownerID = _treeSnapshot$instanc3.ownerID;
// [ts] let owner = treeSnapshot[ownerID];
// [ts] let key = (if (owner) owner.displayName + " > " else "") + displayName;
// [ts] stats.push({
// [ts] flushIndex: flushIndex,
// [ts] instanceID: instanceID,
// [ts] key: key,
// [ts] type: type,
// [ts] ownerID: ownerID,
// [ts] payload: payload
// [ts] });
// [ts] });
// [ts] });
// [ts] return stats;
// [ts] }
// [ts] fn printExclusive(flushHistory: auto) {
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return;
// [ts] }
// [ts] let stats = getExclusive(flushHistory);
// [ts] let table = stats.map((item) => {
// [ts] let key = item.key,
// [ts] instanceCount = item.instanceCount,
// [ts] totalDuration = item.totalDuration;
// [ts] let renderCount = item.counts.render || 0;
// [ts] let renderDuration = item.durations.render || 0;
// [ts] return {
// [ts] Component: key,
// [ts] "Total time (ms)": roundFloat(totalDuration),
// [ts] "Instance count": instanceCount,
// [ts] "Total render time (ms)": roundFloat(renderDuration),
// [ts] if ("Average render time (ms)": renderCount) roundFloat(renderDuration / renderCount) else null,
// [ts] "Render count": renderCount,
// [ts] "Total lifecycle time (ms)": roundFloat(totalDuration - renderDuration)
// [ts] };
// [ts] });
// [ts] consoleTable(table);
// [ts] }
// [ts] fn printInclusive(flushHistory: auto) {
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return;
// [ts] }
// [ts] let stats = getInclusive(flushHistory);
// [ts] let table = stats.map((item) => {
// [ts] let key = item.key,
// [ts] instanceCount = item.instanceCount,
// [ts] inclusiveRenderDuration = item.inclusiveRenderDuration,
// [ts] renderCount = item.renderCount;
// [ts] return {
// [ts] "Owner > Component": key,
// [ts] "Inclusive render time (ms)": roundFloat(inclusiveRenderDuration),
// [ts] "Instance count": instanceCount,
// [ts] "Render count": renderCount
// [ts] };
// [ts] });
// [ts] consoleTable(table);
// [ts] }
// [ts] fn printWasted(flushHistory: auto) {
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return;
// [ts] }
// [ts] let stats = getWasted(flushHistory);
// [ts] let table = stats.map((item) => {
// [ts] let key = item.key,
// [ts] instanceCount = item.instanceCount,
// [ts] inclusiveRenderDuration = item.inclusiveRenderDuration,
// [ts] renderCount = item.renderCount;
// [ts] return {
// [ts] "Owner > Component": key,
// [ts] "Inclusive wasted time (ms)": roundFloat(inclusiveRenderDuration),
// [ts] "Instance count": instanceCount,
// [ts] "Render count": renderCount
// [ts] };
// [ts] });
// [ts] consoleTable(table);
// [ts] }
// [ts] fn printOperations(flushHistory: auto) {
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return;
// [ts] }
// [ts] let stats = getOperations(flushHistory);
// [ts] let table = stats.map((stat) => {
// [ts] return {
// [ts] "Owner > Node": stat.key,
// [ts] Operation: stat.type,
// [ts] Payload: if (@typeName(stat.payload) == "object") JSON.stringify(stat.payload) else stat.payload,
// [ts] "Flush index": stat.flushIndex,
// [ts] "Owner Component ID": stat.ownerID,
// [ts] "DOM Component ID": stat.instanceID
// [ts] };
// [ts] });
// [ts] consoleTable(table);
// [ts] }
// [ts] let warnedAboutPrintDOM = false;
// [ts] fn printDOM(measurements: auto) {
// [ts] lowPriorityWarning(warnedAboutPrintDOM, "`ReactPerf.printDOM(...)` is deprecated. Use " + "`ReactPerf.printOperations(...)` instead.");
// [ts] warnedAboutPrintDOM = true;
// [ts] return printOperations(measurements);
// [ts] }
// [ts] let warnedAboutGetMeasurementsSummaryMap = false;
// [ts] fn getMeasurementsSummaryMap(measurements: auto) {
// [ts] lowPriorityWarning(warnedAboutGetMeasurementsSummaryMap, "`ReactPerf.getMeasurementsSummaryMap(...)` is deprecated. Use " + "`ReactPerf.getWasted(...)` instead.");
// [ts] warnedAboutGetMeasurementsSummaryMap = true;
// [ts] return getWasted(measurements);
// [ts] }
// [ts] fn start() {
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return;
// [ts] }
// [ts] ReactDebugTool.beginProfiling();
// [ts] }
// [ts] fn stop() {
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return;
// [ts] }
// [ts] ReactDebugTool.endProfiling();
// [ts] }
// [ts] fn isRunning() {
// [ts] if (!("development" != "production")) {
// [ts] warnInProduction();
// [ts] return false;
// [ts] }
// [ts] return ReactDebugTool.isProfiling();
// [ts] }
// [ts] let ReactPerfAnalysis = {
// [ts] getLastMeasurements: getLastMeasurements,
// [ts] getExclusive: getExclusive,
// [ts] getInclusive: getInclusive,
// [ts] getWasted: getWasted,
// [ts] getOperations: getOperations,
// [ts] printExclusive: printExclusive,
// [ts] printInclusive: printInclusive,
// [ts] printWasted: printWasted,
// [ts] printOperations: printOperations,
// [ts] start: start,
// [ts] stop: stop,
// [ts] isRunning: isRunning,
// Deprecated:
// [ts] printDOM: printDOM,
// [ts] getMeasurementsSummaryMap: getMeasurementsSummaryMap
// [ts] };
// [ts] module.exports = ReactPerfAnalysis;
// [ts] },{"124":124,"160":160,"50":50}],72:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactPropTypeLocationNames = {};
// [ts] if ("development" != "production") {
// [ts] ReactPropTypeLocationNames = {
// [ts] prop: "prop",
// [ts] context: "context",
// [ts] childContext: "child context"
// [ts] };
// [ts] }
// [ts] module.exports = ReactPropTypeLocationNames;
// [ts] },{}],73:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactPropTypesSecret = "SECRET_DO_NOT_PASS_THIS_OR_YOU_WILL_BE_FIRED";
// [ts] module.exports = ReactPropTypesSecret;
// [ts] },{}],74:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let CallbackQueue = _dereq_(6);
// [ts] let PooledClass = _dereq_(25);
// [ts] let ReactBrowserEventEmitter = _dereq_(26);
// [ts] let ReactInputSelection = _dereq_(62);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let Transaction = _dereq_(100);
// [ts] let ReactUpdateQueue = _dereq_(81);
// [ts] /**
// [ts] * Ensures that, when possible, the selection range (currently selected text
// [ts] * input) is not disturbed by performing the transaction.
// [ts] */
// [ts] let SELECTION_RESTORATION = {
// [ts] /**
// [ts] * @return {Selection} Selection information.
// [ts] */
// [ts] initialize: ReactInputSelection.getSelectionInformation,
// [ts] /**
// [ts] * @param {Selection} sel Selection information returned from `initialize`.
// [ts] */
// [ts] close: ReactInputSelection.restoreSelection
// [ts] };
// [ts] /**
// [ts] * Suppresses events (blur/focus) that could be inadvertently dispatched due to
// [ts] * high level DOM manipulations (like temporarily removing a text input from the
// [ts] * DOM).
// [ts] */
// [ts] let EVENT_SUPPRESSION = {
// [ts] /**
// [ts] * @return {boolean} The enabled status of `ReactBrowserEventEmitter` before
// [ts] * the reconciliation.
// [ts] */
// [ts] initialize: () => {
// [ts] let currentlyEnabled = ReactBrowserEventEmitter.isEnabled();
// [ts] ReactBrowserEventEmitter.setEnabled(false);
// [ts] return currentlyEnabled;
// [ts] },
// [ts] /**
// [ts] * @param {boolean} previouslyEnabled Enabled status of
// [ts] *   `ReactBrowserEventEmitter` before the reconciliation occurred. `close`
// [ts] *   restores the previous value.
// [ts] */
// [ts] close: (previouslyEnabled) => {
// [ts] ReactBrowserEventEmitter.setEnabled(previouslyEnabled);
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Provides a queue for collecting `componentDidMount` and
// [ts] * `componentDidUpdate` callbacks during the transaction.
// [ts] */
// [ts] let ON_DOM_READY_QUEUEING = {
// [ts] /**
// [ts] * Initializes the internal `onDOMReady` queue.
// [ts] */
// [ts] initialize: () => {
// [ts] _.reactMountReady.reset();
// [ts] },
// [ts] /**
// [ts] * After DOM is flushed, invoke all registered `onDOMReady` callbacks.
// [ts] */
// [ts] close: () => {
// [ts] _.reactMountReady.notifyAll();
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Executed within the scope of the `Transaction` instance. Consider these as
// [ts] * being member methods, but with an implied ordering while being isolated from
// [ts] * each other.
// [ts] */
// [ts] let TRANSACTION_WRAPPERS = [SELECTION_RESTORATION, EVENT_SUPPRESSION, ON_DOM_READY_QUEUEING];
// [ts] if ("development" != "production") {
// [ts] TRANSACTION_WRAPPERS.push({
// [ts] initialize: ReactInstrumentation.debugTool.onBeginFlush,
// [ts] close: ReactInstrumentation.debugTool.onEndFlush
// [ts] });
// [ts] }
// [ts] /**
// [ts] * Currently:
// [ts] * - The order that these are listed in the transaction is critical:
// [ts] * - Suppresses events.
// [ts] * - Restores selection range.
// [ts] *
// [ts] * Future:
// [ts] * - Restore document/overflow scroll positions that were unintentionally
// [ts] *   modified via DOM insertions above the top viewport boundary.
// [ts] * - Implement/integrate with customized constraint based layout system and keep
// [ts] *   track of which dimensions must be remeasured.
// [ts] *
// [ts] * @class ReactReconcileTransaction
// [ts] */
// [ts] fn ReactReconcileTransaction(useCreateElement: auto) {
// [ts] _.reinitializeTransaction();
// Only server-side rendering really needs this option (see
// `ReactServerRendering`), but server-side uses
// `ReactServerRenderingTransaction` instead. This option is here so that it's
// accessible and defaults to false when `ReactDOMComponent` and
// `ReactDOMTextComponent` checks it in `mountComponent`.`
// [ts] _.renderToStaticMarkup = false;
// [ts] _.reactMountReady = CallbackQueue.getPooled(null);
// [ts] _.useCreateElement = useCreateElement;
// [ts] }
// [ts] let Mixin = {
// [ts] /**
// [ts] * @see Transaction
// [ts] * @abstract
// [ts] * @final
// [ts] * @return {array<object>} List of operation wrap procedures.
// [ts] *   TODO: convert to array<TransactionWrapper>
// [ts] */
// [ts] getTransactionWrappers: () => {
// [ts] return TRANSACTION_WRAPPERS;
// [ts] },
// [ts] /**
// [ts] * @return {object} The queue to collect `onDOMReady` callbacks with.
// [ts] */
// [ts] getReactMountReady: () => {
// [ts] return _.reactMountReady;
// [ts] },
// [ts] /**
// [ts] * @return {object} The queue to collect React async events.
// [ts] */
// [ts] getUpdateQueue: () => {
// [ts] return ReactUpdateQueue;
// [ts] },
// [ts] /**
// [ts] * Save current transaction state -- if the return value from this method is
// [ts] * passed to `rollback`, the transaction will be reset to that state.
// [ts] */
// [ts] checkpoint: () => {
// reactMountReady is the our only stateful wrapper
// [ts] return _.reactMountReady.checkpoint();
// [ts] },
// [ts] rollback: (checkpoint) => {
// [ts] _.reactMountReady.rollback(checkpoint);
// [ts] },
// [ts] /**
// [ts] * `PooledClass` looks for this, and will invoke this before allowing this
// [ts] * instance to be reused.
// [ts] */
// [ts] destructor: () => {
// [ts] CallbackQueue.release(_.reactMountReady);
// [ts] _.reactMountReady = null;
// [ts] }
// [ts] };
// [ts] _assign(ReactReconcileTransaction.prototype, Transaction, Mixin);
// [ts] PooledClass.addPoolingTo(ReactReconcileTransaction);
// [ts] module.exports = ReactReconcileTransaction;
// [ts] },{"100":100,"160":160,"25":25,"26":26,"6":6,"62":62,"64":64,"81":81}],75:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactRef = _dereq_(76);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let warning = _dereq_(159);
// [ts] /**
// [ts] * Helper to call ReactRef.attachRefs with this composite component, split out
// [ts] * to avoid allocations in the transaction mount-ready queue.
// [ts] */
// [ts] fn attachRefs() {
// [ts] ReactRef.attachRefs(this, _._currentElement);
// [ts] }
// [ts] let ReactReconciler = {
// [ts] /**
// [ts] * Initializes the component, renders markup, and registers event listeners.
// [ts] *
// [ts] * @param {ReactComponent} internalInstance
// [ts] * @param {ReactReconcileTransaction|ReactServerRenderingTransaction} transaction
// [ts] * @param {?object} the containing host component instance
// [ts] * @param {?object} info about the host container
// [ts] * @return {?string} Rendered markup to be inserted into the DOM.
// [ts] * @final
// [ts] * @internal
// [ts] */
// [ts] mountComponent: function (internalInstance, transaction, hostParent, hostContainerInfo, context, parentDebugID) // 0 in production and for roots
// [ts] {
// [ts] if ("development" != "production") {
// [ts] if (internalInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onBeforeMountComponent(internalInstance._debugID, internalInstance._currentElement, parentDebugID);
// [ts] }
// [ts] }
// [ts] let markup = internalInstance.mountComponent(transaction, hostParent, hostContainerInfo, context, parentDebugID);
// [ts] if (internalInstance._currentElement && internalInstance._currentElement.ref != null) {
// [ts] transaction.getReactMountReady().enqueue(attachRefs, internalInstance);
// [ts] }
// [ts] if ("development" != "production") {
// [ts] if (internalInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onMountComponent(internalInstance._debugID);
// [ts] }
// [ts] }
// [ts] return markup;
// [ts] },
// [ts] /**
// [ts] * Returns a value that can be passed to
// [ts] * ReactComponentEnvironment.replaceNodeWithMarkup.
// [ts] */
// [ts] getHostNode: (internalInstance) => {
// [ts] return internalInstance.getHostNode();
// [ts] },
// [ts] /**
// [ts] * Releases any resources allocated by `mountComponent`.
// [ts] *
// [ts] * @final
// [ts] * @internal
// [ts] */
// [ts] unmountComponent: (internalInstance, safely) => {
// [ts] if ("development" != "production") {
// [ts] if (internalInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onBeforeUnmountComponent(internalInstance._debugID);
// [ts] }
// [ts] }
// [ts] ReactRef.detachRefs(internalInstance, internalInstance._currentElement);
// [ts] internalInstance.unmountComponent(safely);
// [ts] if ("development" != "production") {
// [ts] if (internalInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onUnmountComponent(internalInstance._debugID);
// [ts] }
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Update a component using a new element.
// [ts] *
// [ts] * @param {ReactComponent} internalInstance
// [ts] * @param {ReactElement} nextElement
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @param {object} context
// [ts] * @internal
// [ts] */
// [ts] receiveComponent: (internalInstance, nextElement, transaction, context) => {
// [ts] let prevElement = internalInstance._currentElement;
// [ts] if (nextElement == prevElement && context == internalInstance._context) {
// Since elements are immutable after the owner is rendered,
// we can do a cheap identity compare here to determine if this is a
// superfluous reconcile. It's possible for state to be mutable but such
// change should trigger an update of the owner which would recreate
// the element. We explicitly check for the existence of an owner since
// it's possible for an element created outside a composite to be
// deeply mutated and reused.
// TODO: Bailing out early is just a perf optimization right?
// TODO: Removing the return statement should affect correctness?
// [ts] return;
// [ts] }
// [ts] if ("development" != "production") {
// [ts] if (internalInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onBeforeUpdateComponent(internalInstance._debugID, nextElement);
// [ts] }
// [ts] }
// [ts] let refsChanged = ReactRef.shouldUpdateRefs(prevElement, nextElement);
// [ts] if (refsChanged) {
// [ts] ReactRef.detachRefs(internalInstance, prevElement);
// [ts] }
// [ts] internalInstance.receiveComponent(nextElement, transaction, context);
// [ts] if (refsChanged && internalInstance._currentElement && internalInstance._currentElement.ref != null) {
// [ts] transaction.getReactMountReady().enqueue(attachRefs, internalInstance);
// [ts] }
// [ts] if ("development" != "production") {
// [ts] if (internalInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onUpdateComponent(internalInstance._debugID);
// [ts] }
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Flush any dirty changes in a component.
// [ts] *
// [ts] * @param {ReactComponent} internalInstance
// [ts] * @param {ReactReconcileTransaction} transaction
// [ts] * @internal
// [ts] */
// [ts] performUpdateIfNecessary: (internalInstance, transaction, updateBatchNumber) => {
// [ts] if (internalInstance._updateBatchNumber != updateBatchNumber) {
// The component's enqueued batch number should always be the current
// batch or the following one.
// [ts] if ("development" != "production") warning(internalInstance._updateBatchNumber == null || internalInstance._updateBatchNumber == updateBatchNumber + 1, "performUpdateIfNecessary: Unexpected batch number (current %s, " + "pending %s)", updateBatchNumber, internalInstance._updateBatchNumber) else void 0;
// [ts] return;
// [ts] }
// [ts] if ("development" != "production") {
// [ts] if (internalInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onBeforeUpdateComponent(internalInstance._debugID, internalInstance._currentElement);
// [ts] }
// [ts] }
// [ts] internalInstance.performUpdateIfNecessary(transaction);
// [ts] if ("development" != "production") {
// [ts] if (internalInstance._debugID != 0) {
// [ts] ReactInstrumentation.debugTool.onUpdateComponent(internalInstance._debugID);
// [ts] }
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = ReactReconciler;
// [ts] },{"159":159,"64":64,"76":76}],76:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactOwner = _dereq_(70);
// [ts] let ReactRef = {};
// [ts] fn attachRef(ref: auto, component: auto, owner: auto) {
// [ts] if (@typeName(ref) == "function") {
// [ts] ref(component.getPublicInstance());
// [ts] } else {
// Legacy ref
// [ts] ReactOwner.addComponentAsRefTo(component, ref, owner);
// [ts] }
// [ts] }
// [ts] fn detachRef(ref: auto, component: auto, owner: auto) {
// [ts] if (@typeName(ref) == "function") {
// [ts] ref(null);
// [ts] } else {
// Legacy ref
// [ts] ReactOwner.removeComponentAsRefFrom(component, ref, owner);
// [ts] }
// [ts] }
// [ts] ReactRef.attachRefs = (instance, element) => {
// [ts] if (element == null || @typeName(element) != "object") {
// [ts] return;
// [ts] }
// [ts] let ref = element.ref;
// [ts] if (ref != null) {
// [ts] attachRef(ref, instance, element._owner);
// [ts] }
// [ts] };
// [ts] ReactRef.shouldUpdateRefs = (prevElement, nextElement) => {
// If either the owner or a `ref` has changed, make sure the newest owner
// has stored a reference to `this`, and the previous owner (if different)
// has forgotten the reference to `this`. We use the element instead
// of the public this.props because the post processing cannot determine
// a ref. The ref conceptually lives on the element.
// TODO: Should this even be possible? The owner cannot change because
// it's forbidden by shouldUpdateReactComponent. The ref can change
// if you swap the keys of but not the refs. Reconsider where this check
// is made. It probably belongs where the key checking and
// instantiateReactComponent is done.
// [ts] let prevRef = null;
// [ts] let prevOwner = null;
// [ts] if (prevElement != null && @typeName(prevElement) == "object") {
// [ts] prevRef = prevElement.ref;
// [ts] prevOwner = prevElement._owner;
// [ts] }
// [ts] let nextRef = null;
// [ts] let nextOwner = null;
// [ts] if (nextElement != null && @typeName(nextElement) == "object") {
// [ts] nextRef = nextElement.ref;
// [ts] nextOwner = nextElement._owner;
// [ts] }
// [ts] return prevRef != nextRef ||
// If owner changes but we have an unchanged function ref, don't update refs
// [ts] @typeName(nextRef) == "string" && nextOwner != prevOwner;
// [ts] };
// [ts] ReactRef.detachRefs = (instance, element) => {
// [ts] if (element == null || @typeName(element) != "object") {
// [ts] return;
// [ts] }
// [ts] let ref = element.ref;
// [ts] if (ref != null) {
// [ts] detachRef(ref, instance, element._owner);
// [ts] }
// [ts] };
// [ts] module.exports = ReactRef;
// [ts] },{"70":70}],77:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let PooledClass = _dereq_(25);
// [ts] let Transaction = _dereq_(100);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let ReactServerUpdateQueue = _dereq_(78);
// [ts] /**
// [ts] * Executed within the scope of the `Transaction` instance. Consider these as
// [ts] * being member methods, but with an implied ordering while being isolated from
// [ts] * each other.
// [ts] */
// [ts] let TRANSACTION_WRAPPERS = [];
// [ts] if ("development" != "production") {
// [ts] TRANSACTION_WRAPPERS.push({
// [ts] initialize: ReactInstrumentation.debugTool.onBeginFlush,
// [ts] close: ReactInstrumentation.debugTool.onEndFlush
// [ts] });
// [ts] }
// [ts] let noopCallbackQueue = {
// [ts] enqueue: () => {}
// [ts] };
// [ts] /**
// [ts] * @class ReactServerRenderingTransaction
// [ts] * @param {boolean} renderToStaticMarkup
// [ts] */
// [ts] fn ReactServerRenderingTransaction(renderToStaticMarkup: auto) {
// [ts] _.reinitializeTransaction();
// [ts] _.renderToStaticMarkup = renderToStaticMarkup;
// [ts] _.useCreateElement = false;
// [ts] _.updateQueue = ReactServerUpdateQueue.init(this);
// [ts] }
// [ts] let Mixin = {
// [ts] /**
// [ts] * @see Transaction
// [ts] * @abstract
// [ts] * @final
// [ts] * @return {array} Empty list of operation wrap procedures.
// [ts] */
// [ts] getTransactionWrappers: () => {
// [ts] return TRANSACTION_WRAPPERS;
// [ts] },
// [ts] /**
// [ts] * @return {object} The queue to collect `onDOMReady` callbacks with.
// [ts] */
// [ts] getReactMountReady: () => {
// [ts] return noopCallbackQueue;
// [ts] },
// [ts] /**
// [ts] * @return {object} The queue to collect React async events.
// [ts] */
// [ts] getUpdateQueue: () => {
// [ts] return _.updateQueue;
// [ts] },
// [ts] /**
// [ts] * `PooledClass` looks for this, and will invoke this before allowing this
// [ts] * instance to be reused.
// [ts] */
// [ts] destructor: () => {},
// [ts] checkpoint: () => {},
// [ts] rollback: () => {}
// [ts] };
// [ts] _assign(ReactServerRenderingTransaction.prototype, Transaction, Mixin);
// [ts] PooledClass.addPoolingTo(ReactServerRenderingTransaction);
// [ts] module.exports = ReactServerRenderingTransaction;
// [ts] },{"100":100,"160":160,"25":25,"64":64,"78":78}],78:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2015-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] fn _classCallCheck(instance: auto, Constructor: auto) { if (!(instance == Constructor)) { throw TypeError.init("Cannot call a class as a function"); } }
// [ts] let ReactUpdateQueue = _dereq_(81);
// [ts] let warning = _dereq_(159);
// [ts] fn warnNoop(publicInstance: auto, callerName: auto) {
// [ts] if ("development" != "production") {
// [ts] let constructor = publicInstance.constructor;
// [ts] if ("development" != "production") warning(false, "%s(...): Can only update a mounting component. " + "This usually means you called %s() outside componentWillMount() on the server. " + "This is a no-op. Please check the code for the %s component.", callerName, callerName, constructor && (constructor.displayName || constructor.name) || "ReactClass") else void 0;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * This is the update queue used for server rendering.
// [ts] * It delegates to ReactUpdateQueue while server rendering is in progress and
// [ts] * switches to ReactNoopUpdateQueue after the transaction has completed.
// [ts] * @class ReactServerUpdateQueue
// [ts] * @param {Transaction} transaction
// [ts] */
// [ts] let ReactServerUpdateQueue = () => {
// [ts] fn ReactServerUpdateQueue(transaction: auto) {
// [ts] _classCallCheck(this, ReactServerUpdateQueue);
// [ts] _.transaction = transaction;
// [ts] }
// [ts] /**
// [ts] * Checks whether or not this composite component is mounted.
// [ts] * @param {ReactClass} publicInstance The instance we want to test.
// [ts] * @return {boolean} True if mounted, false otherwise.
// [ts] * @protected
// [ts] * @final
// [ts] */
// [ts] ReactServerUpdateQueue.prototype.isMounted = (publicInstance) => {
// [ts] return false;
// [ts] };
// [ts] /**
// [ts] * Enqueue a callback that will be executed after all the pending updates
// [ts] * have processed.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance to use as `this` context.
// [ts] * @param {?function} callback Called after state is updated.
// [ts] * @internal
// [ts] */
// [ts] ReactServerUpdateQueue.prototype.enqueueCallback = (publicInstance, callback, callerName) => {
// [ts] if (_.transaction.isInTransaction()) {
// [ts] ReactUpdateQueue.enqueueCallback(publicInstance, callback, callerName);
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Forces an update. This should only be invoked when it is known with
// [ts] * certainty that we are **not** in a DOM transaction.
// [ts] *
// [ts] * You may want to call this when you know that some deeper aspect of the
// [ts] * component's state has changed but `setState` was not called.
// [ts] *
// [ts] * This will not invoke `shouldComponentUpdate`, but it will invoke
// [ts] * `componentWillUpdate` and `componentDidUpdate`.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @internal
// [ts] */
// [ts] ReactServerUpdateQueue.prototype.enqueueForceUpdate = (publicInstance) => {
// [ts] if (_.transaction.isInTransaction()) {
// [ts] ReactUpdateQueue.enqueueForceUpdate(publicInstance);
// [ts] } else {
// [ts] warnNoop(publicInstance, "forceUpdate");
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Replaces all of the state. Always use this or `setState` to mutate state.
// [ts] * You should treat `this.state` as immutable.
// [ts] *
// [ts] * There is no guarantee that `this.state` will be immediately updated, so
// [ts] * accessing `this.state` after calling this method may return the old value.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @param {object|function} completeState Next state.
// [ts] * @internal
// [ts] */
// [ts] ReactServerUpdateQueue.prototype.enqueueReplaceState = (publicInstance, completeState) => {
// [ts] if (_.transaction.isInTransaction()) {
// [ts] ReactUpdateQueue.enqueueReplaceState(publicInstance, completeState);
// [ts] } else {
// [ts] warnNoop(publicInstance, "replaceState");
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Sets a subset of the state. This only exists because _pendingState is
// [ts] * internal. This provides a merging strategy that is not available to deep
// [ts] * properties which is confusing. TODO: Expose pendingState or don't use it
// [ts] * during the merge.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @param {object|function} partialState Next partial state to be merged with state.
// [ts] * @internal
// [ts] */
// [ts] ReactServerUpdateQueue.prototype.enqueueSetState = (publicInstance, partialState) => {
// [ts] if (_.transaction.isInTransaction()) {
// [ts] ReactUpdateQueue.enqueueSetState(publicInstance, partialState);
// [ts] } else {
// [ts] warnNoop(publicInstance, "setState");
// [ts] }
// [ts] };
// [ts] return ReactServerUpdateQueue;
// [ts] }();
// [ts] module.exports = ReactServerUpdateQueue;
// [ts] },{"159":159,"81":81}],79:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] fn _classCallCheck(instance: auto, Constructor: auto) { if (!(instance == Constructor)) { throw TypeError.init("Cannot call a class as a function"); } }
// [ts] let React = _dereq_(135);
// [ts] let ReactCompositeComponent = _dereq_(30);
// [ts] let ReactDefaultBatchingStrategy = _dereq_(51);
// [ts] let ReactReconciler = _dereq_(75);
// [ts] let ReactReconcileTransaction = _dereq_(74);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let emptyObject = _dereq_(145);
// [ts] let getNextDebugID = _dereq_(136);
// [ts] let invariant = _dereq_(152);
// [ts] fn injectDefaults() {
// [ts] ReactUpdates.injection.injectReconcileTransaction(ReactReconcileTransaction);
// [ts] ReactUpdates.injection.injectBatchingStrategy(ReactDefaultBatchingStrategy);
// [ts] }
// [ts] let NoopInternalComponent = () => {
// [ts] fn NoopInternalComponent(element: auto) {
// [ts] _classCallCheck(this, NoopInternalComponent);
// [ts] _._renderedOutput = element;
// [ts] _._currentElement = element;
// [ts] if ("development" != "production") {
// [ts] _._debugID = getNextDebugID();
// [ts] }
// [ts] }
// [ts] NoopInternalComponent.prototype.mountComponent = () => {};
// [ts] NoopInternalComponent.prototype.receiveComponent = (element) => {
// [ts] _._renderedOutput = element;
// [ts] _._currentElement = element;
// [ts] };
// [ts] NoopInternalComponent.prototype.unmountComponent = () => {};
// [ts] NoopInternalComponent.prototype.getHostNode = () => {
// [ts] return null;
// [ts] };
// [ts] NoopInternalComponent.prototype.getPublicInstance = () => {
// [ts] return null;
// [ts] };
// [ts] return NoopInternalComponent;
// [ts] }();
// [ts] let ShallowComponentWrapper = (element) => {
// TODO: Consolidate with instantiateReactComponent
// [ts] if ("development" != "production") {
// [ts] _._debugID = getNextDebugID();
// [ts] }
// [ts] _.construct(element);
// [ts] };
// [ts] _assign(ShallowComponentWrapper.prototype, ReactCompositeComponent, {
// [ts] _constructComponent: ReactCompositeComponent._constructComponentWithoutOwner,
// [ts] _instantiateReactComponent: (element) => {
// [ts] return NoopInternalComponent.init(element);
// [ts] },
// [ts] _replaceNodeWithMarkup: () => {},
// [ts] _renderValidatedComponent: ReactCompositeComponent._renderValidatedComponentWithoutOwnerOrContext
// [ts] });
// [ts] fn _batchedRender(renderer: auto, element: auto, context: auto) {
// [ts] let transaction = ReactUpdates.ReactReconcileTransaction.getPooled(true);
// [ts] renderer._render(element, transaction, context);
// [ts] ReactUpdates.ReactReconcileTransaction.release(transaction);
// [ts] }
// [ts] let ReactShallowRenderer = () => {
// [ts] fn ReactShallowRenderer() {
// [ts] _classCallCheck(this, ReactShallowRenderer);
// [ts] _._instance = null;
// [ts] }
// [ts] ReactShallowRenderer.prototype.getMountedInstance = () => {
// [ts] return if (_._instance) _._instance._instance else null;
// [ts] };
// [ts] ReactShallowRenderer.prototype.render = (element, context) => {
// Ensure we've done the default injections. This might not be true in the
// case of a simple test that only requires React and the TestUtils in
// conjunction with an inline-requires transform.
// [ts] injectDefaults();
// [ts] if (!React.isValidElement(element)) if ("development" != "production") invariant(if (false, "ReactShallowRenderer render(): Invalid component element.%s", @typeName(element) == "function") " Instead of passing a component class, make sure to instantiate " + "it by passing it to React.createElement." else "") else _prodInvariant(if ("12", @typeName(element) == "function") " Instead of passing a component class, make sure to instantiate " + "it by passing it to React.createElement." else "") else void 0;
// [ts] if (!(@typeName(element.type) != "string")) if ("development" != "production") invariant(false, "ReactShallowRenderer render(): Shallow rendering works only with custom components, not primitives (%s). Instead of calling `.render(el)` and inspecting the rendered output, look at `el.props` directly instead.", element.type) else _prodInvariant("13", element.type) else void 0;
// [ts] if (!context) {
// [ts] context = emptyObject;
// [ts] }
// [ts] ReactUpdates.batchedUpdates(_batchedRender, this, element, context);
// [ts] return _.getRenderOutput();
// [ts] };
// [ts] ReactShallowRenderer.prototype.getRenderOutput = () => {
// [ts] return _._instance && _._instance._renderedComponent && _._instance._renderedComponent._renderedOutput || null;
// [ts] };
// [ts] ReactShallowRenderer.prototype.unmount = () => {
// [ts] if (_._instance) {
// [ts] ReactReconciler.unmountComponent(_._instance, false);
// [ts] }
// [ts] };
// [ts] ReactShallowRenderer.prototype.unstable_batchedUpdates = (callback, bookkeeping) => {
// This is used by Enzyme for fake-simulating events in shallow mode.
// [ts] injectDefaults();
// [ts] return ReactUpdates.batchedUpdates(callback, bookkeeping);
// [ts] };
// [ts] ReactShallowRenderer.prototype._render = (element, transaction, context) => {
// [ts] if (_._instance) {
// [ts] ReactReconciler.receiveComponent(_._instance, element, transaction, context);
// [ts] } else {
// [ts] let instance = ShallowComponentWrapper.init(element);
// [ts] ReactReconciler.mountComponent(instance, transaction, null, null, context, 0);
// [ts] _._instance = instance;
// [ts] }
// [ts] };
// [ts] return ReactShallowRenderer;
// [ts] }();
// [ts] ReactShallowRenderer.createRenderer = () => {
// [ts] return ReactShallowRenderer.init();
// [ts] };
// [ts] module.exports = ReactShallowRenderer;
// [ts] },{"126":126,"135":135,"136":136,"145":145,"152":152,"160":160,"30":30,"51":51,"74":74,"75":75,"82":82}],80:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] let EventConstants = _dereq_(16);
// [ts] let EventPluginHub = _dereq_(17);
// [ts] let EventPluginRegistry = _dereq_(18);
// [ts] let EventPropagators = _dereq_(20);
// [ts] let React = _dereq_(135);
// [ts] let ReactDOM = _dereq_(31);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactBrowserEventEmitter = _dereq_(26);
// [ts] let ReactInstanceMap = _dereq_(63);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] let ReactShallowRenderer = _dereq_(79);
// [ts] let findDOMNode = _dereq_(108);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] let topLevelTypes = EventConstants.topLevelTypes;
// [ts] fn Event(suffix: auto) {}
// In react 16+ shallowRenderer will not be accessible via ReactTestUtils.createRenderer()
// Instead it will be available via react-test-renderer/shallow
// Maintain backwards compat for 15.5.0 release, but warn about using the deprecated method
// [ts] let hasWarnedAboutCreateRenderer = false;
// [ts] fn createRendererWithWarning() {
// [ts] if ("development" != "production") warning(hasWarnedAboutCreateRenderer, "Shallow renderer has been moved to react-test-renderer/shallow. " + "Update references to remove this warning.") else void 0;
// [ts] hasWarnedAboutCreateRenderer = true;
// [ts] return ReactShallowRenderer.init();
// [ts] }
// [ts] /**
// [ts] * @class ReactTestUtils
// [ts] */
// [ts] fn findAllInRenderedTreeInternal(inst: auto, test: auto) {
// [ts] if (!inst || !inst.getPublicInstance) {
// [ts] return [];
// [ts] }
// [ts] let publicInst = inst.getPublicInstance();
// [ts] let ret = if (test(publicInst)) [publicInst] else [];
// [ts] let currentElement = inst._currentElement;
// [ts] if (ReactTestUtils.isDOMComponent(publicInst)) {
// [ts] let renderedChildren = inst._renderedChildren;
// [ts] let key;
// [ts] for (key in renderedChildren) {
// [ts] if (!renderedChildren.hasOwnProperty(key)) {
// [ts] continue;
// [ts] }
// [ts] ret = ret.concat(findAllInRenderedTreeInternal(renderedChildren[key], test));
// [ts] }
// [ts] } else if (React.isValidElement(currentElement) && @typeName(currentElement.type) == "function") {
// [ts] ret = ret.concat(findAllInRenderedTreeInternal(inst._renderedComponent, test));
// [ts] }
// [ts] return ret;
// [ts] }
// [ts] /**
// [ts] * Utilities for making it easy to test React components.
// [ts] *
// [ts] * See https://facebook.github.io/react/docs/test-utils.html
// [ts] *
// [ts] * Todo: Support the entire DOM.scry query syntax. For now, these simple
// [ts] * utilities will suffice for testing purposes.
// [ts] * @lends ReactTestUtils
// [ts] */
// [ts] let ReactTestUtils = {
// [ts] renderIntoDocument: (element) => {
// [ts] let div = document.createElement("div");
// None of our tests actually require attaching the container to the
// DOM, and doing so creates a mess that we rely on test isolation to
// clean up, so we're going to stop honoring the name of this method
// (and probably rename it eventually) if no problems arise.
// document.documentElement.appendChild(div);
// [ts] return ReactDOM.render(element, div);
// [ts] },
// [ts] isElement: (element) => {
// [ts] return React.isValidElement(element);
// [ts] },
// [ts] isElementOfType: (inst, convenienceConstructor) => {
// [ts] return React.isValidElement(inst) && inst.type == convenienceConstructor;
// [ts] },
// [ts] isDOMComponent: (inst) => {
// [ts] return !!(inst && inst.nodeType == 1 && inst.tagName);
// [ts] },
// [ts] isDOMComponentElement: (inst) => {
// [ts] return !!(inst && React.isValidElement(inst) && !!inst.tagName);
// [ts] },
// [ts] isCompositeComponent: (inst) => {
// [ts] if (ReactTestUtils.isDOMComponent(inst)) {
// Accessing inst.setState warns; just return false as that'll be what
// this returns when we have DOM nodes as refs directly
// [ts] return false;
// [ts] }
// [ts] return inst != null && @typeName(inst.render) == "function" && @typeName(inst.setState) == "function";
// [ts] },
// [ts] isCompositeComponentWithType: (inst, type) => {
// [ts] if (!ReactTestUtils.isCompositeComponent(inst)) {
// [ts] return false;
// [ts] }
// [ts] let internalInstance = ReactInstanceMap.get(inst);
// [ts] let constructor = internalInstance._currentElement.type;
// [ts] return constructor == type;
// [ts] },
// [ts] isCompositeComponentElement: (inst) => {
// [ts] if (!React.isValidElement(inst)) {
// [ts] return false;
// [ts] }
// We check the prototype of the type that will get mounted, not the
// instance itself. This is a future proof way of duck typing.
// [ts] let prototype = inst.type.prototype;
// [ts] return @typeName(prototype.render) == "function" && @typeName(prototype.setState) == "function";
// [ts] },
// [ts] isCompositeComponentElementWithType: (inst, type) => {
// [ts] let internalInstance = ReactInstanceMap.get(inst);
// [ts] let constructor = internalInstance._currentElement.type;
// [ts] return !!(ReactTestUtils.isCompositeComponentElement(inst) && constructor == type);
// [ts] },
// [ts] getRenderedChildOfCompositeComponent: (inst) => {
// [ts] if (!ReactTestUtils.isCompositeComponent(inst)) {
// [ts] return null;
// [ts] }
// [ts] let internalInstance = ReactInstanceMap.get(inst);
// [ts] return internalInstance._renderedComponent.getPublicInstance();
// [ts] },
// [ts] findAllInRenderedTree: (inst, test) => {
// [ts] if (!inst) {
// [ts] return [];
// [ts] }
// [ts] if (!ReactTestUtils.isCompositeComponent(inst)) if ("development" != "production") invariant(false, "findAllInRenderedTree(...): instance must be a composite component") else _prodInvariant("10") else void 0;
// [ts] return findAllInRenderedTreeInternal(ReactInstanceMap.get(inst), test);
// [ts] },
// [ts] /**
// [ts] * Finds all instance of components in the rendered tree that are DOM
// [ts] * components with the class name matching `className`.
// [ts] * @return {array} an array of all the matches.
// [ts] */
// [ts] scryRenderedDOMComponentsWithClass: (root, classNames) => {
// [ts] return ReactTestUtils.findAllInRenderedTree(root, (inst) => {
// [ts] if (ReactTestUtils.isDOMComponent(inst)) {
// [ts] let className = inst.className;
// [ts] if (@typeName(className) != "string") {
// SVG, probably.
// [ts] className = inst.getAttribute("class") || "";
// [ts] }
// [ts] let classList = className.split("\\s+");
// [ts] if (!Array.isArray(classNames)) {
// [ts] if (!(classNames != null)) if ("development" != "production") invariant(false, "TestUtils.scryRenderedDOMComponentsWithClass expects a className as a second argument.") else _prodInvariant("11") else void 0;
// [ts] classNames = classNames.split("\\s+");
// [ts] }
// [ts] return classNames.every((name) => {
// [ts] return classList.indexOf(name) != -1;
// [ts] });
// [ts] }
// [ts] return false;
// [ts] });
// [ts] },
// [ts] /**
// [ts] * Like scryRenderedDOMComponentsWithClass but expects there to be one result,
// [ts] * and returns that one result, or throws exception if there is any other
// [ts] * number of matches besides one.
// [ts] * @return {!ReactDOMComponent} The one match.
// [ts] */
// [ts] findRenderedDOMComponentWithClass: (root, className) => {
// [ts] let all = ReactTestUtils.scryRenderedDOMComponentsWithClass(root, className);
// [ts] if (all.length != 1) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Did not find exactly one match (found: ' + all.length + ') ' + 'for class:' + className);)
// [ts] }
// [ts] return all[0];
// [ts] },
// [ts] /**
// [ts] * Finds all instance of components in the rendered tree that are DOM
// [ts] * components with the tag name matching `tagName`.
// [ts] * @return {array} an array of all the matches.
// [ts] */
// [ts] scryRenderedDOMComponentsWithTag: (root, tagName) => {
// [ts] return ReactTestUtils.findAllInRenderedTree(root, (inst) => {
// [ts] return ReactTestUtils.isDOMComponent(inst) && inst.tagName.toUpperCase() == tagName.toUpperCase();
// [ts] });
// [ts] },
// [ts] /**
// [ts] * Like scryRenderedDOMComponentsWithTag but expects there to be one result,
// [ts] * and returns that one result, or throws exception if there is any other
// [ts] * number of matches besides one.
// [ts] * @return {!ReactDOMComponent} The one match.
// [ts] */
// [ts] findRenderedDOMComponentWithTag: (root, tagName) => {
// [ts] let all = ReactTestUtils.scryRenderedDOMComponentsWithTag(root, tagName);
// [ts] if (all.length != 1) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Did not find exactly one match (found: ' + all.length + ') ' + 'for tag:' + tagName);)
// [ts] }
// [ts] return all[0];
// [ts] },
// [ts] /**
// [ts] * Finds all instances of components with type equal to `componentType`.
// [ts] * @return {array} an array of all the matches.
// [ts] */
// [ts] scryRenderedComponentsWithType: (root, componentType) => {
// [ts] return ReactTestUtils.findAllInRenderedTree(root, (inst) => {
// [ts] return ReactTestUtils.isCompositeComponentWithType(inst, componentType);
// [ts] });
// [ts] },
// [ts] /**
// [ts] * Same as `scryRenderedComponentsWithType` but expects there to be one result
// [ts] * and returns that one result, or throws exception if there is any other
// [ts] * number of matches besides one.
// [ts] * @return {!ReactComponent} The one match.
// [ts] */
// [ts] findRenderedComponentWithType: (root, componentType) => {
// [ts] let all = ReactTestUtils.scryRenderedComponentsWithType(root, componentType);
// [ts] if (all.length != 1) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('Did not find exactly one match (found: ' + all.length + ') ' + 'for componentType:' + componentType);)
// [ts] }
// [ts] return all[0];
// [ts] },
// [ts] /**
// [ts] * Pass a mocked component module to this method to augment it with
// [ts] * useful methods that allow it to be used as a dummy React component.
// [ts] * Instead of rendering as usual, the component will become a simple
// [ts] * <div> containing any provided children.
// [ts] *
// [ts] * @param {object} module the mock function object exported from a
// [ts] *                        module that defines the component to be mocked
// [ts] * @param {?string} mockTagName optional dummy root tag name to return
// [ts] *                              from render method (overrides
// [ts] *                              module.mockTagName if provided)
// [ts] * @return {object} the ReactTestUtils object (for chaining)
// [ts] */
// [ts] mockComponent: (module, mockTagName) => {
// [ts] mockTagName = mockTagName || module.mockTagName || "div";
// [ts] module.prototype.render.mockImplementation(() => {
// [ts] return React.createElement(mockTagName, null, _.props.children);
// [ts] });
// [ts] return this;
// [ts] },
// [ts] /**
// [ts] * Simulates a top level event being dispatched from a raw event that occurred
// [ts] * on an `Element` node.
// [ts] * @param {Object} topLevelType A type from `EventConstants.topLevelTypes`
// [ts] * @param {!Element} node The dom to simulate an event occurring on.
// [ts] * @param {?Event} fakeNativeEvent Fake native event to use in SyntheticEvent.
// [ts] */
// [ts] simulateNativeEventOnNode: (topLevelType, node, fakeNativeEvent) => {
// [ts] fakeNativeEvent.target = node;
// [ts] fakeNativeEvent.simulated = true;
// [ts] ReactBrowserEventEmitter.ReactEventListener.dispatchEvent(topLevelType, fakeNativeEvent);
// [ts] },
// [ts] /**
// [ts] * Simulates a top level event being dispatched from a raw event that occurred
// [ts] * on the `ReactDOMComponent` `comp`.
// [ts] * @param {Object} topLevelType A type from `EventConstants.topLevelTypes`.
// [ts] * @param {!ReactDOMComponent} comp
// [ts] * @param {?Event} fakeNativeEvent Fake native event to use in SyntheticEvent.
// [ts] */
// [ts] simulateNativeEventOnDOMComponent: (topLevelType, comp, fakeNativeEvent) => {
// [ts] ReactTestUtils.simulateNativeEventOnNode(topLevelType, findDOMNode(comp), fakeNativeEvent);
// [ts] },
// [ts] nativeTouchData: (x, y) => {
// [ts] return {
// [ts] touches: [{ pageX: x, pageY: y }]
// [ts] };
// [ts] },
// [ts] createRenderer: createRendererWithWarning,
// [ts] Simulate: null,
// [ts] SimulateNative: {}
// [ts] };
// [ts] /**
// [ts] * Exports:
// [ts] *
// [ts] * - `ReactTestUtils.Simulate.click(Element/ReactDOMComponent)`
// [ts] * - `ReactTestUtils.Simulate.mouseMove(Element/ReactDOMComponent)`
// [ts] * - `ReactTestUtils.Simulate.change(Element/ReactDOMComponent)`
// [ts] * - ... (All keys from event plugin `eventTypes` objects)
// [ts] */
// [ts] fn makeSimulator(eventType: auto) {
// [ts] return (domComponentOrNode, eventData) => {
// [ts] let node;
// [ts] if (!!React.isValidElement(domComponentOrNode)) if ("development" != "production") invariant(false, "TestUtils.Simulate expects a component instance and not a ReactElement.TestUtils.Simulate will not work if you are using shallow rendering.") else _prodInvariant("14") else void 0;
// [ts] if (ReactTestUtils.isDOMComponent(domComponentOrNode)) {
// [ts] node = findDOMNode(domComponentOrNode);
// [ts] } else if (domComponentOrNode.tagName) {
// [ts] node = domComponentOrNode;
// [ts] }
// [ts] let dispatchConfig = EventPluginRegistry.eventNameDispatchConfigs[eventType];
// [ts] let fakeNativeEvent = Event.init();
// [ts] fakeNativeEvent.target = node;
// [ts] fakeNativeEvent.type = eventType.toLowerCase();
// We don't use SyntheticEvent.getPooled in order to not have to worry about
// properly destroying any properties assigned from `eventData` upon release
// [ts] let event = SyntheticEvent.init(dispatchConfig, ReactDOMComponentTree.getInstanceFromNode(node), fakeNativeEvent, node);
// Since we aren't using pooling, always persist the event. This will make
// sure it's marked and won't warn when setting additional properties.
// [ts] event.persist();
// [ts] _assign(event, eventData);
// [ts] if (dispatchConfig.phasedRegistrationNames) {
// [ts] EventPropagators.accumulateTwoPhaseDispatches(event);
// [ts] } else {
// [ts] EventPropagators.accumulateDirectDispatches(event);
// [ts] }
// [ts] ReactUpdates.batchedUpdates(() => {
// [ts] EventPluginHub.enqueueEvents(event);
// [ts] EventPluginHub.processEventQueue(true);
// [ts] });
// [ts] };
// [ts] }
// [ts] fn buildSimulators() {
// [ts] ReactTestUtils.Simulate = {};
// [ts] let eventType;
// [ts] for (eventType in EventPluginRegistry.eventNameDispatchConfigs) {
// [ts] /**
// [ts] * @param {!Element|ReactDOMComponent} domComponentOrNode
// [ts] * @param {?object} eventData Fake event data to use in SyntheticEvent.
// [ts] */
// [ts] ReactTestUtils.Simulate[eventType] = makeSimulator(eventType);
// [ts] }
// [ts] }
// Rebuild ReactTestUtils.Simulate whenever event plugins are injected
// [ts] let oldInjectEventPluginOrder = EventPluginHub.injection.injectEventPluginOrder;
// [ts] EventPluginHub.injection.injectEventPluginOrder = () => {
// [ts] oldInjectEventPluginOrder.apply(this, arguments);
// [ts] buildSimulators();
// [ts] };
// [ts] let oldInjectEventPlugins = EventPluginHub.injection.injectEventPluginsByName;
// [ts] EventPluginHub.injection.injectEventPluginsByName = () => {
// [ts] oldInjectEventPlugins.apply(this, arguments);
// [ts] buildSimulators();
// [ts] };
// [ts] buildSimulators();
// [ts] /**
// [ts] * Exports:
// [ts] *
// [ts] * - `ReactTestUtils.SimulateNative.click(Element/ReactDOMComponent)`
// [ts] * - `ReactTestUtils.SimulateNative.mouseMove(Element/ReactDOMComponent)`
// [ts] * - `ReactTestUtils.SimulateNative.mouseIn/ReactDOMComponent)`
// [ts] * - `ReactTestUtils.SimulateNative.mouseOut(Element/ReactDOMComponent)`
// [ts] * - ... (All keys from `EventConstants.topLevelTypes`)
// [ts] *
// [ts] * Note: Top level event types are a subset of the entire set of handler types
// [ts] * (which include a broader set of "synthetic" events). For example, onDragDone
// [ts] * is a synthetic event. Except when testing an event plugin or React's event
// [ts] * handling code specifically, you probably want to use ReactTestUtils.Simulate
// [ts] * to dispatch synthetic events.
// [ts] */
// [ts] fn makeNativeSimulator(eventType: auto) {
// [ts] return (domComponentOrNode, nativeEventData) => {
// [ts] let fakeNativeEvent = Event.init(eventType);
// [ts] _assign(fakeNativeEvent, nativeEventData);
// [ts] if (ReactTestUtils.isDOMComponent(domComponentOrNode)) {
// [ts] ReactTestUtils.simulateNativeEventOnDOMComponent(eventType, domComponentOrNode, fakeNativeEvent);
// [ts] } else if (domComponentOrNode.tagName) {
// Will allow on actual dom nodes.
// [ts] ReactTestUtils.simulateNativeEventOnNode(eventType, domComponentOrNode, fakeNativeEvent);
// [ts] }
// [ts] };
// [ts] }
// [ts] Object.keys(topLevelTypes).forEach((eventType) => {
// Event type is stored as 'topClick' - we transform that to 'click'
// [ts] let convenienceName = if (eventType.indexOf("top") == 0) eventType.charAt(3).toLowerCase() + eventType.substr(4) else eventType;
// [ts] /**
// [ts] * @param {!Element|ReactDOMComponent} domComponentOrNode
// [ts] * @param {?Event} nativeEventData Fake native event to use in SyntheticEvent.
// [ts] */
// [ts] ReactTestUtils.SimulateNative[convenienceName] = makeNativeSimulator(eventType);
// [ts] });
// [ts] module.exports = ReactTestUtils;
// [ts] },{"108":108,"126":126,"135":135,"152":152,"159":159,"16":16,"160":160,"17":17,"18":18,"20":20,"26":26,"31":31,"34":34,"63":63,"79":79,"82":82,"91":91}],81:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2015-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let ReactCurrentOwner = _dereq_(134);
// [ts] let ReactInstanceMap = _dereq_(63);
// [ts] let ReactInstrumentation = _dereq_(64);
// [ts] let ReactUpdates = _dereq_(82);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] fn enqueueUpdate(internalInstance: auto) {
// [ts] ReactUpdates.enqueueUpdate(internalInstance);
// [ts] }
// [ts] fn formatUnexpectedArgument(arg: auto) {
// [ts] let type = @typeName(arg);
// [ts] if (type != "object") {
// [ts] return type;
// [ts] }
// [ts] let displayName = arg.constructor && arg.constructor.name || type;
// [ts] let keys = Object.keys(arg);
// [ts] if (keys.length > 0 && keys.length < 20) {
// [ts] return displayName + " (keys: " + keys.join(", ") + ")";
// [ts] }
// [ts] return displayName;
// [ts] }
// [ts] fn getInternalInstanceReadyForUpdate(publicInstance: auto, callerName: auto) {
// [ts] let internalInstance = ReactInstanceMap.get(publicInstance);
// [ts] if (!internalInstance) {
// [ts] if ("development" != "production") {
// [ts] let ctor = publicInstance.constructor;
// Only warn when we have a callerName. Otherwise we should be silent.
// We're probably calling from enqueueCallback. We don't want to warn
// there because we already warned for the corresponding lifecycle method.
// [ts] if ("development" != "production") warning(!callerName, "%s(...): Can only update a mounted or mounting component. " + "This usually means you called %s() on an unmounted component. " + "This is a no-op. Please check the code for the %s component.", callerName, callerName, ctor && (ctor.displayName || ctor.name) || "ReactClass") else void 0;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(ReactCurrentOwner.current == null, "%s(...): Cannot update during an existing state transition (such as " + "within `render` or another component's constructor). Render methods " + "should be a pure function of props and state; constructor " + "side-effects are an anti-pattern, but can be moved to " + "`componentWillMount`.", callerName) else void 0;
// [ts] }
// [ts] return internalInstance;
// [ts] }
// [ts] /**
// [ts] * ReactUpdateQueue allows for state updates to be scheduled into a later
// [ts] * reconciliation step.
// [ts] */
// [ts] let ReactUpdateQueue = {
// [ts] /**
// [ts] * Checks whether or not this composite component is mounted.
// [ts] * @param {ReactClass} publicInstance The instance we want to test.
// [ts] * @return {boolean} True if mounted, false otherwise.
// [ts] * @protected
// [ts] * @final
// [ts] */
// [ts] isMounted: (publicInstance) => {
// [ts] if ("development" != "production") {
// [ts] let owner = ReactCurrentOwner.current;
// [ts] if (owner != null) {
// [ts] if ("development" != "production") warning(owner._warnedAboutRefsInRender, "%s is accessing isMounted inside its render() function. " + "render() should be a pure function of props and state. It should " + "never access something that requires stale data from the previous " + "render, such as refs. Move this logic to componentDidMount and " + "componentDidUpdate instead.", owner.getName() || "A component") else void 0;
// [ts] owner._warnedAboutRefsInRender = true;
// [ts] }
// [ts] }
// [ts] let internalInstance = ReactInstanceMap.get(publicInstance);
// [ts] if (internalInstance) {
// During componentWillMount and render this will still be null but after
// that will always render to something. At least for now. So we can use
// this hack.
// [ts] return !!internalInstance._renderedComponent;
// [ts] } else {
// [ts] return false;
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Enqueue a callback that will be executed after all the pending updates
// [ts] * have processed.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance to use as `this` context.
// [ts] * @param {?function} callback Called after state is updated.
// [ts] * @param {string} callerName Name of the calling function in the public API.
// [ts] * @internal
// [ts] */
// [ts] enqueueCallback: (publicInstance, callback, callerName) => {
// [ts] ReactUpdateQueue.validateCallback(callback, callerName);
// [ts] let internalInstance = getInternalInstanceReadyForUpdate(publicInstance);
// Previously we would throw an error if we didn't have an internal
// instance. Since we want to make it a no-op instead, we mirror the same
// behavior we have in other enqueue* methods.
// We also need to ignore callbacks in componentWillMount. See
// enqueueUpdates.
// [ts] if (!internalInstance) {
// [ts] return null;
// [ts] }
// [ts] if (internalInstance._pendingCallbacks) {
// [ts] internalInstance._pendingCallbacks.push(callback);
// [ts] } else {
// [ts] internalInstance._pendingCallbacks = [callback];
// [ts] }
// TODO: The callback here is ignored when setState is called from
// componentWillMount. Either fix it or disallow doing so completely in
// favor of getInitialState. Alternatively, we can disallow
// componentWillMount during server-side rendering.
// [ts] enqueueUpdate(internalInstance);
// [ts] },
// [ts] enqueueCallbackInternal: (internalInstance, callback) => {
// [ts] if (internalInstance._pendingCallbacks) {
// [ts] internalInstance._pendingCallbacks.push(callback);
// [ts] } else {
// [ts] internalInstance._pendingCallbacks = [callback];
// [ts] }
// [ts] enqueueUpdate(internalInstance);
// [ts] },
// [ts] /**
// [ts] * Forces an update. This should only be invoked when it is known with
// [ts] * certainty that we are **not** in a DOM transaction.
// [ts] *
// [ts] * You may want to call this when you know that some deeper aspect of the
// [ts] * component's state has changed but `setState` was not called.
// [ts] *
// [ts] * This will not invoke `shouldComponentUpdate`, but it will invoke
// [ts] * `componentWillUpdate` and `componentDidUpdate`.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @internal
// [ts] */
// [ts] enqueueForceUpdate: (publicInstance) => {
// [ts] let internalInstance = getInternalInstanceReadyForUpdate(publicInstance, "forceUpdate");
// [ts] if (!internalInstance) {
// [ts] return;
// [ts] }
// [ts] internalInstance._pendingForceUpdate = true;
// [ts] enqueueUpdate(internalInstance);
// [ts] },
// [ts] /**
// [ts] * Replaces all of the state. Always use this or `setState` to mutate state.
// [ts] * You should treat `this.state` as immutable.
// [ts] *
// [ts] * There is no guarantee that `this.state` will be immediately updated, so
// [ts] * accessing `this.state` after calling this method may return the old value.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @param {object} completeState Next state.
// [ts] * @internal
// [ts] */
// [ts] enqueueReplaceState: (publicInstance, completeState, callback) => {
// [ts] let internalInstance = getInternalInstanceReadyForUpdate(publicInstance, "replaceState");
// [ts] if (!internalInstance) {
// [ts] return;
// [ts] }
// [ts] internalInstance._pendingStateQueue = [completeState];
// [ts] internalInstance._pendingReplaceState = true;
// Future-proof 15.5
// [ts] if (callback != null && callback != null) {
// [ts] ReactUpdateQueue.validateCallback(callback, "replaceState");
// [ts] if (internalInstance._pendingCallbacks) {
// [ts] internalInstance._pendingCallbacks.push(callback);
// [ts] } else {
// [ts] internalInstance._pendingCallbacks = [callback];
// [ts] }
// [ts] }
// [ts] enqueueUpdate(internalInstance);
// [ts] },
// [ts] /**
// [ts] * Sets a subset of the state. This only exists because _pendingState is
// [ts] * internal. This provides a merging strategy that is not available to deep
// [ts] * properties which is confusing. TODO: Expose pendingState or don't use it
// [ts] * during the merge.
// [ts] *
// [ts] * @param {ReactClass} publicInstance The instance that should rerender.
// [ts] * @param {object} partialState Next partial state to be merged with state.
// [ts] * @internal
// [ts] */
// [ts] enqueueSetState: (publicInstance, partialState) => {
// [ts] if ("development" != "production") {
// [ts] ReactInstrumentation.debugTool.onSetState();
// [ts] if ("development" != "production") warning(partialState != null, "setState(...): You passed an undefined or null state object; " + "instead, use forceUpdate().") else void 0;
// [ts] }
// [ts] let internalInstance = getInternalInstanceReadyForUpdate(publicInstance, "setState");
// [ts] if (!internalInstance) {
// [ts] return;
// [ts] }
// [ts] let queue = internalInstance._pendingStateQueue || (internalInstance._pendingStateQueue = []);
// [ts] queue.push(partialState);
// [ts] enqueueUpdate(internalInstance);
// [ts] },
// [ts] enqueueElementInternal: (internalInstance, nextElement, nextContext) => {
// [ts] internalInstance._pendingElement = nextElement;
// TODO: introduce _pendingContext instead of setting it directly.
// [ts] internalInstance._context = nextContext;
// [ts] enqueueUpdate(internalInstance);
// [ts] },
// [ts] validateCallback: (callback, callerName) => {
// [ts] if (!(!callback || @typeName(callback) == "function")) if ("development" != "production") invariant(false, "%s(...): Expected the last optional `callback` argument to be a function. Instead received: %s.", callerName, formatUnexpectedArgument(callback)) else _prodInvariant("122", callerName, formatUnexpectedArgument(callback)) else void 0;
// [ts] }
// [ts] };
// [ts] module.exports = ReactUpdateQueue;
// [ts] },{"126":126,"134":134,"152":152,"159":159,"63":63,"64":64,"82":82}],82:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] let CallbackQueue = _dereq_(6);
// [ts] let PooledClass = _dereq_(25);
// [ts] let ReactFeatureFlags = _dereq_(58);
// [ts] let ReactReconciler = _dereq_(75);
// [ts] let Transaction = _dereq_(100);
// [ts] let invariant = _dereq_(152);
// [ts] let dirtyComponents = [];
// [ts] let updateBatchNumber = 0;
// [ts] let asapCallbackQueue = CallbackQueue.getPooled();
// [ts] let asapEnqueued = false;
// [ts] let batchingStrategy = null;
// [ts] fn ensureInjected() {
// [ts] if (!(ReactUpdates.ReactReconcileTransaction && batchingStrategy)) if ("development" != "production") invariant(false, "ReactUpdates: must inject a reconcile transaction class and batching strategy") else _prodInvariant("123") else void 0;
// [ts] }
// [ts] let NESTED_UPDATES = {
// [ts] initialize: () => {
// [ts] _.dirtyComponentsLength = dirtyComponents.length;
// [ts] },
// [ts] close: () => {
// [ts] if (_.dirtyComponentsLength != dirtyComponents.length) {
// Additional updates were enqueued by componentDidUpdate handlers or
// similar; before our own UPDATE_QUEUEING wrapper closes, we want to run
// these new updates so that if A's componentDidUpdate calls setState on
// B, B will update before the callback A's updater provided when calling
// setState.
// [ts] dirtyComponents.splice(0, _.dirtyComponentsLength);
// [ts] flushBatchedUpdates();
// [ts] } else {
// [ts] dirtyComponents.length = 0;
// [ts] }
// [ts] }
// [ts] };
// [ts] let UPDATE_QUEUEING = {
// [ts] initialize: () => {
// [ts] _.callbackQueue.reset();
// [ts] },
// [ts] close: () => {
// [ts] _.callbackQueue.notifyAll();
// [ts] }
// [ts] };
// [ts] let TRANSACTION_WRAPPERS = [NESTED_UPDATES, UPDATE_QUEUEING];
// [ts] fn ReactUpdatesFlushTransaction() {
// [ts] _.reinitializeTransaction();
// [ts] _.dirtyComponentsLength = null;
// [ts] _.callbackQueue = CallbackQueue.getPooled();
// [ts] _.reconcileTransaction = ReactUpdates.ReactReconcileTransaction.getPooled(
// [ts] /* useCreateElement */true);
// [ts] }
// [ts] _assign(ReactUpdatesFlushTransaction.prototype, Transaction, {
// [ts] getTransactionWrappers: () => {
// [ts] return TRANSACTION_WRAPPERS;
// [ts] },
// [ts] destructor: () => {
// [ts] _.dirtyComponentsLength = null;
// [ts] CallbackQueue.release(_.callbackQueue);
// [ts] _.callbackQueue = null;
// [ts] ReactUpdates.ReactReconcileTransaction.release(_.reconcileTransaction);
// [ts] _.reconcileTransaction = null;
// [ts] },
// [ts] perform: (method, scope, a) => {
// Essentially calls `this.reconcileTransaction.perform(method, scope, a)`
// with this transaction's wrappers around it.
// [ts] return Transaction.perform.call(this, _.reconcileTransaction.perform, _.reconcileTransaction, method, scope, a);
// [ts] }
// [ts] });
// [ts] PooledClass.addPoolingTo(ReactUpdatesFlushTransaction);
// [ts] fn batchedUpdates(callback: auto, a: auto, b: auto, c: auto, d: auto, e: auto) {
// [ts] ensureInjected();
// [ts] return batchingStrategy.batchedUpdates(callback, a, b, c, d, e);
// [ts] }
// [ts] /**
// [ts] * Array comparator for ReactComponents by mount ordering.
// [ts] *
// [ts] * @param {ReactComponent} c1 first component you're comparing
// [ts] * @param {ReactComponent} c2 second component you're comparing
// [ts] * @return {number} Return value usable by Array.prototype.sort().
// [ts] */
// [ts] fn mountOrderComparator(c1: auto, c2: auto) {
// [ts] return c1._mountOrder - c2._mountOrder;
// [ts] }
// [ts] fn runBatchedUpdates(transaction: auto) {
// [ts] let len = transaction.dirtyComponentsLength;
// [ts] if (!(len == dirtyComponents.length)) if ("development" != "production") invariant(false, "Expected flush transaction's stored dirty-components length (%s) to match dirty-components array length (%s).", len, dirtyComponents.length) else _prodInvariant("124", len, dirtyComponents.length) else void 0;
// Since reconciling a component higher in the owner hierarchy usually (not
// always -- see shouldComponentUpdate()) will reconcile children, reconcile
// them before their children by sorting the array.
// [ts] dirtyComponents.sort(mountOrderComparator);
// Any updates enqueued while reconciling must be performed after this entire
// batch. Otherwise, if dirtyComponents is [A, B] where A has children B and
// C, B could update twice in a single batch if C's render enqueues an update
// to B (since B would have already updated, we should skip it, and the only
// way we can know to do so is by checking the batch counter).
// [ts] updateBatchNumber++;
// [ts] let i = 0;
// [ts] while (i < len) { // [cap-migrate] for-step: i++
// If a component is unmounted before pending changes apply, it will still
// be here, but we assume that it has cleared its _pendingCallbacks and
// that performUpdateIfNecessary is a noop.
// [ts] let component = dirtyComponents[i];
// If performUpdateIfNecessary happens to enqueue any new updates, we
// shouldn't execute the callbacks until the next render happens, so
// stash the callbacks first
// [ts] let callbacks = component._pendingCallbacks;
// [ts] component._pendingCallbacks = null;
// [ts] let markerName;
// [ts] if (ReactFeatureFlags.logTopLevelRenders) {
// [ts] let namedComponent = component;
// Duck type TopLevelWrapper. This is probably always true.
// [ts] if (component._currentElement.type.isReactTopLevelWrapper) {
// [ts] namedComponent = component._renderedComponent;
// [ts] }
// [ts] markerName = "React update: " + namedComponent.getName();
// [ts] console.time(markerName);
// [ts] }
// [ts] ReactReconciler.performUpdateIfNecessary(component, transaction.reconcileTransaction, updateBatchNumber);
// [ts] if (markerName) {
// [ts] console.timeEnd(markerName);
// [ts] }
// [ts] if (callbacks) {
// [ts] let j = 0;
// [ts] while (j < callbacks.length) { // [cap-migrate] for-step: j++
// [ts] transaction.callbackQueue.enqueue(callbacks[j], component.getPublicInstance());
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] let flushBatchedUpdates = () => {
// ReactUpdatesFlushTransaction's wrappers will clear the dirtyComponents
// array and perform any updates enqueued by mount-ready handlers (i.e.,
// componentDidUpdate) but we need to check here too in order to catch
// updates enqueued by setState callbacks and asap calls.
// [ts] while (dirtyComponents.length || asapEnqueued) {
// [ts] if (dirtyComponents.length) {
// [ts] let transaction = ReactUpdatesFlushTransaction.getPooled();
// [ts] transaction.perform(runBatchedUpdates, null, transaction);
// [ts] ReactUpdatesFlushTransaction.release(transaction);
// [ts] }
// [ts] if (asapEnqueued) {
// [ts] asapEnqueued = false;
// [ts] let queue = asapCallbackQueue;
// [ts] asapCallbackQueue = CallbackQueue.getPooled();
// [ts] queue.notifyAll();
// [ts] CallbackQueue.release(queue);
// [ts] }
// [ts] }
// [ts] };
// [ts] /**
// [ts] * Mark a component as needing a rerender, adding an optional callback to a
// [ts] * list of functions which will be executed once the rerender occurs.
// [ts] */
// [ts] fn enqueueUpdate(component: auto) {
// [ts] ensureInjected();
// Various parts of our code (such as ReactCompositeComponent's
// _renderValidatedComponent) assume that calls to render aren't nested;
// verify that that's the case. (This is called by each top-level update
// function, like setState, forceUpdate, etc.; creation and
// destruction of top-level components is guarded in ReactMount.)
// [ts] if (!batchingStrategy.isBatchingUpdates) {
// [ts] batchingStrategy.batchedUpdates(enqueueUpdate, component);
// [ts] return;
// [ts] }
// [ts] dirtyComponents.push(component);
// [ts] if (component._updateBatchNumber == null) {
// [ts] component._updateBatchNumber = updateBatchNumber + 1;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Enqueue a callback to be run at the end of the current batching cycle. Throws
// [ts] * if no updates are currently being performed.
// [ts] */
// [ts] fn asap(callback: auto, context: auto) {
// [ts] invariant(batchingStrategy.isBatchingUpdates, "ReactUpdates.asap: Can't enqueue an asap callback in a context where" + "updates are not being batched.");
// [ts] asapCallbackQueue.enqueue(callback, context);
// [ts] asapEnqueued = true;
// [ts] }
// [ts] let ReactUpdatesInjection = {
// [ts] injectReconcileTransaction: (ReconcileTransaction) => {
// [ts] if (!ReconcileTransaction) if ("development" != "production") invariant(false, "ReactUpdates: must provide a reconcile transaction class") else _prodInvariant("126") else void 0;
// [ts] ReactUpdates.ReactReconcileTransaction = ReconcileTransaction;
// [ts] },
// [ts] injectBatchingStrategy: (_batchingStrategy) => {
// [ts] if (!_batchingStrategy) if ("development" != "production") invariant(false, "ReactUpdates: must provide a batching strategy") else _prodInvariant("127") else void 0;
// [ts] if (!(@typeName(_batchingStrategy.batchedUpdates) == "function")) if ("development" != "production") invariant(false, "ReactUpdates: must provide a batchedUpdates() function") else _prodInvariant("128") else void 0;
// [ts] if (!(@typeName(_batchingStrategy.isBatchingUpdates) == "boolean")) if ("development" != "production") invariant(false, "ReactUpdates: must provide an isBatchingUpdates boolean attribute") else _prodInvariant("129") else void 0;
// [ts] batchingStrategy = _batchingStrategy;
// [ts] }
// [ts] };
// [ts] let ReactUpdates = {
// [ts] /**
// [ts] * React references `ReactReconcileTransaction` using this property in order
// [ts] * to allow dependency injection.
// [ts] *
// [ts] * @internal
// [ts] */
// [ts] ReactReconcileTransaction: null,
// [ts] batchedUpdates: batchedUpdates,
// [ts] enqueueUpdate: enqueueUpdate,
// [ts] flushBatchedUpdates: flushBatchedUpdates,
// [ts] injection: ReactUpdatesInjection,
// [ts] asap: asap
// [ts] };
// [ts] module.exports = ReactUpdates;
// [ts] },{"100":100,"126":126,"152":152,"160":160,"25":25,"58":58,"6":6,"75":75}],83:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] module.exports = "15.7.0";
// [ts] },{}],84:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let NS = {
// [ts] xlink: "http://www.w3.org/1999/xlink",
// [ts] xml: "http://www.w3.org/XML/1998/namespace"
// [ts] };
// We use attributes for everything SVG so let's avoid some duplication and run
// code instead.
// The following are all specified in the HTML config already so we exclude here.
// - class (as className)
// - color
// - height
// - id
// - lang
// - max
// - media
// - method
// - min
// - name
// - style
// - target
// - type
// - width
// [ts] let ATTRS = {
// [ts] accentHeight: "accent-height",
// [ts] accumulate: 0,
// [ts] additive: 0,
// [ts] alignmentBaseline: "alignment-baseline",
// [ts] allowReorder: "allowReorder",
// [ts] alphabetic: 0,
// [ts] amplitude: 0,
// [ts] arabicForm: "arabic-form",
// [ts] ascent: 0,
// [ts] attributeName: "attributeName",
// [ts] attributeType: "attributeType",
// [ts] autoReverse: "autoReverse",
// [ts] azimuth: 0,
// [ts] baseFrequency: "baseFrequency",
// [ts] baseProfile: "baseProfile",
// [ts] baselineShift: "baseline-shift",
// [ts] bbox: 0,
// [ts] begin: 0,
// [ts] bias: 0,
// [ts] by: 0,
// [ts] calcMode: "calcMode",
// [ts] capHeight: "cap-height",
// [ts] clip: 0,
// [ts] clipPath: "clip-path",
// [ts] clipRule: "clip-rule",
// [ts] clipPathUnits: "clipPathUnits",
// [ts] colorInterpolation: "color-interpolation",
// [ts] colorInterpolationFilters: "color-interpolation-filters",
// [ts] colorProfile: "color-profile",
// [ts] colorRendering: "color-rendering",
// [ts] contentScriptType: "contentScriptType",
// [ts] contentStyleType: "contentStyleType",
// [ts] cursor: 0,
// [ts] cx: 0,
// [ts] cy: 0,
// [ts] d: 0,
// [ts] decelerate: 0,
// [ts] descent: 0,
// [ts] diffuseConstant: "diffuseConstant",
// [ts] direction: 0,
// [ts] display: 0,
// [ts] divisor: 0,
// [ts] dominantBaseline: "dominant-baseline",
// [ts] dur: 0,
// [ts] dx: 0,
// [ts] dy: 0,
// [ts] edgeMode: "edgeMode",
// [ts] elevation: 0,
// [ts] enableBackground: "enable-background",
// [ts] end: 0,
// [ts] exponent: 0,
// [ts] externalResourcesRequired: "externalResourcesRequired",
// [ts] fill: 0,
// [ts] fillOpacity: "fill-opacity",
// [ts] fillRule: "fill-rule",
// [ts] filter: 0,
// [ts] filterRes: "filterRes",
// [ts] filterUnits: "filterUnits",
// [ts] floodColor: "flood-color",
// [ts] floodOpacity: "flood-opacity",
// [ts] focusable: 0,
// [ts] fontFamily: "font-family",
// [ts] fontSize: "font-size",
// [ts] fontSizeAdjust: "font-size-adjust",
// [ts] fontStretch: "font-stretch",
// [ts] fontStyle: "font-style",
// [ts] fontVariant: "font-variant",
// [ts] fontWeight: "font-weight",
// [ts] format: 0,
// [ts] from: 0,
// [ts] fx: 0,
// [ts] fy: 0,
// [ts] g1: 0,
// [ts] g2: 0,
// [ts] glyphName: "glyph-name",
// [ts] glyphOrientationHorizontal: "glyph-orientation-horizontal",
// [ts] glyphOrientationVertical: "glyph-orientation-vertical",
// [ts] glyphRef: "glyphRef",
// [ts] gradientTransform: "gradientTransform",
// [ts] gradientUnits: "gradientUnits",
// [ts] hanging: 0,
// [ts] horizAdvX: "horiz-adv-x",
// [ts] horizOriginX: "horiz-origin-x",
// [ts] ideographic: 0,
// [ts] imageRendering: "image-rendering",
// [ts] "in": 0,
// [ts] in2: 0,
// [ts] intercept: 0,
// [ts] k: 0,
// [ts] k1: 0,
// [ts] k2: 0,
// [ts] k3: 0,
// [ts] k4: 0,
// [ts] kernelMatrix: "kernelMatrix",
// [ts] kernelUnitLength: "kernelUnitLength",
// [ts] kerning: 0,
// [ts] keyPoints: "keyPoints",
// [ts] keySplines: "keySplines",
// [ts] keyTimes: "keyTimes",
// [ts] lengthAdjust: "lengthAdjust",
// [ts] letterSpacing: "letter-spacing",
// [ts] lightingColor: "lighting-color",
// [ts] limitingConeAngle: "limitingConeAngle",
// [ts] local: 0,
// [ts] markerEnd: "marker-end",
// [ts] markerMid: "marker-mid",
// [ts] markerStart: "marker-start",
// [ts] markerHeight: "markerHeight",
// [ts] markerUnits: "markerUnits",
// [ts] markerWidth: "markerWidth",
// [ts] mask: 0,
// [ts] maskContentUnits: "maskContentUnits",
// [ts] maskUnits: "maskUnits",
// [ts] mathematical: 0,
// [ts] mode: 0,
// [ts] numOctaves: "numOctaves",
// [ts] offset: 0,
// [ts] opacity: 0,
// [ts] operator: 0,
// [ts] order: 0,
// [ts] orient: 0,
// [ts] orientation: 0,
// [ts] origin: 0,
// [ts] overflow: 0,
// [ts] overlinePosition: "overline-position",
// [ts] overlineThickness: "overline-thickness",
// [ts] paintOrder: "paint-order",
// [ts] panose1: "panose-1",
// [ts] pathLength: "pathLength",
// [ts] patternContentUnits: "patternContentUnits",
// [ts] patternTransform: "patternTransform",
// [ts] patternUnits: "patternUnits",
// [ts] pointerEvents: "pointer-events",
// [ts] points: 0,
// [ts] pointsAtX: "pointsAtX",
// [ts] pointsAtY: "pointsAtY",
// [ts] pointsAtZ: "pointsAtZ",
// [ts] preserveAlpha: "preserveAlpha",
// [ts] preserveAspectRatio: "preserveAspectRatio",
// [ts] primitiveUnits: "primitiveUnits",
// [ts] r: 0,
// [ts] radius: 0,
// [ts] refX: "refX",
// [ts] refY: "refY",
// [ts] renderingIntent: "rendering-intent",
// [ts] repeatCount: "repeatCount",
// [ts] repeatDur: "repeatDur",
// [ts] requiredExtensions: "requiredExtensions",
// [ts] requiredFeatures: "requiredFeatures",
// [ts] restart: 0,
// [ts] result: 0,
// [ts] rotate: 0,
// [ts] rx: 0,
// [ts] ry: 0,
// [ts] scale: 0,
// [ts] seed: 0,
// [ts] shapeRendering: "shape-rendering",
// [ts] slope: 0,
// [ts] spacing: 0,
// [ts] specularConstant: "specularConstant",
// [ts] specularExponent: "specularExponent",
// [ts] speed: 0,
// [ts] spreadMethod: "spreadMethod",
// [ts] startOffset: "startOffset",
// [ts] stdDeviation: "stdDeviation",
// [ts] stemh: 0,
// [ts] stemv: 0,
// [ts] stitchTiles: "stitchTiles",
// [ts] stopColor: "stop-color",
// [ts] stopOpacity: "stop-opacity",
// [ts] strikethroughPosition: "strikethrough-position",
// [ts] strikethroughThickness: "strikethrough-thickness",
// [ts] string: 0,
// [ts] stroke: 0,
// [ts] strokeDasharray: "stroke-dasharray",
// [ts] strokeDashoffset: "stroke-dashoffset",
// [ts] strokeLinecap: "stroke-linecap",
// [ts] strokeLinejoin: "stroke-linejoin",
// [ts] strokeMiterlimit: "stroke-miterlimit",
// [ts] strokeOpacity: "stroke-opacity",
// [ts] strokeWidth: "stroke-width",
// [ts] surfaceScale: "surfaceScale",
// [ts] systemLanguage: "systemLanguage",
// [ts] tableValues: "tableValues",
// [ts] targetX: "targetX",
// [ts] targetY: "targetY",
// [ts] textAnchor: "text-anchor",
// [ts] textDecoration: "text-decoration",
// [ts] textRendering: "text-rendering",
// [ts] textLength: "textLength",
// [ts] to: 0,
// [ts] transform: 0,
// [ts] u1: 0,
// [ts] u2: 0,
// [ts] underlinePosition: "underline-position",
// [ts] underlineThickness: "underline-thickness",
// [ts] unicode: 0,
// [ts] unicodeBidi: "unicode-bidi",
// [ts] unicodeRange: "unicode-range",
// [ts] unitsPerEm: "units-per-em",
// [ts] vAlphabetic: "v-alphabetic",
// [ts] vHanging: "v-hanging",
// [ts] vIdeographic: "v-ideographic",
// [ts] vMathematical: "v-mathematical",
// [ts] values: 0,
// [ts] vectorEffect: "vector-effect",
// [ts] version: 0,
// [ts] vertAdvY: "vert-adv-y",
// [ts] vertOriginX: "vert-origin-x",
// [ts] vertOriginY: "vert-origin-y",
// [ts] viewBox: "viewBox",
// [ts] viewTarget: "viewTarget",
// [ts] visibility: 0,
// [ts] widths: 0,
// [ts] wordSpacing: "word-spacing",
// [ts] writingMode: "writing-mode",
// [ts] x: 0,
// [ts] xHeight: "x-height",
// [ts] x1: 0,
// [ts] x2: 0,
// [ts] xChannelSelector: "xChannelSelector",
// [ts] xlinkActuate: "xlink:actuate",
// [ts] xlinkArcrole: "xlink:arcrole",
// [ts] xlinkHref: "xlink:href",
// [ts] xlinkRole: "xlink:role",
// [ts] xlinkShow: "xlink:show",
// [ts] xlinkTitle: "xlink:title",
// [ts] xlinkType: "xlink:type",
// [ts] xmlBase: "xml:base",
// [ts] xmlns: 0,
// [ts] xmlnsXlink: "xmlns:xlink",
// [ts] xmlLang: "xml:lang",
// [ts] xmlSpace: "xml:space",
// [ts] y: 0,
// [ts] y1: 0,
// [ts] y2: 0,
// [ts] yChannelSelector: "yChannelSelector",
// [ts] z: 0,
// [ts] zoomAndPan: "zoomAndPan"
// [ts] };
// [ts] let SVGDOMPropertyConfig = {
// [ts] Properties: {},
// [ts] DOMAttributeNamespaces: {
// [ts] xlinkActuate: NS.xlink,
// [ts] xlinkArcrole: NS.xlink,
// [ts] xlinkHref: NS.xlink,
// [ts] xlinkRole: NS.xlink,
// [ts] xlinkShow: NS.xlink,
// [ts] xlinkTitle: NS.xlink,
// [ts] xlinkType: NS.xlink,
// [ts] xmlBase: NS.xml,
// [ts] xmlLang: NS.xml,
// [ts] xmlSpace: NS.xml
// [ts] },
// [ts] DOMAttributeNames: {}
// [ts] };
// [ts] Object.keys(ATTRS).forEach((key) => {
// [ts] SVGDOMPropertyConfig.Properties[key] = 0;
// [ts] if (ATTRS[key]) {
// [ts] SVGDOMPropertyConfig.DOMAttributeNames[key] = ATTRS[key];
// [ts] }
// [ts] });
// [ts] module.exports = SVGDOMPropertyConfig;
// [ts] },{}],85:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let EventPropagators = _dereq_(20);
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactInputSelection = _dereq_(62);
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] let getActiveElement = _dereq_(147);
// [ts] let isTextInputElement = _dereq_(123);
// [ts] let shallowEqual = _dereq_(158);
// [ts] let skipSelectionChangeEvent = ExecutionEnvironment.canUseDOM && "documentMode" == document && document.documentMode <= 11;
// [ts] let eventTypes = {
// [ts] select: {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: "onSelect",
// [ts] captured: "onSelectCapture"
// [ts] },
// [ts] dependencies: ["topBlur", "topContextMenu", "topFocus", "topKeyDown", "topKeyUp", "topMouseDown", "topMouseUp", "topSelectionChange"]
// [ts] }
// [ts] };
// [ts] let activeElement = null;
// [ts] let activeElementInst = null;
// [ts] let lastSelection = null;
// [ts] let mouseDown = false;
// Track whether a listener exists for this plugin. If none exist, we do
// not extract events. See #3639.
// [ts] let hasListener = false;
// [ts] /**
// [ts] * Get an object which is a unique representation of the current selection.
// [ts] *
// [ts] * The return value will not be consistent across nodes or browsers, but
// [ts] * two identical selections on the same node will return identical objects.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @return {object}
// [ts] */
// [ts] fn getSelection(node: auto) {
// [ts] if ("selectionStart" == node && ReactInputSelection.hasSelectionCapabilities(node)) {
// [ts] return {
// [ts] start: node.selectionStart,
// [ts] end: node.selectionEnd
// [ts] };
// [ts] } else if (window.getSelection) {
// [ts] let selection = window.getSelection();
// [ts] return {
// [ts] anchorNode: selection.anchorNode,
// [ts] anchorOffset: selection.anchorOffset,
// [ts] focusNode: selection.focusNode,
// [ts] focusOffset: selection.focusOffset
// [ts] };
// [ts] } else if (document.selection) {
// [ts] let range = document.selection.createRange();
// [ts] return {
// [ts] parentElement: range.parentElement(),
// [ts] text: range.text,
// [ts] top: range.boundingTop,
// [ts] left: range.boundingLeft
// [ts] };
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Poll selection to see whether it's changed.
// [ts] *
// [ts] * @param {object} nativeEvent
// [ts] * @return {?SyntheticEvent}
// [ts] */
// [ts] fn constructSelectEvent(nativeEvent: auto, nativeEventTarget: auto) {
// Ensure we have the right element, and that the user is not dragging a
// selection (this matches native `select` event behavior). In HTML5, select
// fires only on input and textarea thus if there's no focused element we
// won't dispatch.
// [ts] if (mouseDown || activeElement == null || activeElement != getActiveElement()) {
// [ts] return null;
// [ts] }
// Only fire when selection has actually changed.
// [ts] let currentSelection = getSelection(activeElement);
// [ts] if (!lastSelection || !shallowEqual(lastSelection, currentSelection)) {
// [ts] lastSelection = currentSelection;
// [ts] let syntheticEvent = SyntheticEvent.getPooled(eventTypes.select, activeElementInst, nativeEvent, nativeEventTarget);
// [ts] syntheticEvent.type = "select";
// [ts] syntheticEvent.target = activeElement;
// [ts] EventPropagators.accumulateTwoPhaseDispatches(syntheticEvent);
// [ts] return syntheticEvent;
// [ts] }
// [ts] return null;
// [ts] }
// [ts] /**
// [ts] * This plugin creates an `onSelect` event that normalizes select events
// [ts] * across form elements.
// [ts] *
// [ts] * Supported elements are:
// [ts] * - input (see `isTextInputElement`)
// [ts] * - textarea
// [ts] * - contentEditable
// [ts] *
// [ts] * This differs from native browser implementations in the following ways:
// [ts] * - Fires on contentEditable fields as well as inputs.
// [ts] * - Fires for collapsed selection.
// [ts] * - Fires after user input.
// [ts] */
// [ts] let SelectEventPlugin = {
// [ts] eventTypes: eventTypes,
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget) => {
// [ts] if (!hasListener) {
// [ts] return null;
// [ts] }
// [ts] let targetNode = if (targetInst) ReactDOMComponentTree.getNodeFromInstance(targetInst) else window;
// [ts] switch (topLevelType) {
// Track the input node that has focus.
// [ts] "topFocus" => {
// [ts] if (isTextInputElement(targetNode) || targetNode.contentEditable == "true") {
// [ts] activeElement = targetNode;
// [ts] activeElementInst = targetInst;
// [ts] lastSelection = null;
// [ts] }
// break;
// [ts] },
// [ts] "topBlur" => {
// [ts] activeElement = null;
// [ts] activeElementInst = null;
// [ts] lastSelection = null;
// break;
// Don't fire the event while the user is dragging. This matches the
// semantics of the native select event.
// [ts] },
// [ts] "topMouseDown" => {
// [ts] mouseDown = true;
// break;
// [ts] },
// [ts] "topContextMenu", "topMouseUp" => {
// [ts] mouseDown = false;
// [ts] return constructSelectEvent(nativeEvent, nativeEventTarget);
// Chrome and IE fire non-standard event when selection is changed (and
// sometimes when it hasn't). IE's event fires out of order with respect
// to key and input events on deletion, so we discard it.
//
// Firefox doesn't support selectionchange, so check selection status
// after each key entry. The selection changes after keydown and before
// keyup, but we check on keydown as well in the case of holding down a
// key, when multiple keydown events are fired but only one keyup is.
// This is also our approach for IE handling, for the reason above.
// [ts] },
// [ts] "topSelectionChange" => {
// [ts] if (skipSelectionChangeEvent) {
// [ts] break;
// [ts] }
// falls through
// [ts] },
// [ts] "topKeyDown", "topKeyUp" => {
// [ts] return constructSelectEvent(nativeEvent, nativeEventTarget);
// [ts] }
// [ts] }
// [ts] return null;
// [ts] },
// [ts] didPutListener: (inst, registrationName, listener) => {
// [ts] if (registrationName == "onSelect") {
// [ts] hasListener = true;
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = SelectEventPlugin;
// [ts] },{"123":123,"138":138,"147":147,"158":158,"20":20,"34":34,"62":62,"91":91}],86:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let EventListener = _dereq_(137);
// [ts] let EventPropagators = _dereq_(20);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let SyntheticAnimationEvent = _dereq_(87);
// [ts] let SyntheticClipboardEvent = _dereq_(88);
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] let SyntheticFocusEvent = _dereq_(92);
// [ts] let SyntheticKeyboardEvent = _dereq_(94);
// [ts] let SyntheticMouseEvent = _dereq_(95);
// [ts] let SyntheticDragEvent = _dereq_(90);
// [ts] let SyntheticTouchEvent = _dereq_(96);
// [ts] let SyntheticTransitionEvent = _dereq_(97);
// [ts] let SyntheticUIEvent = _dereq_(98);
// [ts] let SyntheticWheelEvent = _dereq_(99);
// [ts] let emptyFunction = _dereq_(144);
// [ts] let getEventCharCode = _dereq_(111);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Turns
// [ts] * ['abort', ...]
// [ts] * into
// [ts] * eventTypes = {
// [ts] *   'abort': {
// [ts] *     phasedRegistrationNames: {
// [ts] *       bubbled: 'onAbort',
// [ts] *       captured: 'onAbortCapture',
// [ts] *     },
// [ts] *     dependencies: ['topAbort'],
// [ts] *   },
// [ts] *   ...
// [ts] * };
// [ts] * topLevelEventsToDispatchConfig = {
// [ts] *   'topAbort': { sameConfig }
// [ts] * };
// [ts] */
// [ts] let eventTypes = {};
// [ts] let topLevelEventsToDispatchConfig = {};
// [ts] ["abort", "animationEnd", "animationIteration", "animationStart", "blur", "canPlay", "canPlayThrough", "click", "contextMenu", "copy", "cut", "doubleClick", "drag", "dragEnd", "dragEnter", "dragExit", "dragLeave", "dragOver", "dragStart", "drop", "durationChange", "emptied", "encrypted", "ended", "error", "focus", "input", "invalid", "keyDown", "keyPress", "keyUp", "load", "loadedData", "loadedMetadata", "loadStart", "mouseDown", "mouseMove", "mouseOut", "mouseOver", "mouseUp", "paste", "pause", "play", "playing", "progress", "rateChange", "reset", "scroll", "seeked", "seeking", "stalled", "submit", "suspend", "timeUpdate", "touchCancel", "touchEnd", "touchMove", "touchStart", "transitionEnd", "volumeChange", "waiting", "wheel"].forEach((event) => {
// [ts] let capitalizedEvent = event[0].toUpperCase() + event.slice(1);
// [ts] let onEvent = "on" + capitalizedEvent;
// [ts] let topEvent = "top" + capitalizedEvent;
// [ts] let type = {
// [ts] phasedRegistrationNames: {
// [ts] bubbled: onEvent,
// [ts] captured: onEvent + "Capture"
// [ts] },
// [ts] dependencies: [topEvent]
// [ts] };
// [ts] eventTypes[event] = type;
// [ts] topLevelEventsToDispatchConfig[topEvent] = type;
// [ts] });
// [ts] let onClickListeners = {};
// [ts] fn getDictionaryKey(inst: auto) {
// Prevents V8 performance issue:
// https://github.com/facebook/react/pull/7232
// [ts] return "." + inst._rootNodeID;
// [ts] }
// [ts] fn isInteractive(tag: auto) {
// [ts] return tag == "button" || tag == "input" || tag == "select" || tag == "textarea";
// [ts] }
// [ts] let SimpleEventPlugin = {
// [ts] eventTypes: eventTypes,
// [ts] extractEvents: (topLevelType, targetInst, nativeEvent, nativeEventTarget) => {
// [ts] let dispatchConfig = topLevelEventsToDispatchConfig[topLevelType];
// [ts] if (!dispatchConfig) {
// [ts] return null;
// [ts] }
// [ts] let EventConstructor;
// [ts] switch (topLevelType) {
// HTML Events
// @see http://www.w3.org/TR/html5/index.html#events-0
// [ts] "topAbort", "topCanPlay", "topCanPlayThrough", "topDurationChange", "topEmptied", "topEncrypted", "topEnded", "topError", "topInput", "topInvalid", "topLoad", "topLoadedData", "topLoadedMetadata", "topLoadStart", "topPause", "topPlay", "topPlaying", "topProgress", "topRateChange", "topReset", "topSeeked", "topSeeking", "topStalled", "topSubmit", "topSuspend", "topTimeUpdate", "topVolumeChange", "topWaiting" => {
// [ts] EventConstructor = SyntheticEvent;
// break;
// [ts] },
// Firefox creates a keypress event for function keys too. This removes
// the unwanted keypress events. Enter is however both printable and
// non-printable. One would expect Tab to be as well (but it isn't).
// [ts] "topKeyPress" => {
// [ts] if (getEventCharCode(nativeEvent) == 0) {
// [ts] return null;
// [ts] }
// [ts] /* falls through */
// [ts] },
// [ts] "topKeyDown", "topKeyUp" => {
// [ts] EventConstructor = SyntheticKeyboardEvent;
// break;
// [ts] },
// [ts] "topBlur", "topFocus" => {
// [ts] EventConstructor = SyntheticFocusEvent;
// break;
// [ts] },
// Firefox creates a click event on right mouse clicks. This removes the
// unwanted click events.
// [ts] "topClick" => {
// [ts] if (nativeEvent.button == 2) {
// [ts] return null;
// [ts] }
// [ts] /* falls through */
// [ts] },
// TODO: Disabled elements should not respond to mouse events
// [ts] /* falls through */
// [ts] "topDoubleClick", "topMouseDown", "topMouseMove", "topMouseUp", "topMouseOut", "topMouseOver", "topContextMenu" => {
// [ts] EventConstructor = SyntheticMouseEvent;
// break;
// [ts] },
// [ts] "topDrag", "topDragEnd", "topDragEnter", "topDragExit", "topDragLeave", "topDragOver", "topDragStart", "topDrop" => {
// [ts] EventConstructor = SyntheticDragEvent;
// break;
// [ts] },
// [ts] "topTouchCancel", "topTouchEnd", "topTouchMove", "topTouchStart" => {
// [ts] EventConstructor = SyntheticTouchEvent;
// break;
// [ts] },
// [ts] "topAnimationEnd", "topAnimationIteration", "topAnimationStart" => {
// [ts] EventConstructor = SyntheticAnimationEvent;
// break;
// [ts] },
// [ts] "topTransitionEnd" => {
// [ts] EventConstructor = SyntheticTransitionEvent;
// break;
// [ts] },
// [ts] "topScroll" => {
// [ts] EventConstructor = SyntheticUIEvent;
// break;
// [ts] },
// [ts] "topWheel" => {
// [ts] EventConstructor = SyntheticWheelEvent;
// break;
// [ts] },
// [ts] "topCopy", "topCut", "topPaste" => {
// [ts] EventConstructor = SyntheticClipboardEvent;
// break;
// [ts] }
// [ts] }
// [ts] if (!EventConstructor) if ("development" != "production") invariant(false, "SimpleEventPlugin: Unhandled event type, `%s`.", topLevelType) else _prodInvariant("86", topLevelType) else void 0;
// [ts] let event = EventConstructor.getPooled(dispatchConfig, targetInst, nativeEvent, nativeEventTarget);
// [ts] EventPropagators.accumulateTwoPhaseDispatches(event);
// [ts] return event;
// [ts] },
// [ts] didPutListener: (inst, registrationName, listener) => {
// Mobile Safari does not fire properly bubble click events on
// non-interactive elements, which means delegated click listeners do not
// fire. The workaround for this bug involves attaching an empty click
// listener on the target node.
// http://www.quirksmode.org/blog/archives/2010/09/click_event_del.html
// [ts] if (registrationName == "onClick" && !isInteractive(inst._tag)) {
// [ts] let key = getDictionaryKey(inst);
// [ts] let node = ReactDOMComponentTree.getNodeFromInstance(inst);
// [ts] if (!onClickListeners[key]) {
// [ts] onClickListeners[key] = EventListener.listen(node, "click", emptyFunction);
// [ts] }
// [ts] }
// [ts] },
// [ts] willDeleteListener: (inst, registrationName) => {
// [ts] if (registrationName == "onClick" && !isInteractive(inst._tag)) {
// [ts] let key = getDictionaryKey(inst);
// [ts] onClickListeners[key].remove();
// [ts] delete onClickListeners[key];
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = SimpleEventPlugin;
// [ts] },{"111":111,"126":126,"137":137,"144":144,"152":152,"20":20,"34":34,"87":87,"88":88,"90":90,"91":91,"92":92,"94":94,"95":95,"96":96,"97":97,"98":98,"99":99}],87:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/css3-animations/#AnimationEvent-interface
// [ts] * @see https://developer.mozilla.org/en-US/docs/Web/API/AnimationEvent
// [ts] */
// [ts] let AnimationEventInterface = {
// [ts] animationName: null,
// [ts] elapsedTime: null,
// [ts] pseudoElement: null
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticEvent}
// [ts] */
// [ts] fn SyntheticAnimationEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticEvent.augmentClass(SyntheticAnimationEvent, AnimationEventInterface);
// [ts] module.exports = SyntheticAnimationEvent;
// [ts] },{"91":91}],88:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/clipboard-apis/
// [ts] */
// [ts] let ClipboardEventInterface = {
// [ts] clipboardData: (event) => {
// [ts] return if ("clipboardData" == event) event.clipboardData else window.clipboardData;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticUIEvent}
// [ts] */
// [ts] fn SyntheticClipboardEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticEvent.augmentClass(SyntheticClipboardEvent, ClipboardEventInterface);
// [ts] module.exports = SyntheticClipboardEvent;
// [ts] },{"91":91}],89:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/#events-compositionevents
// [ts] */
// [ts] let CompositionEventInterface = {
// [ts] data: null
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticUIEvent}
// [ts] */
// [ts] fn SyntheticCompositionEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticEvent.augmentClass(SyntheticCompositionEvent, CompositionEventInterface);
// [ts] module.exports = SyntheticCompositionEvent;
// [ts] },{"91":91}],90:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticMouseEvent = _dereq_(95);
// [ts] /**
// [ts] * @interface DragEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let DragEventInterface = {
// [ts] dataTransfer: null
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticUIEvent}
// [ts] */
// [ts] fn SyntheticDragEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticMouseEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticMouseEvent.augmentClass(SyntheticDragEvent, DragEventInterface);
// [ts] module.exports = SyntheticDragEvent;
// [ts] },{"95":95}],91:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let PooledClass = _dereq_(25);
// [ts] let emptyFunction = _dereq_(144);
// [ts] let warning = _dereq_(159);
// [ts] let didWarnForAddedNewProperty = false;
// [ts] let isProxySupported = @typeName(Proxy) == "function";
// [ts] let shouldBeReleasedProperties = ["dispatchConfig", "_targetInst", "nativeEvent", "isDefaultPrevented", "isPropagationStopped", "_dispatchListeners", "_dispatchInstances"];
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let EventInterface = {
// [ts] type: null,
// [ts] target: null,
// currentTarget is set when dispatching; no use in copying it here
// [ts] currentTarget: emptyFunction.thatReturnsNull,
// [ts] eventPhase: null,
// [ts] bubbles: null,
// [ts] cancelable: null,
// [ts] timeStamp: (event) => {
// [ts] return event.timeStamp || Date.now();
// [ts] },
// [ts] defaultPrevented: null,
// [ts] isTrusted: null
// [ts] };
// [ts] /**
// [ts] * Synthetic events are dispatched by event plugins, typically in response to a
// [ts] * top-level event delegation handler.
// [ts] *
// [ts] * These systems should generally use pooling to reduce the frequency of garbage
// [ts] * collection. The system should check `isPersistent` to determine whether the
// [ts] * event should be released into the pool after being dispatched. Users that
// [ts] * need a persisted event should invoke `persist`.
// [ts] *
// [ts] * Synthetic events (and subclasses) implement the DOM Level 3 Events API by
// [ts] * normalizing browser quirks. Subclasses do not necessarily have to implement a
// [ts] * DOM interface; custom application-specific events can also subclass this.
// [ts] *
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {*} targetInst Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @param {DOMEventTarget} nativeEventTarget Target node.
// [ts] */
// [ts] fn SyntheticEvent(dispatchConfig: auto, targetInst: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] if ("development" != "production") {
// these have a getter/setter for warnings
// [ts] delete _.nativeEvent;
// [ts] delete _.preventDefault;
// [ts] delete _.stopPropagation;
// [ts] }
// [ts] _.dispatchConfig = dispatchConfig;
// [ts] _._targetInst = targetInst;
// [ts] _.nativeEvent = nativeEvent;
// [ts] let Interface = _.constructor.Interface;
// [ts] for (let propName in Interface) {
// [ts] if (!Interface.hasOwnProperty(propName)) {
// [ts] continue;
// [ts] }
// [ts] if ("development" != "production") {
// [ts] delete this[propName]; // this has a getter/setter for warnings
// [ts] }
// [ts] let normalize = Interface[propName];
// [ts] if (normalize) {
// [ts] this[propName] = normalize(nativeEvent);
// [ts] } else {
// [ts] if (propName == "target") {
// [ts] _.target = nativeEventTarget;
// [ts] } else {
// [ts] this[propName] = nativeEvent[propName];
// [ts] }
// [ts] }
// [ts] }
// [ts] let defaultPrevented = if (nativeEvent.defaultPrevented != null) nativeEvent.defaultPrevented else nativeEvent.returnValue == false;
// [ts] if (defaultPrevented) {
// [ts] _.isDefaultPrevented = emptyFunction.thatReturnsTrue;
// [ts] } else {
// [ts] _.isDefaultPrevented = emptyFunction.thatReturnsFalse;
// [ts] }
// [ts] _.isPropagationStopped = emptyFunction.thatReturnsFalse;
// [ts] return this;
// [ts] }
// [ts] _assign(SyntheticEvent.prototype, {
// [ts] preventDefault: () => {
// [ts] _.defaultPrevented = true;
// [ts] let event = _.nativeEvent;
// [ts] if (!event) {
// [ts] return;
// [ts] }
// [ts] if (event.preventDefault) {
// [ts] event.preventDefault();
// eslint-disable-next-line valid-typeof
// [ts] } else if (@typeName(event.returnValue) != "unknown") {
// [ts] event.returnValue = false;
// [ts] }
// [ts] _.isDefaultPrevented = emptyFunction.thatReturnsTrue;
// [ts] },
// [ts] stopPropagation: () => {
// [ts] let event = _.nativeEvent;
// [ts] if (!event) {
// [ts] return;
// [ts] }
// [ts] if (event.stopPropagation) {
// [ts] event.stopPropagation();
// eslint-disable-next-line valid-typeof
// [ts] } else if (@typeName(event.cancelBubble) != "unknown") {
// The ChangeEventPlugin registers a "propertychange" event for
// IE. This event does not support bubbling or cancelling, and
// any references to cancelBubble throw "Member not found".  A
// typeof check of "unknown" circumvents this issue (and is also
// IE specific).
// [ts] event.cancelBubble = true;
// [ts] }
// [ts] _.isPropagationStopped = emptyFunction.thatReturnsTrue;
// [ts] },
// [ts] /**
// [ts] * We release all dispatched `SyntheticEvent`s after each event loop, adding
// [ts] * them back into the pool. This allows a way to hold onto a reference that
// [ts] * won't be added back into the pool.
// [ts] */
// [ts] persist: () => {
// [ts] _.isPersistent = emptyFunction.thatReturnsTrue;
// [ts] },
// [ts] /**
// [ts] * Checks if this event should be released back into the pool.
// [ts] *
// [ts] * @return {boolean} True if this should not be released, false otherwise.
// [ts] */
// [ts] isPersistent: emptyFunction.thatReturnsFalse,
// [ts] /**
// [ts] * `PooledClass` looks for `destructor` on each instance it releases.
// [ts] */
// [ts] destructor: () => {
// [ts] let Interface = _.constructor.Interface;
// [ts] for (let propName in Interface) {
// [ts] if ("development" != "production") {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, propName, getPooledWarningPropertyDefinition(propName, Interface[propName]));
// [ts] } else {
// [ts] this[propName] = null;
// [ts] }
// [ts] }
// [ts] let i = 0;
// [ts] while (i < shouldBeReleasedProperties.length) { // [cap-migrate] for-step: i++
// [ts] this[shouldBeReleasedProperties[i]] = null;
// [ts] }
// [ts] if ("development" != "production") {
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'nativeEvent', getPooledWarningPropertyDefinition('nativeEvent', null));
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'preventDefault', getPooledWarningPropertyDefinition('preventDefault', emptyFunction));
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(this, 'stopPropagation', getPooledWarningPropertyDefinition('stopPropagation', emptyFunction));
// [ts] }
// [ts] }
// [ts] });
// [ts] SyntheticEvent.Interface = EventInterface;
// [ts] /**
// [ts] * Helper to reduce boilerplate when creating subclasses.
// [ts] *
// [ts] * @param {function} Class
// [ts] * @param {?object} Interface
// [ts] */
// [ts] SyntheticEvent.augmentClass = (Class, Interface) => {
// [ts] let Super = this;
// [ts] let E = () => {};
// [ts] E.prototype = Super.prototype;
// [ts] let prototype = E.init();
// [ts] _assign(prototype, Class.prototype);
// [ts] Class.prototype = prototype;
// [ts] Class.prototype.constructor = Class;
// [ts] Class.Interface = _assign({}, Super.Interface, Interface);
// [ts] Class.augmentClass = Super.augmentClass;
// [ts] PooledClass.addPoolingTo(Class, PooledClass.fourArgumentPooler);
// [ts] };
// [ts] /** Proxying after everything set on SyntheticEvent
// [ts] * to resolve Proxy issue on some WebKit browsers
// [ts] * in which some Event properties are set to undefined (GH#10010)
// [ts] */
// [ts] if ("development" != "production") {
// [ts] if (isProxySupported) {
// [ts] /*eslint-disable no-func-assign */
// [ts] SyntheticEvent = Proxy.init(SyntheticEvent, {
// [ts] construct: (target, args) => {
// [ts] return _.apply(target, Object.create(target.prototype), args);
// [ts] },
// [ts] apply: (constructor, that, args) => {
// [ts] return Proxy.init(constructor.apply(that, args), {
// [ts] set: (target, prop, value) => {
// [ts] if (prop != "isPersistent" && !target.constructor.Interface.hasOwnProperty(prop) && shouldBeReleasedProperties.indexOf(prop) == -1) {
// [ts] if ("development" != "production") warning(didWarnForAddedNewProperty || target.isPersistent(), "This synthetic event is reused for performance reasons. If you're " + "seeing this, you're adding a new property in the synthetic event object. " + "The property is never released. See " + "https://fb.me/react-event-pooling for more information.") else void 0;
// [ts] didWarnForAddedNewProperty = true;
// [ts] }
// [ts] target[prop] = value;
// [ts] return true;
// [ts] }
// [ts] });
// [ts] }
// [ts] });
// [ts] /*eslint-enable no-func-assign */
// [ts] }
// [ts] }
// [ts] PooledClass.addPoolingTo(SyntheticEvent, PooledClass.fourArgumentPooler);
// [ts] module.exports = SyntheticEvent;
// [ts] /**
// [ts] * Helper to nullify syntheticEvent instance properties when destructing
// [ts] *
// [ts] * @param {object} SyntheticEvent
// [ts] * @param {String} propName
// [ts] * @return {object} defineProperty object
// [ts] */
// [ts] fn getPooledWarningPropertyDefinition(propName: auto, getVal: auto) {
// [ts] let isFunction = @typeName(getVal) == "function";
// [ts] return {
// [ts] configurable: true,
// [ts] set: set,
// [ts] get: get
// [ts] };
// [ts] fn set(val: auto) {
// [ts] let action = if (isFunction) "setting the method" else "setting the property";
// [ts] warn(action, "This is effectively a no-op");
// [ts] return val;
// [ts] }
// [ts] fn get() {
// [ts] let action = if (isFunction) "accessing the method" else "accessing the property";
// [ts] let result = if (isFunction) "This is a no-op function" else "This is set to null";
// [ts] warn(action, result);
// [ts] return getVal;
// [ts] }
// [ts] fn warn(action: auto, result: auto) {
// [ts] let warningCondition = false;
// [ts] if ("development" != "production") warning(warningCondition, "This synthetic event is reused for performance reasons. If you're seeing this, " + "you're %s `%s` on a released/nullified synthetic event. %s. " + "If you must keep the original synthetic event around, use event.persist(). " + "See https://fb.me/react-event-pooling for more information.", action, propName, result) else void 0;
// [ts] }
// [ts] }
// [ts] },{"144":144,"159":159,"160":160,"25":25}],92:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticUIEvent = _dereq_(98);
// [ts] /**
// [ts] * @interface FocusEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let FocusEventInterface = {
// [ts] relatedTarget: null
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticUIEvent}
// [ts] */
// [ts] fn SyntheticFocusEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticUIEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticUIEvent.augmentClass(SyntheticFocusEvent, FocusEventInterface);
// [ts] module.exports = SyntheticFocusEvent;
// [ts] },{"98":98}],93:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/2013/WD-DOM-Level-3-Events-20131105
// [ts] *      /#events-inputevents
// [ts] */
// [ts] let InputEventInterface = {
// [ts] data: null
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticUIEvent}
// [ts] */
// [ts] fn SyntheticInputEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticEvent.augmentClass(SyntheticInputEvent, InputEventInterface);
// [ts] module.exports = SyntheticInputEvent;
// [ts] },{"91":91}],94:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticUIEvent = _dereq_(98);
// [ts] let getEventCharCode = _dereq_(111);
// [ts] let getEventKey = _dereq_(112);
// [ts] let getEventModifierState = _dereq_(113);
// [ts] /**
// [ts] * @interface KeyboardEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let KeyboardEventInterface = {
// [ts] key: getEventKey,
// [ts] location: null,
// [ts] ctrlKey: null,
// [ts] shiftKey: null,
// [ts] altKey: null,
// [ts] metaKey: null,
// [ts] repeat: null,
// [ts] locale: null,
// [ts] getModifierState: getEventModifierState,
// Legacy Interface
// [ts] charCode: (event) => {
// `charCode` is the result of a KeyPress event and represents the value of
// the actual printable character.
// KeyPress is deprecated, but its replacement is not yet final and not
// implemented in any major browser. Only KeyPress has charCode.
// [ts] if (event.type == "keypress") {
// [ts] return getEventCharCode(event);
// [ts] }
// [ts] return 0;
// [ts] },
// [ts] keyCode: (event) => {
// `keyCode` is the result of a KeyDown/Up event and represents the value of
// physical keyboard key.
// The actual meaning of the value depends on the users' keyboard layout
// which cannot be detected. Assuming that it is a US keyboard layout
// provides a surprisingly accurate mapping for US and European users.
// Due to this, it is left to the user to implement at this time.
// [ts] if (event.type == "keydown" || event.type == "keyup") {
// [ts] return event.keyCode;
// [ts] }
// [ts] return 0;
// [ts] },
// [ts] which: (event) => {
// `which` is an alias for either `keyCode` or `charCode` depending on the
// type of the event.
// [ts] if (event.type == "keypress") {
// [ts] return getEventCharCode(event);
// [ts] }
// [ts] if (event.type == "keydown" || event.type == "keyup") {
// [ts] return event.keyCode;
// [ts] }
// [ts] return 0;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticUIEvent}
// [ts] */
// [ts] fn SyntheticKeyboardEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticUIEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticUIEvent.augmentClass(SyntheticKeyboardEvent, KeyboardEventInterface);
// [ts] module.exports = SyntheticKeyboardEvent;
// [ts] },{"111":111,"112":112,"113":113,"98":98}],95:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticUIEvent = _dereq_(98);
// [ts] let ViewportMetrics = _dereq_(101);
// [ts] let getEventModifierState = _dereq_(113);
// [ts] /**
// [ts] * @interface MouseEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let MouseEventInterface = {
// [ts] screenX: null,
// [ts] screenY: null,
// [ts] clientX: null,
// [ts] clientY: null,
// [ts] ctrlKey: null,
// [ts] shiftKey: null,
// [ts] altKey: null,
// [ts] metaKey: null,
// [ts] getModifierState: getEventModifierState,
// [ts] button: (event) => {
// Webkit, Firefox, IE9+
// which:  1 2 3
// button: 0 1 2 (standard)
// [ts] let button = event.button;
// [ts] if ("which" == event) {
// [ts] return button;
// [ts] }
// IE<9
// which:  undefined
// button: 0 0 0
// button: 1 4 2 (onmouseup)
// [ts] return if (button == 2) 2 else if (button == 4) 1 else 0;
// [ts] },
// [ts] buttons: null,
// [ts] relatedTarget: (event) => {
// [ts] return event.relatedTarget || (if (event.fromElement == event.srcElement) event.toElement else event.fromElement);
// [ts] },
// "Proprietary" Interface.
// [ts] pageX: (event) => {
// [ts] return if ("pageX" == event) event.pageX else event.clientX + ViewportMetrics.currentScrollLeft;
// [ts] },
// [ts] pageY: (event) => {
// [ts] return if ("pageY" == event) event.pageY else event.clientY + ViewportMetrics.currentScrollTop;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticUIEvent}
// [ts] */
// [ts] fn SyntheticMouseEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticUIEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticUIEvent.augmentClass(SyntheticMouseEvent, MouseEventInterface);
// [ts] module.exports = SyntheticMouseEvent;
// [ts] },{"101":101,"113":113,"98":98}],96:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticUIEvent = _dereq_(98);
// [ts] let getEventModifierState = _dereq_(113);
// [ts] /**
// [ts] * @interface TouchEvent
// [ts] * @see http://www.w3.org/TR/touch-events/
// [ts] */
// [ts] let TouchEventInterface = {
// [ts] touches: null,
// [ts] targetTouches: null,
// [ts] changedTouches: null,
// [ts] altKey: null,
// [ts] metaKey: null,
// [ts] ctrlKey: null,
// [ts] shiftKey: null,
// [ts] getModifierState: getEventModifierState
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticUIEvent}
// [ts] */
// [ts] fn SyntheticTouchEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticUIEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticUIEvent.augmentClass(SyntheticTouchEvent, TouchEventInterface);
// [ts] module.exports = SyntheticTouchEvent;
// [ts] },{"113":113,"98":98}],97:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] /**
// [ts] * @interface Event
// [ts] * @see http://www.w3.org/TR/2009/WD-css3-transitions-20090320/#transition-events-
// [ts] * @see https://developer.mozilla.org/en-US/docs/Web/API/TransitionEvent
// [ts] */
// [ts] let TransitionEventInterface = {
// [ts] propertyName: null,
// [ts] elapsedTime: null,
// [ts] pseudoElement: null
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticEvent}
// [ts] */
// [ts] fn SyntheticTransitionEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticEvent.augmentClass(SyntheticTransitionEvent, TransitionEventInterface);
// [ts] module.exports = SyntheticTransitionEvent;
// [ts] },{"91":91}],98:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticEvent = _dereq_(91);
// [ts] let getEventTarget = _dereq_(114);
// [ts] /**
// [ts] * @interface UIEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let UIEventInterface = {
// [ts] view: (event) => {
// [ts] if (event.view) {
// [ts] return event.view;
// [ts] }
// [ts] let target = getEventTarget(event);
// [ts] if (target.window == target) {
// target is a window object
// [ts] return target;
// [ts] }
// [ts] let doc = target.ownerDocument;
// TODO: Figure out why `ownerDocument` is sometimes undefined in IE8.
// [ts] if (doc) {
// [ts] return doc.defaultView || doc.parentWindow;
// [ts] } else {
// [ts] return window;
// [ts] }
// [ts] },
// [ts] detail: (event) => {
// [ts] return event.detail || 0;
// [ts] }
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticEvent}
// [ts] */
// [ts] fn SyntheticUIEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticEvent.augmentClass(SyntheticUIEvent, UIEventInterface);
// [ts] module.exports = SyntheticUIEvent;
// [ts] },{"114":114,"91":91}],99:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let SyntheticMouseEvent = _dereq_(95);
// [ts] /**
// [ts] * @interface WheelEvent
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/
// [ts] */
// [ts] let WheelEventInterface = {
// [ts] deltaX: (event) => {
// [ts] return if ("deltaX" == event) event.deltaX else  // Fallback to `wheelDeltaX` for Webkit and normalize (right is positive).
// [ts] if ("wheelDeltaX" == event) -event.wheelDeltaX else 0;
// [ts] },
// [ts] deltaY: (event) => {
// [ts] return if ("deltaY" == event) event.deltaY else  // Fallback to `wheelDeltaY` for Webkit and normalize (down is positive).
// [ts] if ("wheelDeltaY" == event) { -event.wheelDeltaY else; } // Fallback to `wheelDelta` for IE<9 and normalize (down is positive).
// [ts] if ("wheelDelta" == event) -event.wheelDelta else 0;
// [ts] },
// [ts] deltaZ: null,
// Browsers without "deltaMode" is reporting in raw wheel delta where one
// notch on the scroll is always +/- 120, roughly equivalent to pixels.
// A good approximation of DOM_DELTA_LINE (1) is 5% of viewport size or
// ~40 pixels, for DOM_DELTA_SCREEN (2) it is 87.5% of viewport size.
// [ts] deltaMode: null
// [ts] };
// [ts] /**
// [ts] * @param {object} dispatchConfig Configuration used to dispatch this event.
// [ts] * @param {string} dispatchMarker Marker identifying the event target.
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @extends {SyntheticMouseEvent}
// [ts] */
// [ts] fn SyntheticWheelEvent(dispatchConfig: auto, dispatchMarker: auto, nativeEvent: auto, nativeEventTarget: auto) {
// [ts] return SyntheticMouseEvent.call(this, dispatchConfig, dispatchMarker, nativeEvent, nativeEventTarget);
// [ts] }
// [ts] SyntheticMouseEvent.augmentClass(SyntheticWheelEvent, WheelEventInterface);
// [ts] module.exports = SyntheticWheelEvent;
// [ts] },{"95":95}],100:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] let OBSERVED_ERROR = {};
// [ts] /**
// [ts] * `Transaction` creates a black box that is able to wrap any method such that
// [ts] * certain invariants are maintained before and after the method is invoked
// [ts] * (Even if an exception is thrown while invoking the wrapped method). Whoever
// [ts] * instantiates a transaction can provide enforcers of the invariants at
// [ts] * creation time. The `Transaction` class itself will supply one additional
// [ts] * automatic invariant for you - the invariant that any transaction instance
// [ts] * should not be run while it is already being run. You would typically create a
// [ts] * single instance of a `Transaction` for reuse multiple times, that potentially
// [ts] * is used to wrap several different methods. Wrappers are extremely simple -
// [ts] * they only require implementing two methods.
// [ts] *
// [ts] * <pre>
// [ts] *                       wrappers (injected at creation time)
// [ts] *                                      +        +
// [ts] *                                      |        |
// [ts] *                    +-----------------|--------|--------------+
// [ts] *                    |                 v        |              |
// [ts] *                    |      +---------------+   |              |
// [ts] *                    |   +--|    wrapper1   |---|----+         |
// [ts] *                    |   |  +---------------+   v    |         |
// [ts] *                    |   |          +-------------+  |         |
// [ts] *                    |   |     +----|   wrapper2  |--------+   |
// [ts] *                    |   |     |    +-------------+  |     |   |
// [ts] *                    |   |     |                     |     |   |
// [ts] *                    |   v     v                     v     v   | wrapper
// [ts] *                    | +---+ +---+   +---------+   +---+ +---+ | invariants
// [ts] * perform(anyMethod) | |   | |   |   |         |   |   | |   | | maintained
// [ts] * +----------------->|-|---|-|---|-->|anyMethod|---|---|-|---|-|-------->
// [ts] *                    | |   | |   |   |         |   |   | |   | |
// [ts] *                    | |   | |   |   |         |   |   | |   | |
// [ts] *                    | |   | |   |   |         |   |   | |   | |
// [ts] *                    | +---+ +---+   +---------+   +---+ +---+ |
// [ts] *                    |  initialize                    close    |
// [ts] *                    +-----------------------------------------+
// [ts] * </pre>
// [ts] *
// [ts] * Use cases:
// [ts] * - Preserving the input selection ranges before/after reconciliation.
// [ts] *   Restoring selection even in the event of an unexpected error.
// [ts] * - Deactivating events while rearranging the DOM, preventing blurs/focuses,
// [ts] *   while guaranteeing that afterwards, the event system is reactivated.
// [ts] * - Flushing a queue of collected DOM mutations to the main UI thread after a
// [ts] *   reconciliation takes place in a worker thread.
// [ts] * - Invoking any collected `componentDidUpdate` callbacks after rendering new
// [ts] *   content.
// [ts] * - (Future use case): Wrapping particular flushes of the `ReactWorker` queue
// [ts] *   to preserve the `scrollTop` (an automatic scroll aware DOM).
// [ts] * - (Future use case): Layout calculations before and after DOM updates.
// [ts] *
// [ts] * Transactional plugin API:
// [ts] * - A module that has an `initialize` method that returns any precomputation.
// [ts] * - and a `close` method that accepts the precomputation. `close` is invoked
// [ts] *   when the wrapped process is completed, or has failed.
// [ts] *
// [ts] * @param {Array<TransactionalWrapper>} transactionWrapper Wrapper modules
// [ts] * that implement `initialize` and `close`.
// [ts] * @return {Transaction} Single transaction for reuse in thread.
// [ts] *
// [ts] * @class Transaction
// [ts] */
// [ts] let TransactionImpl = {
// [ts] /**
// [ts] * Sets up this instance so that it is prepared for collecting metrics. Does
// [ts] * so such that this setup method may be used on an instance that is already
// [ts] * initialized, in a way that does not consume additional memory upon reuse.
// [ts] * That can be useful if you decide to make your subclass of this mixin a
// [ts] * "PooledClass".
// [ts] */
// [ts] reinitializeTransaction: () => {
// [ts] _.transactionWrappers = _.getTransactionWrappers();
// [ts] if (_.wrapperInitData) {
// [ts] _.wrapperInitData.length = 0;
// [ts] } else {
// [ts] _.wrapperInitData = [];
// [ts] }
// [ts] _._isInTransaction = false;
// [ts] },
// [ts] _isInTransaction: false,
// [ts] /**
// [ts] * @abstract
// [ts] * @return {Array<TransactionWrapper>} Array of transaction wrappers.
// [ts] */
// [ts] getTransactionWrappers: null,
// [ts] isInTransaction: () => {
// [ts] return !!_._isInTransaction;
// [ts] },
// [ts] /* eslint-disable space-before-function-paren */
// [ts] /**
// [ts] * Executes the function within a safety window. Use this for the top level
// [ts] * methods that result in large amounts of computation/mutations that would
// [ts] * need to be safety checked. The optional arguments helps prevent the need
// [ts] * to bind in many cases.
// [ts] *
// [ts] * @param {function} method Member of scope to call.
// [ts] * @param {Object} scope Scope to invoke from.
// [ts] * @param {Object?=} a Argument to pass to the method.
// [ts] * @param {Object?=} b Argument to pass to the method.
// [ts] * @param {Object?=} c Argument to pass to the method.
// [ts] * @param {Object?=} d Argument to pass to the method.
// [ts] * @param {Object?=} e Argument to pass to the method.
// [ts] * @param {Object?=} f Argument to pass to the method.
// [ts] *
// [ts] * @return {*} Return value from `method`.
// [ts] */
// [ts] perform: (method, scope, a, b, c, d, e, f) => {
// [ts] /* eslint-enable space-before-function-paren */
// [ts] if (!!_.isInTransaction()) if ("development" != "production") invariant(false, "Transaction.perform(...): Cannot initialize a transaction when there is already an outstanding transaction.") else _prodInvariant("27") else void 0;
// [ts] let errorThrown;
// [ts] let ret;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] _._isInTransaction = true;
// Catching errors makes debugging more difficult, so we start with
// errorThrown set to true before setting it to false after calling
// close -- if it's still set to true in the finally block, it means
// one of these calls threw.
// [ts] errorThrown = true;
// [ts] _.initializeAll(0);
// [ts] ret = method.call(scope, a, b, c, d, e, f);
// [ts] errorThrown = false;
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (errorThrown) {
// If `method` throws, prefer to show that stack trace over any thrown
// by invoking `closeAll`.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] _.closeAll(0);
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {})
// [ts] } else {
// Since `method` didn't throw, we don't want to silence the exception
// here.
// [ts] _.closeAll(0);
// [ts] }
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] _._isInTransaction = false;
// [ts] }
// [ts] }
// [ts] return ret;
// [ts] },
// [ts] initializeAll: (startIndex) => {
// [ts] let transactionWrappers = _.transactionWrappers;
// [ts] let i = startIndex;
// [ts] while (i < transactionWrappers.length) { // [cap-migrate] for-step: i++
// [ts] let wrapper = transactionWrappers[i];
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Catching errors makes debugging more difficult, so we start with the
// OBSERVED_ERROR state before overwriting it with the real return value
// of initialize -- if it's still set to OBSERVED_ERROR in the finally
// block, it means wrapper.initialize threw.
// [ts] _.wrapperInitData[i] = OBSERVED_ERROR;
// [ts] _.wrapperInitData[i] = if (wrapper.initialize) wrapper.initialize.call(this) else null;
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] if (_.wrapperInitData[i] == OBSERVED_ERROR) {
// The initializer for wrapper i threw an error; initialize the
// remaining wrappers but silence any exceptions from them to ensure
// that the first error is the one to bubble up.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] _.initializeAll(i + 1);
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {})
// [ts] }
// [ts] }
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Invokes each of `this.transactionWrappers.close[i]` functions, passing into
// [ts] * them the respective return values of `this.transactionWrappers.init[i]`
// [ts] * (`close`rs that correspond to initializers that failed will not be
// [ts] * invoked).
// [ts] */
// [ts] closeAll: (startIndex) => {
// [ts] if (!_.isInTransaction()) if ("development" != "production") invariant(false, "Transaction.closeAll(): Cannot close transaction when none are open.") else _prodInvariant("28") else void 0;
// [ts] let transactionWrappers = _.transactionWrappers;
// [ts] let i = startIndex;
// [ts] while (i < transactionWrappers.length) { // [cap-migrate] for-step: i++
// [ts] let wrapper = transactionWrappers[i];
// [ts] let initData = _.wrapperInitData[i];
// [ts] let errorThrown;
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// Catching errors makes debugging more difficult, so we start with
// errorThrown set to true before setting it to false after calling
// close -- if it's still set to true in the finally block, it means
// wrapper.close threw.
// [ts] errorThrown = true;
// [ts] if (initData != OBSERVED_ERROR && wrapper.close) {
// [ts] wrapper.close.call(this, initData);
// [ts] }
// [ts] errorThrown = false;
// [ts] } { // TODO(cap-migrate): replace finally with CP defer
// [ts] if (errorThrown) {
// The closer for wrapper i threw an error; close the remaining
// wrappers but silence any exceptions from them to ensure that the
// first error is the one to bubble up.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] _.closeAll(i + 1);
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {})
// [ts] }
// [ts] }
// [ts] }
// [ts] _.wrapperInitData.length = 0;
// [ts] }
// [ts] };
// [ts] module.exports = TransactionImpl;
// [ts] },{"126":126,"152":152}],101:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ViewportMetrics = {
// [ts] currentScrollLeft: 0,
// [ts] currentScrollTop: 0,
// [ts] refreshScrollValues: (scrollPosition) => {
// [ts] ViewportMetrics.currentScrollLeft = scrollPosition.x;
// [ts] ViewportMetrics.currentScrollTop = scrollPosition.y;
// [ts] }
// [ts] };
// [ts] module.exports = ViewportMetrics;
// [ts] },{}],102:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Accumulates items that must not be null or undefined into the first one. This
// [ts] * is used to conserve memory by avoiding array allocations, and thus sacrifices
// [ts] * API cleanness. Since `current` can be null before being passed in and not
// [ts] * null after this function, make sure to assign it back to `current`:
// [ts] *
// [ts] * `a = accumulateInto(a, b);`
// [ts] *
// [ts] * This API should be sparingly used. Try `accumulate` for something cleaner.
// [ts] *
// [ts] * @return {*|array<*>} An accumulation of items.
// [ts] */
// [ts] fn accumulateInto(current: auto, next: auto) {
// [ts] if (!(next != null)) if ("development" != "production") invariant(false, "accumulateInto(...): Accumulated items must not be null or undefined.") else _prodInvariant("30") else void 0;
// [ts] if (current == null) {
// [ts] return next;
// [ts] }
// Both are not empty. Warning: Never call x.concat(y) when you are not
// certain that x is an Array (x could be a string with concat method).
// [ts] if (Array.isArray(current)) {
// [ts] if (Array.isArray(next)) {
// [ts] current.push.apply(current, next);
// [ts] return current;
// [ts] }
// [ts] current.push(next);
// [ts] return current;
// [ts] }
// [ts] if (Array.isArray(next)) {
// A bit too dangerous to mutate `next`.
// [ts] return [current].concat(next);
// [ts] }
// [ts] return [current, next];
// [ts] }
// [ts] module.exports = accumulateInto;
// [ts] },{"126":126,"152":152}],103:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let MOD = 65521;
// adler32 is not cryptographically strong, and is only used to sanity check that
// markup generated on the server matches the markup generated on the client.
// This implementation (a modified version of the SheetJS version) has been optimized
// for our use case, at the expense of conforming to the adler32 specification
// for non-ascii inputs.
// [ts] fn adler32(data: auto) {
// [ts] let a = 1;
// [ts] let b = 0;
// [ts] let i = 0;
// [ts] let l = data.length;
// [ts] let m = l & ~0x3;
// [ts] while (i < m) {
// [ts] let n = Math.min(i + 4096, m);
// [ts] while (i < n) { // [cap-migrate] for-step: i += 4
// [ts] b += (a += data.charCodeAt(i)) + (a += data.charCodeAt(i + 1)) + (a += data.charCodeAt(i + 2)) + (a += data.charCodeAt(i + 3));
// [ts] }
// [ts] a %= MOD;
// [ts] b %= MOD;
// [ts] }
// [ts] while (i < l) { // [cap-migrate] for-step: i++
// [ts] b += a += data.charCodeAt(i);
// [ts] }
// [ts] a %= MOD;
// [ts] b %= MOD;
// [ts] return a | b << 16;
// [ts] }
// [ts] module.exports = adler32;
// [ts] },{}],104:[(_dereq_, module, exports) => {
// [ts] ((process) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let ReactPropTypeLocationNames = _dereq_(72);
// [ts] let ReactPropTypesSecret = _dereq_(73);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] let ReactComponentTreeHook;
// [ts] if (@typeName(process) != "undefined" && process.env && "development" == "test") {
// Temporary hack.
// Inline requires don't work well with Jest:
// https://github.com/facebook/react/issues/7240
// Remove the inline requires when we don't need them anymore:
// https://github.com/facebook/react/pull/7178
// [ts] ReactComponentTreeHook = _dereq_(133);
// [ts] }
// [ts] let loggedTypeFailures = {};
// [ts] /**
// [ts] * Assert that the values match with the type specs.
// [ts] * Error messages are memorized and will only be shown once.
// [ts] *
// [ts] * @param {object} typeSpecs Map of name to a ReactPropType
// [ts] * @param {object} values Runtime values that need to be type-checked
// [ts] * @param {string} location e.g. "prop", "context", "child context"
// [ts] * @param {string} componentName Name of the component for error messages.
// [ts] * @param {?object} element The React element that is being type-checked
// [ts] * @param {?number} debugID The React component instance that is being type-checked
// [ts] * @private
// [ts] */
// [ts] fn checkReactTypeSpec(typeSpecs: auto, values: auto, location: auto, componentName: auto, element: auto, debugID: auto) {
// [ts] for (let typeSpecName in typeSpecs) {
// [ts] if (typeSpecs.hasOwnProperty(typeSpecName)) {
// [ts] let error;
// Prop type validation may throw. In case they do, we don't want to
// fail the render phase where it didn't fail before. So we log it.
// After these have been cleaned up, we'll let them throw.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// This is intentionally an invariant that gets caught. It's the same
// behavior as without this statement except with a better message.
// [ts] if (!(@typeName(typeSpecs[typeSpecName]) == "function")) if ("development" != "production") invariant(false, "%s: %s type `%s` is invalid; it must be a function, usually from React.PropTypes.", componentName || "React class", ReactPropTypeLocationNames[location], typeSpecName) else _prodInvariant("84", componentName || "React class", ReactPropTypeLocationNames[location], typeSpecName) else void 0;
// [ts] error = typeSpecs[typeSpecName](values, typeSpecName, componentName, location, null, ReactPropTypesSecret);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (ex) {)
// [ts] error = ex;
// [ts] }
// [ts] if ("development" != "production") warning(!error || error == Error, "%s: type specification of %s `%s` is invalid; the type checker " + "function must return `null` or an `Error` but returned a %s. " + "You may have forgotten to pass an argument to the type checker " + "creator (arrayOf, instanceOf, objectOf, oneOf, oneOfType, and " + "shape all require an argument).", componentName || "React class", ReactPropTypeLocationNames[location], typeSpecName, @typeName(error)) else void 0;
// [ts] if (error == Error && !(error.message == loggedTypeFailures)) {
// Only monitor this failure once because there tends to be a lot of the
// same error.
// [ts] loggedTypeFailures[error.message] = true;
// [ts] let componentStackInfo = "";
// [ts] if ("development" != "production") {
// [ts] if (!ReactComponentTreeHook) {
// [ts] ReactComponentTreeHook = _dereq_(133);
// [ts] }
// [ts] if (debugID != null) {
// [ts] componentStackInfo = ReactComponentTreeHook.getStackAddendumByID(debugID);
// [ts] } else if (element != null) {
// [ts] componentStackInfo = ReactComponentTreeHook.getCurrentStackAddendum(element);
// [ts] }
// [ts] }
// [ts] if ("development" != "production") warning(false, "Failed %s type: %s%s", location, error.message, componentStackInfo) else void 0;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] module.exports = checkReactTypeSpec;
// [ts] }).call(this,null)
// [ts] },{"126":126,"133":133,"152":152,"159":159,"72":72,"73":73}],105:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] /* globals MSApp */
// [ts] "use strict";
// [ts] /**
// [ts] * Create a function which has 'unsafe' privileges (required by windows8 apps)
// [ts] */
// [ts] let createMicrosoftUnsafeLocalFunction = (func) => {
// [ts] if (@typeName(MSApp) != "undefined" && MSApp.execUnsafeLocalFunction) {
// [ts] return (arg0, arg1, arg2, arg3) => {
// [ts] MSApp.execUnsafeLocalFunction(() => {
// [ts] return func(arg0, arg1, arg2, arg3);
// [ts] });
// [ts] };
// [ts] } else {
// [ts] return func;
// [ts] }
// [ts] };
// [ts] module.exports = createMicrosoftUnsafeLocalFunction;
// [ts] },{}],106:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let CSSProperty = _dereq_(4);
// [ts] let warning = _dereq_(159);
// [ts] let isUnitlessNumber = CSSProperty.isUnitlessNumber;
// [ts] let styleWarnings = {};
// [ts] /**
// [ts] * Convert a value into the proper css writable value. The style name `name`
// [ts] * should be logical (no hyphens), as specified
// [ts] * in `CSSProperty.isUnitlessNumber`.
// [ts] *
// [ts] * @param {string} name CSS property name such as `topMargin`.
// [ts] * @param {*} value CSS property value such as `10px`.
// [ts] * @param {ReactDOMComponent} component
// [ts] * @return {string} Normalized style value with dimensions applied.
// [ts] */
// [ts] fn dangerousStyleValue(name: auto, value: auto, component: auto, isCustomProperty: auto) {
// Note that we've removed escapeTextForBrowser() calls here since the
// whole string will be escaped when the attribute is injected into
// the markup. If you provide unsafe user data here they can inject
// arbitrary CSS which may be problematic (I couldn't repro this):
// https://www.owasp.org/index.php/XSS_Filter_Evasion_Cheat_Sheet
// http://www.thespanner.co.uk/2007/11/26/ultimate-xss-css-injection/
// This is not an XSS hole but instead a potential CSS injection issue
// which has lead to a greater discussion about how we're going to
// trust URLs moving forward. See #2115901
// [ts] let isEmpty = value == null || @typeName(value) == "boolean" || value == "";
// [ts] if (isEmpty) {
// [ts] return "";
// [ts] }
// [ts] let isNonNumeric = isNaN(value);
// [ts] if (isCustomProperty || isNonNumeric || value == 0 || isUnitlessNumber.hasOwnProperty(name) && isUnitlessNumber[name]) {
// [ts] return "" + value; // cast to string
// [ts] }
// [ts] if (@typeName(value) == "string") {
// [ts] if ("development" != "production") {
// Allow '0' to pass through without warning. 0 is already special and
// doesn't require units, so we don't need to warn about it.
// [ts] if (component && value != "0") {
// [ts] let owner = component._currentElement._owner;
// [ts] let ownerName = if (owner) owner.getName() else null;
// [ts] if (ownerName && !styleWarnings[ownerName]) {
// [ts] styleWarnings[ownerName] = {};
// [ts] }
// [ts] let warned = false;
// [ts] if (ownerName) {
// [ts] let warnings = styleWarnings[ownerName];
// [ts] warned = warnings[name];
// [ts] if (!warned) {
// [ts] warnings[name] = true;
// [ts] }
// [ts] }
// [ts] if (!warned) {
// [ts] if ("development" != "production") warning(false, "a `%s` tag (owner: `%s`) was passed a numeric string value " + "for CSS property `%s` (value: `%s`) which will be treated " + "as a unitless number in a future version of React.", component._currentElement.type, ownerName || "unknown", name, value) else void 0;
// [ts] }
// [ts] }
// [ts] }
// [ts] value = value.trim();
// [ts] }
// [ts] return value + "px";
// [ts] }
// [ts] module.exports = dangerousStyleValue;
// [ts] },{"159":159,"4":4}],107:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2016-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] * Based on the escape-html library, which is used under the MIT License below:
// [ts] *
// [ts] * Copyright (c) 2012-2013 TJ Holowaychuk
// [ts] * Copyright (c) 2015 Andreas Lubbe
// [ts] * Copyright (c) 2015 Tiancheng "Timothy" Gu
// [ts] *
// [ts] * Permission is hereby granted, free of charge, to any person obtaining
// [ts] * a copy of this software and associated documentation files (the
// [ts] * 'Software'), to deal in the Software without restriction, including
// [ts] * without limitation the rights to use, copy, modify, merge, publish,
// [ts] * distribute, sublicense, and/or sell copies of the Software, and to
// [ts] * permit persons to whom the Software is furnished to do so, subject to
// [ts] * the following conditions:
// [ts] *
// [ts] * The above copyright notice and this permission notice shall be
// [ts] * included in all copies or substantial portions of the Software.
// [ts] *
// [ts] * THE SOFTWARE IS PROVIDED 'AS IS', WITHOUT WARRANTY OF ANY KIND,
// [ts] * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// [ts] * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// [ts] * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
// [ts] * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// [ts] * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// [ts] * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// [ts] *
// [ts] */
// [ts] "use strict";
// code copied and modified from escape-html
// [ts] /**
// [ts] * Module variables.
// [ts] * @private
// [ts] */
// [ts] let matchHtmlRegExp = "[\"'&<>]";
// [ts] /**
// [ts] * Escape special characters in the given string of html.
// [ts] *
// [ts] * @param  {string} string The string to escape for inserting into HTML
// [ts] * @return {string}
// [ts] * @public
// [ts] */
// [ts] fn escapeHtml(string: auto) {
// [ts] let str = "" + string;
// [ts] let match_val = matchHtmlRegExp.exec(str);
// [ts] if (!match_val) {
// [ts] return str;
// [ts] }
// [ts] let escape;
// [ts] let html = "";
// [ts] let index = 0;
// [ts] let lastIndex = 0;
// [ts] index = match_val.index;
// [ts] while (index < str.length) { // [cap-migrate] for-step: index++
// [ts] switch (str.charCodeAt(index)) {
// "
// [ts] 34 => {
// [ts] escape = "&quot;";
// break;
// [ts] },
// &
// [ts] 38 => {
// [ts] escape = "&amp;";
// break;
// [ts] },
// '
// [ts] 39 => {
// [ts] escape = "&#x27;"; // modified from escape-html; used to be '&#39'
// break;
// [ts] },
// <
// [ts] 60 => {
// [ts] escape = "&lt;";
// break;
// [ts] },
// >
// [ts] 62 => {
// [ts] escape = "&gt;";
// break;
// [ts] },
// [ts] else => {
// [ts] continue;
// [ts] }
// [ts] }
// [ts] if (lastIndex != index) {
// [ts] html += str.substring(lastIndex, index);
// [ts] }
// [ts] lastIndex = index + 1;
// [ts] html += escape;
// [ts] }
// [ts] return if (lastIndex != index) html + str.substring(lastIndex, index) else html;
// [ts] }
// end code copied and modified from escape-html
// [ts] /**
// [ts] * Escapes text to prevent scripting attacks.
// [ts] *
// [ts] * @param {*} text Text value to escape.
// [ts] * @return {string} An escaped string.
// [ts] */
// [ts] fn escapeTextContentForBrowser(text: auto) {
// [ts] if (@typeName(text) == "boolean" || @typeName(text) == "number") {
// this shortcircuit helps perf for types that we know will never have
// special characters, especially given that this function is used often
// for numeric dom ids.
// [ts] return "" + text;
// [ts] }
// [ts] return escapeHtml(text);
// [ts] }
// [ts] module.exports = escapeTextContentForBrowser;
// [ts] },{}],108:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let ReactCurrentOwner = _dereq_(134);
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] let ReactInstanceMap = _dereq_(63);
// [ts] let getHostComponentFromComposite = _dereq_(115);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] /**
// [ts] * Returns the DOM node rendered by this element.
// [ts] *
// [ts] * See https://facebook.github.io/react/docs/top-level-api.html#reactdom.finddomnode
// [ts] *
// [ts] * @param {ReactComponent|DOMElement} componentOrElement
// [ts] * @return {?DOMElement} The root node of this element.
// [ts] */
// [ts] fn findDOMNode(componentOrElement: auto) {
// [ts] if ("development" != "production") {
// [ts] let owner = ReactCurrentOwner.current;
// [ts] if (owner != null) {
// [ts] if ("development" != "production") warning(owner._warnedAboutRefsInRender, "%s is accessing findDOMNode inside its render(). " + "render() should be a pure function of props and state. It should " + "never access something that requires stale data from the previous " + "render, such as refs. Move this logic to componentDidMount and " + "componentDidUpdate instead.", owner.getName() || "A component") else void 0;
// [ts] owner._warnedAboutRefsInRender = true;
// [ts] }
// [ts] }
// [ts] if (componentOrElement == null) {
// [ts] return null;
// [ts] }
// [ts] if (componentOrElement.nodeType == 1) {
// [ts] return componentOrElement;
// [ts] }
// [ts] let inst = ReactInstanceMap.get(componentOrElement);
// [ts] if (inst) {
// [ts] inst = getHostComponentFromComposite(inst);
// [ts] return if (inst) ReactDOMComponentTree.getNodeFromInstance(inst) else null;
// [ts] }
// [ts] if (@typeName(componentOrElement.render) == "function") {
// [ts] if (!false) if ("development" != "production") invariant(false, "findDOMNode was called on an unmounted component.") else _prodInvariant("44") else void 0;
// [ts] } else {
// [ts] if (!false) if ("development" != "production") invariant(false, "Element appears to be neither ReactComponent nor DOMNode (keys: %s)", Object.keys(componentOrElement)) else _prodInvariant("45", Object.keys(componentOrElement)) else void 0;
// [ts] }
// [ts] }
// [ts] module.exports = findDOMNode;
// [ts] },{"115":115,"126":126,"134":134,"152":152,"159":159,"34":34,"63":63}],109:[(_dereq_, module, exports) => {
// [ts] ((process) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let KeyEscapeUtils = _dereq_(23);
// [ts] let traverseAllChildren = _dereq_(131);
// [ts] let warning = _dereq_(159);
// [ts] let ReactComponentTreeHook;
// [ts] if (@typeName(process) != "undefined" && process.env && "development" == "test") {
// Temporary hack.
// Inline requires don't work well with Jest:
// https://github.com/facebook/react/issues/7240
// Remove the inline requires when we don't need them anymore:
// https://github.com/facebook/react/pull/7178
// [ts] ReactComponentTreeHook = _dereq_(133);
// [ts] }
// [ts] /**
// [ts] * @param {function} traverseContext Context passed through traversal.
// [ts] * @param {?ReactComponent} child React child component.
// [ts] * @param {!string} name String name of key path to child.
// [ts] * @param {number=} selfDebugID Optional debugID of the current internal instance.
// [ts] */
// [ts] fn flattenSingleChildIntoContext(traverseContext: auto, child: auto, name: auto, selfDebugID: auto) {
// We found a component instance.
// [ts] if (traverseContext && @typeName(traverseContext) == "object") {
// [ts] let result = traverseContext;
// [ts] let keyUnique = result[name] == null;
// [ts] if ("development" != "production") {
// [ts] if (!ReactComponentTreeHook) {
// [ts] ReactComponentTreeHook = _dereq_(133);
// [ts] }
// [ts] if (!keyUnique) {
// [ts] if ("development" != "production") warning(false, "flattenChildren(...): Encountered two children with the same key, " + "`%s`. Child keys must be unique; when two children share a key, only " + "the first child will be used.%s", KeyEscapeUtils.unescape(name), ReactComponentTreeHook.getStackAddendumByID(selfDebugID)) else void 0;
// [ts] }
// [ts] }
// [ts] if (keyUnique && child != null) {
// [ts] result[name] = child;
// [ts] }
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Flattens children that are typically specified as `props.children`. Any null
// [ts] * children will not be included in the resulting object.
// [ts] * @return {!object} flattened children keyed by name.
// [ts] */
// [ts] fn flattenChildren(children: auto, selfDebugID: auto) {
// [ts] if (children == null) {
// [ts] return children;
// [ts] }
// [ts] let result = {};
// [ts] if ("development" != "production") {
// [ts] traverseAllChildren(children, (traverseContext, child, name) => {
// [ts] return flattenSingleChildIntoContext(traverseContext, child, name, selfDebugID);
// [ts] }, result);
// [ts] } else {
// [ts] traverseAllChildren(children, flattenSingleChildIntoContext, result);
// [ts] }
// [ts] return result;
// [ts] }
// [ts] module.exports = flattenChildren;
// [ts] }).call(this,null)
// [ts] },{"131":131,"133":133,"159":159,"23":23}],110:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * @param {array} arr an "accumulation" of items which is either an Array or
// [ts] * a single item. Useful when paired with the `accumulate` module. This is a
// [ts] * simple utility that allows us to reason about a collection of items, but
// [ts] * handling the case when there is exactly one item (and we do not need to
// [ts] * allocate an array).
// [ts] */
// [ts] fn forEachAccumulated(arr: auto, cb: auto, scope: auto) {
// [ts] if (Array.isArray(arr)) {
// [ts] arr.forEach(cb, scope);
// [ts] } else if (arr) {
// [ts] cb.call(scope, arr);
// [ts] }
// [ts] }
// [ts] module.exports = forEachAccumulated;
// [ts] },{}],111:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * `charCode` represents the actual "character code" and is safe to use with
// [ts] * `String.fromCharCode`. As such, only keys that correspond to printable
// [ts] * characters produce a valid `charCode`, the only exception to this is Enter.
// [ts] * The Tab-key is considered non-printable and does not have a `charCode`,
// [ts] * presumably because it does not produce a tab-character in browsers.
// [ts] *
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {number} Normalized `charCode` property.
// [ts] */
// [ts] fn getEventCharCode(nativeEvent: auto) {
// [ts] let charCode;
// [ts] let keyCode = nativeEvent.keyCode;
// [ts] if ("charCode" == nativeEvent) {
// [ts] charCode = nativeEvent.charCode;
// FF does not set `charCode` for the Enter-key, check against `keyCode`.
// [ts] if (charCode == 0 && keyCode == 13) {
// [ts] charCode = 13;
// [ts] }
// [ts] } else {
// IE8 does not implement `charCode`, but `keyCode` has the correct value.
// [ts] charCode = keyCode;
// [ts] }
// Some non-printable keys are reported in `charCode`/`keyCode`, discard them.
// Must not discard the (non-)printable Enter-key.
// [ts] if (charCode >= 32 || charCode == 13) {
// [ts] return charCode;
// [ts] }
// [ts] return 0;
// [ts] }
// [ts] module.exports = getEventCharCode;
// [ts] },{}],112:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let getEventCharCode = _dereq_(111);
// [ts] /**
// [ts] * Normalization of deprecated HTML5 `key` values
// [ts] * @see https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent#Key_names
// [ts] */
// [ts] let normalizeKey = {
// [ts] Esc: "Escape",
// [ts] Spacebar: " ",
// [ts] Left: "ArrowLeft",
// [ts] Up: "ArrowUp",
// [ts] Right: "ArrowRight",
// [ts] Down: "ArrowDown",
// [ts] Del: "Delete",
// [ts] Win: "OS",
// [ts] Menu: "ContextMenu",
// [ts] Apps: "ContextMenu",
// [ts] Scroll: "ScrollLock",
// [ts] MozPrintableKey: "Unidentified"
// [ts] };
// [ts] /**
// [ts] * Translation from legacy `keyCode` to HTML5 `key`
// [ts] * Only special keys supported, all others depend on keyboard layout or browser
// [ts] * @see https://developer.mozilla.org/en-US/docs/Web/API/KeyboardEvent#Key_names
// [ts] */
// [ts] let translateToKey = {
// [ts] "8": "Backspace",
// [ts] "9": "Tab",
// [ts] "12": "Clear",
// [ts] "13": "Enter",
// [ts] "16": "Shift",
// [ts] "17": "Control",
// [ts] "18": "Alt",
// [ts] "19": "Pause",
// [ts] "20": "CapsLock",
// [ts] "27": "Escape",
// [ts] "32": " ",
// [ts] "33": "PageUp",
// [ts] "34": "PageDown",
// [ts] "35": "End",
// [ts] "36": "Home",
// [ts] "37": "ArrowLeft",
// [ts] "38": "ArrowUp",
// [ts] "39": "ArrowRight",
// [ts] "40": "ArrowDown",
// [ts] "45": "Insert",
// [ts] "46": "Delete",
// [ts] "112": "F1",
// [ts] "113": "F2",
// [ts] "114": "F3",
// [ts] "115": "F4",
// [ts] "116": "F5",
// [ts] "117": "F6",
// [ts] "118": "F7",
// [ts] "119": "F8",
// [ts] "120": "F9",
// [ts] "121": "F10",
// [ts] "122": "F11",
// [ts] "123": "F12",
// [ts] "144": "NumLock",
// [ts] "145": "ScrollLock",
// [ts] "224": "Meta"
// [ts] };
// [ts] /**
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {string} Normalized `key` property.
// [ts] */
// [ts] fn getEventKey(nativeEvent: auto) {
// [ts] if (nativeEvent.key) {
// Normalize inconsistent values reported by browsers due to
// implementations of a working draft specification.
// FireFox implements `key` but returns `MozPrintableKey` for all
// printable characters (normalized to `Unidentified`), ignore it.
// [ts] let key = normalizeKey[nativeEvent.key] || nativeEvent.key;
// [ts] if (key != "Unidentified") {
// [ts] return key;
// [ts] }
// [ts] }
// Browser does not implement `key`, polyfill as much of it as we can.
// [ts] if (nativeEvent.type == "keypress") {
// [ts] let charCode = getEventCharCode(nativeEvent);
// The enter-key is technically both printable and non-printable and can
// thus be captured by `keypress`, no other non-printable key should.
// [ts] return if (charCode == 13) "Enter" else String.fromCharCode(charCode);
// [ts] }
// [ts] if (nativeEvent.type == "keydown" || nativeEvent.type == "keyup") {
// While user keyboard layout determines the actual meaning of each
// `keyCode` value, almost all function keys have a universal value.
// [ts] return translateToKey[nativeEvent.keyCode] || "Unidentified";
// [ts] }
// [ts] return "";
// [ts] }
// [ts] module.exports = getEventKey;
// [ts] },{"111":111}],113:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Translation from modifier key to the associated property in the event.
// [ts] * @see http://www.w3.org/TR/DOM-Level-3-Events/#keys-Modifiers
// [ts] */
// [ts] let modifierKeyToProp = {
// [ts] Alt: "altKey",
// [ts] Control: "ctrlKey",
// [ts] Meta: "metaKey",
// [ts] Shift: "shiftKey"
// [ts] };
// IE8 does not implement getModifierState so we simply map it to the only
// modifier keys exposed by the event itself, does not support Lock-keys.
// Currently, all major browsers except Chrome seems to support Lock-keys.
// [ts] fn modifierStateGetter(keyArg: auto) {
// [ts] let syntheticEvent = this;
// [ts] let nativeEvent = syntheticEvent.nativeEvent;
// [ts] if (nativeEvent.getModifierState) {
// [ts] return nativeEvent.getModifierState(keyArg);
// [ts] }
// [ts] let keyProp = modifierKeyToProp[keyArg];
// [ts] return if (keyProp) !!nativeEvent[keyProp] else false;
// [ts] }
// [ts] fn getEventModifierState(nativeEvent: auto) {
// [ts] return modifierStateGetter;
// [ts] }
// [ts] module.exports = getEventModifierState;
// [ts] },{}],114:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Gets the target node from a native browser event by accounting for
// [ts] * inconsistencies in browser DOM APIs.
// [ts] *
// [ts] * @param {object} nativeEvent Native browser event.
// [ts] * @return {DOMEventTarget} Target node.
// [ts] */
// [ts] fn getEventTarget(nativeEvent: auto) {
// [ts] let target = nativeEvent.target || nativeEvent.srcElement || window;
// Normalize SVG <use> element events #4963
// [ts] if (target.correspondingUseElement) {
// [ts] target = target.correspondingUseElement;
// [ts] }
// Safari may fire events on text nodes (Node.TEXT_NODE is 3).
// @see http://www.quirksmode.org/js/events_properties.html
// [ts] return if (target.nodeType == 3) target.parentNode else target;
// [ts] }
// [ts] module.exports = getEventTarget;
// [ts] },{}],115:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactNodeTypes = _dereq_(69);
// [ts] fn getHostComponentFromComposite(inst: auto) {
// [ts] let type;
// [ts] while ((type = inst._renderedNodeType) == ReactNodeTypes.COMPOSITE) {
// [ts] inst = inst._renderedComponent;
// [ts] }
// [ts] if (type == ReactNodeTypes.HOST) {
// [ts] return inst._renderedComponent;
// [ts] } else if (type == ReactNodeTypes.EMPTY) {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] module.exports = getHostComponentFromComposite;
// [ts] },{"69":69}],116:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /* global Symbol */
// [ts] let ITERATOR_SYMBOL = @typeName(Symbol) == "function" && "__symbol_iterator";
// [ts] let FAUX_ITERATOR_SYMBOL = "@@iterator"; // Before Symbol spec.
// [ts] /**
// [ts] * Returns the iterator method function contained on the iterable object.
// [ts] *
// [ts] * Be sure to invoke the function with the iterable as context:
// [ts] *
// [ts] *     var iteratorFn = getIteratorFn(myIterable);
// [ts] *     if (iteratorFn) {
// [ts] *       var iterator = iteratorFn.call(myIterable);
// [ts] *       ...
// [ts] *     }
// [ts] *
// [ts] * @param {?object} maybeIterable
// [ts] * @return {?function}
// [ts] */
// [ts] fn getIteratorFn(maybeIterable: auto) {
// [ts] let iteratorFn = maybeIterable && (ITERATOR_SYMBOL && maybeIterable[ITERATOR_SYMBOL] || maybeIterable[FAUX_ITERATOR_SYMBOL]);
// [ts] if (@typeName(iteratorFn) == "function") {
// [ts] return iteratorFn;
// [ts] }
// [ts] }
// [ts] module.exports = getIteratorFn;
// [ts] },{}],117:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Given any node return the first leaf node without children.
// [ts] *
// [ts] * @param {DOMElement|DOMTextNode} node
// [ts] * @return {DOMElement|DOMTextNode}
// [ts] */
// [ts] fn getLeafNode(node: auto) {
// [ts] while (node && node.firstChild) {
// [ts] node = node.firstChild;
// [ts] }
// [ts] return node;
// [ts] }
// [ts] /**
// [ts] * Get the next sibling within a container. This will walk up the
// [ts] * DOM if a node's siblings have been exhausted.
// [ts] *
// [ts] * @param {DOMElement|DOMTextNode} node
// [ts] * @return {?DOMElement|DOMTextNode}
// [ts] */
// [ts] fn getSiblingNode(node: auto) {
// [ts] while (node) {
// [ts] if (node.nextSibling) {
// [ts] return node.nextSibling;
// [ts] }
// [ts] node = node.parentNode;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Get object describing the nodes which contain characters at offset.
// [ts] *
// [ts] * @param {DOMElement|DOMTextNode} root
// [ts] * @param {number} offset
// [ts] * @return {?object}
// [ts] */
// [ts] fn getNodeForCharacterOffset(root: auto, offset: auto) {
// [ts] let node = getLeafNode(root);
// [ts] let nodeStart = 0;
// [ts] let nodeEnd = 0;
// [ts] while (node) {
// [ts] if (node.nodeType == 3) {
// [ts] nodeEnd = nodeStart + node.textContent.length;
// [ts] if (nodeStart <= offset && nodeEnd >= offset) {
// [ts] return {
// [ts] node: node,
// [ts] offset: offset - nodeStart
// [ts] };
// [ts] }
// [ts] nodeStart = nodeEnd;
// [ts] }
// [ts] node = getLeafNode(getSiblingNode(node));
// [ts] }
// [ts] }
// [ts] module.exports = getNodeForCharacterOffset;
// [ts] },{}],118:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let contentKey = null;
// [ts] /**
// [ts] * Gets the key used to access text content on a DOM node.
// [ts] *
// [ts] * @return {?string} Key used to access text content.
// [ts] * @internal
// [ts] */
// [ts] fn getTextContentAccessor() {
// [ts] if (!contentKey && ExecutionEnvironment.canUseDOM) {
// Prefer textContent to innerText because many browsers support both but
// SVG <text> elements don't support innerText even when <div> does.
// [ts] contentKey = if ("textContent" == document.documentElement) "textContent" else "innerText";
// [ts] }
// [ts] return contentKey;
// [ts] }
// [ts] module.exports = getTextContentAccessor;
// [ts] },{"138":138}],119:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] /**
// [ts] * Generate a mapping of standard vendor prefixes using the defined style property and event name.
// [ts] *
// [ts] * @param {string} styleProp
// [ts] * @param {string} eventName
// [ts] * @returns {object}
// [ts] */
// [ts] fn makePrefixMap(styleProp: auto, eventName: auto) {
// [ts] let prefixes = {};
// [ts] prefixes[styleProp.toLowerCase()] = eventName.toLowerCase();
// [ts] prefixes["Webkit" + styleProp] = "webkit" + eventName;
// [ts] prefixes["Moz" + styleProp] = "moz" + eventName;
// [ts] prefixes["ms" + styleProp] = "MS" + eventName;
// [ts] prefixes["O" + styleProp] = "o" + eventName.toLowerCase();
// [ts] return prefixes;
// [ts] }
// [ts] /**
// [ts] * A list of event names to a configurable list of vendor prefixes.
// [ts] */
// [ts] let vendorPrefixes = {
// [ts] animationend: makePrefixMap("Animation", "AnimationEnd"),
// [ts] animationiteration: makePrefixMap("Animation", "AnimationIteration"),
// [ts] animationstart: makePrefixMap("Animation", "AnimationStart"),
// [ts] transitionend: makePrefixMap("Transition", "TransitionEnd")
// [ts] };
// [ts] /**
// [ts] * Event names that have already been detected and prefixed (if applicable).
// [ts] */
// [ts] let prefixedEventNames = {};
// [ts] /**
// [ts] * Element to check for prefixes on.
// [ts] */
// [ts] let style = {};
// [ts] /**
// [ts] * Bootstrap if a DOM exists.
// [ts] */
// [ts] if (ExecutionEnvironment.canUseDOM) {
// [ts] style = document.createElement("div").style;
// On some platforms, in particular some releases of Android 4.x,
// the un-prefixed "animation" and "transition" properties are defined on the
// style object but the events that fire will still be prefixed, so we need
// to check if the un-prefixed events are usable, and if not remove them from the map.
// [ts] if (!("AnimationEvent" == window)) {
// [ts] delete vendorPrefixes.animationend.animation;
// [ts] delete vendorPrefixes.animationiteration.animation;
// [ts] delete vendorPrefixes.animationstart.animation;
// [ts] }
// Same as above
// [ts] if (!("TransitionEvent" == window)) {
// [ts] delete vendorPrefixes.transitionend.transition;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Attempts to determine the correct vendor prefixed event name.
// [ts] *
// [ts] * @param {string} eventName
// [ts] * @returns {string}
// [ts] */
// [ts] fn getVendorPrefixedEventName(eventName: auto) {
// [ts] if (prefixedEventNames[eventName]) {
// [ts] return prefixedEventNames[eventName];
// [ts] } else if (!vendorPrefixes[eventName]) {
// [ts] return eventName;
// [ts] }
// [ts] let prefixMap = vendorPrefixes[eventName];
// [ts] for (let styleProp in prefixMap) {
// [ts] if (prefixMap.hasOwnProperty(styleProp) && styleProp == style) {
// [ts] return prefixedEventNames[eventName] = prefixMap[styleProp];
// [ts] }
// [ts] }
// [ts] return "";
// [ts] }
// [ts] module.exports = getVendorPrefixedEventName;
// [ts] },{"138":138}],120:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactDOMComponentTree = _dereq_(34);
// [ts] fn isCheckable(elem: auto) {
// [ts] let type = elem.type;
// [ts] let nodeName = elem.nodeName;
// [ts] return nodeName && nodeName.toLowerCase() == "input" && (type == "checkbox" || type == "radio");
// [ts] }
// [ts] fn getTracker(inst: auto) {
// [ts] return inst._wrapperState.valueTracker;
// [ts] }
// [ts] fn attachTracker(inst: auto, tracker: auto) {
// [ts] inst._wrapperState.valueTracker = tracker;
// [ts] }
// [ts] fn detachTracker(inst: auto) {
// [ts] inst._wrapperState.valueTracker = null;
// [ts] }
// [ts] fn getValueFromNode(node: auto) {
// [ts] let value;
// [ts] if (node) {
// [ts] value = if (isCheckable(node)) "" + node.checked else node.value;
// [ts] }
// [ts] return value;
// [ts] }
// [ts] let inputValueTracking = {
// exposed for testing
// [ts] _getTrackerFromNode: (node) => {
// [ts] return getTracker(ReactDOMComponentTree.getInstanceFromNode(node));
// [ts] },
// [ts] track: (inst) => {
// [ts] if (getTracker(inst)) {
// [ts] return;
// [ts] }
// [ts] let node = ReactDOMComponentTree.getNodeFromInstance(inst);
// [ts] let valueField = if (isCheckable(node)) "checked" else "value";
// [ts] let descriptor = Object.getOwnPropertyDescriptor(node.constructor.prototype, valueField);
// [ts] let currentValue = "" + node[valueField];
// if someone has already defined a value or Safari, then bail
// and don't track value will cause over reporting of changes,
// but it's better then a hard failure
// (needed for certain tests that spyOn input values and Safari)
// [ts] if (node.hasOwnProperty(valueField) || @typeName(descriptor.get) != "function" || @typeName(descriptor.set) != "function") {
// [ts] return;
// [ts] }
// TODO(cap-migrate): dynamic TS metaprogramming not supported in CP: Object.defineProperty(node, valueField, {
// [ts-skip] enumerable: descriptor.enumerable,
// [ts-skip] configurable: true,
// [ts-skip] get: function () {
// [ts-skip] return descriptor.get.call(this);
// [ts-skip] },
// [ts-skip] set: function (value) {
// [ts-skip] currentValue = '' + value;
// [ts-skip] descriptor.set.call(this, value);
// [ts-skip] }
// [ts-skip] });
// [ts] attachTracker(inst, {
// [ts] getValue: () => {
// [ts] return currentValue;
// [ts] },
// [ts] setValue: (value) => {
// [ts] currentValue = "" + value;
// [ts] },
// [ts] stopTracking: () => {
// [ts] detachTracker(inst);
// [ts] delete node[valueField];
// [ts] }
// [ts] });
// [ts] },
// [ts] updateValueIfChanged: (inst) => {
// [ts] if (!inst) {
// [ts] return false;
// [ts] }
// [ts] let tracker = getTracker(inst);
// [ts] if (!tracker) {
// [ts] inputValueTracking.track(inst);
// [ts] return true;
// [ts] }
// [ts] let lastValue = tracker.getValue();
// [ts] let nextValue = getValueFromNode(ReactDOMComponentTree.getNodeFromInstance(inst));
// [ts] if (nextValue != lastValue) {
// [ts] tracker.setValue(nextValue);
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] },
// [ts] stopTracking: (inst) => {
// [ts] let tracker = getTracker(inst);
// [ts] if (tracker) {
// [ts] tracker.stopTracking();
// [ts] }
// [ts] }
// [ts] };
// [ts] module.exports = inputValueTracking;
// [ts] },{"34":34}],121:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126),
// [ts] _assign = _dereq_(160);
// [ts] let ReactCompositeComponent = _dereq_(30);
// [ts] let ReactEmptyComponent = _dereq_(54);
// [ts] let ReactHostComponent = _dereq_(59);
// [ts] let getNextDebugID = _dereq_(136);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// To avoid a cyclic dependency, we create the final class in this module
// [ts] let ReactCompositeComponentWrapper = (element) => {
// [ts] _.construct(element);
// [ts] };
// [ts] fn getDeclarationErrorAddendum(owner: auto) {
// [ts] if (owner) {
// [ts] let name = owner.getName();
// [ts] if (name) {
// [ts] return " Check the render method of `" + name + "`.";
// [ts] }
// [ts] }
// [ts] return "";
// [ts] }
// [ts] /**
// [ts] * Check if the type reference is a known internal type. I.e. not a user
// [ts] * provided composite type.
// [ts] *
// [ts] * @param {function} type
// [ts] * @return {boolean} Returns true if this is a valid internal type.
// [ts] */
// [ts] fn isInternalComponentType(type: auto) {
// [ts] return @typeName(type) == "function" && @typeName(type.prototype) != "undefined" && @typeName(type.prototype.mountComponent) == "function" && @typeName(type.prototype.receiveComponent) == "function";
// [ts] }
// [ts] /**
// [ts] * Given a ReactNode, create an instance that will actually be mounted.
// [ts] *
// [ts] * @param {ReactNode} node
// [ts] * @param {boolean} shouldHaveDebugID
// [ts] * @return {object} A new instance of the element's constructor.
// [ts] * @protected
// [ts] */
// [ts] fn instantiateReactComponent(node: auto, shouldHaveDebugID: auto) {
// [ts] let instance;
// [ts] if (node == null || node == false) {
// [ts] instance = ReactEmptyComponent.create(instantiateReactComponent);
// [ts] } else if (@typeName(node) == "object") {
// [ts] let element = node;
// [ts] let type = element.type;
// [ts] if (@typeName(type) != "function" && @typeName(type) != "string") {
// [ts] let info = "";
// [ts] if ("development" != "production") {
// [ts] if (type == null || @typeName(type) == "object" && type != null && Object.keys(type).length == 0) {
// [ts] info += " You likely forgot to export your component from the file " + "it's defined in.";
// [ts] }
// [ts] }
// [ts] info += getDeclarationErrorAddendum(element._owner);
// [ts] if (!false) if ("development" != "production") invariant(if (false, "Element type is invalid: expected a string (for built-in components) or a class/function (for composite components) but got: %s.%s", type == null) type else @typeName(type), info) else _prodInvariant(if ("130", type == null) type else @typeName(type), info) else void 0;
// [ts] }
// Special case string values
// [ts] if (@typeName(element.type) == "string") {
// [ts] instance = ReactHostComponent.createInternalComponent(element);
// [ts] } else if (isInternalComponentType(element.type)) {
// This is temporarily available for custom components that are not string
// representations. I.e. ART. Once those are updated to use the string
// representation, we can drop this code path.
// [ts] instance = element.type.init(element);
// We renamed this. Allow the old name for compat. :(
// [ts] if (!instance.getHostNode) {
// [ts] instance.getHostNode = instance.getNativeNode;
// [ts] }
// [ts] } else {
// [ts] instance = ReactCompositeComponentWrapper.init(element);
// [ts] }
// [ts] } else if (@typeName(node) == "string" || @typeName(node) == "number") {
// [ts] instance = ReactHostComponent.createInstanceForText(node);
// [ts] } else {
// [ts] if (!false) if ("development" != "production") invariant(false, "Encountered invalid React node of type %s", @typeName(node)) else _prodInvariant("131", @typeName(node)) else void 0;
// [ts] }
// [ts] if ("development" != "production") {
// [ts] if ("development" != "production") warning(@typeName(instance.mountComponent) == "function" && @typeName(instance.receiveComponent) == "function" && @typeName(instance.getHostNode) == "function" && @typeName(instance.unmountComponent) == "function", "Only React Components can be mounted.") else void 0;
// [ts] }
// These two fields are used by the DOM and ART diffing algorithms
// respectively. Instead of using expandos on components, we should be
// storing the state needed by the diffing algorithms elsewhere.
// [ts] instance._mountIndex = 0;
// [ts] instance._mountImage = null;
// [ts] if ("development" != "production") {
// [ts] instance._debugID = if (shouldHaveDebugID) getNextDebugID() else 0;
// [ts] }
// Internal instances should fully constructed at this point, so they should
// not get any new fields added to them at this point.
// [ts] if ("development" != "production") {
// [ts] if (Object.preventExtensions) {
// [ts] Object.preventExtensions(instance);
// [ts] }
// [ts] }
// [ts] return instance;
// [ts] }
// [ts] _assign(ReactCompositeComponentWrapper.prototype, ReactCompositeComponent, {
// [ts] _instantiateReactComponent: instantiateReactComponent
// [ts] });
// [ts] module.exports = instantiateReactComponent;
// [ts] },{"126":126,"136":136,"152":152,"159":159,"160":160,"30":30,"54":54,"59":59}],122:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let useHasFeature;
// [ts] if (ExecutionEnvironment.canUseDOM) {
// [ts] useHasFeature = document.implementation && document.implementation.hasFeature &&
// always returns true in newer browsers as per the standard.
// @see http://dom.spec.whatwg.org/#dom-domimplementation-hasfeature
// [ts] document.implementation.hasFeature("", "") != true;
// [ts] }
// [ts] /**
// [ts] * Checks if an event is supported in the current execution environment.
// [ts] *
// [ts] * NOTE: This will not work correctly for non-generic events such as `change`,
// [ts] * `reset`, `load`, `error`, and `select`.
// [ts] *
// [ts] * Borrows from Modernizr.
// [ts] *
// [ts] * @param {string} eventNameSuffix Event name, e.g. "click".
// [ts] * @param {?boolean} capture Check if the capture phase is supported.
// [ts] * @return {boolean} True if the event is supported.
// [ts] * @internal
// [ts] * @license Modernizr 3.0.0pre (Custom Build) | MIT
// [ts] */
// [ts] fn isEventSupported(eventNameSuffix: auto, capture: auto) {
// [ts] if (!ExecutionEnvironment.canUseDOM || capture && !("addEventListener" == document)) {
// [ts] return false;
// [ts] }
// [ts] let eventName = "on" + eventNameSuffix;
// [ts] let isSupported = eventName == document;
// [ts] if (!isSupported) {
// [ts] let element = document.createElement("div");
// [ts] element.setAttribute(eventName, "return;");
// [ts] isSupported = @typeName(element[eventName]) == "function";
// [ts] }
// [ts] if (!isSupported && useHasFeature && eventNameSuffix == "wheel") {
// This is the only way to test support for the `wheel` event in IE9+.
// [ts] isSupported = document.implementation.hasFeature("Events.wheel", "3.0");
// [ts] }
// [ts] return isSupported;
// [ts] }
// [ts] module.exports = isEventSupported;
// [ts] },{"138":138}],123:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * @see http://www.whatwg.org/specs/web-apps/current-work/multipage/the-input-element.html#input-type-attr-summary
// [ts] */
// [ts] let supportedInputTypes = {
// [ts] color: true,
// [ts] date: true,
// [ts] datetime: true,
// [ts] "datetime-local": true,
// [ts] email: true,
// [ts] month: true,
// [ts] number: true,
// [ts] password: true,
// [ts] range: true,
// [ts] search: true,
// [ts] tel: true,
// [ts] text: true,
// [ts] time: true,
// [ts] url: true,
// [ts] week: true
// [ts] };
// [ts] fn isTextInputElement(elem: auto) {
// [ts] let nodeName = elem && elem.nodeName && elem.nodeName.toLowerCase();
// [ts] if (nodeName == "input") {
// [ts] return !!supportedInputTypes[elem.type];
// [ts] }
// [ts] if (nodeName == "textarea") {
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// [ts] module.exports = isTextInputElement;
// [ts] },{}],124:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2014-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Forked from fbjs/warning:
// [ts] * https://github.com/facebook/fbjs/blob/e66ba20ad5be433eb54423f2b097d829324d9de6/packages/fbjs/src/__forks__/warning.js
// [ts] *
// [ts] * Only change is we use console.warn instead of console.error,
// [ts] * and do nothing when 'console' is not supported.
// [ts] * This really simplifies the code.
// [ts] * ---
// [ts] * Similar to invariant but only logs a warning if the condition is not met.
// [ts] * This can be used to log issues in development environments in critical
// [ts] * paths. Removing the logging code for production environments will keep the
// [ts] * same logic and follow the same code paths.
// [ts] */
// [ts] let lowPriorityWarning = () => {};
// [ts] if ("development" != "production") {
// [ts] let printWarning = (format) => {
// [ts] let _len = arguments.length, args = Array(if (_len > 1) _len - 1 else 0), _key = 1;
// [ts] while (_key < _len) { // [cap-migrate] for-step: _key++
// [ts] args[_key - 1] = arguments[_key];
// [ts] }
// [ts] let argIndex = 0;
// [ts] let message = "Warning: " + format.replace("%s", () => {
// [ts] return args[argIndex++];
// [ts] });
// [ts] if (@typeName(console) != "undefined") {
// [ts] @print(message);
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// --- Welcome to debugging React ---
// This error was thrown as a convenience so that you can use this stack
// to find the callsite that caused this warning to fire.
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(message);)
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
// [ts] };
// [ts] lowPriorityWarning = (condition, format) => {
// [ts] if (format == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('`warning(condition, format, ...args)` requires a warning ' + 'message argument');)
// [ts] }
// [ts] if (!condition) {
// [ts] let _len2 = arguments.length, args = Array(if (_len2 > 2) _len2 - 2 else 0), _key2 = 2;
// [ts] while (_key2 < _len2) { // [cap-migrate] for-step: _key2++
// [ts] args[_key2 - 2] = arguments[_key2];
// [ts] }
// [ts] printWarning.apply(null, [format].concat(args));
// [ts] }
// [ts] };
// [ts] }
// [ts] module.exports = lowPriorityWarning;
// [ts] },{}],125:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let escapeTextContentForBrowser = _dereq_(107);
// [ts] /**
// [ts] * Escapes attribute value to prevent scripting attacks.
// [ts] *
// [ts] * @param {*} value Value to escape.
// [ts] * @return {string} An escaped string.
// [ts] */
// [ts] fn quoteAttributeValueForBrowser(value: auto) {
// [ts] return "\"" + escapeTextContentForBrowser(value) + "\"";
// [ts] }
// [ts] module.exports = quoteAttributeValueForBrowser;
// [ts] },{"107":107}],126:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * WARNING: DO NOT manually require this module.
// [ts] * This is a replacement for `invariant(...)` used by the error code system
// [ts] * and will _only_ be required by the corresponding babel pass.
// [ts] * It always throws.
// [ts] */
// [ts] fn reactProdInvariant(code: auto) {
// [ts] let argCount = arguments.length - 1;
// [ts] let message = "Minified React error #" + code + "; visit " + "http://facebook.github.io/react/docs/error-decoder.html?invariant=" + code;
// [ts] let argIdx = 0;
// [ts] while (argIdx < argCount) { // [cap-migrate] for-step: argIdx++
// [ts] message += "&args[]=" + encodeURIComponent(arguments[argIdx + 1]);
// [ts] }
// [ts] message += " for the full message or use the non-minified dev environment" + " for full errors and additional helpful warnings.";
// [ts] let error = Error.init(message);
// [ts] error.name = "Invariant Violation";
// [ts] error.framesToPop = 1; // we don't care about reactProdInvariant's own frame
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] }
// [ts] module.exports = reactProdInvariant;
// [ts] },{}],127:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ReactMount = _dereq_(67);
// [ts] module.exports = ReactMount.renderSubtreeIntoContainer;
// [ts] },{"67":67}],128:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let DOMNamespaces = _dereq_(10);
// [ts] let WHITESPACE_TEST = "^[ \\r\\n\\t\\f]";
// [ts] let NONVISIBLE_TEST = "<(!--|link|noscript|meta|script|style)[ \\r\\n\\t\\f\\/>]";
// [ts] let createMicrosoftUnsafeLocalFunction = _dereq_(105);
// SVG temp container for IE lacking innerHTML
// [ts] let reusableSVGContainer;
// [ts] /**
// [ts] * Set the innerHTML property of a node, ensuring that whitespace is preserved
// [ts] * even in IE8.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {string} html
// [ts] * @internal
// [ts] */
// [ts] let setInnerHTML = createMicrosoftUnsafeLocalFunction((node, html) => {
// IE does not have innerHTML for SVG nodes, so instead we inject the
// new markup in a temp node and then move the child nodes across into
// the target node
// [ts] if (node.namespaceURI == DOMNamespaces.svg && !("innerHTML" == node)) {
// [ts] reusableSVGContainer = reusableSVGContainer || document.createElement("div");
// [ts] reusableSVGContainer.innerHTML = "<svg>" + html + "</svg>";
// [ts] let svgNode = reusableSVGContainer.firstChild;
// [ts] while (svgNode.firstChild) {
// [ts] node.appendChild(svgNode.firstChild);
// [ts] }
// [ts] } else {
// [ts] node.innerHTML = html;
// [ts] }
// [ts] });
// [ts] if (ExecutionEnvironment.canUseDOM) {
// IE8: When updating a just created node with innerHTML only leading
// whitespace is removed. When updating an existing node with innerHTML
// whitespace in root TextNodes is also collapsed.
// @see quirksmode.org/bugreports/archives/2004/11/innerhtml_and_t.html
// Feature detection; only IE8 is known to behave improperly like this.
// [ts] let testElement = document.createElement("div");
// [ts] testElement.innerHTML = " ";
// [ts] if (testElement.innerHTML == "") {
// [ts] setInnerHTML = (node, html) => {
// Magic theory: IE8 supposedly differentiates between added and updated
// nodes when processing innerHTML, innerHTML on updated nodes suffers
// from worse whitespace behavior. Re-adding a node like this triggers
// the initial and more favorable whitespace behavior.
// TODO: What to do on a detached node?
// [ts] if (node.parentNode) {
// [ts] node.parentNode.replaceChild(node, node);
// [ts] }
// We also implement a workaround for non-visible tags disappearing into
// thin air on IE8, this only happens if there is no visible text
// in-front of the non-visible tags. Piggyback on the whitespace fix
// and simply check if any non-visible tags appear in the source.
// [ts] if (WHITESPACE_TEST.test(html) || html[0] == "<" && NONVISIBLE_TEST.test(html)) {
// Recover leading whitespace by temporarily prepending any character.
// \uFEFF has the potential advantage of being zero-width/invisible.
// UglifyJS drops U+FEFF chars when parsing, so use String.fromCharCode
// in hopes that this is preserved even if "\uFEFF" is transformed to
// the actual Unicode character (by Babel, for example).
// https://github.com/mishoo/UglifyJS2/blob/v2.4.20/lib/parse.js#L216
// [ts] node.innerHTML = String.fromCharCode(0xfeff) + html;
// deleteData leaves an empty `TextNode` which offsets the index of all
// children. Definitely want to avoid this.
// [ts] let textNode = node.firstChild;
// [ts] if (textNode.data.length == 1) {
// [ts] node.removeChild(textNode);
// [ts] } else {
// [ts] textNode.deleteData(0, 1);
// [ts] }
// [ts] } else {
// [ts] node.innerHTML = html;
// [ts] }
// [ts] };
// [ts] }
// [ts] testElement = null;
// [ts] }
// [ts] module.exports = setInnerHTML;
// [ts] },{"10":10,"105":105,"138":138}],129:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let escapeTextContentForBrowser = _dereq_(107);
// [ts] let setInnerHTML = _dereq_(128);
// [ts] /**
// [ts] * Set the textContent property of a node, ensuring that whitespace is preserved
// [ts] * even in IE8. innerText is a poor substitute for textContent and, among many
// [ts] * issues, inserts <br> instead of the literal newline chars. innerHTML behaves
// [ts] * as it should.
// [ts] *
// [ts] * @param {DOMElement} node
// [ts] * @param {string} text
// [ts] * @internal
// [ts] */
// [ts] let setTextContent = (node, text) => {
// [ts] if (text) {
// [ts] let firstChild = node.firstChild;
// [ts] if (firstChild && firstChild == node.lastChild && firstChild.nodeType == 3) {
// [ts] firstChild.nodeValue = text;
// [ts] return;
// [ts] }
// [ts] }
// [ts] node.textContent = text;
// [ts] };
// [ts] if (ExecutionEnvironment.canUseDOM) {
// [ts] if (!("textContent" == document.documentElement)) {
// [ts] setTextContent = (node, text) => {
// [ts] if (node.nodeType == 3) {
// [ts] node.nodeValue = text;
// [ts] return;
// [ts] }
// [ts] setInnerHTML(node, escapeTextContentForBrowser(text));
// [ts] };
// [ts] }
// [ts] }
// [ts] module.exports = setTextContent;
// [ts] },{"107":107,"128":128,"138":138}],130:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Given a `prevElement` and `nextElement`, determines if the existing
// [ts] * instance should be updated as opposed to being destroyed or replaced by a new
// [ts] * instance. Both arguments are elements. This ensures that this logic can
// [ts] * operate on stateless trees without any backing instance.
// [ts] *
// [ts] * @param {?object} prevElement
// [ts] * @param {?object} nextElement
// [ts] * @return {boolean} True if the existing instance should be updated.
// [ts] * @protected
// [ts] */
// [ts] fn shouldUpdateReactComponent(prevElement: auto, nextElement: auto) {
// [ts] let prevEmpty = prevElement == null || prevElement == false;
// [ts] let nextEmpty = nextElement == null || nextElement == false;
// [ts] if (prevEmpty || nextEmpty) {
// [ts] return prevEmpty == nextEmpty;
// [ts] }
// [ts] let prevType = @typeName(prevElement);
// [ts] let nextType = @typeName(nextElement);
// [ts] if (prevType == "string" || prevType == "number") {
// [ts] return nextType == "string" || nextType == "number";
// [ts] } else {
// [ts] return nextType == "object" && prevElement.type == nextElement.type && prevElement.key == nextElement.key;
// [ts] }
// [ts] }
// [ts] module.exports = shouldUpdateReactComponent;
// [ts] },{}],131:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _prodInvariant = _dereq_(126);
// [ts] let ReactCurrentOwner = _dereq_(134);
// [ts] let REACT_ELEMENT_TYPE = _dereq_(53);
// [ts] let getIteratorFn = _dereq_(116);
// [ts] let invariant = _dereq_(152);
// [ts] let KeyEscapeUtils = _dereq_(23);
// [ts] let warning = _dereq_(159);
// [ts] let SEPARATOR = ".";
// [ts] let SUBSEPARATOR = ":";
// [ts] /**
// [ts] * This is inlined from ReactElement since this file is shared between
// [ts] * isomorphic and renderers. We could extract this to a
// [ts] *
// [ts] */
// [ts] /**
// [ts] * TODO: Test that a single child and an array with one item have the same key
// [ts] * pattern.
// [ts] */
// [ts] let didWarnAboutMaps = false;
// [ts] /**
// [ts] * Generate a key string that identifies a component within a set.
// [ts] *
// [ts] * @param {*} component A component that could contain a manual key.
// [ts] * @param {number} index Index that is used if a manual key is not provided.
// [ts] * @return {string}
// [ts] */
// [ts] fn getComponentKey(component: auto, index: auto) {
// Do some typechecking here since we call this blindly. We want to ensure
// that we don't block potential future ES APIs.
// [ts] if (component && @typeName(component) == "object" && component.key != null) {
// Explicit key
// [ts] return KeyEscapeUtils.escape(component.key);
// [ts] }
// Implicit key determined by the index in the set
// [ts] return index.toString(36);
// [ts] }
// [ts] /**
// [ts] * @param {?*} children Children tree container.
// [ts] * @param {!string} nameSoFar Name of the key path so far.
// [ts] * @param {!function} callback Callback to invoke with each child found.
// [ts] * @param {?*} traverseContext Used to pass information throughout the traversal
// [ts] * process.
// [ts] * @return {!number} The number of children in this subtree.
// [ts] */
// [ts] fn traverseAllChildrenImpl(children: auto, nameSoFar: auto, callback: auto, traverseContext: auto) {
// [ts] let type = @typeName(children);
// [ts] if (type == "undefined" || type == "boolean") {
// All of the above are perceived as null.
// [ts] children = null;
// [ts] }
// [ts] if (children == null || type == "string" || type == "number" ||
// The following is inlined from ReactElement. This means we can optimize
// some checks. React Fiber also inlines this logic for similar purposes.
// [ts] type == "object" && children.$$typeof == REACT_ELEMENT_TYPE) {
// [ts] callback(traverseContext, children,
// If it's the only child, treat the name as if it was wrapped in an array
// so that it's consistent if the number of children grows.
// [ts] if (nameSoFar == "") SEPARATOR + getComponentKey(children, 0) else nameSoFar);
// [ts] return 1;
// [ts] }
// [ts] let child;
// [ts] let nextName;
// [ts] let subtreeCount = 0; // Count of children found in the current subtree.
// [ts] let nextNamePrefix = if (nameSoFar == "") SEPARATOR else nameSoFar + SUBSEPARATOR;
// [ts] if (Array.isArray(children)) {
// [ts] let i = 0;
// [ts] while (i < children.length) { // [cap-migrate] for-step: i++
// [ts] child = children[i];
// [ts] nextName = nextNamePrefix + getComponentKey(child, i);
// [ts] subtreeCount += traverseAllChildrenImpl(child, nextName, callback, traverseContext);
// [ts] }
// [ts] } else {
// [ts] let iteratorFn = getIteratorFn(children);
// [ts] if (iteratorFn) {
// [ts] let iterator = iteratorFn.call(children);
// [ts] let step;
// [ts] if (iteratorFn != children.entries) {
// [ts] let ii = 0;
// [ts] while (!(step = iterator.next()).done) {
// [ts] child = step.value;
// [ts] nextName = nextNamePrefix + getComponentKey(child, ii++);
// [ts] subtreeCount += traverseAllChildrenImpl(child, nextName, callback, traverseContext);
// [ts] }
// [ts] } else {
// [ts] if ("development" != "production") {
// [ts] let mapsAsChildrenAddendum = "";
// [ts] if (ReactCurrentOwner.current) {
// [ts] let mapsAsChildrenOwnerName = ReactCurrentOwner.current.getName();
// [ts] if (mapsAsChildrenOwnerName) {
// [ts] mapsAsChildrenAddendum = " Check the render method of `" + mapsAsChildrenOwnerName + "`.";
// [ts] }
// [ts] }
// [ts] if ("development" != "production") warning(didWarnAboutMaps, "Using Maps as children is not yet fully supported. It is an " + "experimental feature that might be removed. Convert it to a " + "sequence / iterable of keyed ReactElements instead.%s", mapsAsChildrenAddendum) else void 0;
// [ts] didWarnAboutMaps = true;
// [ts] }
// Iterator will provide entry [k,v] tuples rather than values.
// [ts] while (!(step = iterator.next()).done) {
// [ts] let entry = step.value;
// [ts] if (entry) {
// [ts] child = entry[1];
// [ts] nextName = nextNamePrefix + KeyEscapeUtils.escape(entry[0]) + SUBSEPARATOR + getComponentKey(child, 0);
// [ts] subtreeCount += traverseAllChildrenImpl(child, nextName, callback, traverseContext);
// [ts] }
// [ts] }
// [ts] }
// [ts] } else if (type == "object") {
// [ts] let addendum = "";
// [ts] if ("development" != "production") {
// [ts] addendum = " If you meant to render a collection of children, use an array " + "instead or wrap the object using createFragment(object) from the " + "React add-ons.";
// [ts] if (children._isReactElement) {
// [ts] addendum = " It looks like you're using an element created by a different " + "version of React. Make sure to use only one copy of React.";
// [ts] }
// [ts] if (ReactCurrentOwner.current) {
// [ts] let name = ReactCurrentOwner.current.getName();
// [ts] if (name) {
// [ts] addendum += " Check the render method of `" + name + "`.";
// [ts] }
// [ts] }
// [ts] }
// [ts] let childrenString = String(children);
// [ts] if (!false) if ("development" != "production") invariant(if (false, "Objects are not valid as a React child (found: %s).%s", childrenString == "[object Object]") "object with keys {" + Object.keys(children).join(", ") + "}" else childrenString, addendum) else _prodInvariant(if ("31", childrenString == "[object Object]") "object with keys {" + Object.keys(children).join(", ") + "}" else childrenString, addendum) else void 0;
// [ts] }
// [ts] }
// [ts] return subtreeCount;
// [ts] }
// [ts] /**
// [ts] * Traverses children that are typically specified as `props.children`, but
// [ts] * might also be specified through attributes:
// [ts] *
// [ts] * - `traverseAllChildren(this.props.children, ...)`
// [ts] * - `traverseAllChildren(this.props.leftPanelChildren, ...)`
// [ts] *
// [ts] * The `traverseContext` is an optional argument that is passed through the
// [ts] * entire traversal. It can be used to store accumulations or anything else that
// [ts] * the callback might find relevant.
// [ts] *
// [ts] * @param {?*} children Children tree object.
// [ts] * @param {!function} callback To invoke upon traversing each child.
// [ts] * @param {?*} traverseContext Context for traversal.
// [ts] * @return {!number} The number of children in this subtree.
// [ts] */
// [ts] fn traverseAllChildren(children: auto, callback: auto, traverseContext: auto) {
// [ts] if (children == null) {
// [ts] return 0;
// [ts] }
// [ts] return traverseAllChildrenImpl(children, "", callback, traverseContext);
// [ts] }
// [ts] module.exports = traverseAllChildren;
// [ts] },{"116":116,"126":126,"134":134,"152":152,"159":159,"23":23,"53":53}],132:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2015-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let _assign = _dereq_(160);
// [ts] let emptyFunction = _dereq_(144);
// [ts] let warning = _dereq_(159);
// [ts] let validateDOMNesting = emptyFunction;
// [ts] if ("development" != "production") {
// This validation code was written based on the HTML5 parsing spec:
// https://html.spec.whatwg.org/multipage/syntax.html#has-an-element-in-scope
//
// Note: this does not catch all invalid nesting, nor does it try to (as it's
// not clear what practical benefit doing so provides); instead, we warn only
// for cases where the parser will give a parse tree differing from what React
// intended. For example, <b><div></div></b> is invalid but we don't warn
// because it still parses correctly; we do warn for other cases like nested
// <p> tags where the beginning of the second element implicitly closes the
// first, causing a confusing mess.
// https://html.spec.whatwg.org/multipage/syntax.html#special
// [ts] let specialTags = ["address", "applet", "area", "article", "aside", "base", "basefont", "bgsound", "blockquote", "body", "br", "button", "caption", "center", "col", "colgroup", "dd", "details", "dir", "div", "dl", "dt", "embed", "fieldset", "figcaption", "figure", "footer", "form", "frame", "frameset", "h1", "h2", "h3", "h4", "h5", "h6", "head", "header", "hgroup", "hr", "html", "iframe", "img", "input", "isindex", "li", "link", "listing", "main", "marquee", "menu", "menuitem", "meta", "nav", "noembed", "noframes", "noscript", "object", "ol", "p", "param", "plaintext", "pre", "script", "section", "select", "source", "style", "summary", "table", "tbody", "td", "template", "textarea", "tfoot", "th", "thead", "title", "tr", "track", "ul", "wbr", "xmp"];
// https://html.spec.whatwg.org/multipage/syntax.html#has-an-element-in-scope
// [ts] let inScopeTags = ["applet", "caption", "html", "table", "td", "th", "marquee", "object", "template",
// https://html.spec.whatwg.org/multipage/syntax.html#html-integration-point
// TODO: Distinguish by namespace here -- for <title>, including it here
// errs on the side of fewer warnings
// [ts] "foreignObject", "desc", "title"];
// https://html.spec.whatwg.org/multipage/syntax.html#has-an-element-in-button-scope
// [ts] let buttonScopeTags = inScopeTags.concat(["button"]);
// https://html.spec.whatwg.org/multipage/syntax.html#generate-implied-end-tags
// [ts] let impliedEndTags = ["dd", "dt", "li", "option", "optgroup", "p", "rp", "rt"];
// [ts] let emptyAncestorInfo = {
// [ts] current: null,
// [ts] formTag: null,
// [ts] aTagInScope: null,
// [ts] buttonTagInScope: null,
// [ts] nobrTagInScope: null,
// [ts] pTagInButtonScope: null,
// [ts] listItemTagAutoclosing: null,
// [ts] dlItemTagAutoclosing: null
// [ts] };
// [ts] let updatedAncestorInfo = (oldInfo, tag, instance) => {
// [ts] let ancestorInfo = _assign({}, oldInfo || emptyAncestorInfo);
// [ts] let info = { tag: tag, instance: instance };
// [ts] if (inScopeTags.indexOf(tag) != -1) {
// [ts] ancestorInfo.aTagInScope = null;
// [ts] ancestorInfo.buttonTagInScope = null;
// [ts] ancestorInfo.nobrTagInScope = null;
// [ts] }
// [ts] if (buttonScopeTags.indexOf(tag) != -1) {
// [ts] ancestorInfo.pTagInButtonScope = null;
// [ts] }
// See rules for 'li', 'dd', 'dt' start tags in
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-inbody
// [ts] if (specialTags.indexOf(tag) != -1 && tag != "address" && tag != "div" && tag != "p") {
// [ts] ancestorInfo.listItemTagAutoclosing = null;
// [ts] ancestorInfo.dlItemTagAutoclosing = null;
// [ts] }
// [ts] ancestorInfo.current = info;
// [ts] if (tag == "form") {
// [ts] ancestorInfo.formTag = info;
// [ts] }
// [ts] if (tag == "a") {
// [ts] ancestorInfo.aTagInScope = info;
// [ts] }
// [ts] if (tag == "button") {
// [ts] ancestorInfo.buttonTagInScope = info;
// [ts] }
// [ts] if (tag == "nobr") {
// [ts] ancestorInfo.nobrTagInScope = info;
// [ts] }
// [ts] if (tag == "p") {
// [ts] ancestorInfo.pTagInButtonScope = info;
// [ts] }
// [ts] if (tag == "li") {
// [ts] ancestorInfo.listItemTagAutoclosing = info;
// [ts] }
// [ts] if (tag == "dd" || tag == "dt") {
// [ts] ancestorInfo.dlItemTagAutoclosing = info;
// [ts] }
// [ts] return ancestorInfo;
// [ts] };
// [ts] /**
// [ts] * Returns whether
// [ts] */
// [ts] let isTagValidWithParent = (tag, parentTag) => {
// First, let's check if we're in an unusual parsing mode...
// [ts] switch (parentTag) {
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-inselect
// [ts] "select" => {
// [ts] return tag == "option" || tag == "optgroup" || tag == "#text";
// [ts] },
// [ts] "optgroup" => {
// [ts] return tag == "option" || tag == "#text";
// Strictly speaking, seeing an <option> doesn't mean we're in a <select>
// but
// [ts] },
// [ts] "option" => {
// [ts] return tag == "#text";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-intd
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-incaption
// No special behavior since these rules fall back to "in body" mode for
// all except special table nodes which cause bad parsing behavior anyway.
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-intr
// [ts] },
// [ts] "tr" => {
// [ts] return tag == "th" || tag == "td" || tag == "style" || tag == "script" || tag == "template";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-intbody
// [ts] },
// [ts] "tbody", "thead", "tfoot" => {
// [ts] return tag == "tr" || tag == "style" || tag == "script" || tag == "template";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-incolgroup
// [ts] },
// [ts] "colgroup" => {
// [ts] return tag == "col" || tag == "template";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-intable
// [ts] },
// [ts] "table" => {
// [ts] return tag == "caption" || tag == "colgroup" || tag == "tbody" || tag == "tfoot" || tag == "thead" || tag == "style" || tag == "script" || tag == "template";
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-inhead
// [ts] },
// [ts] "head" => {
// [ts] return tag == "base" || tag == "basefont" || tag == "bgsound" || tag == "link" || tag == "meta" || tag == "title" || tag == "noscript" || tag == "noframes" || tag == "style" || tag == "script" || tag == "template";
// https://html.spec.whatwg.org/multipage/semantics.html#the-html-element
// [ts] },
// [ts] "html" => {
// [ts] return tag == "head" || tag == "body";
// [ts] },
// [ts] "#document" => {
// [ts] return tag == "html";
// [ts] }
// [ts] }
// Probably in the "in body" parsing mode, so we outlaw only tag combos
// where the parsing rules cause implicit opens or closes to be added.
// https://html.spec.whatwg.org/multipage/syntax.html#parsing-main-inbody
// [ts] switch (tag) {
// [ts] "h1", "h2", "h3", "h4", "h5", "h6" => {
// [ts] return parentTag != "h1" && parentTag != "h2" && parentTag != "h3" && parentTag != "h4" && parentTag != "h5" && parentTag != "h6";
// [ts] },
// [ts] "rp", "rt" => {
// [ts] return impliedEndTags.indexOf(parentTag) == -1;
// [ts] },
// These tags are only valid with a few parents that have special child
// parsing rules -- if we're down here, then none of those matched and
// so we allow it only if we don't know what the parent is, as all other
// cases are invalid.
// [ts] "body", "caption", "col", "colgroup", "frame", "head", "html", "tbody", "td", "tfoot", "th", "thead", "tr" => {
// [ts] return parentTag == null;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] };
// [ts] /**
// [ts] * Returns whether
// [ts] */
// [ts] let findInvalidAncestorForTag = (tag, ancestorInfo) => {
// [ts] switch (tag) {
// [ts] "address", "article", "aside", "blockquote", "center", "details", "dialog", "dir", "div", "dl", "fieldset", "figcaption", "figure", "footer", "header", "hgroup", "main", "menu", "nav", "ol", "p", "section", "summary", "ul", "pre", "listing", "table", "hr", "xmp", "h1", "h2", "h3", "h4", "h5", "h6" => {
// [ts] return ancestorInfo.pTagInButtonScope;
// [ts] },
// [ts] "form" => {
// [ts] return ancestorInfo.formTag || ancestorInfo.pTagInButtonScope;
// [ts] },
// [ts] "li" => {
// [ts] return ancestorInfo.listItemTagAutoclosing;
// [ts] },
// [ts] "dd", "dt" => {
// [ts] return ancestorInfo.dlItemTagAutoclosing;
// [ts] },
// [ts] "button" => {
// [ts] return ancestorInfo.buttonTagInScope;
// [ts] },
// Spec says something about storing a list of markers, but it sounds
// equivalent to this check.
// [ts] "a" => {
// [ts] return ancestorInfo.aTagInScope;
// [ts] },
// [ts] "nobr" => {
// [ts] return ancestorInfo.nobrTagInScope;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] };
// [ts] /**
// [ts] * Given a ReactCompositeComponent instance, return a list of its recursive
// [ts] * owners, starting at the root and ending with the instance itself.
// [ts] */
// [ts] let findOwnerStack = (instance) => {
// [ts] if (!instance) {
// [ts] return [];
// [ts] }
// [ts] let stack = [];
// [ts] do {
// [ts] stack.push(instance);
// [ts] } while (instance = instance._currentElement._owner);
// [ts] stack.reverse();
// [ts] return stack;
// [ts] };
// [ts] let didWarn = {};
// [ts] validateDOMNesting = (childTag, childText, childInstance, ancestorInfo) => {
// [ts] ancestorInfo = ancestorInfo || emptyAncestorInfo;
// [ts] let parentInfo = ancestorInfo.current;
// [ts] let parentTag = parentInfo && parentInfo.tag;
// [ts] if (childText != null) {
// [ts] if ("development" != "production") warning(childTag == null, "validateDOMNesting: when childText is passed, childTag should be null") else void 0;
// [ts] childTag = "#text";
// [ts] }
// [ts] let invalidParent = if (isTagValidWithParent(childTag, parentTag)) null else parentInfo;
// [ts] let invalidAncestor = if (invalidParent) null else findInvalidAncestorForTag(childTag, ancestorInfo);
// [ts] let problematic = invalidParent || invalidAncestor;
// [ts] if (problematic) {
// [ts] let ancestorTag = problematic.tag;
// [ts] let ancestorInstance = problematic.instance;
// [ts] let childOwner = childInstance && childInstance._currentElement._owner;
// [ts] let ancestorOwner = ancestorInstance && ancestorInstance._currentElement._owner;
// [ts] let childOwners = findOwnerStack(childOwner);
// [ts] let ancestorOwners = findOwnerStack(ancestorOwner);
// [ts] let minStackLen = Math.min(childOwners.length, ancestorOwners.length);
// [ts] let i;
// [ts] let deepestCommon = -1;
// [ts] i = 0;
// [ts] while (i < minStackLen) { // [cap-migrate] for-step: i++
// [ts] if (childOwners[i] == ancestorOwners[i]) {
// [ts] deepestCommon = i;
// [ts] } else {
// [ts] break;
// [ts] }
// [ts] }
// [ts] let UNKNOWN = "(unknown)";
// [ts] let childOwnerNames = childOwners.slice(deepestCommon + 1).map((inst) => {
// [ts] return inst.getName() || UNKNOWN;
// [ts] });
// [ts] let ancestorOwnerNames = ancestorOwners.slice(deepestCommon + 1).map((inst) => {
// [ts] return inst.getName() || UNKNOWN;
// [ts] });
// [ts] let ownerInfo = [].concat(
// If the parent and child instances have a common owner ancestor, start
// with that -- otherwise we just start with the parent's owners.
// [ts] if (deepestCommon != -1) childOwners[deepestCommon].getName() || UNKNOWN else [], ancestorOwnerNames, ancestorTag,
// If we're warning about an invalid (non-parent) ancestry, add '...'
// [ts] if (invalidAncestor) ["..."] else [], childOwnerNames, childTag).join(" > ");
// [ts] let warnKey = !!invalidParent + "|" + childTag + "|" + ancestorTag + "|" + ownerInfo;
// [ts] if (didWarn[warnKey]) {
// [ts] return;
// [ts] }
// [ts] didWarn[warnKey] = true;
// [ts] let tagDisplayName = childTag;
// [ts] let whitespaceInfo = "";
// [ts] if (childTag == "#text") {
// [ts] if ("\\S".test(childText)) {
// [ts] tagDisplayName = "Text nodes";
// [ts] } else {
// [ts] tagDisplayName = "Whitespace text nodes";
// [ts] whitespaceInfo = " Make sure you don't have any extra whitespace between tags on " + "each line of your source code.";
// [ts] }
// [ts] } else {
// [ts] tagDisplayName = "<" + childTag + ">";
// [ts] }
// [ts] if (invalidParent) {
// [ts] let info = "";
// [ts] if (ancestorTag == "table" && childTag == "tr") {
// [ts] info += " Add a <tbody> to your code to match the DOM tree generated by " + "the browser.";
// [ts] }
// [ts] if ("development" != "production") warning(false, "validateDOMNesting(...): %s cannot appear as a child of <%s>.%s " + "See %s.%s", tagDisplayName, ancestorTag, whitespaceInfo, ownerInfo, info) else void 0;
// [ts] } else {
// [ts] if ("development" != "production") warning(false, "validateDOMNesting(...): %s cannot appear as a descendant of " + "<%s>. See %s.", tagDisplayName, ancestorTag, ownerInfo) else void 0;
// [ts] }
// [ts] }
// [ts] };
// [ts] validateDOMNesting.updatedAncestorInfo = updatedAncestorInfo;
// For testing
// [ts] validateDOMNesting.isTagValidInContext = (tag, ancestorInfo) => {
// [ts] ancestorInfo = ancestorInfo || emptyAncestorInfo;
// [ts] let parentInfo = ancestorInfo.current;
// [ts] let parentTag = parentInfo && parentInfo.tag;
// [ts] return isTagValidWithParent(tag, parentTag) && !findInvalidAncestorForTag(tag, ancestorInfo);
// [ts] };
// [ts] }
// [ts] module.exports = validateDOMNesting;
// [ts] },{"144":144,"159":159,"160":160}],133:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] /* globals React */
// [ts] "use strict";
// [ts] let ReactInternals = React.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED;
// [ts] module.exports = ReactInternals.ReactComponentTreeHook;
// [ts] },{}],134:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] /* globals React */
// [ts] "use strict";
// [ts] let ReactInternals = React.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED;
// [ts] module.exports = ReactInternals.ReactCurrentOwner;
// [ts] },{}],135:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] /* globals React */
// [ts] "use strict";
// [ts] module.exports = React;
// [ts] },{}],136:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * This source code is licensed under the MIT license found in the
// [ts] * LICENSE file in the root directory of this source tree.
// [ts] *
// [ts] */
// [ts] /* globals React */
// [ts] "use strict";
// [ts] let ReactInternals = React.__SECRET_INTERNALS_DO_NOT_USE_OR_YOU_WILL_BE_FIRED;
// [ts] module.exports = ReactInternals.getNextDebugID;
// [ts] },{}],137:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] *
// [ts] * Licensed under the Apache License, Version 2.0 (the "License");
// [ts] * you may not use this file except in compliance with the License.
// [ts] * You may obtain a copy of the License at
// [ts] *
// [ts] * http://www.apache.org/licenses/LICENSE-2.0
// [ts] *
// [ts] * Unless required by applicable law or agreed to in writing, software
// [ts] * distributed under the License is distributed on an "AS IS" BASIS,
// [ts] * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// [ts] * See the License for the specific language governing permissions and
// [ts] * limitations under the License.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] let emptyFunction = _dereq_(144);
// [ts] /**
// [ts] * Upstream version of event listener. Does not take into account specific
// [ts] * nature of platform.
// [ts] */
// [ts] let EventListener = {
// [ts] /**
// [ts] * Listen to DOM events during the bubble phase.
// [ts] *
// [ts] * @param {DOMEventTarget} target DOM element to register listener on.
// [ts] * @param {string} eventType Event type, e.g. 'click' or 'mouseover'.
// [ts] * @param {function} callback Callback function.
// [ts] * @return {object} Object with a `remove` method.
// [ts] */
// [ts] listen: (target, eventType, callback) => {
// [ts] if (target.addEventListener) {
// [ts] target.addEventListener(eventType, callback, false);
// [ts] return {
// [ts] remove: () => {
// [ts] target.removeEventListener(eventType, callback, false);
// [ts] }
// [ts] };
// [ts] } else if (target.attachEvent) {
// [ts] target.attachEvent("on" + eventType, callback);
// [ts] return {
// [ts] remove: () => {
// [ts] target.detachEvent("on" + eventType, callback);
// [ts] }
// [ts] };
// [ts] }
// [ts] },
// [ts] /**
// [ts] * Listen to DOM events during the capture phase.
// [ts] *
// [ts] * @param {DOMEventTarget} target DOM element to register listener on.
// [ts] * @param {string} eventType Event type, e.g. 'click' or 'mouseover'.
// [ts] * @param {function} callback Callback function.
// [ts] * @return {object} Object with a `remove` method.
// [ts] */
// [ts] capture: (target, eventType, callback) => {
// [ts] if (target.addEventListener) {
// [ts] target.addEventListener(eventType, callback, true);
// [ts] return {
// [ts] remove: () => {
// [ts] target.removeEventListener(eventType, callback, true);
// [ts] }
// [ts] };
// [ts] } else {
// [ts] if ("development" != "production") {
// [ts] @print("Attempted to listen to events during the capture phase on a " + "browser that does not support the capture phase. Your application " + "will not receive some events.");
// [ts] }
// [ts] return {
// [ts] remove: emptyFunction
// [ts] };
// [ts] }
// [ts] },
// [ts] registerDefault: () => {}
// [ts] };
// [ts] module.exports = EventListener;
// [ts] },{"144":144}],138:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let canUseDOM = !!(@typeName(window) != "undefined" && window.document && window.document.createElement);
// [ts] /**
// [ts] * Simple, lightweight module assisting with the detection and context of
// [ts] * Worker. Helps avoid circular dependencies and allows code to reason about
// [ts] * whether or not they are in a Worker, even if they never include the main
// [ts] * `ReactWorker` dependency.
// [ts] */
// [ts] let ExecutionEnvironment = {
// [ts] canUseDOM: canUseDOM,
// [ts] canUseWorkers: @typeName(Worker) != "undefined",
// [ts] canUseEventListeners: canUseDOM && !!(window.addEventListener || window.attachEvent),
// [ts] canUseViewport: canUseDOM && !!window.screen,
// [ts] isInWorker: !canUseDOM // For now, this is true - might change in the future.
// [ts] };
// [ts] module.exports = ExecutionEnvironment;
// [ts] },{}],139:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] let _hyphenPattern = "-(.)";
// [ts] /**
// [ts] * Camelcases a hyphenated string, for example:
// [ts] *
// [ts] *   > camelize('background-color')
// [ts] *   < "backgroundColor"
// [ts] *
// [ts] * @param {string} string
// [ts] * @return {string}
// [ts] */
// [ts] fn camelize(string: auto) {
// [ts] return string.replace(_hyphenPattern, (_unused, character) => {
// [ts] return character.toUpperCase();
// [ts] });
// [ts] }
// [ts] module.exports = camelize;
// [ts] },{}],140:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] "use strict";
// [ts] let camelize = _dereq_(139);
// [ts] let msPattern = "^-ms-";
// [ts] /**
// [ts] * Camelcases a hyphenated CSS property name, for example:
// [ts] *
// [ts] *   > camelizeStyleName('background-color')
// [ts] *   < "backgroundColor"
// [ts] *   > camelizeStyleName('-moz-transition')
// [ts] *   < "MozTransition"
// [ts] *   > camelizeStyleName('-ms-transition')
// [ts] *   < "msTransition"
// [ts] *
// [ts] * As Andi Smith suggests
// [ts] * (http://www.andismith.com/blog/2012/02/modernizr-prefixed/), an `-ms` prefix
// [ts] * is converted to lowercase `ms`.
// [ts] *
// [ts] * @param {string} string
// [ts] * @return {string}
// [ts] */
// [ts] fn camelizeStyleName(string: auto) {
// [ts] return camelize(string.replace(msPattern, "ms-"));
// [ts] }
// [ts] module.exports = camelizeStyleName;
// [ts] },{"139":139}],141:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] *
// [ts] */
// [ts] let isTextNode = _dereq_(154);
// [ts] /*eslint-disable no-bitwise */
// [ts] /**
// [ts] * Checks if a given DOM node contains or is another DOM node.
// [ts] */
// [ts] fn containsNode(outerNode: auto, innerNode: auto) {
// [ts] if (!outerNode || !innerNode) {
// [ts] return false;
// [ts] } else if (outerNode == innerNode) {
// [ts] return true;
// [ts] } else if (isTextNode(outerNode)) {
// [ts] return false;
// [ts] } else if (isTextNode(innerNode)) {
// [ts] return containsNode(outerNode, innerNode.parentNode);
// [ts] } else if ("contains" == outerNode) {
// [ts] return outerNode.contains(innerNode);
// [ts] } else if (outerNode.compareDocumentPosition) {
// [ts] return !!(outerNode.compareDocumentPosition(innerNode) & 16);
// [ts] } else {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] module.exports = containsNode;
// [ts] },{"154":154}],142:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Convert array-like objects to arrays.
// [ts] *
// [ts] * This API assumes the caller knows the contents of the data type. For less
// [ts] * well defined inputs use createArrayFromMixed.
// [ts] *
// [ts] * @param {object|function|filelist} obj
// [ts] * @return {array}
// [ts] */
// [ts] fn toArray(obj: auto) {
// [ts] let length = obj.length;
// Some browsers builtin objects can report typeof 'function' (e.g. NodeList
// in old versions of Safari).
// [ts] if (!(!Array.isArray(obj) && (@typeName(obj) == "object" || @typeName(obj) == "function"))) if ("development" != "production") invariant(false, "toArray: Array-like object expected") else invariant(false) else void 0;
// [ts] if (!(@typeName(length) == "number")) if ("development" != "production") invariant(false, "toArray: Object needs a length property") else invariant(false) else void 0;
// [ts] if (!(length == 0 || length - 1 == obj)) if ("development" != "production") invariant(false, "toArray: Object should have keys for indices") else invariant(false) else void 0;
// [ts] if (!(@typeName(obj.callee) != "function")) if ("development" != "production") invariant(false, "toArray: Object can't be `arguments`. Use rest params " + "(function(...args) {}) or Array.from() instead.") else invariant(false) else void 0;
// Old IE doesn't give collections access to hasOwnProperty. Assume inputs
// without method will throw during the slice call and skip straight to the
// fallback.
// [ts] if (obj.hasOwnProperty) {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return Array.prototype.slice.call(obj);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// IE < 9 does not support Array#slice on collections objects
// [ts] }
// [ts] }
// Fall back to copying key by key. This assumes all keys have a value,
// so will not preserve sparsely populated inputs.
// [ts] let ret = Array(length);
// [ts] let ii = 0;
// [ts] while (ii < length) { // [cap-migrate] for-step: ii++
// [ts] ret[ii] = obj[ii];
// [ts] }
// [ts] return ret;
// [ts] }
// [ts] /**
// [ts] * Perform a heuristic test to determine if an object is "array-like".
// [ts] *
// [ts] *   A monk asked Joshu, a Zen master, "Has a dog Buddha nature?"
// [ts] *   Joshu replied: "Mu."
// [ts] *
// [ts] * This function determines if its argument has "array nature": it returns
// [ts] * true if the argument is an actual array, an `arguments' object, or an
// [ts] * HTMLCollection (e.g. node.childNodes or node.getElementsByTagName()).
// [ts] *
// [ts] * It will return false for other array-like objects like Filelist.
// [ts] *
// [ts] * @param {*} obj
// [ts] * @return {boolean}
// [ts] */
// [ts] fn hasArrayNature(obj: auto) {
// [ts] return (
// not null/false
// [ts] !!obj && (
// arrays are objects, NodeLists are functions in Safari
// [ts] @typeName(obj) == "object" || @typeName(obj) == "function") &&
// quacks like an array
// [ts] "length" == obj &&
// not window
// [ts] !("setInterval" == obj) &&
// no DOM node should be considered an array-like
// a 'select' element has 'length' and 'item' properties on IE8
// [ts] @typeName(obj.nodeType) != "number" && (
// a real array
// [ts] Array.isArray(obj) ||
// arguments
// [ts] "callee" == obj ||
// HTMLCollection/NodeList
// [ts] "item" == obj)
// [ts] );
// [ts] }
// [ts] /**
// [ts] * Ensure that the argument is an array by wrapping it in an array if it is not.
// [ts] * Creates a copy of the argument if it is already an array.
// [ts] *
// [ts] * This is mostly useful idiomatically:
// [ts] *
// [ts] *   var createArrayFromMixed = require('createArrayFromMixed');
// [ts] *
// [ts] *   function takesOneOrMoreThings(things) {
// [ts] *     things = createArrayFromMixed(things);
// [ts] *     ...
// [ts] *   }
// [ts] *
// [ts] * This allows you to treat `things' as an array, but accept scalars in the API.
// [ts] *
// [ts] * If you need to convert an array-like object, like `arguments`, into an array
// [ts] * use toArray instead.
// [ts] *
// [ts] * @param {*} obj
// [ts] * @return {array}
// [ts] */
// [ts] fn createArrayFromMixed(obj: auto) {
// [ts] if (!hasArrayNature(obj)) {
// [ts] return [obj];
// [ts] } else if (Array.isArray(obj)) {
// [ts] return obj.slice();
// [ts] } else {
// [ts] return toArray(obj);
// [ts] }
// [ts] }
// [ts] module.exports = createArrayFromMixed;
// [ts] },{"152":152}],143:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] /*eslint-disable fb-www/unsafe-html*/
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let createArrayFromMixed = _dereq_(142);
// [ts] let getMarkupWrap = _dereq_(148);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Dummy container used to render all markup.
// [ts] */
// [ts] let dummyNode = if (ExecutionEnvironment.canUseDOM) document.createElement("div") else null;
// [ts] /**
// [ts] * Pattern used by `getNodeName`.
// [ts] */
// [ts] let nodeNamePattern = "^\\s*<(\\w+)";
// [ts] /**
// [ts] * Extracts the `nodeName` of the first element in a string of markup.
// [ts] *
// [ts] * @param {string} markup String of markup.
// [ts] * @return {?string} Node name of the supplied markup.
// [ts] */
// [ts] fn getNodeName(markup: auto) {
// [ts] let nodeNameMatch = markup.match(nodeNamePattern);
// [ts] return nodeNameMatch && nodeNameMatch[1].toLowerCase();
// [ts] }
// [ts] /**
// [ts] * Creates an array containing the nodes rendered from the supplied markup. The
// [ts] * optionally supplied `handleScript` function will be invoked once for each
// [ts] * <script> element that is rendered. If no `handleScript` function is supplied,
// [ts] * an exception is thrown if any <script> elements are rendered.
// [ts] *
// [ts] * @param {string} markup A string of valid HTML markup.
// [ts] * @param {?function} handleScript Invoked once for each rendered <script>.
// [ts] * @return {array<DOMElement|DOMTextNode>} An array of rendered nodes.
// [ts] */
// [ts] fn createNodesFromMarkup(markup: auto, handleScript: auto) {
// [ts] let node = dummyNode;
// [ts] if (!!!dummyNode) if ("development" != "production") invariant(false, "createNodesFromMarkup dummy not initialized") else invariant(false) else void 0;
// [ts] let nodeName = getNodeName(markup);
// [ts] let wrap = nodeName && getMarkupWrap(nodeName);
// [ts] if (wrap) {
// [ts] node.innerHTML = wrap[1] + markup + wrap[2];
// [ts] let wrapDepth = wrap[0];
// [ts] while (wrapDepth--) {
// [ts] node = node.lastChild;
// [ts] }
// [ts] } else {
// [ts] node.innerHTML = markup;
// [ts] }
// [ts] let scripts = node.getElementsByTagName("script");
// [ts] if (scripts.length) {
// [ts] if (!handleScript) if ("development" != "production") invariant(false, "createNodesFromMarkup(...): Unexpected <script> element rendered.") else invariant(false) else void 0;
// [ts] createArrayFromMixed(scripts).forEach(handleScript);
// [ts] }
// [ts] let nodes = Array.from(node.childNodes);
// [ts] while (node.lastChild) {
// [ts] node.removeChild(node.lastChild);
// [ts] }
// [ts] return nodes;
// [ts] }
// [ts] module.exports = createNodesFromMarkup;
// [ts] },{"138":138,"142":142,"148":148,"152":152}],144:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] *
// [ts] */
// [ts] fn makeEmptyFunction(arg: auto) {
// [ts] return () => {
// [ts] return arg;
// [ts] };
// [ts] }
// [ts] /**
// [ts] * This function accepts and discards inputs; it has no side effects. This is
// [ts] * primarily useful idiomatically for overridable function endpoints which
// [ts] * always need to be callable, since JS lacks a null-call idiom ala Cocoa.
// [ts] */
// [ts] let emptyFunction = () => {};
// [ts] emptyFunction.thatReturns = makeEmptyFunction;
// [ts] emptyFunction.thatReturnsFalse = makeEmptyFunction(false);
// [ts] emptyFunction.thatReturnsTrue = makeEmptyFunction(true);
// [ts] emptyFunction.thatReturnsNull = makeEmptyFunction(null);
// [ts] emptyFunction.thatReturnsThis = () => {
// [ts] return this;
// [ts] };
// [ts] emptyFunction.thatReturnsArgument = (arg) => {
// [ts] return arg;
// [ts] };
// [ts] module.exports = emptyFunction;
// [ts] },{}],145:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let emptyObject = {};
// [ts] if ("development" != "production") {
// [ts] Object.freeze(emptyObject);
// [ts] }
// [ts] module.exports = emptyObject;
// [ts] },{}],146:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * @param {DOMElement} node input/textarea to focus
// [ts] */
// [ts] fn focusNode(node: auto) {
// IE8 can throw "Can't move focus to the control because it is invisible,
// not enabled, or of a type that does not accept the focus." for all kinds of
// reasons that are too expensive and fragile to test.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] node.focus();
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {})
// [ts] }
// [ts] module.exports = focusNode;
// [ts] },{}],147:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] /* eslint-disable fb-www/typeof-undefined */
// [ts] /**
// [ts] * Same as document.activeElement but wraps in a try-catch block. In IE it is
// [ts] * not safe to call document.activeElement if there is nothing focused.
// [ts] *
// [ts] * The activeElement will be null only if the document or document body is not
// [ts] * yet defined.
// [ts] *
// [ts] * @param {?DOMDocument} doc Defaults to current document.
// [ts] * @return {?DOMElement}
// [ts] */
// [ts] fn getActiveElement(doc: auto) {
// [ts] doc = doc || (if (@typeName(document) != "undefined") document else null);
// [ts] if (@typeName(doc) == "undefined") {
// [ts] return null;
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] return doc.activeElement || doc.body;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (e) {)
// [ts] return doc.body;
// [ts] }
// [ts] }
// [ts] module.exports = getActiveElement;
// [ts] },{}],148:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] */
// [ts] /*eslint-disable fb-www/unsafe-html */
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let invariant = _dereq_(152);
// [ts] /**
// [ts] * Dummy container used to detect which wraps are necessary.
// [ts] */
// [ts] let dummyNode = if (ExecutionEnvironment.canUseDOM) document.createElement("div") else null;
// [ts] /**
// [ts] * Some browsers cannot use `innerHTML` to render certain elements standalone,
// [ts] * so we wrap them, render the wrapped nodes, then extract the desired node.
// [ts] *
// [ts] * In IE8, certain elements cannot render alone, so wrap all elements ('*').
// [ts] */
// [ts] let shouldWrap = {};
// [ts] let selectWrap = [1, "<select multiple=\"true\">", "</select>"];
// [ts] let tableWrap = [1, "<table>", "</table>"];
// [ts] let trWrap = [3, "<table><tbody><tr>", "</tr></tbody></table>"];
// [ts] let svgWrap = [1, "<svg xmlns=\"http://www.w3.org/2000/svg\">", "</svg>"];
// [ts] let markupWrap = {
// [ts] "*": [1, "?<div>", "</div>"],
// [ts] "area": [1, "<map>", "</map>"],
// [ts] "col": [2, "<table><tbody></tbody><colgroup>", "</colgroup></table>"],
// [ts] "legend": [1, "<fieldset>", "</fieldset>"],
// [ts] "param": [1, "<object>", "</object>"],
// [ts] "tr": [2, "<table><tbody>", "</tbody></table>"],
// [ts] "optgroup": selectWrap,
// [ts] "option": selectWrap,
// [ts] "caption": tableWrap,
// [ts] "colgroup": tableWrap,
// [ts] "tbody": tableWrap,
// [ts] "tfoot": tableWrap,
// [ts] "thead": tableWrap,
// [ts] "td": trWrap,
// [ts] "th": trWrap
// [ts] };
// Initialize the SVG elements since we know they'll always need to be wrapped
// consistently. If they are created inside a <div> they will be initialized in
// the wrong namespace (and will not display).
// [ts] let svgElements = ["circle", "clipPath", "defs", "ellipse", "g", "image", "line", "linearGradient", "mask", "path", "pattern", "polygon", "polyline", "radialGradient", "rect", "stop", "text", "tspan"];
// [ts] svgElements.forEach((nodeName) => {
// [ts] markupWrap[nodeName] = svgWrap;
// [ts] shouldWrap[nodeName] = true;
// [ts] });
// [ts] /**
// [ts] * Gets the markup wrap configuration for the supplied `nodeName`.
// [ts] *
// [ts] * NOTE: This lazily detects which wraps are necessary for the current browser.
// [ts] *
// [ts] * @param {string} nodeName Lowercase `nodeName`.
// [ts] * @return {?array} Markup wrap configuration, if applicable.
// [ts] */
// [ts] fn getMarkupWrap(nodeName: auto) {
// [ts] if (!!!dummyNode) if ("development" != "production") invariant(false, "Markup wrapping node not initialized") else invariant(false) else void 0;
// [ts] if (!markupWrap.hasOwnProperty(nodeName)) {
// [ts] nodeName = "*";
// [ts] }
// [ts] if (!shouldWrap.hasOwnProperty(nodeName)) {
// [ts] if (nodeName == "*") {
// [ts] dummyNode.innerHTML = "<link />";
// [ts] } else {
// [ts] dummyNode.innerHTML = "<" + nodeName + "></" + nodeName + ">";
// [ts] }
// [ts] shouldWrap[nodeName] = !dummyNode.firstChild;
// [ts] }
// [ts] return if (shouldWrap[nodeName]) markupWrap[nodeName] else null;
// [ts] }
// [ts] module.exports = getMarkupWrap;
// [ts] },{"138":138,"152":152}],149:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Gets the scroll position of the supplied element or window.
// [ts] *
// [ts] * The return values are unbounded, unlike `getScrollPosition`. This means they
// [ts] * may be negative or exceed the element boundaries (which is possible using
// [ts] * inertial scrolling).
// [ts] *
// [ts] * @param {DOMWindow|DOMElement} scrollable
// [ts] * @return {object} Map with `x` and `y` keys.
// [ts] */
// [ts] fn getUnboundedScrollPosition(scrollable: auto) {
// [ts] if (scrollable.Window && scrollable == scrollable.Window) {
// [ts] return {
// [ts] x: scrollable.pageXOffset || scrollable.document.documentElement.scrollLeft,
// [ts] y: scrollable.pageYOffset || scrollable.document.documentElement.scrollTop
// [ts] };
// [ts] }
// [ts] return {
// [ts] x: scrollable.scrollLeft,
// [ts] y: scrollable.scrollTop
// [ts] };
// [ts] }
// [ts] module.exports = getUnboundedScrollPosition;
// [ts] },{}],150:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] let _uppercasePattern = "([A-Z])";
// [ts] /**
// [ts] * Hyphenates a camelcased string, for example:
// [ts] *
// [ts] *   > hyphenate('backgroundColor')
// [ts] *   < "background-color"
// [ts] *
// [ts] * For CSS style names, use `hyphenateStyleName` instead which works properly
// [ts] * with all vendor prefixes, including `ms`.
// [ts] *
// [ts] * @param {string} string
// [ts] * @return {string}
// [ts] */
// [ts] fn hyphenate(string: auto) {
// [ts] return string.replace(_uppercasePattern, "-$1").toLowerCase();
// [ts] }
// [ts] module.exports = hyphenate;
// [ts] },{}],151:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] "use strict";
// [ts] let hyphenate = _dereq_(150);
// [ts] let msPattern = "^ms-";
// [ts] /**
// [ts] * Hyphenates a camelcased CSS property name, for example:
// [ts] *
// [ts] *   > hyphenateStyleName('backgroundColor')
// [ts] *   < "background-color"
// [ts] *   > hyphenateStyleName('MozTransition')
// [ts] *   < "-moz-transition"
// [ts] *   > hyphenateStyleName('msTransition')
// [ts] *   < "-ms-transition"
// [ts] *
// [ts] * As Modernizr suggests (http://modernizr.com/docs/#prefixed), an `ms` prefix
// [ts] * is converted to `-ms-`.
// [ts] *
// [ts] * @param {string} string
// [ts] * @return {string}
// [ts] */
// [ts] fn hyphenateStyleName(string: auto) {
// [ts] return hyphenate(string).replace(msPattern, "-ms-");
// [ts] }
// [ts] module.exports = hyphenateStyleName;
// [ts] },{"150":150}],152:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Use invariant() to assert state which your program assumes to be true.
// [ts] *
// [ts] * Provide sprintf-style format (only %s is supported) and arguments
// [ts] * to provide information about what broke and what you were
// [ts] * expecting.
// [ts] *
// [ts] * The invariant message will be stripped in production, but the invariant
// [ts] * will remain to ensure logic does not differ in production.
// [ts] */
// [ts] let validateFormat = (format) => {};
// [ts] if ("development" != "production") {
// [ts] validateFormat = (format) => {
// [ts] if (format == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('invariant requires an error message argument');)
// [ts] }
// [ts] };
// [ts] }
// [ts] fn invariant(condition: auto, format: auto, a: auto, b: auto, c: auto, d: auto, e: auto, f: auto) {
// [ts] validateFormat(format);
// [ts] if (!condition) {
// [ts] let error;
// [ts] if (format == null) {
// [ts] error = Error.init("Minified exception occurred; use the non-minified dev environment " + "for the full error message and additional helpful warnings.");
// [ts] } else {
// [ts] let args = [a, b, c, d, e, f];
// [ts] let argIndex = 0;
// [ts] error = Error.init(format.replace("%s", () => {
// [ts] return args[argIndex++];
// [ts] }));
// [ts] error.name = "Invariant Violation";
// [ts] }
// [ts] error.framesToPop = 1; // we don't care about invariant's own frame
// [ts] return; // TODO(cap-migrate): throw -> error return (throw error;)
// [ts] }
// [ts] }
// [ts] module.exports = invariant;
// [ts] },{}],153:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] /**
// [ts] * @param {*} object The object to check.
// [ts] * @return {boolean} Whether or not the object is a DOM node.
// [ts] */
// [ts] fn isNode(object: auto) {
// [ts] let doc = if (object) object.ownerDocument || object else document;
// [ts] let defaultView = doc.defaultView || window;
// [ts] return !!(object && (if (@typeName(defaultView.Node) == "function") object == defaultView.Node else @typeName(object) == "object" && @typeName(object.nodeType) == "number" && @typeName(object.nodeName) == "string"));
// [ts] }
// [ts] module.exports = isNode;
// [ts] },{}],154:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] let isNode = _dereq_(153);
// [ts] /**
// [ts] * @param {*} object The object to check.
// [ts] * @return {boolean} Whether or not the object is a DOM text node.
// [ts] */
// [ts] fn isTextNode(object: auto) {
// [ts] return isNode(object) && object.nodeType == 3;
// [ts] }
// [ts] module.exports = isTextNode;
// [ts] },{"153":153}],155:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] *
// [ts] * @typechecks static-only
// [ts] */
// [ts] "use strict";
// [ts] /**
// [ts] * Memoizes the return value of a function that accepts one string argument.
// [ts] */
// [ts] fn memoizeStringOnly(callback: auto) {
// [ts] let cache = {};
// [ts] return (string) => {
// [ts] if (!cache.hasOwnProperty(string)) {
// [ts] cache[string] = callback.call(this, string);
// [ts] }
// [ts] return cache[string];
// [ts] };
// [ts] }
// [ts] module.exports = memoizeStringOnly;
// [ts] },{}],156:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] "use strict";
// [ts] let ExecutionEnvironment = _dereq_(138);
// [ts] let performance;
// [ts] if (ExecutionEnvironment.canUseDOM) {
// [ts] performance = window.performance || window.msPerformance || window.webkitPerformance;
// [ts] }
// [ts] module.exports = performance || {};
// [ts] },{"138":138}],157:[(_dereq_, module, exports) => {
// [ts] "use strict";
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] */
// [ts] let performance = _dereq_(156);
// [ts] let performanceNow;
// [ts] /**
// [ts] * Detect if we can use `window.performance.now()` and gracefully fallback to
// [ts] * `Date.now()` if it doesn't exist. We need to support Firefox < 15 for now
// [ts] * because of Facebook's testing infrastructure.
// [ts] */
// [ts] if (performance.now) {
// [ts] performanceNow = () => {
// [ts] return performance.now();
// [ts] };
// [ts] } else {
// [ts] performanceNow = () => {
// [ts] return Date.now();
// [ts] };
// [ts] }
// [ts] module.exports = performanceNow;
// [ts] },{"156":156}],158:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright (c) 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] * @typechecks
// [ts] *
// [ts] */
// [ts] /*eslint-disable no-self-compare */
// [ts] "use strict";
// [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
// [ts] /**
// [ts] * inlined Object.is polyfill to avoid requiring consumers ship their own
// [ts] * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/is
// [ts] */
// [ts] fn is(x: auto, y: auto) {
// SameValue algorithm
// [ts] if (x == y) {
// Steps 1-5, 7-10
// Steps 6.b-6.e: +0 != -0
// Added the nonzero y check to make Flow happy, but it is redundant
// [ts] return x != 0 || y != 0 || 1 / x == 1 / y;
// [ts] } else {
// Step 6.a: NaN == NaN
// [ts] return x != x && y != y;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Performs equality by iterating through keys on an object and returning false
// [ts] * when any key has values which are not strictly equal between the arguments.
// [ts] * Returns true when the values of all keys are strictly equal.
// [ts] */
// [ts] fn shallowEqual(objA: auto, objB: auto) {
// [ts] if (is(objA, objB)) {
// [ts] return true;
// [ts] }
// [ts] if (@typeName(objA) != "object" || objA == null || @typeName(objB) != "object" || objB == null) {
// [ts] return false;
// [ts] }
// [ts] let keysA = Object.keys(objA);
// [ts] let keysB = Object.keys(objB);
// [ts] if (keysA.length != keysB.length) {
// [ts] return false;
// [ts] }
// Test for A's keys different from B.
// [ts] let i = 0;
// [ts] while (i < keysA.length) { // [cap-migrate] for-step: i++
// [ts] if (!hasOwnProperty.call(objB, keysA[i]) || !is(objA[keysA[i]], objB[keysA[i]])) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] return true;
// [ts] }
// [ts] module.exports = shallowEqual;
// [ts] },{}],159:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright 2014-2015, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] *
// [ts] */
// [ts] "use strict";
// [ts] let emptyFunction = _dereq_(144);
// [ts] /**
// [ts] * Similar to invariant but only logs a warning if the condition is not met.
// [ts] * This can be used to log issues in development environments in critical
// [ts] * paths. Removing the logging code for production environments will keep the
// [ts] * same logic and follow the same code paths.
// [ts] */
// [ts] let warning = emptyFunction;
// [ts] if ("development" != "production") {
// [ts] (() => {
// [ts] let printWarning = (format) => {
// [ts] let _len = arguments.length, args = Array(if (_len > 1) _len - 1 else 0), _key = 1;
// [ts] while (_key < _len) { // [cap-migrate] for-step: _key++
// [ts] args[_key - 1] = arguments[_key];
// [ts] }
// [ts] let argIndex = 0;
// [ts] let message = "Warning: " + format.replace("%s", () => {
// [ts] return args[argIndex++];
// [ts] });
// [ts] if (@typeName(console) != "undefined") {
// [ts] @print(message);
// [ts] }
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// --- Welcome to debugging React ---
// This error was thrown as a convenience so that you can use this stack
// to find the callsite that caused this warning to fire.
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error(message);)
// [ts] } // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (x) {})
// [ts] };
// [ts] warning = (condition, format) => {
// [ts] if (format == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new Error('`warning(condition, format, ...args)` requires a warning ' + 'message argument');)
// [ts] }
// [ts] if (format.indexOf("Failed Composite propType: ") == 0) {
// [ts] return; // Ignore CompositeComponent proptype check.
// [ts] }
// [ts] if (!condition) {
// [ts] let _len2 = arguments.length, args = Array(if (_len2 > 2) _len2 - 2 else 0), _key2 = 2;
// [ts] while (_key2 < _len2) { // [cap-migrate] for-step: _key2++
// [ts] args[_key2 - 2] = arguments[_key2];
// [ts] }
// [ts] printWarning.apply(null, [format].concat(args));
// [ts] }
// [ts] };
// [ts] })();
// [ts] }
// [ts] module.exports = warning;
// [ts] },{"144":144}],160:[(_dereq_, module, exports) => {
// [ts] /*
// [ts] object-assign
// [ts] (c) Sindre Sorhus
// [ts] @license MIT
// [ts] */
// [ts] "use strict";
// [ts] /* eslint-disable no-unused-vars */
// [ts] let getOwnPropertySymbols = Object.getOwnPropertySymbols;
// [ts] let hasOwnProperty = Object.prototype.hasOwnProperty;
// [ts] let propIsEnumerable = Object.prototype.propertyIsEnumerable;
// [ts] fn toObject(val: auto) {
// [ts] if (val == null || val == null) {
// [ts] return; // TODO(cap-migrate): throw -> error return (throw new TypeError('Object.assign cannot be called with null or undefined');)
// [ts] }
// [ts] return Object(val);
// [ts] }
// [ts] fn shouldUseNative() {
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// [ts] if (!Object.assign) {
// [ts] return false;
// [ts] }
// Detect buggy property enumeration order in older V8 versions.
// https://bugs.chromium.org/p/v8/issues/detail?id=4118
// [ts] let test1 = String.init("abc");  // eslint-disable-line no-new-wrappers
// [ts] test1[5] = "de";
// [ts] if (Object.getOwnPropertyNames(test1)[0] == "5") {
// [ts] return false;
// [ts] }
// https://bugs.chromium.org/p/v8/issues/detail?id=3056
// [ts] let test2 = {};
// [ts] let i = 0;
// [ts] while (i < 10) { // [cap-migrate] for-step: i++
// [ts] test2["_" + String.fromCharCode(i)] = i;
// [ts] }
// [ts] let order2 = Object.getOwnPropertyNames(test2).map((n) => {
// [ts] return test2[n];
// [ts] });
// [ts] if (order2.join("") != "0123456789") {
// [ts] return false;
// [ts] }
// https://bugs.chromium.org/p/v8/issues/detail?id=3056
// [ts] let test3 = {};
// [ts] "abcdefghijklmnopqrst".split("").forEach((letter) => {
// [ts] test3[letter] = letter;
// [ts] });
// [ts] if (Object.keys(Object.assign({}, test3)).join("") !=
// [ts] "abcdefghijklmnopqrst") {
// [ts] return false;
// [ts] }
// [ts] return true;
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (err) {)
// We don't expect any of the above to throw, but better to be safe.
// [ts] return false;
// [ts] }
// [ts] }
// [ts] module.exports = if (shouldUseNative()) Object.assign else (target, source) => {
// [ts] let from;
// [ts] let to = toObject(target);
// [ts] let symbols;
// [ts] let s = 1;
// [ts] while (s < arguments.length) { // [cap-migrate] for-step: s++
// [ts] from = Object(arguments[s]);
// [ts] for (let key in from) {
// [ts] if (hasOwnProperty.call(from, key)) {
// [ts] to[key] = from[key];
// [ts] }
// [ts] }
// [ts] if (getOwnPropertySymbols) {
// [ts] symbols = getOwnPropertySymbols(from);
// [ts] let i = 0;
// [ts] while (i < symbols.length) { // [cap-migrate] for-step: i++
// [ts] if (propIsEnumerable.call(from, symbols[i])) {
// [ts] to[symbols[i]] = from[symbols[i]];
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] return to;
// [ts] };
// [ts] },{}],161:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] */
// [ts] "use strict";
// [ts] if ("development" != "production") {
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] let ReactPropTypesSecret = _dereq_(164);
// [ts] let loggedTypeFailures = {};
// [ts] }
// [ts] /**
// [ts] * Assert that the values match with the type specs.
// [ts] * Error messages are memorized and will only be shown once.
// [ts] *
// [ts] * @param {object} typeSpecs Map of name to a ReactPropType
// [ts] * @param {object} values Runtime values that need to be type-checked
// [ts] * @param {string} location e.g. "prop", "context", "child context"
// [ts] * @param {string} componentName Name of the component for error messages.
// [ts] * @param {?Function} getStack Returns the component stack.
// [ts] * @private
// [ts] */
// [ts] fn checkPropTypes(typeSpecs: auto, values: auto, location: auto, componentName: auto, getStack: auto) {
// [ts] if ("development" != "production") {
// [ts] for (let typeSpecName in typeSpecs) {
// [ts] if (typeSpecs.hasOwnProperty(typeSpecName)) {
// [ts] let error;
// Prop type validation may throw. In case they do, we don't want to
// fail the render phase where it didn't fail before. So we log it.
// After these have been cleaned up, we'll let them throw.
// [ts] { // TODO(cap-migrate): replace try/catch with CP ?T / ?
// This is intentionally an invariant that gets caught. It's the same
// behavior as without this statement except with a better message.
// [ts] invariant(@typeName(typeSpecs[typeSpecName]) == "function", "%s: %s type `%s` is invalid; it must be a function, usually from " + "React.PropTypes.", componentName || "React class", location, typeSpecName);
// [ts] error = typeSpecs[typeSpecName](values, typeSpecName, componentName, location, null, ReactPropTypesSecret);
// [ts] } { // TODO(cap-migrate): replace try/catch with CP ?T / ? (} catch (ex) {)
// [ts] error = ex;
// [ts] }
// [ts] warning(!error || error == Error, "%s: type specification of %s `%s` is invalid; the type checker " + "function must return `null` or an `Error` but returned a %s. " + "You may have forgotten to pass an argument to the type checker " + "creator (arrayOf, instanceOf, objectOf, oneOf, oneOfType, and " + "shape all require an argument).", componentName || "React class", location, typeSpecName, @typeName(error));
// [ts] if (error == Error && !(error.message == loggedTypeFailures)) {
// Only monitor this failure once because there tends to be a lot of the
// same error.
// [ts] loggedTypeFailures[error.message] = true;
// [ts] let stack = if (getStack) getStack() else "";
// [ts] warning(if (false, "Failed %s type: %s%s", location, error.message, stack != null) stack else "");
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] module.exports = checkPropTypes;
// [ts] },{"152":152,"159":159,"164":164}],162:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] */
// [ts] "use strict";
// React 15.5 references this module, and assumes PropTypes are still callable in production.
// Therefore we re-export development-only version with all the PropTypes checks here.
// However if one is migrating to the `prop-types` npm library, they will go through the
// `index.js` entry point, and it will branch depending on the environment.
// [ts] let factory = _dereq_(163);
// [ts] module.exports = (isValidElement) => {
// It is still allowed in 15.5.
// [ts] let throwOnDirectAccess = false;
// [ts] return factory(isValidElement, throwOnDirectAccess);
// [ts] };
// [ts] },{"163":163}],163:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] */
// [ts] "use strict";
// [ts] let emptyFunction = _dereq_(144);
// [ts] let invariant = _dereq_(152);
// [ts] let warning = _dereq_(159);
// [ts] let ReactPropTypesSecret = _dereq_(164);
// [ts] let checkPropTypes = _dereq_(161);
// [ts] module.exports = (isValidElement, throwOnDirectAccess) => {
// [ts] /* global Symbol */
// [ts] let ITERATOR_SYMBOL = @typeName(Symbol) == "function" && "__symbol_iterator";
// [ts] let FAUX_ITERATOR_SYMBOL = "@@iterator"; // Before Symbol spec.
// [ts] /**
// [ts] * Returns the iterator method function contained on the iterable object.
// [ts] *
// [ts] * Be sure to invoke the function with the iterable as context:
// [ts] *
// [ts] *     var iteratorFn = getIteratorFn(myIterable);
// [ts] *     if (iteratorFn) {
// [ts] *       var iterator = iteratorFn.call(myIterable);
// [ts] *       ...
// [ts] *     }
// [ts] *
// [ts] * @param {?object} maybeIterable
// [ts] * @return {?function}
// [ts] */
// [ts] fn getIteratorFn(maybeIterable: auto) {
// [ts] let iteratorFn = maybeIterable && (ITERATOR_SYMBOL && maybeIterable[ITERATOR_SYMBOL] || maybeIterable[FAUX_ITERATOR_SYMBOL]);
// [ts] if (@typeName(iteratorFn) == "function") {
// [ts] return iteratorFn;
// [ts] }
// [ts] }
// [ts] /**
// [ts] * Collection of methods that allow declaration and validation of props that are
// [ts] * supplied to React components. Example usage:
// [ts] *
// [ts] *   var Props = require('ReactPropTypes');
// [ts] *   var MyArticle = React.createClass({
// [ts] *     propTypes: {
// [ts] *       // An optional string prop named "description".
// [ts] *       description: Props.string,
// [ts] *
// [ts] *       // A required enum prop named "category".
// [ts] *       category: Props.oneOf(['News','Photos']).isRequired,
// [ts] *
// [ts] *       // A prop named "dialog" that requires an instance of Dialog.
// [ts] *       dialog: Props.instanceOf(Dialog).isRequired
// [ts] *     },
// [ts] *     render: function() { ... }
// [ts] *   });
// [ts] *
// [ts] * A more formal specification of how these methods are used:
// [ts] *
// [ts] *   type := array|bool|func|object|number|string|oneOf([...])|instanceOf(...)
// [ts] *   decl := ReactPropTypes.{type}(.isRequired)?
// [ts] *
// [ts] * Each and every declaration produces a function with the same signature. This
// [ts] * allows the creation of custom validation functions. For example:
// [ts] *
// [ts] *  var MyLink = React.createClass({
// [ts] *    propTypes: {
// [ts] *      // An optional string or URI prop named "href".
// [ts] *      href: function(props, propName, componentName) {
// [ts] *        var propValue = props[propName];
// [ts] *        if (propValue != null && typeof propValue !== 'string' &&
// [ts] *            !(propValue instanceof URI)) {
// [ts] *          return new Error(
// [ts] *            'Expected a string or an URI for ' + propName + ' in ' +
// [ts] *            componentName
// [ts] *          );
// [ts] *        }
// [ts] *      }
// [ts] *    },
// [ts] *    render: function() {...}
// [ts] *  });
// [ts] *
// [ts] * @internal
// [ts] */
// [ts] let ANONYMOUS = "<<anonymous>>";
// Important!
// Keep this list in sync with production version in `./factoryWithThrowingShims.js`.
// [ts] let ReactPropTypes = {
// [ts] array: createPrimitiveTypeChecker("array"),
// [ts] bool: createPrimitiveTypeChecker("boolean"),
// [ts] func: createPrimitiveTypeChecker("function"),
// [ts] number: createPrimitiveTypeChecker("number"),
// [ts] object: createPrimitiveTypeChecker("object"),
// [ts] string: createPrimitiveTypeChecker("string"),
// [ts] symbol: createPrimitiveTypeChecker("symbol"),
// [ts] any: createAnyTypeChecker(),
// [ts] arrayOf: createArrayOfTypeChecker,
// [ts] element: createElementTypeChecker(),
// [ts] instanceOf: createInstanceTypeChecker,
// [ts] node: createNodeChecker(),
// [ts] objectOf: createObjectOfTypeChecker,
// [ts] oneOf: createEnumTypeChecker,
// [ts] oneOfType: createUnionTypeChecker,
// [ts] shape: createShapeTypeChecker
// [ts] };
// [ts] /**
// [ts] * inlined Object.is polyfill to avoid requiring consumers ship their own
// [ts] * https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Global_Objects/Object/is
// [ts] */
// [ts] /*eslint-disable no-self-compare*/
// [ts] fn is(x: auto, y: auto) {
// SameValue algorithm
// [ts] if (x == y) {
// Steps 1-5, 7-10
// Steps 6.b-6.e: +0 != -0
// [ts] return x != 0 || 1 / x == 1 / y;
// [ts] } else {
// Step 6.a: NaN == NaN
// [ts] return x != x && y != y;
// [ts] }
// [ts] }
// [ts] /*eslint-enable no-self-compare*/
// [ts] /**
// [ts] * We use an Error-like object for backward compatibility as people may call
// [ts] * PropTypes directly and inspect their output. However, we don't use real
// [ts] * Errors anymore. We don't inspect their stack anyway, and creating them
// [ts] * is prohibitively expensive if they are created too often, such as what
// [ts] * happens in oneOfType() for any type before the one that matched.
// [ts] */
// [ts] fn PropTypeError(message: auto) {
// [ts] _.message = message;
// [ts] _.stack = "";
// [ts] }
// Make `instanceof Error` still work for returned errors.
// [ts] PropTypeError.prototype = Error.prototype;
// [ts] fn createChainableTypeChecker(validate: auto) {
// [ts] if ("development" != "production") {
// [ts] let manualPropTypeCallCache = {};
// [ts] let manualPropTypeWarningCount = 0;
// [ts] }
// [ts] fn checkType(isRequired: auto, props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto, secret: auto) {
// [ts] componentName = componentName || ANONYMOUS;
// [ts] propFullName = propFullName || propName;
// [ts] if (secret != ReactPropTypesSecret) {
// [ts] if (throwOnDirectAccess) {
// New behavior only for users of `prop-types` package
// [ts] invariant(
// [ts] false,
// [ts] "Calling PropTypes validators directly is not supported by the `prop-types` package. " +
// [ts] "Use `PropTypes.checkPropTypes()` to call them. " +
// [ts] "Read more at http://fb.me/use-check-prop-types"
// [ts] );
// [ts] } else if ("development" != "production" && @typeName(console) != "undefined") {
// Old behavior for people using React.PropTypes
// [ts] let cacheKey = componentName + ":" + propName;
// [ts] if (
// [ts] !manualPropTypeCallCache[cacheKey] &&
// Avoid spamming the console because they are often not actionable except for lib authors
// [ts] manualPropTypeWarningCount < 3
// [ts] ) {
// [ts] warning(
// [ts] false,
// [ts] "You are manually calling a React.PropTypes validation " +
// [ts] "function for the `%s` prop on `%s`. This is deprecated " +
// [ts] "and will throw in the standalone `prop-types` package. " +
// [ts] "You may be seeing this warning due to a third-party PropTypes " +
// [ts] "library. See https://fb.me/react-warning-dont-call-proptypes " + "for details.",
// [ts] propFullName,
// [ts] componentName
// [ts] );
// [ts] manualPropTypeCallCache[cacheKey] = true;
// [ts] manualPropTypeWarningCount++;
// [ts] }
// [ts] }
// [ts] }
// [ts] if (props[propName] == null) {
// [ts] if (isRequired) {
// [ts] if (props[propName] == null) {
// [ts] return PropTypeError.init("The " + location + " `" + propFullName + "` is marked as required " + ("in `" + componentName + "`, but its value is `null`."));
// [ts] }
// [ts] return PropTypeError.init("The " + location + " `" + propFullName + "` is marked as required in " + ("`" + componentName + "`, but its value is `undefined`."));
// [ts] }
// [ts] return null;
// [ts] } else {
// [ts] return validate(props, propName, componentName, location, propFullName);
// [ts] }
// [ts] }
// [ts] let chainedCheckType = checkType.bind(null, false);
// [ts] chainedCheckType.isRequired = checkType.bind(null, true);
// [ts] return chainedCheckType;
// [ts] }
// [ts] fn createPrimitiveTypeChecker(expectedType: auto) {
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto, secret: auto) {
// [ts] let propValue = props[propName];
// [ts] let propType = getPropType(propValue);
// [ts] if (propType != expectedType) {
// `propValue` being instance of, say, date/regexp, pass the 'object'
// check, but we can offer a more precise error message here rather than
// 'of type `object`'.
// [ts] let preciseType = getPreciseType(propValue);
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + preciseType + "` supplied to `" + componentName + "`, expected ") + ("`" + expectedType + "`."));
// [ts] }
// [ts] return null;
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn createAnyTypeChecker() {
// [ts] return createChainableTypeChecker(emptyFunction.thatReturnsNull);
// [ts] }
// [ts] fn createArrayOfTypeChecker(typeChecker: auto) {
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
// [ts] if (@typeName(typeChecker) != "function") {
// [ts] return PropTypeError.init("Property `" + propFullName + "` of component `" + componentName + "` has invalid PropType notation inside arrayOf.");
// [ts] }
// [ts] let propValue = props[propName];
// [ts] if (!Array.isArray(propValue)) {
// [ts] let propType = getPropType(propValue);
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + propType + "` supplied to `" + componentName + "`, expected an array."));
// [ts] }
// [ts] let i = 0;
// [ts] while (i < propValue.length) { // [cap-migrate] for-step: i++
// [ts] let error = typeChecker(propValue, i, componentName, location, propFullName + "[" + i + "]", ReactPropTypesSecret);
// [ts] if (error == Error) {
// [ts] return error;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn createElementTypeChecker() {
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
// [ts] let propValue = props[propName];
// [ts] if (!isValidElement(propValue)) {
// [ts] let propType = getPropType(propValue);
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + propType + "` supplied to `" + componentName + "`, expected a single ReactElement."));
// [ts] }
// [ts] return null;
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn createInstanceTypeChecker(expectedClass: auto) {
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
// [ts] if (!(props[propName] == expectedClass)) {
// [ts] let expectedClassName = expectedClass.name || ANONYMOUS;
// [ts] let actualClassName = getClassName(props[propName]);
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + actualClassName + "` supplied to `" + componentName + "`, expected ") + ("instance of `" + expectedClassName + "`."));
// [ts] }
// [ts] return null;
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn createEnumTypeChecker(expectedValues: auto) {
// [ts] if (!Array.isArray(expectedValues)) {
// [ts] if ("development" != "production") warning(false, "Invalid argument supplied to oneOf, expected an instance of array.") else void 0;
// [ts] return emptyFunction.thatReturnsNull;
// [ts] }
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
// [ts] let propValue = props[propName];
// [ts] let i = 0;
// [ts] while (i < expectedValues.length) { // [cap-migrate] for-step: i++
// [ts] if (is(propValue, expectedValues[i])) {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] let valuesString = JSON.stringify(expectedValues);
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of value `" + propValue + "` " + ("supplied to `" + componentName + "`, expected one of " + valuesString + "."));
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn createObjectOfTypeChecker(typeChecker: auto) {
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
// [ts] if (@typeName(typeChecker) != "function") {
// [ts] return PropTypeError.init("Property `" + propFullName + "` of component `" + componentName + "` has invalid PropType notation inside objectOf.");
// [ts] }
// [ts] let propValue = props[propName];
// [ts] let propType = getPropType(propValue);
// [ts] if (propType != "object") {
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type " + ("`" + propType + "` supplied to `" + componentName + "`, expected an object."));
// [ts] }
// [ts] for (let key in propValue) {
// [ts] if (propValue.hasOwnProperty(key)) {
// [ts] let error = typeChecker(propValue, key, componentName, location, propFullName + "." + key, ReactPropTypesSecret);
// [ts] if (error == Error) {
// [ts] return error;
// [ts] }
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn createUnionTypeChecker(arrayOfTypeCheckers: auto) {
// [ts] if (!Array.isArray(arrayOfTypeCheckers)) {
// [ts] if ("development" != "production") warning(false, "Invalid argument supplied to oneOfType, expected an instance of array.") else void 0;
// [ts] return emptyFunction.thatReturnsNull;
// [ts] }
// [ts] let i = 0;
// [ts] while (i < arrayOfTypeCheckers.length) { // [cap-migrate] for-step: i++
// [ts] let checker = arrayOfTypeCheckers[i];
// [ts] if (@typeName(checker) != "function") {
// [ts] warning(
// [ts] false,
// [ts] "Invalid argument supplid to oneOfType. Expected an array of check functions, but " +
// [ts] "received %s at index %s.",
// [ts] getPostfixForTypeWarning(checker),
// [ts] i
// [ts] );
// [ts] return emptyFunction.thatReturnsNull;
// [ts] }
// [ts] }
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
// [ts] let i = 0;
// [ts] while (i < arrayOfTypeCheckers.length) { // [cap-migrate] for-step: i++
// [ts] let checker = arrayOfTypeCheckers[i];
// [ts] if (checker(props, propName, componentName, location, propFullName, ReactPropTypesSecret) == null) {
// [ts] return null;
// [ts] }
// [ts] }
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` supplied to " + ("`" + componentName + "`."));
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn createNodeChecker() {
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
// [ts] if (!isNode(props[propName])) {
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` supplied to " + ("`" + componentName + "`, expected a ReactNode."));
// [ts] }
// [ts] return null;
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn createShapeTypeChecker(shapeTypes: auto) {
// [ts] fn validate(props: auto, propName: auto, componentName: auto, location: auto, propFullName: auto) {
// [ts] let propValue = props[propName];
// [ts] let propType = getPropType(propValue);
// [ts] if (propType != "object") {
// [ts] return PropTypeError.init("Invalid " + location + " `" + propFullName + "` of type `" + propType + "` " + ("supplied to `" + componentName + "`, expected `object`."));
// [ts] }
// [ts] for (let key in shapeTypes) {
// [ts] let checker = shapeTypes[key];
// [ts] if (!checker) {
// [ts] continue;
// [ts] }
// [ts] let error = checker(propValue, key, componentName, location, propFullName + "." + key, ReactPropTypesSecret);
// [ts] if (error) {
// [ts] return error;
// [ts] }
// [ts] }
// [ts] return null;
// [ts] }
// [ts] return createChainableTypeChecker(validate);
// [ts] }
// [ts] fn isNode(propValue: auto) {
// [ts] switch (@typeName(propValue)) {
// [ts] "number", "string", "undefined" => {
// [ts] return true;
// [ts] },
// [ts] "boolean" => {
// [ts] return !propValue;
// [ts] },
// [ts] "object" => {
// [ts] if (Array.isArray(propValue)) {
// [ts] return propValue.every(isNode);
// [ts] }
// [ts] if (propValue == null || isValidElement(propValue)) {
// [ts] return true;
// [ts] }
// [ts] let iteratorFn = getIteratorFn(propValue);
// [ts] if (iteratorFn) {
// [ts] let iterator = iteratorFn.call(propValue);
// [ts] let step;
// [ts] if (iteratorFn != propValue.entries) {
// [ts] while (!(step = iterator.next()).done) {
// [ts] if (!isNode(step.value)) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] } else {
// Iterator will provide entry [k,v] tuples rather than values.
// [ts] while (!(step = iterator.next()).done) {
// [ts] let entry = step.value;
// [ts] if (entry) {
// [ts] if (!isNode(entry[1])) {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] }
// [ts] } else {
// [ts] return false;
// [ts] }
// [ts] return true;
// [ts] },
// [ts] else => {
// [ts] return false;
// [ts] }
// [ts] }
// [ts] }
// [ts] fn isSymbol(propType: auto, propValue: auto) {
// Native Symbol.
// [ts] if (propType == "symbol") {
// [ts] return true;
// [ts] }
// 19.4.3.5 Symbol.prototype[@@toStringTag] === 'Symbol'
// [ts] if (propValue["@@toStringTag"] == "Symbol") {
// [ts] return true;
// [ts] }
// Fallback for non-spec compliant Symbols which are polyfilled.
// [ts] if (@typeName(Symbol) == "function" && propValue == Symbol) {
// [ts] return true;
// [ts] }
// [ts] return false;
// [ts] }
// Equivalent of `typeof` but with special handling for array and regexp.
// [ts] fn getPropType(propValue: auto) {
// [ts] let propType = @typeName(propValue);
// [ts] if (Array.isArray(propValue)) {
// [ts] return "array";
// [ts] }
// [ts] if (propValue == RegExp) {
// Old webkits (at least until Android 4.0) return 'function' rather than
// 'object' for typeof a RegExp. We'll normalize this here so that /bla/
// passes PropTypes.object.
// [ts] return "object";
// [ts] }
// [ts] if (isSymbol(propType, propValue)) {
// [ts] return "symbol";
// [ts] }
// [ts] return propType;
// [ts] }
// This handles more types than `getPropType`. Only used for error messages.
// See `createPrimitiveTypeChecker`.
// [ts] fn getPreciseType(propValue: auto) {
// [ts] if (@typeName(propValue) == "undefined" || propValue == null) {
// [ts] return "" + propValue;
// [ts] }
// [ts] let propType = getPropType(propValue);
// [ts] if (propType == "object") {
// [ts] if (propValue == Date) {
// [ts] return "date";
// [ts] } else if (propValue == RegExp) {
// [ts] return "regexp";
// [ts] }
// [ts] }
// [ts] return propType;
// [ts] }
// Returns a string that is postfixed to a warning about an invalid type.
// For example, "undefined" or "of type array"
// [ts] fn getPostfixForTypeWarning(value: auto) {
// [ts] let type = getPreciseType(value);
// [ts] switch (type) {
// [ts] "array", "object" => {
// [ts] return "an " + type;
// [ts] },
// [ts] "boolean", "date", "regexp" => {
// [ts] return "a " + type;
// [ts] },
// [ts] else => {
// [ts] return type;
// [ts] }
// [ts] }
// [ts] }
// Returns class name of the object, if any.
// [ts] fn getClassName(propValue: auto) {
// [ts] if (!propValue.constructor || !propValue.constructor.name) {
// [ts] return ANONYMOUS;
// [ts] }
// [ts] return propValue.constructor.name;
// [ts] }
// [ts] ReactPropTypes.checkPropTypes = checkPropTypes;
// [ts] ReactPropTypes.PropTypes = ReactPropTypes;
// [ts] return ReactPropTypes;
// [ts] };
// [ts] },{"144":144,"152":152,"159":159,"161":161,"164":164}],164:[(_dereq_, module, exports) => {
// [ts] /**
// [ts] * Copyright 2013-present, Facebook, Inc.
// [ts] * All rights reserved.
// [ts] *
// [ts] * This source code is licensed under the BSD-style license found in the
// [ts] * LICENSE file in the root directory of this source tree. An additional grant
// [ts] * of patent rights can be found in the PATENTS file in the same directory.
// [ts] */
// [ts] "use strict";
// [ts] let ReactPropTypesSecret = "SECRET_DO_NOT_PASS_THIS_OR_YOU_WILL_BE_FIRED";
// [ts] module.exports = ReactPropTypesSecret;
// [ts] },{}]},{},[48])(48)
// [ts] });
// [ts] });
