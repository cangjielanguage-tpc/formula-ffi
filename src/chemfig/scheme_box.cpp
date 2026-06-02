#include "scheme_box.h"
#include "chemfig_constants.h"
#include "chemfig_box.h"
#include "core/formula.h"
#include "atom/atom_basic.h"
#include "atom/box.h"
#include <cmath>
#include <algorithm>
#include <set>
#include <vector>

namespace tex {

namespace {
    using namespace chemfig;

    inline float getEffectiveAngle(float angle) {
        return (angle == ARROW_PARAM_UNSET) ? 0.0f : angle;
    }

    inline float getEffectiveLengthCoeff(float coeff) {
        return (coeff == ARROW_PARAM_UNSET) ? 1.0f : coeff;
    }

    std::string delimCharToSymbol(const std::wstring& delim) {
        if (delim == L"[") return "lsqbrack";
        if (delim == L"]") return "rsqbrack";
        if (delim == L"(") return "lbrack";
        if (delim == L")") return "rbrack";
        if (delim == L"\\{") return "lbrace";
        if (delim == L"\\}") return "rbrace";
        if (delim == L"|") return "vert";
        if (delim == L"||") return "Vert";
        if (delim == L"<") return "langle";
        if (delim == L">") return "rangle";
        if (delim == L"\\langle") return "langle";
        if (delim == L"\\rangle") return "rangle";
        if (delim == L"\\lfloor") return "lfloor";
        if (delim == L"\\rfloor") return "rfloor";
        if (delim == L"\\lceil") return "lceil";
        if (delim == L"\\rceil") return "rceil";
        if (delim == L".") return "";
        return "";
    }

