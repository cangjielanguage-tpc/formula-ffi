#include "scheme_config.h"
#include "chemfig_constants.h"
#include <algorithm>

namespace tex {

namespace {
    using namespace chemfig;

    bool isEmptyValue(const std::wstring& value) {
        return value.empty() || value == L"{}";
    }

    float clampPositive(float val, float minVal = 0.01f, float maxVal = 100.0f) {
        return std::max(minVal, std::min(val, maxVal));
    }

    float clampRange(float val, float minVal, float maxVal) {
        return std::max(minVal, std::min(val, maxVal));
    }
}

void SchemeConfig::set(const std::wstring& key, const std::wstring& value) {
    if (key.empty()) return;
    if (isEmptyValue(value)) {
        reset();
        return;
    }

    std::lock_guard<std::recursive_mutex> lock(_mutex);

    if (key == L"arrow coeff") arrowCoeff = clampPositive(safeStof(value), 0.01f, 50.0f);
    else if (key == L"compound sep") compoundSep = clampPositive(safeStof(value), 0.0f, 50.0f);
    else if (key == L"arrow angle") arrowAngle = clampRange(safeStof(value), -360.0f, 360.0f);
    else if (key == L"arrow curve") defaultCurveHeight = clampRange(safeStof(value), -10.0f, 10.0f);
    else if (key == L"arrow head length") arrowHeadLength = clampPositive(safeStof(value), 0.01f, 10.0f);
    else if (key == L"arrow head width") arrowHeadWidth = clampPositive(safeStof(value), 0.01f, 10.0f);
    else if (key == L"scheme debug") debugMode = (value == L"true");
    else if (key == L"auto number") autoNumber = (value != L"false");
    else if (key == L"name offset") nameOffset = clampRange(safeStof(value), -50.0f, 50.0f);
    else if (key == L"num offset") numOffset = clampRange(safeStof(value), -50.0f, 50.0f);
    else if (key == L"line spacing") lineSpacing = clampPositive(safeStof(value), 0.0f, 50.0f);
    else if (key == L"plus sep") plusSep = clampPositive(safeStof(value), 0.0f, 50.0f);
    else if (key == L"+ sep left") plusSep = clampPositive(safeStof(value), 0.0f, 50.0f);
    else if (key == L"+ sep right") plusSep = clampPositive(safeStof(value), 0.0f, 50.0f);
    else if (key == L"+ vshift") {}
    else if (key == L"arrow style") {
        arrowStyleColor.clear();
        arrowStyleHasLineWidth = false;
        arrowStyleLineWidth = ARROW_LINE_WIDTH;

        static const wchar_t* colorNames[] = {
            L"red", L"blue", L"green", L"yellow", L"black", L"white",
            L"cyan", L"magenta", L"orange", L"purple", L"brown", L"gray",
            L"pink", L"violet", L"olive", L"teal", L"lime", L"darkgray",
            L"lightgray", L"darkblue", L"darkgreen", L"darkred"
        };

        for (const auto& colorName : colorNames) {
            if (value.find(colorName) != std::wstring::npos) {
                arrowStyleColor = colorName;
                break;
            }
        }

        if (value.find(L"thick") != std::wstring::npos) {
            arrowStyleLineWidth = ARROW_LINE_WIDTH * 1.5f;
            arrowStyleHasLineWidth = true;
        } else if (value.find(L"thin") != std::wstring::npos) {
            arrowStyleLineWidth = ARROW_LINE_WIDTH * 0.5f;
            arrowStyleHasLineWidth = true;
        } else if (value.find(L"bold") != std::wstring::npos) {
            arrowStyleLineWidth = ARROW_LINE_WIDTH * 2.0f;
            arrowStyleHasLineWidth = true;
        }
    }
    else if (key == L"arrow offset") {
        arrowOffsetRaw = value;
        arrowOffset = clampRange(safeStof(value), -50.0f, 50.0f);
    }
    else if (key == L"arrow label sep") {
        arrowLabelSepRaw = value;
        arrowLabelSep = clampRange(safeStof(value), 0.0f, 50.0f);
    }
    else if (key == L"arrow head") arrowHeadStyle = value;
    else if (key == L"arrow double sep") {
        arrowDoubleSepRaw = value;
        arrowDoubleSep = clampPositive(safeStof(value), 0.01f, 50.0f);
    }
    else if (key == L"arrow double coeff") arrowDoubleCoeff = clampRange(safeStof(value), 0.01f, 10.0f);
    else if (key == L"arrow double harpoon") arrowDoubleHarpoon = (value != L"false" && value != L"0");
    else if (key == L"delim height scale") delimHeightScale = clampRange(safeStof(value), 0.3f, 1.5f);
}

void SchemeConfig::reset() {
    std::lock_guard<std::recursive_mutex> lock(_mutex);
    arrowCoeff = 1.0f;
    compoundSep = 2.8f;
    arrowAngle = 0.0f;
    arrowHeadLength = 0.3f;
    arrowHeadWidth = 0.15f;
    defaultCurveHeight = 0.5f;
    plusSep = 0.5f;
    nameOffset = 0.5f;
    numOffset = 0.3f;
    lineSpacing = 2.0f;
    arrowOffset = 0.0f;
    arrowOffsetRaw.clear();
    arrowLabelSep = 0.3f;
    arrowLabelSepRaw.clear();
    arrowDoubleSep = 0.35f;
    arrowDoubleSepRaw.clear();
    arrowDoubleCoeff = 0.6f;
    arrowDoubleHarpoon = false;
    arrowHeadStyle.clear();
    arrowStyleColor.clear();
    arrowStyleLineWidth = ARROW_LINE_WIDTH;
    arrowStyleHasLineWidth = false;
    debugMode = false;
    autoNumber = false;
}

} // namespace tex
