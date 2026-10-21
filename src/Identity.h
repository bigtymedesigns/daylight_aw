#pragma once

// Fixed identity and limits. Changing anything here after a build has been
// used in a DAW session breaks saved sessions - see docs/BUILD_SPEC.md.

namespace dlaw
{
inline constexpr const char* kPluginName  = "Daylight_aw";
inline constexpr const char* kVendorName  = "big_tyme_designs";
inline constexpr const char* kVersion     = DAYLIGHT_VERSION_STRING;

// Number of generic parameter slots the host sees (largest Airwindows effect = 37, + 50 headroom).
inline constexpr int kNumSlots = 87;

// Version of the saved session / preset format. Increase only together with migration code.
inline constexpr int kStateFormatVersion = 1;

// Window geometry at 100 % zoom (identical for every theme).
inline constexpr int kWindowWidth  = 700;
inline constexpr int kSmallHeight  = 240;
inline constexpr int kLargeHeight  = 575;

inline constexpr int kControlsPerRow = 5;
inline constexpr int kRowsPerPage    = 3;
} // namespace dlaw