    float parseLengthValue(const std::wstring& s, float scale, float emBase) {
        if (s.empty()) return 0.0f;
        
        std::wstring cleaned = s;
        if (cleaned.front() == L'{') {
            size_t end = cleaned.rfind(L'}');
            if (end != std::wstring::npos) {
                cleaned = cleaned.substr(1, end - 1);
            }
        }
        
        float value = 0.0f;
        std::wstring unit;
        
        size_t i = 0;
        bool negative = false;
        if (i < cleaned.size() && cleaned[i] == L'-') {
            negative = true;
            i++;
        } else if (i < cleaned.size() && cleaned[i] == L'+') {
            i++;
        }
        
        size_t numStart = i;
        while (i < cleaned.size() && (iswdigit(cleaned[i]) || cleaned[i] == L'.')) {
            i++;
        }
        
        if (i > numStart) {
            std::wstring numStr = cleaned.substr(numStart, i - numStart);
            try {
                value = std::stof(numStr);
            } catch (...) {
                return 0.0f;
            }
        }
        
        if (negative) value = -value;
        
        if (i < cleaned.size()) {
            unit = cleaned.substr(i);
        }
        
        float result = value;
        if (unit == L"em") {
            result = value * emBase;
        } else if (unit == L"pt") {
            result = value * emBase * 0.1f;
        } else if (unit == L"cm") {
            result = value * emBase * 2.84528f;
        } else if (unit == L"mm") {
            result = value * emBase * 0.284528f;
        } else {
            result = value * emBase;
        }
        
        return result;
    }
}

sptr<Box> SchemeBox::createLabelBox(const std::wstring& text, TeXEnvironment& env) {
    if (text.empty()) return nullptr;
    try {
        TeXFormula formula(text);
        return formula.createBox(env);
    } catch (...) {
        return nullptr;
    }
}

SchemeBox::SchemeBox(const ReactionScheme& scheme, color c,
                     const sptr<Font>& font, float sizeFactor,
                     TeXEnvironment& env)
    : _scheme(scheme), _color(c), _font(font), _sizeFactor(sizeFactor) {
    _compoundGap = SchemeConfig::instance().compoundSep;
    _arrowLength = _compoundGap;
    _scale = BOND_SCALE;
    _xOffset = 0.0f;
    calculateLayout(env);
}

void SchemeBox::calculateCompoundAnchors(CompoundLayout& cl) {
    cl.anchors = ReactionScheme::calculateCompoundAnchors(
        cl.x, cl.y, cl.width, cl.height, cl.boxHeight, cl.boxDepth);
}

ChemPoint SchemeBox::resolveArrowEndpoint(
    int compoundIdx,
    const ArrowRef& ref,
    const ArrowAnchor& anchor,
    float angle,
    bool isFrom,
    int subschemeIdx) {
    if (subschemeIdx >= 0 && subschemeIdx < static_cast<int>(_subschemeLayouts.size())) {
        const auto& sl = _subschemeLayouts[subschemeIdx];
        bool hasAngle = (std::abs(angle) > EPSILON);
        bool isExplicitRef = !anchor.compoundRef.empty() || !ref.compoundRef.empty();

        std::wstring effectiveAnchorName;
        if (!anchor.anchorName.empty()) {
            effectiveAnchorName = anchor.anchorName;
        } else if (!ref.anchorName.empty() && !isExplicitRef) {
            effectiveAnchorName = ref.anchorName;
        }

        if (!effectiveAnchorName.empty()) {
            ChemPoint pt = sl.anchors.getAnchor(effectiveAnchorName);
            if (std::abs(pt.x - sl.anchors.center.x) > ANCHOR_EQUALITY_THRESHOLD ||
                std::abs(pt.y - sl.anchors.center.y) > ANCHOR_EQUALITY_THRESHOLD) {
                return pt;
            }
        }

        if (hasAngle) {
            float entryAngle = isFrom ? angle : (angle + 180.0f);
            return sl.anchors.getAnchor(std::to_wstring(static_cast<int>(entryAngle)));
        }

        return isFrom ? sl.anchors.east : sl.anchors.west;
    }

    if (compoundIdx < 0 || compoundIdx >= static_cast<int>(_compoundLayouts.size())) {
        return ChemPoint();
    }

    auto& cl = _compoundLayouts[compoundIdx];
    bool hasAngle = (std::abs(angle) > EPSILON);
    bool isExplicitRef = !anchor.compoundRef.empty() || !ref.compoundRef.empty();

    std::wstring effectiveAnchorName;
    if (!anchor.anchorName.empty()) {
        effectiveAnchorName = anchor.anchorName;
    } else if (!ref.anchorName.empty() && !isExplicitRef) {
        effectiveAnchorName = ref.anchorName;
    }

    if (!effectiveAnchorName.empty()) {
        ChemPoint pt = cl.anchors.getAnchor(effectiveAnchorName);
        if (std::abs(pt.x - cl.anchors.center.x) > ANCHOR_EQUALITY_THRESHOLD ||
            std::abs(pt.y - cl.anchors.center.y) > ANCHOR_EQUALITY_THRESHOLD) {
            return pt;
        }
        if (!isExplicitRef) {
            ChemPoint atomPos = _scheme.getAtomPosition(compoundIdx, effectiveAnchorName);
            if (atomPos.x != 0 || atomPos.y != 0) {
                return ChemPoint(cl.x + atomPos.x * _scale, cl.y + atomPos.y * _scale);
            }
        }
    }

    if (hasAngle) {
        float entryAngle = isFrom ? angle : (angle + 180.0f);
        return cl.anchors.getAnchor(std::to_wstring(static_cast<int>(entryAngle)));
    }

    return isFrom ? cl.anchors.east : cl.anchors.west;
}

void SchemeBox::createCompoundBoxes(TeXEnvironment& env) {
    _compoundLayouts.resize(_scheme.compoundCount());
    for (int i = 0; i < _scheme.compoundCount(); i++) {
        const auto& info = _scheme.compounds[i];
        sptr<Box> mbox(new ChemfigBox(info.molecule, _color, _font, _sizeFactor));
        CompoundLayout& layout = _compoundLayouts[i];
        layout.box = mbox;
        layout.width = mbox->_width;
        layout.height = mbox->_height + mbox->_depth;
        layout.boxHeight = mbox->_height;
        layout.boxDepth = mbox->_depth;
        if (!info.name.empty()) layout.nameBox = createLabelBox(info.name, env);
        if (info.number > 0) {
            std::wstring numText = L"(" + std::to_wstring(info.number) + L")";
            layout.numberBox = createLabelBox(numText, env);
        }
    }
}

void SchemeBox::layoutSubschemeInternals() {
    for (size_t si = 0; si < _scheme.subschemes.size(); si++) {
        const auto& subInfo = _scheme.subschemes[si];
        int firstIdx = subInfo.firstCompoundIndex;
        if (firstIdx < 0 || firstIdx >= static_cast<int>(_compoundLayouts.size())) continue;
        auto& firstL = _compoundLayouts[firstIdx];
        firstL.x = 0; firstL.y = 0; firstL.positioned = true;
        calculateCompoundAnchors(firstL);
        for (int arrowIdx : subInfo.internalArrows) {
            if (arrowIdx < 0 || arrowIdx >= static_cast<int>(_scheme.arrows.size())) continue;
            const ArrowElement& arrow = _scheme.arrows[arrowIdx];
            int fromIdx = arrow.fromCompound, toIdx = arrow.toCompound;
            if (fromIdx < 0 || toIdx < 0) continue;
            if (fromIdx >= static_cast<int>(_compoundLayouts.size()) || toIdx >= static_cast<int>(_compoundLayouts.size())) continue;
            auto& fromL = _compoundLayouts[fromIdx];
            if (!fromL.positioned) continue;
            float effectiveAngle = getEffectiveAngle(arrow.params.angle);
            float effectiveLengthCoeff = getEffectiveLengthCoeff(arrow.params.lengthCoeff);
            bool hasAngle = (std::abs(effectiveAngle) > EPSILON);
            float arrowLen = std::max(_arrowLength * effectiveLengthCoeff * _scale, _compoundGap);
            ChemPoint toPos, fromPos;
            if (hasAngle) {
                float angleRad = effectiveAngle * CHEM_PI / 180.0f;
                fromPos = resolveArrowEndpoint(fromIdx, arrow.params.fromRef, arrow.params.fromAnchor, effectiveAngle, true);
                toPos = ChemPoint(fromPos.x + std::cos(angleRad) * arrowLen, fromPos.y - std::sin(angleRad) * arrowLen);
            } else {
                fromPos = resolveArrowEndpoint(fromIdx, arrow.params.fromRef, arrow.params.fromAnchor, effectiveAngle, true);
                toPos = ChemPoint(fromPos.x + arrowLen, fromPos.y);
            }
            auto& toL = _compoundLayouts[toIdx];
            toL.x = toPos.x - toL.width * 0.5f;
            toL.y = toPos.y - (toL.boxDepth - toL.boxHeight) * 0.5f;
            toL.positioned = true;
            calculateCompoundAnchors(toL);
        }
    }
}

void SchemeBox::layoutSubschemeBounds(TeXEnvironment& env) {
    (void)env;
    _subschemeLayouts.resize(_scheme.subschemes.size());
    for (size_t si = 0; si < _scheme.subschemes.size(); si++) {
        const auto& subInfo = _scheme.subschemes[si];
        SubschemeLayout& sl = _subschemeLayouts[si];
        sl.subschemeIndex = static_cast<int>(si);
        sl.firstCompound = subInfo.firstCompoundIndex;
        sl.lastCompound = subInfo.endCompound;
        int firstIdx = subInfo.firstCompoundIndex;
        if (firstIdx < 0 || firstIdx >= static_cast<int>(_compoundLayouts.size())) continue;
        const auto& firstL = _compoundLayouts[firstIdx];
        if (!firstL.positioned) continue;
        sl.minX = firstL.x; sl.maxX = firstL.x + firstL.width;
        sl.minY = firstL.y - firstL.boxHeight; sl.maxY = firstL.y + firstL.boxDepth;
        for (int j = subInfo.startCompound; j <= subInfo.endCompound; j++) {
            const auto& cl = _compoundLayouts[j];
            if (!cl.positioned) continue;
            if (cl.x < sl.minX) sl.minX = cl.x;
            if (cl.x + cl.width > sl.maxX) sl.maxX = cl.x + cl.width;
            if (cl.y - cl.boxHeight < sl.minY) sl.minY = cl.y - cl.boxHeight;
            if (cl.y + cl.boxDepth > sl.maxY) sl.maxY = cl.y + cl.boxDepth;
        }
        sl.width = sl.maxX - sl.minX; sl.height = sl.maxY - sl.minY;
        sl.centerX = sl.minX + sl.width * 0.5f; sl.centerY = sl.minY + sl.height * 0.5f;
        float boxHeight = sl.height * 0.5f; float boxDepth = sl.height * 0.5f;
        sl.anchors = ReactionScheme::calculateCompoundAnchors(sl.minX, sl.centerY, sl.width, sl.height, boxHeight, boxDepth);
        if (!subInfo.leftDelim.empty() && subInfo.leftDelim != L".") {
            std::string symName = delimCharToSymbol(subInfo.leftDelim);
            if (!symName.empty()) {
                try {
                    float delimHeight = sl.height * SchemeConfig::instance().delimHeightScale;
                    sl.leftDelimBox = DelimiterFactory::create(symName, env, delimHeight);
                    sl.leftDelimWidth = sl.leftDelimBox->_width;
                } catch (...) { sl.leftDelimBox = nullptr; sl.leftDelimWidth = 0; }
            }
        }
        if (!subInfo.rightDelim.empty() && subInfo.rightDelim != L".") {
            std::string symName = delimCharToSymbol(subInfo.rightDelim);
            if (!symName.empty()) {
                try {
                    float delimHeight = sl.height * SchemeConfig::instance().delimHeightScale;
                    sl.rightDelimBox = DelimiterFactory::create(symName, env, delimHeight);
                    sl.rightDelimWidth = sl.rightDelimBox->_width;
                } catch (...) { sl.rightDelimBox = nullptr; sl.rightDelimWidth = 0; }
            }
        }
    }
}

static float gapAfterArrow(int arrowIdx, const std::vector<ArrowElement>& arrows, float arrowLen, float compoundGap, float scale) {
    if (arrowIdx < 0 || arrowIdx >= static_cast<int>(arrows.size())) return compoundGap;
    const auto& arrow = arrows[arrowIdx];
    float ea = getEffectiveAngle(arrow.params.angle);
    float elc = getEffectiveLengthCoeff(arrow.params.lengthCoeff);
    if (std::abs(ea) <= EPSILON) return arrowLen * elc * scale;
    return compoundGap;
}

void SchemeBox::buildLayoutElements(std::vector<std::pair<bool, int>>& layoutElements) {
    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first == ELEM_LINEBREAK) {
            layoutElements.push_back(std::make_pair(true, 0));
        } else if (elem.first == ELEM_COMPOUND) {
            layoutElements.push_back(std::make_pair(false, elem.second));
        } else if (elem.first == ELEM_SUBSCHEME) {
            layoutElements.push_back(std::make_pair(false, -elem.second - 1));
        } else if (elem.first == ELEM_ARROW) {
            layoutElements.push_back(std::make_pair(false, -elem.second - 1000));
        } else if (elem.first == ELEM_PLUS) {
            layoutElements.push_back(std::make_pair(false, -elem.second - 2000));
        }
    }
}

