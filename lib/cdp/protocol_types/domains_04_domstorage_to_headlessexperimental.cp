// Source: Absorbed from _ref/playwright/packages/playwright-core/src/server/chromium/protocol.d.ts | Synced: 2026-11-28

import { ProtocolBinary, ProtocolJsonValue, ProtocolStringMap } from "./common";
import {
    Network_ErrorReason,
    Network_Request,
    Network_RequestId,
    Network_ResourceType,
    Network_TimeSinceEpoch,
} from "./domain_06_network";
import { DOM_RGBA, DOM_Rect } from "./domains_03_cast_to_domsnapshot";
import { IO_StreamHandle } from "./domains_05_io_to_memory";
import { Page_FrameId, Page_Viewport } from "./domains_08_page_to_performance";
import { Storage_SerializedStorageKey } from "./domains_09_performancetimeline_to_systeminfo";

// ============================================================================
// Domain: DOMStorage
// Query and modify DOM storage.
// ============================================================================

pub type DOMStorage_SerializedStorageKey = []const u8;
/// DOM Storage identifier.
pub struct DOMStorage_StorageId {
    securityOrigin: ([]const u8)? = null,
    storageKey: DOMStorage_SerializedStorageKey? = null,
    isLocalStorage: bool = false,
}
/// DOM Storage item.
pub type DOMStorage_Item = []const []const u8;
pub struct DOMStorage_domStorageItemAddedPayload {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
    newValue: []const u8 = "",
}
pub struct DOMStorage_domStorageItemRemovedPayload {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
}
pub struct DOMStorage_domStorageItemUpdatedPayload {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
    oldValue: []const u8 = "",
    newValue: []const u8 = "",
}
pub struct DOMStorage_domStorageItemsClearedPayload {
    storageId: DOMStorage_StorageId = .{},
}
pub struct DOMStorage_clearParameters {
    storageId: DOMStorage_StorageId = .{},
}
pub struct DOMStorage_clearReturnValue {}
/// Disables storage tracking, prevents storage events from being sent to the client.
pub struct DOMStorage_disableParameters {}
pub struct DOMStorage_disableReturnValue {}
/// Enables storage tracking, storage events will now be delivered to the client.
pub struct DOMStorage_enableParameters {}
pub struct DOMStorage_enableReturnValue {}
pub struct DOMStorage_getDOMStorageItemsParameters {
    storageId: DOMStorage_StorageId = .{},
}
pub struct DOMStorage_getDOMStorageItemsReturnValue {
    entries: []const DOMStorage_Item = &.{},
}
pub struct DOMStorage_removeDOMStorageItemParameters {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
}
pub struct DOMStorage_removeDOMStorageItemReturnValue {}
pub struct DOMStorage_setDOMStorageItemParameters {
    storageId: DOMStorage_StorageId = .{},
    key: []const u8 = "",
    value: []const u8 = "",
}
pub struct DOMStorage_setDOMStorageItemReturnValue {}

// ============================================================================
// Domain: DeviceAccess
// ============================================================================

/// Device request id.
pub type DeviceAccess_RequestId = []const u8;
/// A device id.
pub type DeviceAccess_DeviceId = []const u8;
/// Device information displayed in a user prompt to select a device.
pub struct DeviceAccess_PromptDevice {
    id: DeviceAccess_DeviceId = "",
    name: []const u8 = "",
}
/// A device request opened a user prompt to select a device. Respond with the
pub struct DeviceAccess_deviceRequestPromptedPayload {
    id: DeviceAccess_RequestId = "",
    devices: []const DeviceAccess_PromptDevice = &.{},
}
/// Enable events in this domain.
pub struct DeviceAccess_enableParameters {}
pub struct DeviceAccess_enableReturnValue {}
/// Disable events in this domain.
pub struct DeviceAccess_disableParameters {}
pub struct DeviceAccess_disableReturnValue {}
/// Select a device in response to a DeviceAccess.deviceRequestPrompted event.
pub struct DeviceAccess_selectPromptParameters {
    id: DeviceAccess_RequestId = "",
    deviceId: DeviceAccess_DeviceId = "",
}
pub struct DeviceAccess_selectPromptReturnValue {}
/// Cancel a prompt in response to a DeviceAccess.deviceRequestPrompted event.
pub struct DeviceAccess_cancelPromptParameters {
    id: DeviceAccess_RequestId = "",
}
pub struct DeviceAccess_cancelPromptReturnValue {}

// ============================================================================
// Domain: DeviceOrientation
// ============================================================================

/// Clears the overridden Device Orientation.
pub struct DeviceOrientation_clearDeviceOrientationOverrideParameters {}
pub struct DeviceOrientation_clearDeviceOrientationOverrideReturnValue {}
/// Overrides the Device Orientation.
pub struct DeviceOrientation_setDeviceOrientationOverrideParameters {
    alpha: f64 = 0.0,
    beta: f64 = 0.0,
    gamma: f64 = 0.0,
}
pub struct DeviceOrientation_setDeviceOrientationOverrideReturnValue {}

