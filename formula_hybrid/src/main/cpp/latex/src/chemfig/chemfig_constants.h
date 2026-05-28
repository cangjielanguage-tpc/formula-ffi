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

constexpr float ARROW_LINE_WIDTH = 0.08f;
constexpr float ARROW_DOUBLE_BOND_OFFSET = 0.35f;
constexpr float ARROW_HARP_RADIUS = 0.3f;
constexpr float ARROW_DASH_LENGTH = 0.3f;
constexpr float ARROW_DASH_GAP = 0.3f;
constexpr float MERGE_BEND_OFFSET_RATIO = 0.3f;
constexpr float COMPOUND_NUMBER_GAP = 0.2f;
constexpr float ANCHOR_EQUALITY_THRESHOLD = 0.0001f;

inline float degToRad(float deg) { return deg * CHEM_PI / 180.0f; }
inline float radToDeg(float rad) { return rad * 180.0f / CHEM_PI; }
inline float normalizeAngle(float angle) {
    while (angle >= CHEM_TWO_PI) angle -= CHEM_TWO_PI;
    while (angle < 0.0f) angle += CHEM_TWO_PI;
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
    std::string narrow(s.begin(), s.end());
    const char* p = narrow.c_str();
    char* end = nullptr;
    float val = std::strtof(p, &end);
    return (end != p) ? val : 0.0f;
}

inline int safeStoi(const std::wstring& s) {
    if (s.empty()) return 0;
    std::string narrow(s.begin(), s.end());
    const char* p = narrow.c_str();
    char* end = nullptr;
    long val = std::strtol(p, &end, 10);
    return (end != p) ? static_cast<int>(val) : 0;
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

} // namespace chemfig

} // namespace tex

#endif // CHEMFIG_CONSTANTS_H_INCLUDED