void SchemeBox::linearLayoutPass(const std::vector<std::pair<bool, int>>& layoutElements) {
    float lineSpacing = SchemeConfig::instance().lineSpacing;
    float currentY = 0.0f, currentX = 0.0f;
    for (size_t i = 0; i < layoutElements.size(); i++) {
        const auto& le = layoutElements[i];
        if (le.first) { currentY += lineSpacing * _scale; currentX = 0; continue; }
        int idx = le.second;
        if (idx >= 0) {
            auto& layout = _compoundLayouts[idx];
            float gap = 0.0f;
            if (i > 0) {
                const auto& prevLe = layoutElements[i - 1];
                if (!prevLe.first && prevLe.second < -999 && prevLe.second >= -1999) {
                    int arrowIdx = -prevLe.second - 1000;
                    gap = gapAfterArrow(arrowIdx, _scheme.arrows, _arrowLength, _compoundGap, _scale);
                    if (std::abs(_scheme.arrows[arrowIdx].params.angle) <= EPSILON) {
                        bool hasBaseAnchor = false;
                        if (!_scheme.arrows[arrowIdx].params.fromAnchor.isDefault()) {
                            std::wstring fa = _scheme.arrows[arrowIdx].params.fromAnchor.anchorName;
                            std::transform(fa.begin(), fa.end(), fa.begin(), ::tolower);
                            hasBaseAnchor = (fa.find(L"base") != std::wstring::npos);
                        }
                        if (!_scheme.arrows[arrowIdx].params.toAnchor.isDefault()) {
                            std::wstring ta = _scheme.arrows[arrowIdx].params.toAnchor.anchorName;
                            std::transform(ta.begin(), ta.end(), ta.begin(), ::tolower);
                            hasBaseAnchor = hasBaseAnchor || (ta.find(L"base") != std::wstring::npos);
                        }
                        if (hasBaseAnchor) {
                            int fromIdx = _scheme.arrows[arrowIdx].fromCompound;
                            if (fromIdx >= 0 && fromIdx < static_cast<int>(_compoundLayouts.size())) {
                                float ea = getEffectiveAngle(_scheme.arrows[arrowIdx].params.angle);
                                ChemPoint fromPos = resolveArrowEndpoint(fromIdx, _scheme.arrows[arrowIdx].params.fromRef, _scheme.arrows[arrowIdx].params.fromAnchor, ea, true);
                                layout.y = fromPos.y - layout.boxDepth;
                                layout.positioned = true;
                                calculateCompoundAnchors(layout);
                            }
                        }
                    }
                } else if (!(prevLe.first || prevLe.second < -1999)) {
                    gap = _compoundGap;
                }
            }
            currentX += gap;
            float offsetX = currentX - layout.x;
            float offsetY = currentY - layout.y;
            layout.x += offsetX;
            if (!layout.positioned) layout.y += offsetY;
            calculateCompoundAnchors(layout);
            currentX += layout.width;
        } else if (idx < -1999) {
            int plusIdx = -idx - 2000;
            if (plusIdx >= 0 && plusIdx < static_cast<int>(_scheme.pluses.size())) {
                const auto& plus = _scheme.pluses[plusIdx];
                float emBase = _compoundGap / 5.0f;
                float sepLeft = emBase * 0.5f, sepRight = emBase * 0.5f;
                float plusWidth = PLUS_SIGN_SIZE * _scale;
                if (plus.hasCustomSep) {
                    if (!plus.sepLeftRaw.empty()) sepLeft = parseLengthValue(plus.sepLeftRaw, _scale, emBase);
                    if (!plus.sepRightRaw.empty()) sepRight = parseLengthValue(plus.sepRightRaw, _scale, emBase);
                }
                currentX += sepLeft + plusWidth + sepRight;
            }
        } else if (idx < -999) {
        } else {
            int subIdx = -idx - 1;
            if (subIdx >= 0 && subIdx < static_cast<int>(_subschemeLayouts.size())) {
                auto& sl = _subschemeLayouts[subIdx];
                const auto& subInfo = _scheme.subschemes[subIdx];
                float gap = 0.0f;
                if (i > 0) {
                    const auto& prevLe = layoutElements[i - 1];
                    if (!prevLe.first && prevLe.second < -999 && prevLe.second >= -1999) {
                        int arrowIdx = -prevLe.second - 1000;
                        gap = gapAfterArrow(arrowIdx, _scheme.arrows, _arrowLength, _compoundGap, _scale);
                    } else if (!(prevLe.first || prevLe.second < -1999)) {
                        gap = _compoundGap;
                    }
                }
                currentX += gap;
                float delimGap = _compoundGap * 0.15f;
                float leftDelimOffset = sl.leftDelimWidth + delimGap;
                float offsetX = currentX + leftDelimOffset - sl.minX;
                float offsetY = currentY - sl.centerY;
                for (int j = subInfo.startCompound; j <= subInfo.endCompound; j++) {
                    auto& cl = _compoundLayouts[j];
                    cl.x += offsetX; cl.y += offsetY; calculateCompoundAnchors(cl);
                }
                sl.minX += offsetX; sl.maxX += offsetX;
                sl.minY += offsetY; sl.maxY += offsetY;
                sl.centerX += offsetX; sl.centerY += offsetY;
                float slBoxHeight = sl.height * 0.5f; float slBoxDepth = sl.height * 0.5f;
                float anchorX = sl.minX; float anchorW = sl.width;
                if (sl.leftDelimBox) { anchorX -= (sl.leftDelimWidth + delimGap); anchorW += (sl.leftDelimWidth + delimGap); }
                if (sl.rightDelimBox) { anchorW += (sl.rightDelimWidth + delimGap); }
                sl.anchors = ReactionScheme::calculateCompoundAnchors(anchorX, sl.centerY, anchorW, sl.height, slBoxHeight, slBoxDepth);
                currentX += leftDelimOffset + sl.width + sl.rightDelimWidth + delimGap;
            }
        }
    }
}

void SchemeBox::assignRowDepths(const std::vector<std::pair<bool, int>>& layoutElements) {
    float curMinY = 0, curMaxY = 0, curMaxDepth = 0;
    bool firstInRow = true;
    for (size_t i = 0; i < layoutElements.size(); i++) {
        const auto& le = layoutElements[i];
        if (le.first) {
            if (!firstInRow) {
                for (auto& cl : _compoundLayouts) if (cl.y >= curMinY && cl.y <= curMaxY) cl.rowMaxDepth = curMaxDepth;
            }
            curMinY = 0; curMaxY = 0; curMaxDepth = 0; firstInRow = true; continue;
        }
        int idx = le.second;
        if (idx >= 0 && idx < static_cast<int>(_compoundLayouts.size())) {
            auto& cl = _compoundLayouts[idx];
            float bottom = cl.y + cl.boxDepth;
            if (firstInRow) { curMinY = cl.y - cl.boxHeight; curMaxY = bottom; curMaxDepth = cl.boxDepth; firstInRow = false; }
            else {
                curMinY = std::min(curMinY, cl.y - cl.boxHeight); curMaxY = std::max(curMaxY, bottom);
                curMaxDepth = std::max(curMaxDepth, cl.boxDepth);
            }
        } else if (idx < 0 && idx > -1000) {
            int subIdx = -idx - 1;
            if (subIdx >= 0 && subIdx < static_cast<int>(_subschemeLayouts.size())) {
                const auto& sl = _subschemeLayouts[subIdx];
                const auto& si = _scheme.subschemes[subIdx];
                float bottom = sl.maxY; float maxD = 0;
                for (int j = si.startCompound; j <= si.endCompound; j++)
                    if (j >= 0 && j < static_cast<int>(_compoundLayouts.size())) maxD = std::max(maxD, _compoundLayouts[j].boxDepth);
                if (firstInRow) { curMinY = sl.minY; curMaxY = bottom; curMaxDepth = maxD; firstInRow = false; }
                else {
                    curMinY = std::min(curMinY, sl.minY); curMaxY = std::max(curMaxY, bottom); curMaxDepth = std::max(curMaxDepth, maxD);
                }
            }
        }
    }
    if (!firstInRow) for (auto& cl : _compoundLayouts) if (cl.y >= curMinY && cl.y <= curMaxY) cl.rowMaxDepth = curMaxDepth;
}

