#include "scheme_box.h"
#include "chemfig_constants.h"
#include "chemfig_box.h"
#include "core/formula.h"
#include "atom/atom_basic.h"
#include <cmath>
#include <algorithm>

namespace tex {

namespace {
    using namespace chemfig;

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
            
            bool hasAngle = (std::abs(arrow.params.angle) > EPSILON);
            if (!hasAngle) continue;
            
            float angleRad = arrow.params.angle * CHEM_PI / 180.0f;
            
            ChemPoint fromPos = resolveArrowEndpoint(
                fromIdx, arrow.params.fromRef, arrow.params.fromAnchor,
                arrow.params.angle, true);
            
            float arrowLen = std::max(
                _arrowLength * arrow.params.lengthCoeff * _scale,
                _compoundGap);
            
            float centerX = fromPos.x + std::cos(angleRad) * arrowLen;
            float centerY = fromPos.y - std::sin(angleRad) * arrowLen;
            
            auto& toL = _compoundLayouts[toIdx];
            toL.x = centerX - toL.width * 0.5f;
            toL.y = centerY - (toL.boxDepth - toL.boxHeight) * 0.5f;
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
                
                float offsetX = currentX - sl.minX;
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
                sl.anchors = ReactionScheme::calculateCompoundAnchors(
                    sl.minX, sl.centerY, sl.width, sl.height, slBoxHeight, slBoxDepth);
                
                currentX += sl.width;
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
                        if (!isAngledTarget) {
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
                float vshift = 0.0f;
                float halfSize = PLUS_SIGN_SIZE * 0.5f * _scale;
                
                if (plus.hasCustomSep) {
                    if (!plus.sepLeftRaw.empty()) {
                        sepLeft = parseLengthValue(plus.sepLeftRaw, _scale, emBase);
                    }
                    if (!plus.vshiftRaw.empty()) {
                        vshift = parseLengthValue(plus.vshiftRaw, _scale, emBase);
                    }
                }
                
                playout.x = cl.x + cl.width + sepLeft + halfSize;
                playout.y = cl.y + (cl.boxDepth - cl.boxHeight) * 0.5f - vshift;
            }
            _plusLayouts.push_back(playout);
        } else if (elem.first == ELEM_MERGE) {
            if (elem.second < 0 || elem.second >= static_cast<int>(_scheme.merges.size())) continue;
            const MergeElement& merge = _scheme.merges[elem.second];
            MergeLayout mlayout;
            mlayout.mergeIndex = elem.second;
            mlayout.direction = merge.direction;
            mlayout.segmentCoeff = merge.geometry.segmentCoeff;

            for (const auto& src : merge.sources) {
                int srcIdx = src.compoundIndex;
                if (srcIdx >= 0 && srcIdx < static_cast<int>(_compoundLayouts.size())) {
                    auto& cl = _compoundLayouts[srcIdx];
                    if (!src.anchorName.empty()) {
                        mlayout.fromPoints.push_back(cl.anchors.getAnchor(src.anchorName));
                    } else {
                        switch (merge.direction) {
                            case MERGE_RIGHT:
                            case MERGE_LEFT:
                                mlayout.fromPoints.push_back(cl.anchors.south);
                                break;
                            case MERGE_UP:
                            case MERGE_DOWN:
                                mlayout.fromPoints.push_back(cl.anchors.east);
                                break;
                        }
                    }
                }
            }

            int targetIdx = merge.target.compoundIndex;
            if (targetIdx >= 0 && targetIdx < static_cast<int>(_compoundLayouts.size())) {
                auto& cl = _compoundLayouts[targetIdx];
                if (!merge.target.anchorName.empty()) {
                    mlayout.toPoint = cl.anchors.getAnchor(merge.target.anchorName);
                } else {
                    switch (merge.direction) {
                        case MERGE_RIGHT:
                        case MERGE_LEFT:
                            mlayout.toPoint = cl.anchors.north;
                            break;
                        case MERGE_UP:
                        case MERGE_DOWN:
                            mlayout.toPoint = cl.anchors.west;
                            break;
                    }
                }
            }

            _mergeLayouts.push_back(mlayout);
        }
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
    float maxBottom = 0.0f;
    float maxTop = 0.0f;

