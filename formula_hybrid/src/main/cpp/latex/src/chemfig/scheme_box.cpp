#include "scheme_box.h"
#include "chemfig_constants.h"
#include "chemfig_box.h"
#include "core/formula.h"
#include "atom/atom_basic.h"
#include "atom/box.h"
#include <cmath>
#include <algorithm>
#include <set>

namespace tex {

namespace {
    using namespace chemfig;

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

void SchemeBox::calculateLayout(TeXEnvironment& env) {
    _compoundLayouts.clear();
    _arrowLayouts.clear();
    _plusLayouts.clear();
    _mergeLayouts.clear();
    _subschemeLayouts.clear();

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

        if (!info.name.empty()) {
            layout.nameBox = createLabelBox(info.name, env);
        }
        if (info.number > 0) {
            std::wstring numText = L"(" + std::to_wstring(info.number) + L")";
            layout.numberBox = createLabelBox(numText, env);
        }
    }

    for (size_t si = 0; si < _scheme.subschemes.size(); si++) {
        const auto& subInfo = _scheme.subschemes[si];
        int firstIdx = subInfo.firstCompoundIndex;
        if (firstIdx < 0 || firstIdx >= static_cast<int>(_compoundLayouts.size())) continue;
        
        auto& firstL = _compoundLayouts[firstIdx];
        firstL.x = 0;
        firstL.y = 0;
        firstL.positioned = true;
        calculateCompoundAnchors(firstL);
        
        for (int arrowIdx : subInfo.internalArrows) {
            if (arrowIdx < 0 || arrowIdx >= static_cast<int>(_scheme.arrows.size())) continue;
            const ArrowElement& arrow = _scheme.arrows[arrowIdx];
            
            int fromIdx = arrow.fromCompound;
            int toIdx = arrow.toCompound;
            
            if (fromIdx < 0 || toIdx < 0) continue;
            if (fromIdx >= static_cast<int>(_compoundLayouts.size()) ||
                toIdx >= static_cast<int>(_compoundLayouts.size())) continue;
            
            auto& fromL = _compoundLayouts[fromIdx];
            if (!fromL.positioned) continue;
            
            bool hasAngle = (std::abs(arrow.params.angle) > EPSILON);
            
            float arrowLen = std::max(
                _arrowLength * arrow.params.lengthCoeff * _scale,
                _compoundGap);
            
            ChemPoint toPos;
            if (hasAngle) {
                float angleRad = arrow.params.angle * CHEM_PI / 180.0f;
                ChemPoint fromPos = resolveArrowEndpoint(
                    fromIdx, arrow.params.fromRef, arrow.params.fromAnchor,
                    arrow.params.angle, true);
                
                float centerX = fromPos.x + std::cos(angleRad) * arrowLen;
                float centerY = fromPos.y - std::sin(angleRad) * arrowLen;
                toPos = ChemPoint(centerX, centerY);
            } else {
                toPos = ChemPoint(
                    fromL.x + fromL.width + arrowLen,
                    fromL.y
                );
            }
            
            auto& toL = _compoundLayouts[toIdx];
            toL.x = toPos.x - toL.width * 0.5f;
            toL.y = toPos.y - (toL.boxDepth - toL.boxHeight) * 0.5f;
            toL.positioned = true;
            calculateCompoundAnchors(toL);
        }
    }

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
        
        sl.minX = firstL.x;
        sl.maxX = firstL.x + firstL.width;
        sl.minY = firstL.y - firstL.boxHeight;
        sl.maxY = firstL.y + firstL.boxDepth;

        for (int j = subInfo.startCompound; j <= subInfo.endCompound; j++) {
            const auto& cl = _compoundLayouts[j];
            if (!cl.positioned) continue;
            if (cl.x < sl.minX) sl.minX = cl.x;
            if (cl.x + cl.width > sl.maxX) sl.maxX = cl.x + cl.width;
            if (cl.y - cl.boxHeight < sl.minY) sl.minY = cl.y - cl.boxHeight;
            if (cl.y + cl.boxDepth > sl.maxY) sl.maxY = cl.y + cl.boxDepth;
        }

        sl.width = sl.maxX - sl.minX;
        sl.height = sl.maxY - sl.minY;
        sl.centerX = sl.minX + sl.width * 0.5f;
        sl.centerY = sl.minY + sl.height * 0.5f;

