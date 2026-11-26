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

// Window geometry at 100 % zoom: Small = 1 row of controls, Medium = 2 rows, Large = 3 rows
// (+ the page tabs when an effect needs more than 3 rows).
inline constexpr int kWindowWidth  = 700;
inline constexpr int kSmallHeight  = 240;
inline constexpr int kMediumHeight = 395;
inline constexpr int kLargeHeight  = 575;

inline constexpr int windowHeightForRows (int rows)
{
    return rows <= 1 ? kSmallHeight : rows == 2 ? kMediumHeight : kLargeHeight;
}

inline constexpr int kControlsPerRow = 5;
inline constexpr int kRowsPerPage    = 3;
} // namespace dlaw