void SchemeBox::layoutArrowFromOrder(const ArrowElement& arrow, ArrowLayout& alayout, TeXEnvironment& env) {
    int fromIdx = arrow.fromCompound, toIdx = arrow.toCompound;
    if (_scheme.compoundCount() == 0) return;
    if (fromIdx < 0) fromIdx = 0;
    if (toIdx < 0) toIdx = std::min(fromIdx + 1, _scheme.compoundCount() - 1);
    if (fromIdx >= _scheme.compoundCount()) fromIdx = _scheme.compoundCount() - 1;
    if (toIdx >= _scheme.compoundCount()) toIdx = _scheme.compoundCount() - 1;
    float effectiveAngle = getEffectiveAngle(arrow.params.angle);
    bool hasAngle = (std::abs(effectiveAngle) > EPSILON);
    float angleRad = hasAngle ? effectiveAngle * CHEM_PI / 180.0f : 0.0f;
    ChemPoint fromPos = resolveArrowEndpoint(fromIdx, arrow.params.fromRef, arrow.params.fromAnchor, effectiveAngle, true, arrow.fromSubschemeIdx);
    ChemPoint toPos = resolveArrowEndpoint(toIdx, arrow.params.toRef, arrow.params.toAnchor, effectiveAngle, false, arrow.toSubschemeIdx);
    if (!hasAngle && fromIdx >= 0 && toIdx >= 0 && fromIdx < static_cast<int>(_compoundLayouts.size()) && toIdx < static_cast<int>(_compoundLayouts.size()) && arrow.params.fromAnchor.isDefault() && arrow.params.toAnchor.isDefault()) {
        auto& fl = _compoundLayouts[fromIdx]; auto& tl = _compoundLayouts[toIdx];
        ChemPoint fc = (arrow.fromSubschemeIdx >= 0 && arrow.fromSubschemeIdx < static_cast<int>(_subschemeLayouts.size())) ? _subschemeLayouts[arrow.fromSubschemeIdx].anchors.center : fl.anchors.center;
        ChemPoint tc = (arrow.toSubschemeIdx >= 0 && arrow.toSubschemeIdx < static_cast<int>(_subschemeLayouts.size())) ? _subschemeLayouts[arrow.toSubschemeIdx].anchors.center : tl.anchors.center;
        float dx = tc.x - fc.x, dy = tc.y - fc.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist > EPSILON) {
            float aa = std::atan2(-dy, dx) * 180.0f / CHEM_PI;
            fromPos = (arrow.fromSubschemeIdx >= 0 && arrow.fromSubschemeIdx < static_cast<int>(_subschemeLayouts.size())) ? _subschemeLayouts[arrow.fromSubschemeIdx].anchors.getAnchor(std::to_wstring(static_cast<int>(aa))) : fl.anchors.getAnchor(std::to_wstring(static_cast<int>(aa)));
            toPos = (arrow.toSubschemeIdx >= 0 && arrow.toSubschemeIdx < static_cast<int>(_subschemeLayouts.size())) ? _subschemeLayouts[arrow.toSubschemeIdx].anchors.getAnchor(std::to_wstring(static_cast<int>(aa + 180.0f))) : tl.anchors.getAnchor(std::to_wstring(static_cast<int>(aa + 180.0f)));
        }
    }
    if (toIdx >= 0 && toIdx < static_cast<int>(_compoundLayouts.size())) {
        auto& tl = _compoundLayouts[toIdx];
        if (hasAngle) {
            float elc = getEffectiveLengthCoeff(arrow.params.lengthCoeff);
            float arrowLen = _arrowLength * elc * _scale;
            float cx = fromPos.x + std::cos(angleRad) * arrowLen, cy = fromPos.y - std::sin(angleRad) * arrowLen;
            tl.x = cx - tl.width * 0.5f; tl.y = cy - (tl.boxDepth - tl.boxHeight) * 0.5f;
            tl.positioned = true; calculateCompoundAnchors(tl);
            toPos = tl.anchors.getAnchor(std::to_wstring(static_cast<int>(effectiveAngle + 180.0f)));
            float nextX = tl.x + tl.width;
            for (int k = toIdx + 1; k < static_cast<int>(_compoundLayouts.size()); k++) {
                auto& nl = _compoundLayouts[k];
                bool isAT = false, isAAT = false;
                for (const auto& e2 : _scheme.elementOrder) {
                    if (e2.first == ELEM_ARROW && e2.second >= 0 && e2.second < static_cast<int>(_scheme.arrows.size())) {
                        if (_scheme.arrows[e2.second].toCompound == k) { isAT = true; if (std::abs(getEffectiveAngle(_scheme.arrows[e2.second].params.angle)) > EPSILON) isAAT = true; break; }
                    }
                }
                if (!isAAT && !nl.invisible) { float g = isAT ? _compoundGap : 0.0f; nl.x = nextX + g; nl.y = tl.y; calculateCompoundAnchors(nl); nextX += nl.width + g; } else break;
            }
        }
    } else if (hasAngle) {
        float elc = getEffectiveLengthCoeff(arrow.params.lengthCoeff);
        toPos = ChemPoint(fromPos.x + std::cos(angleRad) * _arrowLength * elc * _scale, fromPos.y - std::sin(angleRad) * _arrowLength * elc * _scale);
    }
    float elc = getEffectiveLengthCoeff(arrow.params.lengthCoeff);
    float arrowLen = _arrowLength * elc * _scale;
    if (std::abs(toPos.x - fromPos.x) < EPSILON && std::abs(toPos.y - fromPos.y) < EPSILON) toPos = ChemPoint(fromPos.x + arrowLen, fromPos.y);
    alayout.from = fromPos; alayout.to = toPos;
    if (!arrow.params.yShiftRaw.empty()) { auto len = SpaceAtom::getLength(arrow.params.yShiftRaw); alayout.yShift = SpaceAtom::getSize(len.first, len.second, env); }
    if (!arrow.params.labelAbove.empty()) alayout.labelAboveBox = createLabelBox(arrow.params.labelAbove, env);
    if (!arrow.params.labelBelow.empty()) alayout.labelBelowBox = createLabelBox(arrow.params.labelBelow, env);
}

void SchemeBox::layoutPlusFromOrder(const PlusElement& plus, const CompoundLayout& afterCL) {
    PlusLayout playout;
    int afterIdx = plus.afterCompound;
    if (afterIdx >= 0 && afterIdx < static_cast<int>(_compoundLayouts.size())) {
        float emBase = _compoundGap / 5.0f;
        float sepLeft = emBase * 0.5f, sepRight = emBase * 0.5f;
        float vshift = 0.0f;
        float halfSize = PLUS_SIGN_SIZE * 0.5f * _scale;
        if (plus.hasCustomSep) {
            if (!plus.sepLeftRaw.empty()) sepLeft = parseLengthValue(plus.sepLeftRaw, _scale, emBase);
            if (!plus.sepRightRaw.empty()) sepRight = parseLengthValue(plus.sepRightRaw, _scale, emBase);
            if (!plus.vshiftRaw.empty()) vshift = parseLengthValue(plus.vshiftRaw, _scale, emBase);
        }
        playout.x = afterCL.x + afterCL.width + sepLeft + halfSize - std::min(0.0f, sepRight);
        playout.y = afterCL.y + (afterCL.boxDepth - afterCL.boxHeight) * 0.5f - vshift;
    }
    _plusLayouts.push_back(playout);
}

void SchemeBox::layoutArrows(TeXEnvironment& env) {
    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first == ELEM_ARROW) {
            if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.arrows.size())) continue;
            ArrowLayout alayout; alayout.arrowIndex = elem.second;
            layoutArrowFromOrder(_scheme.arrows[elem.second], alayout, env);
            _arrowLayouts.push_back(alayout);
        } else if (elem.first == ELEM_PLUS) {
            if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.pluses.size())) continue;
            layoutPlusFromOrder(_scheme.pluses[elem.second], _compoundLayouts[_scheme.pluses[elem.second].afterCompound]);
        }
    }
}