        float boxHeight = sl.height * 0.5f;
        float boxDepth = sl.height * 0.5f;
        sl.anchors = ReactionScheme::calculateCompoundAnchors(
            sl.minX, sl.centerY, sl.width, sl.height, boxHeight, boxDepth);

        if (!subInfo.leftDelim.empty() && subInfo.leftDelim != L".") {
            std::string symName = delimCharToSymbol(subInfo.leftDelim);
            if (!symName.empty()) {
                try {
                    float delimHeight = sl.height * SchemeConfig::instance().delimHeightScale;
                    sl.leftDelimBox = DelimiterFactory::create(symName, env, delimHeight);
                    sl.leftDelimWidth = sl.leftDelimBox->_width;
                } catch (...) {
                    sl.leftDelimBox = nullptr;
                    sl.leftDelimWidth = 0;
                }
            }
        }
        if (!subInfo.rightDelim.empty() && subInfo.rightDelim != L".") {
            std::string symName = delimCharToSymbol(subInfo.rightDelim);
            if (!symName.empty()) {
                try {
                    float delimHeight = sl.height * SchemeConfig::instance().delimHeightScale;
                    sl.rightDelimBox = DelimiterFactory::create(symName, env, delimHeight);
                    sl.rightDelimWidth = sl.rightDelimBox->_width;
                } catch (...) {
                    sl.rightDelimBox = nullptr;
                    sl.rightDelimWidth = 0;
                }
            }
        }
    }

    std::vector<std::pair<bool, int>> layoutElements;
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

    float lineSpacing = SchemeConfig::instance().lineSpacing;
    float currentY = 0.0f;
    float currentX = 0.0f;

    for (size_t i = 0; i < layoutElements.size(); i++) {
        const auto& le = layoutElements[i];
        if (le.first) {
            currentY += 100;
            currentX = 0;
            continue;
        }
        
        int idx = le.second;
        if (idx >= 0) {
            auto& layout = _compoundLayouts[idx];
            
            float gap = 0.0f;
            if (i > 0) {
                const auto& prevLe = layoutElements[i - 1];
                if (!prevLe.first && prevLe.second < -999 && prevLe.second >= -1999) {
                    int arrowIdx = -prevLe.second - 1000;
                    if (arrowIdx >= 0 && arrowIdx < static_cast<int>(_scheme.arrows.size())) {
                        const auto& arrow = _scheme.arrows[arrowIdx];
                        bool hasAngle = (std::abs(arrow.params.angle) > EPSILON);
                        if (!hasAngle) {
                            gap = _arrowLength * arrow.params.lengthCoeff * _scale;
                        } else {
                            gap = _compoundGap;
                        }
                    } else {
                        gap = _compoundGap;
                    }
                } else if (!prevLe.first && prevLe.second < -1999) {
                } else {
                    gap = _compoundGap;
                }
            }
            
            currentX += gap;
            
            float offsetX = currentX - layout.x;
            float offsetY = currentY - layout.y;
            layout.x += offsetX;
            layout.y += offsetY;
            calculateCompoundAnchors(layout);
            currentX += layout.width;
        } else if (idx < -1999) {
            int plusIdx = -idx - 2000;
            if (plusIdx >= 0 && plusIdx < static_cast<int>(_scheme.pluses.size())) {
                const auto& plus = _scheme.pluses[plusIdx];
                float emBase = _compoundGap / 5.0f;
                float sepLeft = emBase * 0.5f;
                float sepRight = emBase * 0.5f;
                float plusWidth = PLUS_SIGN_SIZE * _scale;
                
                if (plus.hasCustomSep) {
                    if (!plus.sepLeftRaw.empty()) {
                        sepLeft = parseLengthValue(plus.sepLeftRaw, _scale, emBase);
                    }
                    if (!plus.sepRightRaw.empty()) {
                        sepRight = parseLengthValue(plus.sepRightRaw, _scale, emBase);
                    }
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
                        if (arrowIdx >= 0 && arrowIdx < static_cast<int>(_scheme.arrows.size())) {
                            const auto& arrow = _scheme.arrows[arrowIdx];
                            bool hasAngle = (std::abs(arrow.params.angle) > EPSILON);
                            if (!hasAngle) {
                                gap = _arrowLength * arrow.params.lengthCoeff * _scale;
                            } else {
                                gap = _compoundGap;
                            }
                        } else {
                            gap = _compoundGap;
                        }
                    } else if (!prevLe.first && prevLe.second < -1999) {
                    } else {
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
                    cl.x += offsetX;
                    cl.y += offsetY;
                    calculateCompoundAnchors(cl);
                }
                
                sl.minX += offsetX;
                sl.maxX += offsetX;
                sl.minY += offsetY;
                sl.maxY += offsetY;
                sl.centerX += offsetX;
                sl.centerY += offsetY;

                float slBoxHeight = sl.height * 0.5f;
                float slBoxDepth = sl.height * 0.5f;
                float anchorX = sl.minX;
                float anchorW = sl.width;
                if (sl.leftDelimBox) {
                    anchorX -= (sl.leftDelimWidth + delimGap);
                    anchorW += (sl.leftDelimWidth + delimGap);
                }
                if (sl.rightDelimBox) {
                    anchorW += (sl.rightDelimWidth + delimGap);
                }
                sl.anchors = ReactionScheme::calculateCompoundAnchors(
                    anchorX, sl.centerY, anchorW, sl.height, slBoxHeight, slBoxDepth);
                
                currentX += leftDelimOffset + sl.width + sl.rightDelimWidth + delimGap;
            }
        }
    }

