#ifndef SCHEME_CONFIG_H_INCLUDED
#define SCHEME_CONFIG_H_INCLUDED

#include "common.h"
#include <string>
#include <mutex>

namespace tex {

struct SchemeConfig {
    float arrowCoeff;
    float compoundSep;
    float arrowAngle;
    float arrowHeadLength;
    float arrowHeadWidth;
    float defaultCurveHeight;
    float plusSep;
    float nameOffset;
    float numOffset;
    float lineSpacing;
    float arrowOffset;
    std::wstring arrowOffsetRaw;
    float arrowLabelSep;
    float arrowDoubleSep;
    float arrowDoubleCoeff;
    float delimHeightScale;
    bool arrowDoubleHarpoon;
    std::wstring arrowHeadStyle;
    bool debugMode;
    bool autoNumber;

    static SchemeConfig& instance() {
        static SchemeConfig cfg;
        return cfg;
    }

    void set(const std::wstring& key, const std::wstring& value);

    void reset();

private:
    mutable std::recursive_mutex _mutex;

    SchemeConfig()
        : arrowCoeff(1.0f),
          compoundSep(2.8f),
          arrowAngle(0.0f),
          arrowHeadLength(0.3f),
          arrowHeadWidth(0.15f),
          defaultCurveHeight(0.5f),
          plusSep(0.5f),
          nameOffset(0.5f),
          numOffset(0.3f),
          lineSpacing(2.0f),
          arrowOffset(0.0f),
          arrowLabelSep(0.3f),
          arrowDoubleSep(0.35f),
          arrowDoubleCoeff(0.6f),
          arrowDoubleHarpoon(false),
          delimHeightScale(0.85f),
          debugMode(false),
          autoNumber(false) {}

    SchemeConfig(const SchemeConfig&) = delete;
    SchemeConfig& operator=(const SchemeConfig&) = delete;
};

} // namespace tex

#endif // SCHEME_CONFIG_H_INCLUDED