void SchemeBox::layoutMerges() {
    std::set<int> repositionedMergeTargets;
    static float mergeAngles[] = { 0.0f, 180.0f, 90.0f, -90.0f };
    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first != ELEM_MERGE) continue;
        if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.merges.size())) continue;
        const MergeElement& merge = _scheme.merges[elem.second];
        int targetIdx = merge.target.compoundIndex;
        if (targetIdx < 0 || targetIdx >= _scheme.compoundCount()) continue;
        if (repositionedMergeTargets.count(targetIdx)) continue;
        float srcMinX = 0, srcMaxX = 0, srcASX = 0, srcASY = 0; int srcCount = 0; bool hasVS = false;
        for (const auto& src : merge.sources) {
            if (src.compoundIndex < 0 || src.compoundIndex >= _scheme.compoundCount()) continue;
            if (src.compoundIndex == targetIdx) continue;
            const auto& scl = _compoundLayouts[src.compoundIndex];
            if (!hasVS) { srcMinX = scl.x; srcMaxX = scl.x + scl.width; hasVS = true; }
            else { srcMinX = std::min(srcMinX, scl.x); srcMaxX = std::max(srcMaxX, scl.x + scl.width); }
            ChemPoint apt = !src.anchorName.empty() ? scl.anchors.getAnchor(src.anchorName) : merge.direction == MERGE_DOWN ? scl.anchors.south : merge.direction == MERGE_UP ? scl.anchors.north : merge.direction == MERGE_RIGHT ? scl.anchors.east : scl.anchors.west;
            srcASX += apt.x; srcASY += apt.y; srcCount++;
        }
        if (!hasVS) continue;
        auto& tcl = _compoundLayouts[targetIdx];
        float ma = merge.direction >= 0 && merge.direction < 4 ? mergeAngles[merge.direction] : 0.0f;
        float ar = ma * CHEM_PI / 180.0f;
        ChemPoint fa(srcASX / srcCount, srcASY / srcCount);
        float tcX = fa.x + std::cos(ar) * _arrowLength * 2.0f * _scale, tcY = fa.y - std::sin(ar) * _arrowLength * 2.0f * _scale;
        tcl.x = tcX - tcl.width * 0.5f; tcl.y = tcY - (tcl.boxDepth - tcl.boxHeight) * 0.5f;
        calculateCompoundAnchors(tcl); tcl.invisible = true; repositionedMergeTargets.insert(targetIdx);
    }
    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first != ELEM_MERGE) continue;
        if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.merges.size())) continue;
        const MergeElement& merge = _scheme.merges[elem.second];
        MergeLayout ml; ml.mergeIndex = elem.second; ml.direction = merge.direction; ml.segmentCoeff = merge.geometry.segmentCoeff;
        for (const auto& src : merge.sources) {
            if (src.compoundIndex >= 0 && src.compoundIndex < static_cast<int>(_compoundLayouts.size()) && src.compoundIndex != merge.target.compoundIndex) {
                const auto& cl = _compoundLayouts[src.compoundIndex];
                ml.fromPoints.push_back(!src.anchorName.empty() ? cl.anchors.getAnchor(src.anchorName) : merge.direction == MERGE_RIGHT ? cl.anchors.east : merge.direction == MERGE_LEFT ? cl.anchors.west : merge.direction == MERGE_UP ? cl.anchors.north : cl.anchors.south);
            }
        }
        bool isH = (ml.direction == MERGE_RIGHT || ml.direction == MERGE_LEFT);
        if (!isH && ml.fromPoints.size() > 1) ml.fromPoints.resize(1);
        if (ml.fromPoints.empty()) ml.fromPoints.resize(1);
        int tIdx = merge.target.compoundIndex;
        bool hvT = false;
        if (tIdx >= 0 && tIdx < static_cast<int>(_compoundLayouts.size())) {
            const auto& cl = _compoundLayouts[tIdx];
            ml.toPoint = !merge.target.anchorName.empty() ? cl.anchors.getAnchor(merge.target.anchorName) : merge.direction == MERGE_RIGHT ? cl.anchors.west : merge.direction == MERGE_LEFT ? cl.anchors.east : merge.direction == MERGE_UP ? cl.anchors.south : cl.anchors.north;
            hvT = true;
        }
        if (!hvT && !ml.fromPoints.empty()) {
            float sx = 0, sy = 0;
            for (const auto& fp : ml.fromPoints) { sx += fp.x; sy += fp.y; }
            sx /= ml.fromPoints.size(); sy /= ml.fromPoints.size();
            float ext = _arrowLength * _scale * 0.6f;
            ml.toPoint = merge.direction == MERGE_RIGHT ? ChemPoint(sx + ext, sy) : merge.direction == MERGE_LEFT ? ChemPoint(sx - ext, sy) : merge.direction == MERGE_UP ? ChemPoint(sx, sy - ext) : ChemPoint(sx, sy + ext);
        }
        _mergeLayouts.push_back(ml);
    }
}

void SchemeBox::layoutSubschemeArrows(TeXEnvironment& env) {
    for (size_t si = 0; si < _scheme.subschemes.size(); si++) {
        const auto& subInfo = _scheme.subschemes[si];
        for (int arrowIdx : subInfo.internalArrows) {
            if (arrowIdx < 0 || arrowIdx >= static_cast<int>(_scheme.arrows.size())) continue;
            const ArrowElement& arrow = _scheme.arrows[arrowIdx];
            if (arrow.fromCompound < 0 || arrow.toCompound < 0) continue;
            if (arrow.fromCompound >= static_cast<int>(_compoundLayouts.size()) || arrow.toCompound >= static_cast<int>(_compoundLayouts.size())) continue;
            ArrowLayout a; a.arrowIndex = arrowIdx;
            float ea = getEffectiveAngle(arrow.params.angle);
            a.from = resolveArrowEndpoint(arrow.fromCompound, arrow.params.fromRef, arrow.params.fromAnchor, ea, true);
            a.to = resolveArrowEndpoint(arrow.toCompound, arrow.params.toRef, arrow.params.toAnchor, ea, false);
            if (!arrow.params.labelAbove.empty()) a.labelAboveBox = createLabelBox(arrow.params.labelAbove, env);
            if (!arrow.params.labelBelow.empty()) a.labelBelowBox = createLabelBox(arrow.params.labelBelow, env);
            _arrowLayouts.push_back(a);
        }
    }
}

void SchemeBox::calculateBounds() {
    float tw = 0.0f, ml = 0.0f, mb = 0.0f, mt = 0.0f;
    for (const auto& cl : _compoundLayouts) {
        float r = cl.x + cl.width; if (r > tw) tw = r; if (cl.x < ml) ml = cl.x;
        float b = cl.y + cl.boxDepth, t = cl.y - cl.boxHeight;
        if (b > mb) mb = b; if (t < mt) mt = t;
        if (cl.nameBox) { float nb = cl.y + cl.boxDepth + SchemeConfig::instance().nameOffset + cl.nameBox->_height + cl.nameBox->_depth; if (nb > mb) mb = nb; }
        if (cl.numberBox) { float nb = cl.y + cl.boxDepth + SchemeConfig::instance().numOffset + cl.numberBox->_height + cl.numberBox->_depth; if (nb > mb) mb = nb; }
    }
    float emBase = _compoundGap / 5.0f, aofb = SchemeConfig::instance().arrowOffsetRaw.empty() ? SchemeConfig::instance().arrowOffset * emBase : parseLengthValue(SchemeConfig::instance().arrowOffsetRaw, _scale, emBase);
    for (const auto& al : _arrowLayouts) {
        float mnY = std::min(al.from.y - aofb, al.to.y - aofb), mxY = std::max(al.from.y - aofb, al.to.y - aofb);
        float mnX = std::min(al.from.x, al.to.x), mxX = std::max(al.from.x, al.to.x);
        if (al.arrowIndex >= 0 && al.arrowIndex < static_cast<int>(_scheme.arrows.size())) {
            const auto& ar = _scheme.arrows[al.arrowIndex];
            if (ar.params.isCurved()) { ChemPoint c = ArrowRenderer::computeCurveControlPoint(al.from, al.to, ar.params.effectiveCurveHeight()); c.y -= aofb; mnX = std::min(mnX, c.x); mxX = std::max(mxX, c.x); mnY = std::min(mnY, c.y); mxY = std::max(mxY, c.y); }
        }
        tw = std::max(tw, mxX); ml = std::min(ml, mnX); mt = std::min(mt, mnY); mb = std::max(mb, mxY);
        if (al.labelAboveBox) { float ly = (al.from.y + al.to.y) * 0.5f - aofb - LABEL_OFFSET - al.labelAboveBox->_depth - al.labelAboveBox->_height; if (ly < mt) mt = ly; }
        if (al.labelBelowBox) { float ly = (al.from.y + al.to.y) * 0.5f - aofb + LABEL_OFFSET + al.labelBelowBox->_height + al.labelBelowBox->_depth; if (ly > mb) mb = ly; }
    }
    for (const auto& pl : _plusLayouts) { if (pl.x + PLUS_SIGN_SIZE > tw) tw = pl.x + PLUS_SIGN_SIZE; }
    for (const auto& ml2 : _mergeLayouts) {
        for (const auto& fp : ml2.fromPoints) { tw = std::max(tw, fp.x); ml = std::min(ml, fp.x); mb = std::max(mb, fp.y); mt = std::min(mt, fp.y); }
        tw = std::max(tw, ml2.toPoint.x); ml = std::min(ml, ml2.toPoint.x); mb = std::max(mb, ml2.toPoint.y); mt = std::min(mt, ml2.toPoint.y);
    }
    for (const auto& sl : _subschemeLayouts) {
        if (sl.leftDelimBox) { float dt = sl.centerY - sl.leftDelimBox->_height, db = sl.centerY + sl.leftDelimBox->_depth; if (dt < mt) mt = dt; if (db > mb) mb = db; }
        if (sl.rightDelimBox) { float dr = sl.maxX + sl.rightDelimWidth; if (dr > tw) tw = dr; float dt = sl.centerY - sl.rightDelimBox->_height, db = sl.centerY + sl.rightDelimBox->_depth; if (dt < mt) mt = dt; if (db > mb) mb = db; }
    }
    _width = tw - ml + 2 * PADDING; _height = (-mt) + PADDING; _depth = mb + PADDING; _xOffset = -ml + PADDING; _foreground = _color;
}