// ============================================================================
// Domain: DigitalCredentials
// This domain allows interacting with the Digital Credentials API for automation.
// ============================================================================

/// The type of virtual wallet action.
/// enum: "respond"|"decline"|"wait"|"clear"
pub type DigitalCredentials_VirtualWalletAction = []const u8;
/// Sets the behavior of the virtual wallet for digital credential requests
pub struct DigitalCredentials_setVirtualWalletBehaviorParameters {
    action: DigitalCredentials_VirtualWalletAction = "",
    protocol: ([]const u8)? = null,
    response: ProtocolStringMap? = null,
    frameId: Page_FrameId? = null,
}
pub struct DigitalCredentials_setVirtualWalletBehaviorReturnValue {}

// ============================================================================
// Domain: Emulation
// This domain emulates different environments for the page.
// ============================================================================

pub struct Emulation_SafeAreaInsets {
    top: f64? = null,
    topMax: f64? = null,
    left: f64? = null,
    leftMax: f64? = null,
    bottom: f64? = null,
    bottomMax: f64? = null,
    right: f64? = null,
    rightMax: f64? = null,
}
/// Screen orientation.
pub struct Emulation_ScreenOrientation {
    type_: []const u8 = "", // protocol field: "type"; enum: "portraitPrimary"|"portraitSecondary"|"landscapePrimary"|"landscapeSecondary"
    angle: f64 = 0.0,
}
pub struct Emulation_DisplayFeature {
    orientation: []const u8 = "", // enum: "vertical"|"horizontal"
    offset: f64 = 0.0,
    maskLength: f64 = 0.0,
}
pub struct Emulation_DevicePosture {
    type_: []const u8 = "", // protocol field: "type"; enum: "continuous"|"folded"
}
pub struct Emulation_MediaFeature {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// advance: If the scheduler runs out of immediate work, the virtual time base may fast forward to
/// enum: "advance"|"pause"|"pauseIfNetworkFetchesPending"
pub type Emulation_VirtualTimePolicy = []const u8;
/// Used to specify User Agent Client Hints to emulate. See https://wicg.github.io/ua-client-hints
pub struct Emulation_UserAgentBrandVersion {
    brand: []const u8 = "",
    version: []const u8 = "",
}
/// Used to specify User Agent Client Hints to emulate. See https://wicg.github.io/ua-client-hints
pub struct Emulation_UserAgentMetadata {
    brands: ([]const Emulation_UserAgentBrandVersion)? = null,
    fullVersionList: ([]const Emulation_UserAgentBrandVersion)? = null,
    fullVersion: ([]const u8)? = null,
    platform: []const u8 = "",
    platformVersion: []const u8 = "",
    architecture: []const u8 = "",
    model: []const u8 = "",
    mobile: bool = false,
    bitness: ([]const u8)? = null,
    wow64: bool? = null,
    formFactors: ([]const []const u8)? = null,
}
/// Used to specify sensor types to emulate.
/// enum: "absolute-orientation"|"accelerometer"|"ambient-light"|"gravity"|"gyroscope"|"linear-acceleration"|"magnetometer"|"re...
pub type Emulation_SensorType = []const u8;
pub struct Emulation_SensorMetadata {
    available: bool? = null,
    minimumFrequency: f64? = null,
    maximumFrequency: f64? = null,
}
pub struct Emulation_SensorReadingSingle {
    value: f64 = 0.0,
}
pub struct Emulation_SensorReadingXYZ {
    x: f64 = 0.0,
    y: f64 = 0.0,
    z: f64 = 0.0,
}
pub struct Emulation_SensorReadingQuaternion {
    x: f64 = 0.0,
    y: f64 = 0.0,
    z: f64 = 0.0,
    w: f64 = 0.0,
}
pub struct Emulation_SensorReading {
    single: Emulation_SensorReadingSingle? = null,
    xyz: Emulation_SensorReadingXYZ? = null,
    quaternion: Emulation_SensorReadingQuaternion? = null,
}
/// enum: "cpu"
pub type Emulation_PressureSource = []const u8;
/// enum: "nominal"|"fair"|"serious"|"critical"
pub type Emulation_PressureState = []const u8;
pub struct Emulation_PressureMetadata {
    available: bool? = null,
}
pub struct Emulation_WorkAreaInsets {
    top: f64? = null,
    left: f64? = null,
    bottom: f64? = null,
    right: f64? = null,
}
pub type Emulation_ScreenId = []const u8;
/// Screen information similar to the one returned by window.getScreenDetails() method,
pub struct Emulation_ScreenInfo {
    left: f64 = 0.0,
    top: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
    availLeft: f64 = 0.0,
    availTop: f64 = 0.0,
    availWidth: f64 = 0.0,
    availHeight: f64 = 0.0,
    devicePixelRatio: f64 = 0.0,
    orientation: Emulation_ScreenOrientation = .{},
    colorDepth: f64 = 0.0,
    isExtended: bool = false,
    isInternal: bool = false,
    isPrimary: bool = false,
    label: []const u8 = "",
    id: Emulation_ScreenId = "",
}
/// Enum of image types that can be disabled.
/// enum: "avif"|"jxl"|"webp"
pub type Emulation_DisabledImageType = []const u8;
/// Notification sent after the virtual time budget for the current VirtualTimePolicy has run out.
pub type Emulation_virtualTimeBudgetExpiredPayload = ProtocolJsonValue;
/// Fired when a page calls screen.orientation.lock() or screen.orientation.unlock()
pub struct Emulation_screenOrientationLockChangedPayload {
    locked: bool = false,
    orientation: Emulation_ScreenOrientation? = null,
}
/// Tells whether emulation is supported.
pub struct Emulation_canEmulateParameters {}
pub struct Emulation_canEmulateReturnValue {
    result: bool = false,
}
/// Clears the overridden device metrics.
pub struct Emulation_clearDeviceMetricsOverrideParameters {}
pub struct Emulation_clearDeviceMetricsOverrideReturnValue {}
/// Clears the overridden Geolocation Position and Error.
pub struct Emulation_clearGeolocationOverrideParameters {}
pub struct Emulation_clearGeolocationOverrideReturnValue {}
/// Requests that page scale factor is reset to initial values.
pub struct Emulation_resetPageScaleFactorParameters {}
pub struct Emulation_resetPageScaleFactorReturnValue {}
/// Enables or disables simulating a focused and active page.
pub struct Emulation_setFocusEmulationEnabledParameters {
    enabled: bool = false,
}
pub struct Emulation_setFocusEmulationEnabledReturnValue {}
/// Automatically render all web contents using a dark theme.
pub struct Emulation_setAutoDarkModeOverrideParameters {
    enabled: bool? = null,
}
pub struct Emulation_setAutoDarkModeOverrideReturnValue {}
/// Enables CPU throttling to emulate slow CPUs.
pub struct Emulation_setCPUThrottlingRateParameters {
    rate: f64 = 0.0,
}
pub struct Emulation_setCPUThrottlingRateReturnValue {}
/// Sets or clears an override of the default background color of the frame. This override is used
pub struct Emulation_setDefaultBackgroundColorOverrideParameters {
    color: DOM_RGBA? = null,
}
pub struct Emulation_setDefaultBackgroundColorOverrideReturnValue {}
/// Overrides the values for env(safe-area-inset-*) and env(safe-area-max-inset-*). Unset values will cause the
pub struct Emulation_setSafeAreaInsetsOverrideParameters {
    insets: Emulation_SafeAreaInsets = .{},
}
pub struct Emulation_setSafeAreaInsetsOverrideReturnValue {}
/// Overrides virtual keyboard geometry in CSS pixels, relative to the top-level viewport. The
pub struct Emulation_setVirtualKeyboardGeometryOverrideParameters {
    keyboardRect: DOM_Rect? = null,
}
pub struct Emulation_setVirtualKeyboardGeometryOverrideReturnValue {}
/// Overrides the values of device screen dimensions (window.screen.width, window.screen.height,
pub struct Emulation_setDeviceMetricsOverrideParameters {
    width: f64 = 0.0,
    height: f64 = 0.0,
    deviceScaleFactor: f64 = 0.0,
    mobile: bool = false,
    scale: f64? = null,
    screenWidth: f64? = null,
    screenHeight: f64? = null,
    positionX: f64? = null,
    positionY: f64? = null,
    dontSetVisibleSize: bool? = null,
    screenOrientation: Emulation_ScreenOrientation? = null,
    viewport: Page_Viewport? = null,
    displayFeature: Emulation_DisplayFeature? = null,
    devicePosture: Emulation_DevicePosture? = null,
    scrollbarType: ([]const u8)? = null, // enum: "overlay"|"default"
    screenOrientationLockEmulation: bool? = null,
    viewportMeta: ([]const u8)? = null, // enum: "enable"|"default"
    textLayoutMode: ([]const u8)? = null, // enum: "mobile"|"default"
}
pub struct Emulation_setDeviceMetricsOverrideReturnValue {}
/// Start reporting the given posture value to the Device Posture API.
pub struct Emulation_setDevicePostureOverrideParameters {
    posture: Emulation_DevicePosture = .{},
}
pub struct Emulation_setDevicePostureOverrideReturnValue {}
/// Clears a device posture override set with either setDeviceMetricsOverride()
pub struct Emulation_clearDevicePostureOverrideParameters {}
pub struct Emulation_clearDevicePostureOverrideReturnValue {}
/// Start using the given display features to pupulate the Viewport Segments API.
pub struct Emulation_setDisplayFeaturesOverrideParameters {
    features: []const Emulation_DisplayFeature = &.{},
}
pub struct Emulation_setDisplayFeaturesOverrideReturnValue {}
/// Clears the display features override set with either setDeviceMetricsOverride()
pub struct Emulation_clearDisplayFeaturesOverrideParameters {}
pub struct Emulation_clearDisplayFeaturesOverrideReturnValue {}
pub struct Emulation_setScrollbarsHiddenParameters {
    hidden: bool = false,
}
pub struct Emulation_setScrollbarsHiddenReturnValue {}
pub struct Emulation_setDocumentCookieDisabledParameters {
    disabled: bool = false,
}
pub struct Emulation_setDocumentCookieDisabledReturnValue {}
pub struct Emulation_setEmitTouchEventsForMouseParameters {
    enabled: bool = false,
    configuration: ([]const u8)? = null, // enum: "mobile"|"desktop"
}
pub struct Emulation_setEmitTouchEventsForMouseReturnValue {}
/// Emulates the given media type or media feature for CSS media queries.
pub struct Emulation_setEmulatedMediaParameters {
    media: ([]const u8)? = null,
    features: ([]const Emulation_MediaFeature)? = null,
}
pub struct Emulation_setEmulatedMediaReturnValue {}
/// Emulates the given vision deficiency.
pub struct Emulation_setEmulatedVisionDeficiencyParameters {
    type_: []const u8 = "", // protocol field: "type"; enum: "none"|"blurredVision"|"reducedContrast"|"achromatopsia"|"deuteranopia"|"protanopia"|"tritanopia"
}
pub struct Emulation_setEmulatedVisionDeficiencyReturnValue {}
/// Emulates the given OS text scale.
pub struct Emulation_setEmulatedOSTextScaleParameters {
    scale: f64? = null,
}
pub struct Emulation_setEmulatedOSTextScaleReturnValue {}
/// Overrides the Geolocation Position or Error. Omitting latitude, longitude or
pub struct Emulation_setGeolocationOverrideParameters {
    latitude: f64? = null,
    longitude: f64? = null,
    accuracy: f64? = null,
    altitude: f64? = null,
    altitudeAccuracy: f64? = null,
    heading: f64? = null,
    speed: f64? = null,
}
pub struct Emulation_setGeolocationOverrideReturnValue {}
pub struct Emulation_getOverriddenSensorInformationParameters {
    type_: Emulation_SensorType = "", // protocol field: "type"
}
pub struct Emulation_getOverriddenSensorInformationReturnValue {
    requestedSamplingFrequency: f64 = 0.0,
}
/// Overrides a platform sensor of a given type. If |enabled| is true, calls to
pub struct Emulation_setSensorOverrideEnabledParameters {
    enabled: bool = false,
    type_: Emulation_SensorType = "", // protocol field: "type"
    metadata: Emulation_SensorMetadata? = null,
}
pub struct Emulation_setSensorOverrideEnabledReturnValue {}
/// Updates the sensor readings reported by a sensor type previously overridden
pub struct Emulation_setSensorOverrideReadingsParameters {
    type_: Emulation_SensorType = "", // protocol field: "type"
    reading: Emulation_SensorReading = .{},
}
pub struct Emulation_setSensorOverrideReadingsReturnValue {}
/// Overrides a pressure source of a given type, as used by the Compute
pub struct Emulation_setPressureSourceOverrideEnabledParameters {
    enabled: bool = false,
    source: Emulation_PressureSource = "",
    metadata: Emulation_PressureMetadata? = null,
}
pub struct Emulation_setPressureSourceOverrideEnabledReturnValue {}
/// Provides a given pressure state that will be processed and eventually be
pub struct Emulation_setPressureStateOverrideParameters {
    source: Emulation_PressureSource = "",
    state: Emulation_PressureState = "",
}
pub struct Emulation_setPressureStateOverrideReturnValue {}
/// Overrides the Idle state.
pub struct Emulation_setIdleOverrideParameters {
    isUserActive: bool = false,
    isScreenUnlocked: bool = false,
}
pub struct Emulation_setIdleOverrideReturnValue {}
/// Clears Idle state overrides.
pub struct Emulation_clearIdleOverrideParameters {}
pub struct Emulation_clearIdleOverrideReturnValue {}
/// Overrides value returned by the javascript navigator object.
pub struct Emulation_setNavigatorOverridesParameters {
    platform: []const u8 = "",
}
pub struct Emulation_setNavigatorOverridesReturnValue {}
/// Sets a specified page scale factor.
pub struct Emulation_setPageScaleFactorParameters {
    pageScaleFactor: f64 = 0.0,
}
pub struct Emulation_setPageScaleFactorReturnValue {}
/// Switches script execution in the page.
pub struct Emulation_setScriptExecutionDisabledParameters {
    value: bool = false,
}
pub struct Emulation_setScriptExecutionDisabledReturnValue {}
/// Enables touch on platforms which do not support them.
pub struct Emulation_setTouchEmulationEnabledParameters {
    enabled: bool = false,
    maxTouchPoints: f64? = null,
}
pub struct Emulation_setTouchEmulationEnabledReturnValue {}
/// Turns on virtual time for all frames (replacing real-time with a synthetic time source) and sets
pub struct Emulation_setVirtualTimePolicyParameters {
    policy: Emulation_VirtualTimePolicy = "",
    budget: f64? = null,
    maxVirtualTimeTaskStarvationCount: f64? = null,
    initialVirtualTime: Network_TimeSinceEpoch? = null,
}
pub struct Emulation_setVirtualTimePolicyReturnValue {
    virtualTimeTicksBase: f64 = 0.0,
}
/// Overrides default host system locale with the specified one.
pub struct Emulation_setLocaleOverrideParameters {
    locale: ([]const u8)? = null,
}
pub struct Emulation_setLocaleOverrideReturnValue {}
/// Overrides default host system timezone with the specified one.
pub struct Emulation_setTimezoneOverrideParameters {
    timezoneId: []const u8 = "",
}
pub struct Emulation_setTimezoneOverrideReturnValue {}
/// Resizes the frame/viewport of the page. Note that this does not affect the frame's container
pub struct Emulation_setVisibleSizeParameters {
    width: f64 = 0.0,
    height: f64 = 0.0,
}
pub struct Emulation_setVisibleSizeReturnValue {}
pub struct Emulation_setDisabledImageTypesParameters {
    imageTypes: []const Emulation_DisabledImageType = &.{},
}
pub struct Emulation_setDisabledImageTypesReturnValue {}
/// Override the value of navigator.connection.saveData
pub struct Emulation_setDataSaverOverrideParameters {
    dataSaverEnabled: bool? = null,
}
pub struct Emulation_setDataSaverOverrideReturnValue {}
pub struct Emulation_setHardwareConcurrencyOverrideParameters {
    hardwareConcurrency: f64 = 0.0,
}
pub struct Emulation_setHardwareConcurrencyOverrideReturnValue {}
/// Overrides the value of navigator.cpuPerformance
pub struct Emulation_setCPUPerformanceOverrideParameters {
    performanceTier: ([]const u8)? = null, // enum: "unknown"|"low"|"mid"|"high"|"ultra"
}
pub struct Emulation_setCPUPerformanceOverrideReturnValue {}
/// Allows overriding user agent with the given string.
pub struct Emulation_setUserAgentOverrideParameters {
    userAgent: []const u8 = "",
    acceptLanguage: ([]const u8)? = null,
    platform: ([]const u8)? = null,
    userAgentMetadata: Emulation_UserAgentMetadata? = null,
}
pub struct Emulation_setUserAgentOverrideReturnValue {}
/// Allows overriding the automation flag.
pub struct Emulation_setAutomationOverrideParameters {
    enabled: bool = false,
}
pub struct Emulation_setAutomationOverrideReturnValue {}
/// Allows overriding the difference between the small and large viewport sizes, which determine the
pub struct Emulation_setSmallViewportHeightDifferenceOverrideParameters {
    difference: f64 = 0.0,
}
pub struct Emulation_setSmallViewportHeightDifferenceOverrideReturnValue {}
/// Returns device's screen configuration. In headful mode, the physical screens configuration is returned,
pub struct Emulation_getScreenInfosParameters {}
pub struct Emulation_getScreenInfosReturnValue {
    screenInfos: []const Emulation_ScreenInfo = &.{},
}
/// Add a new screen to the device. Only supported in headless mode.
pub struct Emulation_addScreenParameters {
    left: f64 = 0.0,
    top: f64 = 0.0,
    width: f64 = 0.0,
    height: f64 = 0.0,
    workAreaInsets: Emulation_WorkAreaInsets? = null,
    devicePixelRatio: f64? = null,
    rotation: f64? = null,
    colorDepth: f64? = null,
    label: ([]const u8)? = null,
    isInternal: bool? = null,
}
pub struct Emulation_addScreenReturnValue {
    screenInfo: Emulation_ScreenInfo = .{},
}
/// Updates specified screen parameters. Only supported in headless mode.
pub struct Emulation_updateScreenParameters {
    screenId: Emulation_ScreenId = "",
    left: f64? = null,
    top: f64? = null,
    width: f64? = null,
    height: f64? = null,
    workAreaInsets: Emulation_WorkAreaInsets? = null,
    devicePixelRatio: f64? = null,
    rotation: f64? = null,
    colorDepth: f64? = null,
    label: ([]const u8)? = null,
    isInternal: bool? = null,
}
pub struct Emulation_updateScreenReturnValue {
    screenInfo: Emulation_ScreenInfo = .{},
}
/// Remove screen from the device. Only supported in headless mode.
pub struct Emulation_removeScreenParameters {
    screenId: Emulation_ScreenId = "",
}
pub struct Emulation_removeScreenReturnValue {}
/// Set primary screen. Only supported in headless mode.
pub struct Emulation_setPrimaryScreenParameters {
    screenId: Emulation_ScreenId = "",
}
pub struct Emulation_setPrimaryScreenReturnValue {}

// ============================================================================
// Domain: EventBreakpoints
// EventBreakpoints permits setting JavaScript breakpoints on operations and events
// occurring in native code invoked from JavaScript. Once breakpoint is hit, it is
// ============================================================================

/// Sets breakpoint on particular native event.
pub struct EventBreakpoints_setInstrumentationBreakpointParameters {
    eventName: []const u8 = "",
}
pub struct EventBreakpoints_setInstrumentationBreakpointReturnValue {}
/// Removes breakpoint on particular native event.
pub struct EventBreakpoints_removeInstrumentationBreakpointParameters {
    eventName: []const u8 = "",
}
pub struct EventBreakpoints_removeInstrumentationBreakpointReturnValue {}
/// Removes all breakpoints
pub struct EventBreakpoints_disableParameters {}
pub struct EventBreakpoints_disableReturnValue {}

// ============================================================================
// Domain: Extensions
// Defines commands and events for browser extensions.
// ============================================================================

/// Storage areas.
/// enum: "session"|"local"|"sync"|"managed"
pub type Extensions_StorageArea = []const u8;
/// Detailed information about an extension.
pub struct Extensions_ExtensionInfo {
    id: []const u8 = "",
    name: []const u8 = "",
    version: []const u8 = "",
    path: []const u8 = "",
    enabled: bool = false,
}
/// Runs an extension default action.
pub struct Extensions_triggerActionParameters {
    id: []const u8 = "",
    targetId: []const u8 = "",
}
pub struct Extensions_triggerActionReturnValue {}
/// Installs an unpacked extension from the filesystem similar to
pub struct Extensions_loadUnpackedParameters {
    path: []const u8 = "",
    enableInIncognito: bool? = null,
}
pub struct Extensions_loadUnpackedReturnValue {
    id: []const u8 = "",
}
/// Gets a list of all unpacked extensions.
pub struct Extensions_getExtensionsParameters {}
pub struct Extensions_getExtensionsReturnValue {
    extensions: []const Extensions_ExtensionInfo = &.{},
}
/// Uninstalls an unpacked extension (others not supported) from the profile.
pub struct Extensions_uninstallParameters {
    id: []const u8 = "",
}
pub struct Extensions_uninstallReturnValue {}
/// Gets data from extension storage in the given `storageArea`. If `keys` is
pub struct Extensions_getStorageItemsParameters {
    id: []const u8 = "",
    storageArea: Extensions_StorageArea = "",
    keys: ([]const []const u8)? = null,
}
pub struct Extensions_getStorageItemsReturnValue {
    data: ProtocolStringMap = "",
}
/// Removes `keys` from extension storage in the given `storageArea`.
pub struct Extensions_removeStorageItemsParameters {
    id: []const u8 = "",
    storageArea: Extensions_StorageArea = "",
    keys: []const []const u8 = &.{},
}
pub struct Extensions_removeStorageItemsReturnValue {}
/// Clears extension storage in the given `storageArea`.
pub struct Extensions_clearStorageItemsParameters {
    id: []const u8 = "",
    storageArea: Extensions_StorageArea = "",
}
pub struct Extensions_clearStorageItemsReturnValue {}
/// Sets `values` in extension storage in the given `storageArea`. The provided `values`
pub struct Extensions_setStorageItemsParameters {
    id: []const u8 = "",
    storageArea: Extensions_StorageArea = "",
    values: ProtocolStringMap = "",
}
pub struct Extensions_setStorageItemsReturnValue {}

// ============================================================================
// Domain: FedCm
// This domain allows interacting with the FedCM dialog.
// ============================================================================

/// Whether this is a sign-up or sign-in action for this account, i.e.
/// enum: "SignIn"|"SignUp"
pub type FedCm_LoginState = []const u8;
/// The types of FedCM dialogs.
/// enum: "AccountChooser"|"AutoReauthn"|"ConfirmIdpLogin"|"Error"
pub type FedCm_DialogType = []const u8;
/// The buttons on the FedCM dialog.
/// enum: "ConfirmIdpLoginContinue"|"ErrorGotIt"|"ErrorMoreDetails"
pub type FedCm_DialogButton = []const u8;
/// The URLs that each account has
/// enum: "TermsOfService"|"PrivacyPolicy"
pub type FedCm_AccountUrlType = []const u8;
/// Corresponds to IdentityRequestAccount
pub struct FedCm_Account {
    accountId: []const u8 = "",
    email: []const u8 = "",
    name: []const u8 = "",
    givenName: []const u8 = "",
    pictureUrl: []const u8 = "",
    idpConfigUrl: []const u8 = "",
    idpLoginUrl: []const u8 = "",
    loginState: FedCm_LoginState = "",
    termsOfServiceUrl: ([]const u8)? = null,
    privacyPolicyUrl: ([]const u8)? = null,
}
pub struct FedCm_dialogShownPayload {
    dialogId: []const u8 = "",
    dialogType: FedCm_DialogType = "",
    accounts: []const FedCm_Account = &.{},
    title: []const u8 = "",
    subtitle: ([]const u8)? = null,
}
/// Triggered when a dialog is closed, either by user action, JS abort,
pub struct FedCm_dialogClosedPayload {
    dialogId: []const u8 = "",
}
pub struct FedCm_enableParameters {
    disableRejectionDelay: bool? = null,
}
pub struct FedCm_enableReturnValue {}
pub struct FedCm_disableParameters {}
pub struct FedCm_disableReturnValue {}
pub struct FedCm_selectAccountParameters {
    dialogId: []const u8 = "",
    accountIndex: f64 = 0.0,
}
pub struct FedCm_selectAccountReturnValue {}
pub struct FedCm_clickDialogButtonParameters {
    dialogId: []const u8 = "",
    dialogButton: FedCm_DialogButton = "",
}
pub struct FedCm_clickDialogButtonReturnValue {}
pub struct FedCm_openUrlParameters {
    dialogId: []const u8 = "",
    accountIndex: f64 = 0.0,
    accountUrlType: FedCm_AccountUrlType = "",
}
pub struct FedCm_openUrlReturnValue {}
pub struct FedCm_dismissDialogParameters {
    dialogId: []const u8 = "",
    triggerCooldown: bool? = null,
}
pub struct FedCm_dismissDialogReturnValue {}
/// Resets the cooldown time, if any, to allow the next FedCM call to show
pub struct FedCm_resetCooldownParameters {}
pub struct FedCm_resetCooldownReturnValue {}

// ============================================================================
// Domain: Fetch
// A domain for letting clients substitute browser's network layer with client code.
// ============================================================================

/// Unique request identifier.
pub type Fetch_RequestId = []const u8;
/// Stages of the request to handle. Request will intercept before the request is
/// enum: "Request"|"Response"
pub type Fetch_RequestStage = []const u8;
pub struct Fetch_RequestPattern {
    urlPattern: ([]const u8)? = null,
    resourceType: Network_ResourceType? = null,
    requestStage: Fetch_RequestStage? = null,
}
/// Response HTTP header entry
pub struct Fetch_HeaderEntry {
    name: []const u8 = "",
    value: []const u8 = "",
}
/// Authorization challenge for HTTP status code 401 or 407.
pub struct Fetch_AuthChallenge {
    source: ([]const u8)? = null, // enum: "Server"|"Proxy"
    origin: []const u8 = "",
    scheme: []const u8 = "",
    realm: []const u8 = "",
}
/// Response to an AuthChallenge.
pub struct Fetch_AuthChallengeResponse {
    response: []const u8 = "", // enum: "Default"|"CancelAuth"|"ProvideCredentials"
    username: ([]const u8)? = null,
    password: ([]const u8)? = null,
}
/// Issued when the domain is enabled and the request URL matches the
pub struct Fetch_requestPausedPayload {
    requestId: Fetch_RequestId = "",
    request: Network_Request = .{},
    frameId: Page_FrameId = "",
    resourceType: Network_ResourceType = "",
    responseErrorReason: Network_ErrorReason? = null,
    responseStatusCode: f64? = null,
    responseStatusText: ([]const u8)? = null,
    responseHeaders: ([]const Fetch_HeaderEntry)? = null,
    networkId: Network_RequestId? = null,
    redirectedRequestId: Fetch_RequestId? = null,
}
/// Issued when the domain is enabled with handleAuthRequests set to true.
pub struct Fetch_authRequiredPayload {
    requestId: Fetch_RequestId = "",
    request: Network_Request = .{},
    frameId: Page_FrameId = "",
    resourceType: Network_ResourceType = "",
    authChallenge: Fetch_AuthChallenge = .{},
}
/// Disables the fetch domain.
pub struct Fetch_disableParameters {}
pub struct Fetch_disableReturnValue {}
/// Enables issuing of requestPaused events. A request will be paused until client
pub struct Fetch_enableParameters {
    patterns: ([]const Fetch_RequestPattern)? = null,
    handleAuthRequests: bool? = null,
}
pub struct Fetch_enableReturnValue {}
/// Causes the request to fail with specified reason.
pub struct Fetch_failRequestParameters {
    requestId: Fetch_RequestId = "",
    errorReason: Network_ErrorReason = "",
}
pub struct Fetch_failRequestReturnValue {}
/// Provides response to the request.
pub struct Fetch_fulfillRequestParameters {
    requestId: Fetch_RequestId = "",
    responseCode: f64 = 0.0,
    responseHeaders: ([]const Fetch_HeaderEntry)? = null,
    binaryResponseHeaders: ProtocolBinary? = null,
    body: ProtocolBinary? = null,
    responsePhrase: ([]const u8)? = null,
}
pub struct Fetch_fulfillRequestReturnValue {}
/// Continues the request, optionally modifying some of its parameters.
pub struct Fetch_continueRequestParameters {
    requestId: Fetch_RequestId = "",
    url: ([]const u8)? = null,
    method: ([]const u8)? = null,
    postData: ProtocolBinary? = null,
    headers: ([]const Fetch_HeaderEntry)? = null,
    interceptResponse: bool? = null,
}
pub struct Fetch_continueRequestReturnValue {}
/// Continues a request supplying authChallengeResponse following authRequired event.
pub struct Fetch_continueWithAuthParameters {
    requestId: Fetch_RequestId = "",
    authChallengeResponse: Fetch_AuthChallengeResponse = .{},
}
pub struct Fetch_continueWithAuthReturnValue {}
/// Continues loading of the paused response, optionally modifying the
pub struct Fetch_continueResponseParameters {
    requestId: Fetch_RequestId = "",
    responseCode: f64? = null,
    responsePhrase: ([]const u8)? = null,
    responseHeaders: ([]const Fetch_HeaderEntry)? = null,
    binaryResponseHeaders: ProtocolBinary? = null,
}
pub struct Fetch_continueResponseReturnValue {}
/// Causes the body of the response to be received from the server and
pub struct Fetch_getResponseBodyParameters {
    requestId: Fetch_RequestId = "",
}
pub struct Fetch_getResponseBodyReturnValue {
    body: []const u8 = "",
    base64Encoded: bool = false,
}
/// Returns a handle to the stream representing the response body.
pub struct Fetch_takeResponseBodyAsStreamParameters {
    requestId: Fetch_RequestId = "",
}
pub struct Fetch_takeResponseBodyAsStreamReturnValue {
    stream: IO_StreamHandle = "",
}

// ============================================================================
// Domain: FileSystem
// ============================================================================

pub struct FileSystem_File {
    name: []const u8 = "",
    lastModified: Network_TimeSinceEpoch = 0.0,
    size: f64 = 0.0,
    type_: []const u8 = "", // protocol field: "type"
}
pub struct FileSystem_Directory {
    name: []const u8 = "",
    nestedDirectories: []const []const u8 = &.{},
    nestedFiles: []const FileSystem_File = &.{},
}
pub struct FileSystem_BucketFileSystemLocator {
    storageKey: Storage_SerializedStorageKey = "",
    bucketName: ([]const u8)? = null,
    pathComponents: []const []const u8 = &.{},
}
pub struct FileSystem_getDirectoryParameters {
    bucketFileSystemLocator: FileSystem_BucketFileSystemLocator = .{},
}
pub struct FileSystem_getDirectoryReturnValue {
    directory: FileSystem_Directory = .{},
}

// ============================================================================
// Domain: FindInPage
// This domain provides commands to trigger the "Find in page" feature.
// ============================================================================

/// Forwards `query` to the find-in-page facility, starting a new find session.
pub struct FindInPage_findFirstParameters {
    query: []const u8 = "",
}
pub struct FindInPage_findFirstReturnValue {}
/// Moves to the next match for the query passed to the most recent
pub struct FindInPage_findNextParameters {}
pub struct FindInPage_findNextReturnValue {}
/// Moves to the previous match for the query passed to the most recent
pub struct FindInPage_findPrevParameters {}
pub struct FindInPage_findPrevReturnValue {}
/// Ends the current find session, if any, and clears its highlighting.
pub struct FindInPage_stopParameters {}
pub struct FindInPage_stopReturnValue {}

// ============================================================================
// Domain: HeadlessExperimental
// This domain provides experimental commands only supported in headless mode.
// ============================================================================

/// Encoding options for a screenshot.
pub struct HeadlessExperimental_ScreenshotParams {
    format: ([]const u8)? = null, // enum: "jpeg"|"png"|"webp"
    quality: f64? = null,
    optimizeForSpeed: bool? = null,
}
/// Sends a BeginFrame to the target and returns when the frame was completed. Optionally captures a
pub struct HeadlessExperimental_beginFrameParameters {
    frameTimeTicks: f64? = null,
    interval: f64? = null,
    noDisplayUpdates: bool? = null,
    screenshot: HeadlessExperimental_ScreenshotParams? = null,
}
pub struct HeadlessExperimental_beginFrameReturnValue {
    hasDamage: bool = false,
    screenshotData: ProtocolBinary? = null,
}
/// Disables headless events for the target.
pub struct HeadlessExperimental_disableParameters {}
pub struct HeadlessExperimental_disableReturnValue {}
/// Enables headless events for the target.
pub struct HeadlessExperimental_enableParameters {}
pub struct HeadlessExperimental_enableReturnValue {}