    std::vector<std::pair<float, float>> rowRanges;
    float currentRowMinY = 0;
    float currentRowMaxY = 0;
    float currentRowMaxDepth = 0;
    bool firstInRow = true;
    
    for (size_t i = 0; i < layoutElements.size(); i++) {
        const auto& le = layoutElements[i];
        if (le.first) {
            if (!firstInRow) {
                rowRanges.push_back(std::make_pair(currentRowMinY, currentRowMaxY));
                for (auto& cl : _compoundLayouts) {
                    if (cl.y >= currentRowMinY && cl.y <= currentRowMaxY) {
                        cl.rowMaxDepth = currentRowMaxDepth;
                    }
                }
            }
            currentRowMinY = 0;
            currentRowMaxY = 0;
            currentRowMaxDepth = 0;
            firstInRow = true;
            continue;
        }
        
        int idx = le.second;
        if (idx >= 0 && idx < static_cast<int>(_compoundLayouts.size())) {
            auto& cl = _compoundLayouts[idx];
            float bottom = cl.y + cl.boxDepth;
            if (firstInRow) {
                currentRowMinY = cl.y - cl.boxHeight;
                currentRowMaxY = bottom;
                currentRowMaxDepth = cl.boxDepth;
                firstInRow = false;
            } else {
                currentRowMinY = std::min(currentRowMinY, cl.y - cl.boxHeight);
                currentRowMaxY = std::max(currentRowMaxY, bottom);
                currentRowMaxDepth = std::max(currentRowMaxDepth, cl.boxDepth);
            }
        } else if (idx < 0 && idx > -1000) {
            int subIdx = -idx - 1;
            if (subIdx >= 0 && subIdx < static_cast<int>(_subschemeLayouts.size())) {
                const auto& sl = _subschemeLayouts[subIdx];
                const auto& subInfo = _scheme.subschemes[subIdx];
                float bottom = sl.maxY;
                float maxDepth = 0;
                for (int j = subInfo.startCompound; j <= subInfo.endCompound; j++) {
                    if (j >= 0 && j < static_cast<int>(_compoundLayouts.size())) {
                        maxDepth = std::max(maxDepth, _compoundLayouts[j].boxDepth);
                    }
                }
                if (firstInRow) {
                    currentRowMinY = sl.minY;
                    currentRowMaxY = bottom;
                    currentRowMaxDepth = maxDepth;
                    firstInRow = false;
                } else {
                    currentRowMinY = std::min(currentRowMinY, sl.minY);
                    currentRowMaxY = std::max(currentRowMaxY, bottom);
                    currentRowMaxDepth = std::max(currentRowMaxDepth, maxDepth);
                }
            }
        }
    }
    if (!firstInRow) {
        rowRanges.push_back(std::make_pair(currentRowMinY, currentRowMaxY));
        for (auto& cl : _compoundLayouts) {
            if (cl.y >= currentRowMinY && cl.y <= currentRowMaxY) {
                cl.rowMaxDepth = currentRowMaxDepth;
            }
        }
    }

    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first == ELEM_ARROW) {
            if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.arrows.size())) continue;
            const ArrowElement& arrow = _scheme.arrows[elem.second];
            ArrowLayout alayout;
            alayout.arrowIndex = elem.second;

            int fromIdx = arrow.fromCompound;
            int toIdx = arrow.toCompound;

            if (_scheme.compoundCount() == 0) continue;
            if (fromIdx < 0) fromIdx = 0;
            if (toIdx < 0) toIdx = std::min(fromIdx + 1, _scheme.compoundCount() - 1);
            if (fromIdx >= _scheme.compoundCount()) fromIdx = _scheme.compoundCount() - 1;
            if (toIdx >= _scheme.compoundCount()) toIdx = _scheme.compoundCount() - 1;

            bool hasAngle = (std::abs(arrow.params.angle) > EPSILON);
            float angleRad = hasAngle ? arrow.params.angle * CHEM_PI / 180.0f : 0.0f;

            ChemPoint fromPos = resolveArrowEndpoint(
                fromIdx, arrow.params.fromRef, arrow.params.fromAnchor,
                arrow.params.angle, true, arrow.fromSubschemeIdx);

            ChemPoint toPos = resolveArrowEndpoint(
                toIdx, arrow.params.toRef, arrow.params.toAnchor,
                arrow.params.angle, false, arrow.toSubschemeIdx);

            if (!hasAngle && fromIdx >= 0 && toIdx >= 0 &&
                fromIdx < static_cast<int>(_compoundLayouts.size()) &&
                toIdx < static_cast<int>(_compoundLayouts.size())) {
                auto& fL = _compoundLayouts[fromIdx];
                auto& tL = _compoundLayouts[toIdx];

                ChemPoint fromCenter = (arrow.fromSubschemeIdx >= 0 && arrow.fromSubschemeIdx < static_cast<int>(_subschemeLayouts.size()))
                    ? _subschemeLayouts[arrow.fromSubschemeIdx].anchors.center : fL.anchors.center;
                ChemPoint toCenter = (arrow.toSubschemeIdx >= 0 && arrow.toSubschemeIdx < static_cast<int>(_subschemeLayouts.size()))
                    ? _subschemeLayouts[arrow.toSubschemeIdx].anchors.center : tL.anchors.center;

                float dx = toCenter.x - fromCenter.x;
                float dy = toCenter.y - fromCenter.y;
                float dist = std::sqrt(dx * dx + dy * dy);
                if (dist > EPSILON) {
                    float actualAngle = std::atan2(-dy, dx) * 180.0f / CHEM_PI;
                    float fromAngle = actualAngle;
                    float entryAngle = actualAngle + 180.0f;

                    if (arrow.fromSubschemeIdx >= 0 && arrow.fromSubschemeIdx < static_cast<int>(_subschemeLayouts.size())) {
                        fromPos = _subschemeLayouts[arrow.fromSubschemeIdx].anchors.getAnchor(std::to_wstring(static_cast<int>(fromAngle)));
                    } else {
                        fromPos = fL.anchors.getAnchor(std::to_wstring(static_cast<int>(fromAngle)));
                    }
                    if (arrow.toSubschemeIdx >= 0 && arrow.toSubschemeIdx < static_cast<int>(_subschemeLayouts.size())) {
                        toPos = _subschemeLayouts[arrow.toSubschemeIdx].anchors.getAnchor(std::to_wstring(static_cast<int>(entryAngle)));
                    } else {
                        toPos = tL.anchors.getAnchor(std::to_wstring(static_cast<int>(entryAngle)));
                    }
                }
            }

            if (toIdx >= 0 && toIdx < static_cast<int>(_compoundLayouts.size())) {
                auto& tl = _compoundLayouts[toIdx];
                bool shouldReposition = hasAngle;
                if (shouldReposition) {
                    float arrowLen = _arrowLength * arrow.params.lengthCoeff * _scale;
                    float centerX = fromPos.x + std::cos(angleRad) * arrowLen;
                    float centerY = fromPos.y - std::sin(angleRad) * arrowLen;
                    tl.x = centerX - tl.width * 0.5f;
                    tl.y = centerY - (tl.boxDepth - tl.boxHeight) * 0.5f;
                    tl.positioned = true;
                    calculateCompoundAnchors(tl);
                    float entryAngle = arrow.params.angle + 180.0f;
                    toPos = tl.anchors.getAnchor(
                        std::to_wstring(static_cast<int>(entryAngle)));

                    float nextX = tl.x + tl.width;
                    for (int k = toIdx + 1; k < static_cast<int>(_compoundLayouts.size()); k++) {
                        auto& nextL = _compoundLayouts[k];
                        bool isArrowTarget = false;
                        bool isAngledTarget = false;
                        for (const auto& e2 : _scheme.elementOrder) {
                            if (e2.first == ELEM_ARROW) {
                                int aIdx = e2.second;
                                if (aIdx >= 0 && aIdx < static_cast<int>(_scheme.arrows.size())) {
                                    const auto& a = _scheme.arrows[aIdx];
                                    if (a.toCompound == k) {
                                        isArrowTarget = true;
                                        if (std::abs(a.params.angle) > EPSILON) {
                                            isAngledTarget = true;
                                        }
                                        break;
                                    }
                                }
                            }
                        }
                        if (!isAngledTarget && !nextL.invisible) {
                            float gap = isArrowTarget ? _compoundGap : 0.0f;
                            nextL.x = nextX + gap;
                            nextL.y = tl.y;
                            calculateCompoundAnchors(nextL);
                            nextX += nextL.width + gap;
                        } else {
                            break;
                        }
                    }
                }
            } else if (hasAngle) {
                float arrowLen = _arrowLength * arrow.params.lengthCoeff * _scale;
                toPos = ChemPoint(
                    fromPos.x + std::cos(angleRad) * arrowLen,
                    fromPos.y - std::sin(angleRad) * arrowLen
                );
            }

            float arrowLen = _arrowLength * arrow.params.lengthCoeff * _scale;
            if (std::abs(toPos.x - fromPos.x) < EPSILON &&
                std::abs(toPos.y - fromPos.y) < EPSILON) {
                toPos = ChemPoint(fromPos.x + arrowLen, fromPos.y);
            }

            alayout.from = fromPos;
            alayout.to = toPos;

            if (!arrow.params.yShiftRaw.empty()) {
                auto len = SpaceAtom::getLength(arrow.params.yShiftRaw);
                alayout.yShift = SpaceAtom::getSize(len.first, len.second, env);
            }

            if (!arrow.params.labelAbove.empty()) {
                alayout.labelAboveBox = createLabelBox(arrow.params.labelAbove, env);
            }
            if (!arrow.params.labelBelow.empty()) {
                alayout.labelBelowBox = createLabelBox(arrow.params.labelBelow, env);
            }

            _arrowLayouts.push_back(alayout);
        } else if (elem.first == ELEM_PLUS) {
            if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.pluses.size())) continue;
            const auto& plus = _scheme.pluses[elem.second];
            PlusLayout playout;
            int afterIdx = plus.afterCompound;
            if (afterIdx >= 0 && afterIdx < static_cast<int>(_compoundLayouts.size())) {
                auto& cl = _compoundLayouts[afterIdx];
                float emBase = _compoundGap / 5.0f;
                float sepLeft = emBase * 0.5f;
                float sepRight = emBase * 0.5f;
                float vshift = 0.0f;
                float halfSize = PLUS_SIGN_SIZE * 0.5f * _scale;

                if (plus.hasCustomSep) {
                    if (!plus.sepLeftRaw.empty()) {
                        sepLeft = parseLengthValue(plus.sepLeftRaw, _scale, emBase);
                    }
                    if (!plus.sepRightRaw.empty()) {
                        sepRight = parseLengthValue(plus.sepRightRaw, _scale, emBase);
                    }
                    if (!plus.vshiftRaw.empty()) {
                        vshift = parseLengthValue(plus.vshiftRaw, _scale, emBase);
                    }
                }

                playout.x = cl.x + cl.width + sepLeft + halfSize - std::min(0.0f, sepRight);
                playout.y = cl.y + (cl.boxDepth - cl.boxHeight) * 0.5f - vshift;
            }
            _plusLayouts.push_back(playout);
        }
    }

    std::set<int> repositionedMergeTargets;
    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first != ELEM_MERGE) continue;
        if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.merges.size())) continue;
        const MergeElement& merge = _scheme.merges[elem.second];
        int targetIdx = merge.target.compoundIndex;
        if (targetIdx < 0 || targetIdx >= _scheme.compoundCount()) continue;
        if (repositionedMergeTargets.count(targetIdx)) continue;

        float srcMinX = 0, srcMaxX = 0;
        float srcAnchorSumX = 0, srcAnchorSumY = 0;
        int srcCount = 0;
        bool hasValidSource = false;
        for (const auto& src : merge.sources) {
            if (src.compoundIndex < 0 || src.compoundIndex >= _scheme.compoundCount()) continue;
            if (src.compoundIndex == targetIdx) continue;
            const auto& scl = _compoundLayouts[src.compoundIndex];
            if (!hasValidSource) {
                srcMinX = scl.x; srcMaxX = scl.x + scl.width;
                hasValidSource = true;
            } else {
                srcMinX = std::min(srcMinX, scl.x);
                srcMaxX = std::max(srcMaxX, scl.x + scl.width);
            }
            ChemPoint anchorPt;
            if (!src.anchorName.empty()) {
                anchorPt = scl.anchors.getAnchor(src.anchorName);
            } else {
                switch (merge.direction) {
                    case MERGE_DOWN:  anchorPt = scl.anchors.south; break;
                    case MERGE_UP:    anchorPt = scl.anchors.north; break;
                    case MERGE_RIGHT: anchorPt = scl.anchors.east; break;
                    case MERGE_LEFT:  anchorPt = scl.anchors.west; break;
                    default:          anchorPt = scl.anchors.south; break;
                }
            }
            srcAnchorSumX += anchorPt.x;
            srcAnchorSumY += anchorPt.y;
            srcCount++;
        }
        if (!hasValidSource) continue;

        auto& tcl = _compoundLayouts[targetIdx];
        float baseMergeDist = _arrowLength * 2.0f * _scale;
        float hCenterX = (srcMinX + srcMaxX) * 0.5f;
        float avgSrcAnchorX = srcAnchorSumX / srcCount;
        float avgSrcAnchorY = srcAnchorSumY / srcCount;

        float mergeAngle = 0.0f;
        switch (merge.direction) {
            case MERGE_DOWN:  mergeAngle = -90.0f; break;
            case MERGE_UP:    mergeAngle = 90.0f;  break;
            case MERGE_RIGHT: mergeAngle = 0.0f;   break;
            case MERGE_LEFT:  mergeAngle = 180.0f; break;
        }
        float angleRad = mergeAngle * CHEM_PI / 180.0f;
        ChemPoint fromAnchor(avgSrcAnchorX, avgSrcAnchorY);
        float targetCenterX = fromAnchor.x + std::cos(angleRad) * baseMergeDist;
        float targetCenterY = fromAnchor.y - std::sin(angleRad) * baseMergeDist;
        tcl.x = targetCenterX - tcl.width * 0.5f;
        tcl.y = targetCenterY - (tcl.boxDepth - tcl.boxHeight) * 0.5f;
        calculateCompoundAnchors(tcl);
        tcl.invisible = true;
        repositionedMergeTargets.insert(targetIdx);
    }

    for (const auto& elem : _scheme.elementOrder) {
        if (elem.first != ELEM_MERGE) continue;
        if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.merges.size())) continue;
        const MergeElement& merge = _scheme.merges[elem.second];
        MergeLayout mlayout;
        mlayout.mergeIndex = elem.second;
        mlayout.direction = merge.direction;
        mlayout.segmentCoeff = merge.geometry.segmentCoeff;

        for (const auto& src : merge.sources) {
            int srcIdx = src.compoundIndex;
            if (srcIdx >= 0 && srcIdx < static_cast<int>(_compoundLayouts.size())) {
                if (srcIdx == merge.target.compoundIndex) continue;
                auto& cl = _compoundLayouts[srcIdx];
                ChemPoint pt;
                if (!src.anchorName.empty()) {
                    pt = cl.anchors.getAnchor(src.anchorName);
                } else {
                    switch (merge.direction) {
                        case MERGE_RIGHT: pt = cl.anchors.east; break;
                        case MERGE_LEFT:  pt = cl.anchors.west; break;
                        case MERGE_UP:    pt = cl.anchors.north; break;
                        case MERGE_DOWN:  pt = cl.anchors.south; break;
                    }
                }
                mlayout.fromPoints.push_back(pt);
            }
        }

        bool isHorizontalMerge = (mlayout.direction == MERGE_RIGHT || mlayout.direction == MERGE_LEFT);
        if (!isHorizontalMerge && mlayout.fromPoints.size() > 1) {
            mlayout.fromPoints.resize(1);
        }
        if (mlayout.fromPoints.empty()) {
            mlayout.fromPoints.resize(1);
        }

        int targetIdx = merge.target.compoundIndex;
        bool hasValidTarget = false;
        if (targetIdx >= 0 && targetIdx < static_cast<int>(_compoundLayouts.size())) {
            auto& cl = _compoundLayouts[targetIdx];
            if (!merge.target.anchorName.empty()) {
                mlayout.toPoint = cl.anchors.getAnchor(merge.target.anchorName);
            } else {
                switch (merge.direction) {
                    case MERGE_RIGHT: mlayout.toPoint = cl.anchors.west; break;
                    case MERGE_LEFT: mlayout.toPoint = cl.anchors.east; break;
                    case MERGE_UP: mlayout.toPoint = cl.anchors.south; break;
                    case MERGE_DOWN: mlayout.toPoint = cl.anchors.north; break;
                }
            }
            hasValidTarget = true;
        }

        if (!hasValidTarget && !mlayout.fromPoints.empty()) {
            float sumX = 0, sumY = 0;
            for (const auto& fp : mlayout.fromPoints) { sumX += fp.x; sumY += fp.y; }
            float avgX = sumX / mlayout.fromPoints.size();
            float avgY = sumY / mlayout.fromPoints.size();
            float ext = _arrowLength * _scale * 0.6f;
            switch (merge.direction) {
                case MERGE_RIGHT: mlayout.toPoint = ChemPoint(avgX + ext, avgY); break;
                case MERGE_LEFT:  mlayout.toPoint = ChemPoint(avgX - ext, avgY); break;
                case MERGE_UP:    mlayout.toPoint = ChemPoint(avgX, avgY - ext); break;
                case MERGE_DOWN:  mlayout.toPoint = ChemPoint(avgX, avgY + ext); break;
            }
        }

        _mergeLayouts.push_back(mlayout);
    }

    for (size_t si = 0; si < _scheme.subschemes.size(); si++) {
        const auto& subInfo = _scheme.subschemes[si];
        for (int arrowIdx : subInfo.internalArrows) {
            if (arrowIdx < 0 || arrowIdx >= static_cast<int>(_scheme.arrows.size())) continue;
            const ArrowElement& arrow = _scheme.arrows[arrowIdx];
            
            int fromIdx = arrow.fromCompound;
            int toIdx = arrow.toCompound;
            
            if (fromIdx < 0 || toIdx < 0) continue;
            if (fromIdx >= static_cast<int>(_compoundLayouts.size()) ||
                toIdx >= static_cast<int>(_compoundLayouts.size())) continue;
            
            ArrowLayout alayout;
            alayout.arrowIndex = arrowIdx;
            
            ChemPoint fromPos = resolveArrowEndpoint(
                fromIdx, arrow.params.fromRef, arrow.params.fromAnchor,
                arrow.params.angle, true);
            
            ChemPoint toPos = resolveArrowEndpoint(
                toIdx, arrow.params.toRef, arrow.params.toAnchor,
                arrow.params.angle, false);
            
            alayout.from = fromPos;
            alayout.to = toPos;
            
            if (!arrow.params.labelAbove.empty()) {
                alayout.labelAboveBox = createLabelBox(arrow.params.labelAbove, env);
            }
            if (!arrow.params.labelBelow.empty()) {
                alayout.labelBelowBox = createLabelBox(arrow.params.labelBelow, env);
            }
            
            _arrowLayouts.push_back(alayout);
        }
    }

    float totalWidth = 0.0f;
    float minLeft = 0.0f;
    float maxBottom = 0.0f;
    float maxTop = 0.0f;

    for (const auto& cl : _compoundLayouts) {
        float right = cl.x + cl.width;
        if (right > totalWidth) totalWidth = right;
        if (cl.x < minLeft) minLeft = cl.x;

        float bottom = cl.y + cl.boxDepth;
        float top = cl.y - cl.boxHeight;
        if (bottom > maxBottom) maxBottom = bottom;
        if (top < maxTop) maxTop = top;

        if (cl.nameBox) {
            float nameBottom = cl.y + cl.boxDepth +
                               SchemeConfig::instance().nameOffset + cl.nameBox->_height + cl.nameBox->_depth;
            if (nameBottom > maxBottom) maxBottom = nameBottom;
        }
        if (cl.numberBox) {
            float numBottom = cl.y + cl.boxDepth +
                              SchemeConfig::instance().numOffset + cl.numberBox->_height + cl.numberBox->_depth;
            if (numBottom > maxBottom) maxBottom = numBottom;
        }
    }

    for (const auto& al : _arrowLayouts) {
        float minX = std::min(al.from.x, al.to.x);
        float maxX = std::max(al.from.x, al.to.x);
        float minY = std::min(al.from.y, al.to.y);
        float maxY = std::max(al.from.y, al.to.y);

        if (al.arrowIndex >= 0 && al.arrowIndex < static_cast<int>(_scheme.arrows.size())) {
            const auto& arrow = _scheme.arrows[al.arrowIndex];
            if (arrow.params.isCurved()) {
                ChemPoint ctrl = ArrowRenderer::computeCurveControlPoint(
                    al.from, al.to, arrow.params.effectiveCurveHeight());
                if (ctrl.x < minX) minX = ctrl.x;
                if (ctrl.x > maxX) maxX = ctrl.x;
                if (ctrl.y < minY) minY = ctrl.y;
                if (ctrl.y > maxY) maxY = ctrl.y;
            }
        }

        if (maxX > totalWidth) totalWidth = maxX;
        if (minX < minLeft) minLeft = minX;
        if (minY < maxTop) maxTop = minY;
        if (maxY > maxBottom) maxBottom = maxY;

        if (al.labelAboveBox) {
            float labelMidY = (al.from.y + al.to.y) * 0.5f;
            float labelTop = labelMidY - LABEL_OFFSET - al.labelAboveBox->_depth - al.labelAboveBox->_height;
            if (labelTop < maxTop) maxTop = labelTop;
        }
        if (al.labelBelowBox) {
            float labelMidY = (al.from.y + al.to.y) * 0.5f;
            float labelBottom = labelMidY + LABEL_OFFSET + al.labelBelowBox->_height + al.labelBelowBox->_depth;
            if (labelBottom > maxBottom) maxBottom = labelBottom;
        }
    }

    for (const auto& pl : _plusLayouts) {
        float plusRight = pl.x + PLUS_SIGN_SIZE;
        if (plusRight > totalWidth) totalWidth = plusRight;
    }

    for (const auto& ml : _mergeLayouts) {
        for (const auto& fp : ml.fromPoints) {
            if (fp.x > totalWidth) totalWidth = fp.x;
            if (fp.x < minLeft) minLeft = fp.x;
            if (fp.y > maxBottom) maxBottom = fp.y;
            if (fp.y < maxTop) maxTop = fp.y;
        }
        if (ml.toPoint.x > totalWidth) totalWidth = ml.toPoint.x;
        if (ml.toPoint.x < minLeft) minLeft = ml.toPoint.x;
        if (ml.toPoint.y > maxBottom) maxBottom = ml.toPoint.y;
        if (ml.toPoint.y < maxTop) maxTop = ml.toPoint.y;
    }

    for (const auto& sl : _subschemeLayouts) {
        if (sl.leftDelimBox) {
            float delimTop = sl.centerY - sl.leftDelimBox->_height;
            float delimBottom = sl.centerY + sl.leftDelimBox->_depth;
            if (delimTop < maxTop) maxTop = delimTop;
            if (delimBottom > maxBottom) maxBottom = delimBottom;
        }
        if (sl.rightDelimBox) {
            float delimRight = sl.maxX + sl.rightDelimWidth;
            if (delimRight > totalWidth) totalWidth = delimRight;
            float delimTop = sl.centerY - sl.rightDelimBox->_height;
            float delimBottom = sl.centerY + sl.rightDelimBox->_depth;
            if (delimTop < maxTop) maxTop = delimTop;
            if (delimBottom > maxBottom) maxBottom = delimBottom;
        }
    }

    _width = totalWidth - minLeft + 2 * PADDING;
    _height = (-maxTop) + PADDING;
    _depth = maxBottom + PADDING;
    _xOffset = -minLeft + PADDING;
    _foreground = _color;
}

void SchemeBox::drawArrows(Graphics2D& g2, float ox, float oy) {
    SchemeConfig& cfg = SchemeConfig::instance();
    ArrowStyle style;
    style.headLength = cfg.arrowHeadLength;
    style.headWidth = cfg.arrowHeadWidth;
    style.lineWidth = ARROW_LINE_WIDTH;
    style.doubleBondOffset = cfg.arrowDoubleSep;
    style.harpRadius = ARROW_HARP_RADIUS;

    float arrowOffset = cfg.arrowOffset * _scale;
    float labelSep = cfg.arrowLabelSep * _scale;

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

        ArrowRenderer::drawArrow(g2, arrow.params.type,
                                 fromPos, toPos, _scale,
                                 arrow.params, style);

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
                spineStart = (target.x >= maxSpineEnd) ? maxSpineEnd : minSpineEnd;
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

        const int numPoints = 50;
        ChemPoint curvePoints[numPoints];
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