void SchemeBox::calculateLayout(TeXEnvironment& env) {
    _compoundLayouts.clear();
    _arrowLayouts.clear();
    _plusLayouts.clear();
    _mergeLayouts.clear();
    _subschemeLayouts.clear();

    createCompoundBoxes(env);
    layoutSubschemeInternals();
    layoutSubschemeBounds(env);

    std::vector<std::pair<bool, int>> layoutElements;
    buildLayoutElements(layoutElements);
    linearLayoutPass(layoutElements);
    assignRowDepths(layoutElements);

    layoutArrows(env);
    layoutMerges();
    layoutSubschemeArrows(env);

    calculateBounds();
}

void SchemeBox::drawArrows(Graphics2D& g2, float ox, float oy) {
    SchemeConfig& cfg = SchemeConfig::instance();
    ArrowStyle style;
    style.headLength = cfg.arrowHeadLength;
    style.headWidth = cfg.arrowHeadWidth;
    style.lineWidth = cfg.arrowStyleHasLineWidth ? cfg.arrowStyleLineWidth : ARROW_LINE_WIDTH;
    
    float emBase = _compoundGap / 5.0f;
    if (!cfg.arrowDoubleSepRaw.empty()) {
        style.doubleBondOffset = parseLengthValue(cfg.arrowDoubleSepRaw, _scale, emBase);
    } else {
        style.doubleBondOffset = cfg.arrowDoubleSep * emBase;
    }
    style.doubleCoeff = cfg.arrowDoubleCoeff;
    style.doubleHarpoon = cfg.arrowDoubleHarpoon;
    style.harpRadius = ARROW_HARP_RADIUS;

    float arrowOffset = 0.0f;
    if (!cfg.arrowOffsetRaw.empty()) {
        arrowOffset = parseLengthValue(cfg.arrowOffsetRaw, _scale, emBase);
    } else {
        arrowOffset = cfg.arrowOffset * emBase;
    }
    float labelSep = 0.0f;
    if (!cfg.arrowLabelSepRaw.empty()) {
        labelSep = parseLengthValue(cfg.arrowLabelSepRaw, _scale, emBase);
    } else {
        labelSep = cfg.arrowLabelSep * _scale;
    }

    for (const auto& al : _arrowLayouts) {
        if (al.arrowIndex < 0 || al.arrowIndex >= static_cast<int>(_scheme.arrows.size())) continue;
        const auto& arrow = _scheme.arrows[al.arrowIndex];

        ChemPoint fromPos(al.from.x + ox, al.from.y + oy - al.yShift - arrowOffset);
        ChemPoint toPos(al.to.x + ox, al.to.y + oy - al.yShift - arrowOffset);

        switch (arrow.params.dashPattern) {
            case DASH_DOTTED:
                style.dashLength = 0.05f;
                style.dashGap = 0.25f;
                break;
            case DASH_DENSELY_DASHED:
                style.dashLength = 0.2f;
                style.dashGap = 0.1f;
                break;
            case DASH_LOOSELY_DASHED:
                style.dashLength = 0.5f;
                style.dashGap = 0.5f;
                break;
            case DASH_DASHED:
            default:
                style.dashLength = ARROW_DASH_LENGTH;
                style.dashGap = ARROW_DASH_LENGTH;
                break;
        }

        ArrowParams paramsWithStyle = arrow.params;
        if (paramsWithStyle.color.empty() && !cfg.arrowStyleColor.empty()) {
            paramsWithStyle.color = cfg.arrowStyleColor;
        }

        ArrowRenderer::drawArrow(g2, arrow.params.type,
                                 fromPos, toPos, _scale,
                                 paramsWithStyle, style);

        if (al.labelAboveBox || al.labelBelowBox) {
            ChemPoint labelRef;
            float arrowAngle = 0.0f;
            
            if (arrow.params.isCurved()) {
                ChemPoint ctrl = ArrowRenderer::computeCurveControlPoint(
                    fromPos, toPos, arrow.params.effectiveCurveHeight());
                float t = 0.5f;
                float t1 = 1.0f - t;
                labelRef = ChemPoint(
                    t1 * t1 * fromPos.x + 2.0f * t1 * t * ctrl.x + t * t * toPos.x,
                    t1 * t1 * fromPos.y + 2.0f * t1 * t * ctrl.y + t * t * toPos.y
                );
            } else {
                labelRef = ChemPoint(
                    (fromPos.x + toPos.x) * 0.5f,
                    (fromPos.y + toPos.y) * 0.5f
                );
                
                float dx = toPos.x - fromPos.x;
                float dy = toPos.y - fromPos.y;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > EPSILON) {
                    arrowAngle = std::atan2(dy, dx);
                }
            }

            float perpX = std::sin(arrowAngle);
            float perpY = -std::cos(arrowAngle);

            if (al.labelAboveBox) {
                float offsetDist = LABEL_OFFSET + labelSep + al.labelAboveBox->_depth;
                float cx = labelRef.x + perpX * offsetDist;
                float cy = labelRef.y + perpY * offsetDist;
                g2.rotate(arrowAngle, cx, cy);
                float lx = cx - al.labelAboveBox->_width * 0.5f;
                float ly = cy;
                al.labelAboveBox->draw(g2, lx, ly);
                g2.rotate(-arrowAngle, cx, cy);
            }
            if (al.labelBelowBox) {
                float offsetDist = LABEL_OFFSET + labelSep + al.labelBelowBox->_height;
                float cx = labelRef.x - perpX * offsetDist;
                float cy = labelRef.y - perpY * offsetDist;
                g2.rotate(arrowAngle, cx, cy);
                float lx = cx - al.labelBelowBox->_width * 0.5f;
                float ly = cy;
                al.labelBelowBox->draw(g2, lx, ly);
                g2.rotate(-arrowAngle, cx, cy);
            }
        }
    }
}

void SchemeBox::drawPlusSigns(Graphics2D& g2, float ox, float oy) {
    color oldColor = g2.getColor();
    if (!istrans(_color)) g2.setColor(_color);

    float halfSize = PLUS_SIGN_SIZE * 0.5f;

    for (const auto& pl : _plusLayouts) {
        float px = pl.x + ox;
        float py = pl.y + oy;
        g2.drawLine(px - halfSize, py, px + halfSize, py);
        g2.drawLine(px, py - halfSize, px, py + halfSize);
    }

    g2.setColor(oldColor);
}

