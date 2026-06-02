#ifndef CHEMFIG_CONSTANTS_H_INCLUDED
#define CHEMFIG_CONSTANTS_H_INCLUDED

#include <cmath>
#include <cstdlib>
#include <string>

namespace tex {

namespace chemfig {

constexpr float CHEM_PI = 3.14159265358979323846f;
constexpr float CHEM_TWO_PI = 2.0f * CHEM_PI;
constexpr float EPSILON = 0.0001f;
constexpr float PADDING = 0.5f;
constexpr float BOND_SCALE = 1.0f;
constexpr float DEFAULT_BOND_LENGTH = 3.0f;
constexpr float ANGLE_INCREMENT_DEG = 45.0f;
constexpr float ANGLE_INCREMENT_RAD = ANGLE_INCREMENT_DEG * CHEM_PI / 180.0f;

constexpr float BOND_LINE_WIDTH = 0.055f;
constexpr float DOUBLE_BOND_OFFSET = 0.1f;
constexpr float TRIPLE_BOND_OFFSET = 0.4f;
constexpr float WEDGE_WIDTH = 0.18f;
constexpr float DEFAULT_BOND_WIDTH_PT = 0.8f;

constexpr float TEXT_BOND_GAP = 0.12f;
constexpr float SUBSCRIPT_SCALE = 0.7f;
constexpr float SUPERSCRIPT_RISE = 0.35f;

constexpr float DEFAULT_ARROW_LENGTH = 1.5f;
constexpr float PLUS_SIGN_SIZE = 0.8f;
constexpr float LABEL_OFFSET = 0.4f;
constexpr int CURVE_SEGMENTS = 24;
constexpr int CUBIC_CURVE_SEGMENTS = 50;

constexpr float ARROW_LINE_WIDTH = 0.08f;
constexpr float ARROW_DOUBLE_BOND_OFFSET = 0.35f;
constexpr float ARROW_HARP_RADIUS = 0.3f;
constexpr float ARROW_DASH_LENGTH = 0.3f;
constexpr float ARROW_DASH_GAP = 0.3f;
constexpr float MERGE_BEND_OFFSET_RATIO = 0.3f;
constexpr float COMPOUND_NUMBER_GAP = 0.2f;
constexpr float ANCHOR_EQUALITY_THRESHOLD = 0.0001f;

constexpr float BOND_OFFSET_SCALE = 0.06f;
constexpr float TEXT_SCALE_BASE = 0.1f;

constexpr float INNER_CIRCLE_RADIUS_RATIO = 0.75f;
constexpr float WEDGE_NARROW_RATIO = 0.2f;
constexpr float WEDGE_STEP_RATIO = 0.02f;

constexpr float TRIANGLE_SHORTEN_RATIO = 0.618f;
constexpr float RING_SHORTEN_RATIO = 0.75f;

constexpr float RING_RADIUS_FACTOR_TRI = 0.577f;
constexpr float RING_RADIUS_FACTOR_SQUARE = 0.707f;
constexpr float RING_RADIUS_FACTOR_PENTAGON = 0.85f;
constexpr float RING_RADIUS_BASE_FACTOR = 1.15f;

constexpr float HARPOON_SLASH_ANGLE_DEG = 45.0f;
constexpr float HARPOON_SLASH_RATIO = 3.0f;
constexpr float HARPOON_SLASH_SPACING_RATIO = 0.24f;
constexpr float HARPOON_SLASH_OFFSET_RATIO = 0.5f;

constexpr float ARROW_MARGIN_RATIO = 0.8f;
constexpr float EQUILIBRIUM_OFFSET_DIVISOR = 3.0f;
constexpr float LONG_EQ_LENGTH_RATIO = 2.0f;
constexpr float HALF_ARPOON_SLASH_LENGTH_MULTIPLIER = 0.5f;

constexpr float CHARGE_RADIUS_FACTOR = 0.55f;
constexpr float CHARGE_DISPLAY_BASE = 10.0f;
constexpr float CHARGE_RECT_WIDTH_FACTOR = 0.45f;
constexpr float CHARGE_RECT_HEIGHT_FACTOR = 0.12f;
constexpr float CHARGE_LINE_LENGTH_FACTOR = 0.5f;
constexpr float CHARGE_DOT_SIZE_FACTOR = 0.12f;
constexpr float CHARGE_DOT_SPACING_FACTOR = 0.18f;
constexpr float CHARGE_CIRCLE_RADIUS_FACTOR = 0.20f;
constexpr float CHARGE_CIRCLE_LINE_RATIO = 0.8f;

constexpr float CURVE_FROM_ATOM_EXTRA = 6.0f;
constexpr float CURVE_TO_ATOM_EXTRA = 6.0f;
constexpr float CURVE_SHORTEN_SCALE = 0.1f;
constexpr float CURVE_DISTANCE_SCALE = 0.4f;
constexpr float CURVE_HEAD_LENGTH = 0.25f;
constexpr float CURVE_HEAD_WIDTH = 0.13f;
constexpr float CURVE_ARROW_ANGLE = 0.4f;

constexpr float DELIM_GAP_RATIO = 0.15f;
constexpr float EXTENSION_FACTOR = 0.6f;
constexpr float LABEL_MID_RATIO = 0.5f;
constexpr float ANCHOR_CENTER_RATIO = 0.5f;

constexpr float DASH_PATTERN_DOTTED_LEN = 0.05f;
constexpr float DASH_PATTERN_DOTTED_GAP = 0.25f;
constexpr float DASH_PATTERN_DENSELY_LEN = 0.2f;
constexpr float DASH_PATTERN_DENSELY_GAP = 0.1f;
constexpr float DASH_PATTERN_LOOSELY_LEN = 0.5f;
constexpr float DASH_PATTERN_LOOSELY_GAP = 0.5f;

constexpr float BOND_RENDERER_DASH_LEN_RATIO = 0.12f;
constexpr float BOND_RENDERER_DASH_GAP_RATIO = 0.08f;
constexpr float BOND_RENDERER_THIN_FACTOR = 0.5f;
constexpr float BOND_RENDERER_BOLD_FACTOR = 2.0f;
constexpr float BOND_RENDERER_THICK_FACTOR = 1.5f;

constexpr float MIN_VISIBLE_LEN_FACTOR = 2.0f;
constexpr float SHORTEN_RATIO_MIN = 0.0f;

constexpr float BOND_PARAMS_DEFAULT_COEFF_MIN = 0.0f;
constexpr float BOND_PARAMS_DEFAULT_COEFF_VAL = 1.0f;

constexpr float EM_TO_PT_RATIO = 0.1f;
constexpr float EM_TO_MM_RATIO = 0.284528f;

constexpr float BOND_TEXT_CENTER_RATIO = 0.5f;
constexpr float TEXT_WIDTH_EXTRA = 0.6f;

constexpr float LARGE_VALUE = 1e30f;
constexpr float LARGE_FLOAT_VALUE = 1e6f;
constexpr float MIN_ANGLE_THRESHOLD = 0.001f;

inline float degToRad(float deg) { return deg * CHEM_PI / 180.0f; }
inline float radToDeg(float rad) { return rad * 180.0f / CHEM_PI; }
inline float normalizeAngle(float angle) {
    angle = std::fmod(angle, CHEM_TWO_PI);
    if (angle < 0.0f) angle += CHEM_TWO_PI;
    return angle;
}

inline float calculateRingRadius(int ringSize) {
    float baseRadius = DEFAULT_BOND_LENGTH * 1.15f;
    switch (ringSize) {
        case 3: return baseRadius * 0.577f;
        case 4: return baseRadius * 0.707f;
        case 5: return baseRadius * 0.85f;
        default: return baseRadius;
    }
}

inline float safeStof(const std::wstring& s) {
    if (s.empty()) return 0.0f;
    wchar_t* end = nullptr;
    float val = static_cast<float>(std::wcstod(s.c_str(), &end));
    return (end != s.c_str()) ? val : 0.0f;
}

inline int safeStoi(const std::wstring& s) {
    if (s.empty()) return 0;
    wchar_t* end = nullptr;
    long val = std::wcstol(s.c_str(), &end, 10);
    return (end != s.c_str()) ? static_cast<int>(val) : 0;
}

inline bool isWordBoundary(const std::wstring& s, size_t pos, size_t len) {
    return pos + len >= s.length() || s[pos + len] == L',' || s[pos + len] == L' ';
}

inline std::string wstringToUtf8(const std::wstring& wstr) {
    if (wstr.empty()) return std::string();
    std::string result;
    result.reserve(wstr.size() * 3);
    for (wchar_t wc : wstr) {
        if (wc < 0x80) {
            result += static_cast<char>(wc);
        } else if (wc < 0x800) {
            result += static_cast<char>(0xC0 | (wc >> 6));
            result += static_cast<char>(0x80 | (wc & 0x3F));
        } else if (wc < 0x10000) {
            result += static_cast<char>(0xE0 | (wc >> 12));
            result += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (wc & 0x3F));
        } else {
            result += static_cast<char>(0xF0 | (wc >> 18));
            result += static_cast<char>(0x80 | ((wc >> 12) & 0x3F));
            result += static_cast<char>(0x80 | ((wc >> 6) & 0x3F));
            result += static_cast<char>(0x80 | (wc & 0x3F));
        }
    }
    return result;
    }