    for (const auto& cl : _compoundLayouts) {
        float right = cl.x + cl.width;
        if (right > totalWidth) totalWidth = right;

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
            if (fp.y > maxBottom) maxBottom = fp.y;
            if (fp.y < maxTop) maxTop = fp.y;
        }
        if (ml.toPoint.x > totalWidth) totalWidth = ml.toPoint.x;
        if (ml.toPoint.y > maxBottom) maxBottom = ml.toPoint.y;
        if (ml.toPoint.y < maxTop) maxTop = ml.toPoint.y;
    }

    _width = totalWidth + 2 * PADDING;
    _height = (-maxTop) + PADDING;
    _depth = maxBottom + PADDING;
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
    style.dashLength = ARROW_DASH_LENGTH;
    style.dashGap = ARROW_DASH_GAP;

    float arrowOffset = cfg.arrowOffset * _scale;
    float labelSep = cfg.arrowLabelSep * _scale;

    for (const auto& al : _arrowLayouts) {
        if (al.arrowIndex < 0 || al.arrowIndex >= static_cast<int>(_scheme.arrows.size())) continue;
        const auto& arrow = _scheme.arrows[al.arrowIndex];

        ChemPoint fromPos(al.from.x + ox, al.from.y + oy - al.yShift - arrowOffset);
        ChemPoint toPos(al.to.x + ox, al.to.y + oy - al.yShift - arrowOffset);

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

    float headLength = SchemeConfig::instance().arrowHeadLength * _scale;
    float headWidth = SchemeConfig::instance().arrowHeadWidth * _scale;

    for (const auto& ml : _mergeLayouts) {
        if (ml.fromPoints.empty()) continue;

        ChemPoint target(ml.toPoint.x + ox, ml.toPoint.y + oy);

        for (const auto& fromPt : ml.fromPoints) {
            ChemPoint source(fromPt.x + ox, fromPt.y + oy);

            float dx = target.x - source.x;
            float dy = target.y - source.y;
            float len = std::sqrt(dx * dx + dy * dy);
            if (len < EPSILON) continue;

            float dirX = dx / len;
            float dirY = dy / len;
            ChemPoint perp(-dirY, dirX);

            float segLen = len * ml.segmentCoeff;
            float offset = len * MERGE_BEND_OFFSET_RATIO;
            float bendSign = (ml.direction == MERGE_RIGHT || ml.direction == MERGE_DOWN) ? 1.0f : -1.0f;
            bool isVertical = (ml.direction == MERGE_UP || ml.direction == MERGE_DOWN);

            if (isVertical) {
                float vertOffset = len * MERGE_BEND_OFFSET_RATIO * bendSign;
                ChemPoint bend(source.x, source.y + vertOffset);
                g2.drawLine(source.x, source.y, bend.x, bend.y);
                g2.drawLine(bend.x, bend.y, target.x, target.y);
            } else {
                ChemPoint midPoint(source.x + dirX * segLen, source.y + dirY * segLen);
                ChemPoint bend(midPoint.x + perp.x * offset * bendSign,
                               midPoint.y + perp.y * offset * bendSign);
                g2.drawLine(source.x, source.y, bend.x, bend.y);
                g2.drawLine(bend.x, bend.y, target.x, target.y);
            }

            ArrowRenderer::drawMergeArrowHead(g2, target, source, _scale, headLength, headWidth);
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
    float ox = x + PADDING;
    float oy = y;

    for (const auto& cl : _compoundLayouts) {
        if (cl.box) {
            cl.box->draw(g2, cl.x + ox, cl.y + oy);
        }
    }

    drawArrows(g2, ox, oy);
    drawPlusSigns(g2, ox, oy);
    drawMerges(g2, ox, oy);
    drawCompoundNames(g2, ox, oy);
    drawCompoundNumbers(g2, ox, oy);

    if (SchemeConfig::instance().debugMode) {
        drawDebug(g2, ox, oy);
    }
}

int SchemeBox::getLastFontId() {
    if (!_compoundLayouts.empty() && _compoundLayouts[0].box) {
        return _compoundLayouts[0].box->getLastFontId();
    }
    return TeXFont::NO_FONT;
}

} // namespace tex