void SchemeBox::drawMerges(Graphics2D& g2, float ox, float oy) {
    color oldColor = g2.getColor();
    const Stroke& oldStroke = g2.getStroke();
    if (!istrans(_color)) g2.setColor(_color);

    g2.setStroke(Stroke(ARROW_LINE_WIDTH * _scale, CAP_ROUND, JOIN_ROUND));

    float headLength = SchemeConfig::instance().arrowHeadLength;
    float headWidth = SchemeConfig::instance().arrowHeadWidth;

    for (const auto& ml : _mergeLayouts) {
        if (ml.fromPoints.empty()) continue;
        if (ml.fromPoints.size() < 2) {
            ChemPoint target(ml.toPoint.x + ox, ml.toPoint.y + oy);
            ChemPoint source(ml.fromPoints[0].x + ox, ml.fromPoints[0].y + oy);
            g2.drawLine(source.x, source.y, target.x, target.y);
            ArrowRenderer::drawMergeArrowHead(g2, target, source, _scale, headLength, headWidth);
            continue;
        }

        ChemPoint target(ml.toPoint.x + ox, ml.toPoint.y + oy);

        bool isVertical = (ml.direction == MERGE_UP || ml.direction == MERGE_DOWN);
        bool isDownOrRight = (ml.direction == MERGE_DOWN || ml.direction == MERGE_RIGHT);

        float spineCoord = 0.0f;
        float minSpineEnd = 0.0f;
        float maxSpineEnd = 0.0f;
        bool first = true;

        std::vector<ChemPoint> spineEntryPoints;

        for (size_t i = 0; i < ml.fromPoints.size(); i++) {
            ChemPoint src(ml.fromPoints[i].x + ox, ml.fromPoints[i].y + oy);

            float segLen = 5.0f * _scale * ml.segmentCoeff;
            ChemPoint bendPt;

            if (isVertical) {
                float sign = isDownOrRight ? 1.0f : -1.0f;
                bendPt = ChemPoint(src.x, src.y + segLen * sign);

                if (first) { spineCoord = bendPt.x; minSpineEnd = bendPt.y; maxSpineEnd = bendPt.y; first = false; }
                else { spineCoord = (spineCoord * i + bendPt.x) / (i + 1); }
                minSpineEnd = std::min(minSpineEnd, bendPt.y);
                maxSpineEnd = std::max(maxSpineEnd, bendPt.y);

                g2.drawLine(src.x, src.y, bendPt.x, bendPt.y);
                spineEntryPoints.push_back(bendPt);
            } else {
                float sign = isDownOrRight ? 1.0f : -1.0f;
                bendPt = ChemPoint(src.x + segLen * sign, src.y);

                if (first) { spineCoord = bendPt.y; minSpineEnd = bendPt.x; maxSpineEnd = bendPt.x; first = false; }
                else { spineCoord = (spineCoord * i + bendPt.y) / (i + 1); }
                minSpineEnd = std::min(minSpineEnd, bendPt.x);
                maxSpineEnd = std::max(maxSpineEnd, bendPt.x);

                g2.drawLine(src.x, src.y, bendPt.x, bendPt.y);
                spineEntryPoints.push_back(bendPt);
            }
        }

        for (const auto& ep : spineEntryPoints) {
            if (isVertical) {
                g2.drawLine(ep.x, ep.y, spineCoord, ep.y);
            } else {
                g2.drawLine(ep.x, ep.y, ep.x, spineCoord);
            }
        }

        float spineExt = 2.5f * _scale;

        if (isVertical) {
            float spineStart;
            if (isDownOrRight) {
                spineStart = (target.y >= maxSpineEnd) ? maxSpineEnd : minSpineEnd;
            } else {
                spineStart = (target.y <= minSpineEnd) ? minSpineEnd : maxSpineEnd;
            }
            float sign = isDownOrRight ? 1.0f : -1.0f;
            float spineEnd = spineStart + spineExt * sign;
            g2.drawLine(spineCoord, spineStart, spineCoord, spineEnd);

            ChemPoint tip(spineCoord, spineEnd);
            ChemPoint from(spineCoord, spineStart);
            ArrowRenderer::drawMergeArrowHead(g2, tip, from, _scale, headLength, headWidth);
        } else {
            float spineStart;
            if (isDownOrRight) {
                spineStart = (target.x >= maxSpineEnd) ? maxSpineEnd : minSpineEnd;
            } else {
                spineStart = (target.x <= minSpineEnd) ? minSpineEnd : maxSpineEnd;
            }
            float sign = isDownOrRight ? 1.0f : -1.0f;
            float spineEnd = spineStart + spineExt * sign;
            g2.drawLine(spineStart, spineCoord, spineEnd, spineCoord);

            ChemPoint tip(spineEnd, spineCoord);
            ChemPoint from(spineStart, spineCoord);
            ArrowRenderer::drawMergeArrowHead(g2, tip, from, _scale, headLength, headWidth);
        }
    }

    g2.setStroke(oldStroke);
    g2.setColor(oldColor);
}

void SchemeBox::drawCompoundNames(Graphics2D& g2, float ox, float oy) {
    for (size_t i = 0; i < _compoundLayouts.size(); i++) {
        const auto& cl = _compoundLayouts[i];
        if (!cl.nameBox) continue;

        float nx = cl.x + ox + (cl.width - cl.nameBox->_width) * 0.5f;
        float ny = cl.y + oy + cl.rowMaxDepth + SchemeConfig::instance().nameOffset + cl.nameBox->_height;
        cl.nameBox->draw(g2, nx, ny);
    }
}

void SchemeBox::drawCompoundNumbers(Graphics2D& g2, float ox, float oy) {
    for (size_t i = 0; i < _compoundLayouts.size(); i++) {
        const auto& cl = _compoundLayouts[i];
        if (!cl.numberBox) continue;

        float nx = cl.x + ox + cl.width + COMPOUND_NUMBER_GAP;
        float ny = cl.y + oy + cl.rowMaxDepth + SchemeConfig::instance().numOffset + cl.numberBox->_height;
        cl.numberBox->draw(g2, nx, ny);
    }
}