    constexpr int CURVE_SEGMENTS_MIN = 10;
    constexpr int CURVE_SEGMENTS_MAX = 120;
    constexpr float CURVE_SEGMENTS_TARGET_LEN = 0.5f;

    struct CubicBezierApprox {
        float p0x, p0y, p1x, p1y, p2x, p2y, p3x, p3y;
    };

    inline int computeCurveSegments(const CubicBezierApprox& bez) {
        float len = 0.0f;
        float dx = bez.p1x - bez.p0x, dy = bez.p1y - bez.p0y;
        len += std::sqrt(dx * dx + dy * dy);
        dx = bez.p2x - bez.p1x; dy = bez.p2y - bez.p1y;
        len += std::sqrt(dx * dx + dy * dy);
        dx = bez.p3x - bez.p2x; dy = bez.p3y - bez.p2y;
        len += std::sqrt(dx * dx + dy * dy);
        int segments = static_cast<int>(std::ceil(len / CURVE_SEGMENTS_TARGET_LEN));
        if (segments < CURVE_SEGMENTS_MIN) segments = CURVE_SEGMENTS_MIN;
        if (segments > CURVE_SEGMENTS_MAX) segments = CURVE_SEGMENTS_MAX;
        return segments;
    }

} // namespace chemfig

} // namespace tex

#endif // CHEMFIG_CONSTANTS_H_INCLUDED
