#include "EditHistory.h"
#include <cstdlib>
#include <iostream>
#include <map>

void Check(bool ok, const char* reason) {
    if (!ok) { std::cerr << reason << '\n'; std::exit(1); }
}
int main() {
    using namespace HPUI;
    EditValues start{0, 0, 0, 0, 0, 0, 1, 1, 1, 0, 0};
    EditHistory h;
    h.Observe(start);
    auto current = start;
    h.Record(current, 2, 0); current[2] = 1;
    h.Record(current, 2, 1); current[2] = 2;
    Check(h.undo.size() == 1 && SameValues(h.undo.back(), start), "Drag must undo as one operation");
    h.EndGesture(2);
    h.Record(current, 2, 2); current[2] = 3;
    Check(h.undo.size() == 2 && h.undo.back()[2] == 2, "Separate drags must not merge");
    h.EndGesture();
    h.Record(current, 34, 3); current[2] += .01f;
    h.Record(current, 34, 3.1); current[2] += .01f;
    Check(h.undo.size() == 3 && h.undo.back()[2] == 3, "Continuous wheel burst must merge");
    h.Record(current, 34, 3.6);
    Check(h.undo.size() == 4, "A later wheel burst needs its own undo");
    auto count = h.undo.size();
    auto restore = h.undo.back();
    Check(h.undo.size() == count, "Pending/failed undo must retain history");
    h.UndoSucceeded();
    Check(h.undo.size() == count-1 && SameValues(*h.baseline, start), "Successful undo pops one entry, never baseline");
    h.Record(current, -1, 4); // Successful baseline restore records what it replaced.
    Check(SameValues(h.undo.back(), current), "Start-state restore itself must be undoable");
    std::map<std::pair<int,int>, EditHistory> targets;
    targets[{2,-16777215}].Record(start, -1, 0);
    Check(targets[{3,110}].undo.empty(), "Target histories must be isolated, including high FormIDs");
    for (int i=0; i<100; ++i) { current[0] = static_cast<float>(i); h.Record(current, -1, 5+i); }
    Check(h.undo.size() == 64 && SameValues(*h.baseline,start), "History is bounded without losing baseline");
    Check(ClampEdit(15,0,1) == 10 && ClampEdit(15,0,2) == 15, "Beard has its own movement bounds");
    Check(ClampEdit(0,6,0) == .5f && ClampEdit(9,10,0) == 1, "Scale and toggle bounds");
    Check(std::abs(PrecisionStep(0,0)-.001f)<1e-6f && std::abs(PrecisionStep(3,1)-.1f)<1e-6f, "Precision mapping");
    EditHistory reopened;
    reopened.Observe(current);
    Check(reopened.undo.empty() && SameValues(*reopened.baseline,current), "Reopened session needs fresh baseline");
    std::cout << "History grouping, undo commit, restore, target isolation, bounds and precision passed\n";
}