void SchemeBox::drawCurves(Graphics2D& g2, float ox, float oy) {    
    if (_scheme.curves.empty()) {
        return;
    }

    const Stroke& oldStroke = g2.getStroke();
    g2.setStroke(Stroke(BOND_LINE_WIDTH, CAP_ROUND, JOIN_ROUND));

    for (const auto& curve : _scheme.curves) {
        ChemPoint fromPos, toPos;
        bool foundFrom = false, foundTo = false;
        bool fromIsAtom = false, toIsAtom = false;
        float fromBondExtra = 0.0f, toBondExtra = 0.0f;
        
        for (size_t ci = 0; ci < _scheme.compounds.size(); ci++) {
            const auto& compound = _scheme.compounds[ci];
            const auto& layout = _compoundLayouts[ci];
            if (ci < _compoundLayouts.size()) {
            } else {
                continue;
            }
                        
            for (const auto& anchor : compound.molecule.anchors) {                
                ChemfigBox* cbox = dynamic_cast<ChemfigBox*>(layout.box.get());
                float internalOffsetX = 0.0f;
                float internalOffsetY = 0.0f;
                if (cbox) {
                    float tbMinX = cbox->getTextBoundsMinX();
                    float tbMinY = cbox->getTextBoundsMinY();
                    float tbMaxY = cbox->getTextBoundsMaxY();
                    internalOffsetX = (-tbMinX + PADDING) * _scale;
                    internalOffsetY = (-tbMinY - (tbMaxY - tbMinY) * 0.5f) * _scale;
                }
                
                if (anchor.name == curve.fromName) {
                    if (anchor.bondIndex >= 0 && anchor.bondIndex < static_cast<int>(compound.molecule.bonds.size())) {
                        const auto& bond = compound.molecule.bonds[anchor.bondIndex];
                        const auto& fromAtom = compound.molecule.atoms[bond.fromAtom];
                        const auto& toAtom = compound.molecule.atoms[bond.toAtom];
                        
                        float pos = (anchor.bondPosition >= 0.0f) ? anchor.bondPosition : 0.5f;
                        float baseX = fromAtom.position.x * (1.0f - pos) + toAtom.position.x * pos;
                        float baseY = fromAtom.position.y * (1.0f - pos) + toAtom.position.y * pos;
                        
                        fromPos.x = baseX * _scale + layout.x + ox + internalOffsetX;
                        fromPos.y = baseY * _scale + layout.y + oy + internalOffsetY;
                        if (bond.type == BOND_DOUBLE) {
                            fromBondExtra = 1.0f;
                        } else if (bond.type == BOND_TRIPLE) {
                            fromBondExtra = 2.0f;
                        }
                    } else if (anchor.atomIndex >= 0 && anchor.atomIndex < static_cast<int>(compound.molecule.atoms.size())) {
                        const auto& atom = compound.molecule.atoms[anchor.atomIndex];
                        fromPos.x = atom.position.x * _scale + layout.x + ox + internalOffsetX;
                        fromPos.y = atom.position.y * _scale + layout.y + oy + internalOffsetY;
                        fromIsAtom = !atom.text.empty() || !atom.charges.empty();
                    }
                    foundFrom = true;
                }
                if (anchor.name == curve.toName) {
                    if (anchor.bondIndex >= 0 && anchor.bondIndex < static_cast<int>(compound.molecule.bonds.size())) {
                        const auto& bond = compound.molecule.bonds[anchor.bondIndex];
                        const auto& fromAtom = compound.molecule.atoms[bond.fromAtom];
                        const auto& toAtom = compound.molecule.atoms[bond.toAtom];
                        
                        float pos = (anchor.bondPosition >= 0.0f) ? anchor.bondPosition : 0.5f;
                        float baseX = fromAtom.position.x * (1.0f - pos) + toAtom.position.x * pos;
                        float baseY = fromAtom.position.y * (1.0f - pos) + toAtom.position.y * pos;
                        
                        toPos.x = baseX * _scale + layout.x + ox + internalOffsetX;
                        toPos.y = baseY * _scale + layout.y + oy + internalOffsetY;
                        if (bond.type == BOND_DOUBLE) {
                            toBondExtra = 1.0f;
                        } else if (bond.type == BOND_TRIPLE) {
                            toBondExtra = 2.0f;
                        }                        
                    } else if (anchor.atomIndex >= 0 && anchor.atomIndex < static_cast<int>(compound.molecule.atoms.size())) {
                        const auto& atom = compound.molecule.atoms[anchor.atomIndex];
                        toPos.x = atom.position.x * _scale + layout.x + ox + internalOffsetX;
                        toPos.y = atom.position.y * _scale + layout.y + oy + internalOffsetY;
                        toIsAtom = !atom.text.empty() || !atom.charges.empty();
                    }
                    foundTo = true;
                }
            }
        }

        if (!foundFrom || !foundTo) {
            continue;
        }
        if (curve.controlPoints.size() < 2) {
            continue;
        }
        
        float controlAngle1 = curve.controlPoints[0].point.angle * CHEM_PI / 180.0f;
        float controlDist1 = curve.controlPoints[0].point.distance * _scale * 0.4f;
        float controlAngle2 = curve.controlPoints[1].point.angle * CHEM_PI / 180.0f;
        float controlDist2 = curve.controlPoints[1].point.distance * _scale * 0.4f;

        float cp1x = fromPos.x + controlDist1 * std::cos(controlAngle1);
        float cp1y = fromPos.y - controlDist1 * std::sin(controlAngle1);
        float cp2x = toPos.x + controlDist2 * std::cos(controlAngle2);
        float cp2y = toPos.y - controlDist2 * std::sin(controlAngle2);

        {
            float fromExtra = fromIsAtom ? 6.0f : fromBondExtra;
            float startShorten = curve.shortenStart + fromExtra;
            if (startShorten > 0.0f) {
                float dx = cp1x - fromPos.x;
                float dy = cp1y - fromPos.y;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.001f) {
                    float s = startShorten * _scale * 0.1f;
                    fromPos.x += dx / len * s;
                    fromPos.y += dy / len * s;
                }
            }
        }
        {
            float toExtra = toIsAtom ? 6.0f : toBondExtra;
            float endShorten = curve.shortenEnd + toExtra;
            if (endShorten > 0.0f) {
                float dx = toPos.x - cp2x;
                float dy = toPos.y - cp2y;
                float len = std::sqrt(dx * dx + dy * dy);
                if (len > 0.001f) {
                    float s = endShorten * _scale * 0.1f;
                    toPos.x -= dx / len * s;
                    toPos.y -= dy / len * s;
                }
            }
        }

        chemfig::CubicBezierApprox bez = {fromPos.x, fromPos.y, cp1x, cp1y, cp2x, cp2y, toPos.x, toPos.y};
        const int numPoints = chemfig::computeCurveSegments(bez);
        std::vector<ChemPoint> curvePoints(numPoints);
        for (int i = 0; i < numPoints; i++) {
            float t = static_cast<float>(i) / (numPoints - 1);
            float mt = 1.0f - t;
            curvePoints[i].x = mt*mt*mt*fromPos.x + 3*mt*mt*t*cp1x + 3*mt*t*t*cp2x + t*t*t*toPos.x;
            curvePoints[i].y = mt*mt*mt*fromPos.y + 3*mt*mt*t*cp1y + 3*mt*t*t*cp2y + t*t*t*toPos.y;
        }

        for (int i = 0; i < numPoints - 1; i++) {
            g2.drawLine(curvePoints[i].x, curvePoints[i].y, curvePoints[i+1].x, curvePoints[i+1].y);
        }

        if (curve.hasArrow) {
            float headLength = 0.25f;
            float headWidth = 0.13f;

            float lastAngle = std::atan2(curvePoints[numPoints-1].y - curvePoints[numPoints-2].y,
                                         curvePoints[numPoints-1].x - curvePoints[numPoints-2].x);

            float baseX = toPos.x - headLength * std::cos(lastAngle);
            float baseY = toPos.y - headLength * std::sin(lastAngle);

            float perpX = -std::sin(lastAngle);
            float perpY = std::cos(lastAngle);

            float leftX = baseX + perpX * headWidth;
            float leftY = baseY + perpY * headWidth;
            float rightX = baseX - perpX * headWidth;
            float rightY = baseY - perpY * headWidth;

            color oldColor = g2.getColor();
            const Stroke& savedStroke = g2.getStroke();
            float oldLineWidth = savedStroke.lineWidth;

            g2.setColor(black);
            g2.setStrokeWidth(BOND_LINE_WIDTH);

            int fillLines = 15;
            for (int i = 0; i < fillLines; i++) {
                float t = static_cast<float>(i + 1) / fillLines;
                float flx = toPos.x + (leftX - toPos.x) * t;
                float fly = toPos.y + (leftY - toPos.y) * t;
                float frx = toPos.x + (rightX - toPos.x) * t;
                float fry = toPos.y + (rightY - toPos.y) * t;
                g2.drawLine(flx, fly, frx, fry);
            }

            g2.drawLine(leftX, leftY, rightX, rightY);
            g2.drawLine(rightX, rightY, toPos.x, toPos.y);
            g2.drawLine(toPos.x, toPos.y, leftX, leftY);

            g2.setStrokeWidth(oldLineWidth);
            g2.setColor(oldColor);
        }
    }

    g2.setStroke(oldStroke);
}

void SchemeBox::drawDebug(Graphics2D& g2, float ox, float oy) {
    color oldColor = g2.getColor();
    g2.setColor(0xFF0000FF);

    for (const auto& cl : _compoundLayouts) {
        float x = cl.x + ox;
        float y = cl.y + oy;
        g2.drawLine(x, y - cl.boxHeight, x + cl.width, y - cl.boxHeight);
        g2.drawLine(x, y + cl.boxDepth, x + cl.width, y + cl.boxDepth);
        g2.drawLine(x, y - cl.boxHeight, x, y + cl.boxDepth);
        g2.drawLine(x + cl.width, y - cl.boxHeight, x + cl.width, y + cl.boxDepth);
    }

    g2.setColor(oldColor);
}

void SchemeBox::draw(Graphics2D& g2, float x, float y) {
    float ox = x + _xOffset;
    float oy = y;

    for (const auto& cl : _compoundLayouts) {
        if (cl.box && !cl.invisible) {
            cl.box->draw(g2, cl.x + ox, cl.y + oy);
        }
    }

    drawArrows(g2, ox, oy);
    drawPlusSigns(g2, ox, oy);
    drawMerges(g2, ox, oy);
    drawCompoundNames(g2, ox, oy);
    drawCompoundNumbers(g2, ox, oy);
    drawCurves(g2, ox, oy);
    drawSubschemeDelimiters(g2, ox, oy);

    if (SchemeConfig::instance().debugMode) {
        drawDebug(g2, ox, oy);
    }
}

void SchemeBox::drawSubschemeDelimiters(Graphics2D& g2, float ox, float oy) {
    for (size_t si = 0; si < _subschemeLayouts.size(); si++) {
        const auto& sl = _subschemeLayouts[si];
        if (sl.leftDelimBox) {
            float dx = sl.minX - sl.leftDelimWidth + ox;
            float totalH = sl.leftDelimBox->_height + sl.leftDelimBox->_depth;
            float dy = sl.centerY - totalH * 0.5f + sl.leftDelimBox->_height + oy;
            sl.leftDelimBox->draw(g2, dx, dy);
        }
        if (sl.rightDelimBox) {
            float dx = sl.maxX + ox;
            float totalH = sl.rightDelimBox->_height + sl.rightDelimBox->_depth;
            float dy = sl.centerY - totalH * 0.5f + sl.rightDelimBox->_height + oy;
            sl.rightDelimBox->draw(g2, dx, dy);
        }
    }
}

int SchemeBox::getLastFontId() {
    if (!_compoundLayouts.empty() && _compoundLayouts[0].box) {
        return _compoundLayouts[0].box->getLastFontId();
    }
    return TeXFont::NO_FONT;
}

} // namespace tex
