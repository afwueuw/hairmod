// SPDX-License-Identifier: GPL-3.0-or-later
#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <optional>
#include <vector>

namespace HPUI {
using EditValues = std::array<float, 11>;
inline bool SameValues(const EditValues& a, const EditValues& b) {
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (!std::isfinite(a[i]) || !std::isfinite(b[i]) || std::abs(a[i] - b[i]) > 0.00001f) { return false; }
    }
    return true;
}
inline float PrecisionStep(int row, int level) {
    constexpr float steps[] = {0.001f, 0.01f, 0.1f};
    return steps[std::clamp(level, 0, 2)] * (row >= 3 && row < 6 ? 10.f : 1.f);
}
inline float ClampEdit(float value, int row, int part) {
    const float move = part == 1 ? 10.f : 15.f;
    float lo = row < 3 ? -move : row < 6 ? -90.f : row < 9 ? .5f : 0.f;
    float hi = row < 3 ? move : row < 6 ? 90.f : row == 10 ? 1.f : 2.f;
    value = std::clamp(value, lo, hi);
    return row >= 9 ? std::round(value) : value;
}
struct EditHistory {
    std::optional<EditValues> baseline;
    std::vector<EditValues> undo;
    int group = -1;
    double lastChange = 0;
    void Observe(const EditValues& values) { if (!baseline) { baseline = values; } }
    // 0..8: active drag/text edit; 32..40: wheel burst; -1: single action.
    void Record(const EditValues& before, int nextGroup, double now) {
        Observe(before);
        bool sameGesture = nextGroup >= 0 && nextGroup == group &&
            (nextGroup < 32 || now - lastChange <= 0.35);
        if (!sameGesture) {
            if (undo.size() == 64) { undo.erase(undo.begin()); }
            undo.push_back(before);
        }
        group = nextGroup; lastChange = now;
    }
    void EndGesture(int id = -1) { if (id < 0 || group == id) { group = -1; } }
    void UndoSucceeded() { if (!undo.empty()) { undo.pop_back(); } EndGesture(); }
};
}
