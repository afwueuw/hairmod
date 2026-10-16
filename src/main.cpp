// SPDX-License-Identifier: GPL-3.0-or-later
#include "pch.h"
#include "SKSEMenuFramework.h"
#include <charconv>
#include <map>
#include <set>
#include <deque>
#include <iomanip>
#include <sstream>
#include <limits>
#include <cwchar>
#include <string_view>
#include "EditHistory.h"
namespace ImGui = ImGuiMCP;

namespace HPUI
{
    using Clock = std::chrono::steady_clock;
    constexpr char Script[] = "HPUIBridge";
    constexpr std::size_t Rows = 11;
    struct Target {
        std::int32_t part = 0;
        std::int32_t item = 0;
        std::string name = "헤어";
        bool Same(const Target& other) const { return part == other.part && item == other.item; }
    };
    struct State {
        std::array<float, 15> snapshot{};
        std::vector<Target> targets;
        Target selected;
        bool catalogRefresh = true;
        bool catalogBusy = false;
        Clock::time_point lastCatalog{};
        std::array<std::optional<float>, Rows> queued{};
        std::optional<std::pair<int, float>> activeEdit;
        bool valid = false;
        bool busy = false;                 // Any Papyrus request is in flight.
        bool writeBusy = false;            // Only writes block editing/selection.
        bool resetQueued = false;
        bool refresh = true;
        bool beginSession = true;
        std::map<std::pair<std::int32_t, std::int32_t>, EditHistory> histories;
        std::set<std::pair<std::int32_t, std::int32_t>> restoredThisSession;
        std::deque<std::pair<Target, EditValues>> autoRestoreQueue;
        std::optional<Target> autoRestoreActive;
        struct Restore { EditValues desired; EditValues before; bool undo; };
        std::optional<Restore> restoreQueued;
        std::optional<Restore> restoreActive;
        std::uint64_t generation = 0;
        std::string error;
        Clock::time_point started{};
        Clock::time_point lastReply{};
    };
    State state;
    std::mutex stateMutex;
    SKSEMenuFramework::Model::WindowInterface* window = nullptr;
    std::atomic<bool> inGame{false};
    constexpr std::uint32_t DefaultHotkey = RE::BSKeyboardDevice::Key::kF3;
    std::atomic<std::uint32_t> hotkey{DefaultHotkey};
    std::atomic<bool> capturingKey{false};
    std::atomic<int> precision{1};
    std::atomic<bool> manipulating{false};
    std::mutex settingsMutex;
    std::string settingsMessage;
    std::filesystem::path settingsPath;
    void Pump();

    auto Identity(const Target& target) { return std::pair{target.part, target.item}; }
    EditValues Values(const std::array<float, 15>& snapshot) {
        EditValues result{};
        std::copy_n(snapshot.begin(), 10, result.begin()); result[10] = snapshot[14];
        return result;
    }
    EditValues EffectiveValues(const State& source) {
        auto result = Values(source.snapshot);
        if (source.activeEdit) { result[source.activeEdit->first] = source.activeEdit->second; }
        for (std::size_t i = 0; i < Rows; ++i) { if (source.queued[i]) { result[i] = *source.queued[i]; } }
        return result;
    }
    double Now() { return std::chrono::duration<double>(Clock::now().time_since_epoch()).count(); }
    bool Pending(const State& source) {
        // Read-only snapshots/catalog scans are background work. They must not
        // disable the controls every second; only edits/restores do.
        return source.writeBusy || source.resetQueued || source.restoreQueued || source.restoreActive ||
            std::any_of(source.queued.begin(), source.queued.end(), [](const auto& v) { return v.has_value(); });
    }

    std::wstring TargetSection(const Target& target) {
        return L"Target_" + std::to_wstring(target.part) + L"_" + std::to_wstring(target.item);
    }

    bool LoadPersistentValues(const Target& target, EditValues& out) {
        std::scoped_lock lock(settingsMutex);
        const auto section = TargetSection(target);
        wchar_t buffer[128]{};
        GetPrivateProfileStringW(section.c_str(), L"Saved", L"", buffer, static_cast<DWORD>(std::size(buffer)), settingsPath.c_str());
        if (std::wstring_view(buffer) != L"1") { return false; }
        for (std::size_t i = 0; i < out.size(); ++i) {
            const auto key = L"V" + std::to_wstring(i);
            buffer[0] = L'\0';
            GetPrivateProfileStringW(section.c_str(), key.c_str(), L"", buffer, static_cast<DWORD>(std::size(buffer)), settingsPath.c_str());
            if (buffer[0] == L'\0') { return false; }
            wchar_t* end = nullptr;
            const float value = std::wcstof(buffer, &end);
            if (end == buffer || *end != L'\0' || !std::isfinite(value)) { return false; }
            out[i] = value;
        }
        for (std::size_t i = 0; i < out.size(); ++i) { out[i] = ClampEdit(out[i], static_cast<int>(i), target.part); }
        return true;
    }

    void SavePersistentValues(const Target& target, const EditValues& values) {
        std::scoped_lock lock(settingsMutex);
        const auto section = TargetSection(target);
        for (std::size_t i = 0; i < values.size(); ++i) {
            char narrow[64]{};
            const auto result = std::to_chars(narrow, narrow + sizeof(narrow), values[i], std::chars_format::general, std::numeric_limits<float>::max_digits10);
            if (result.ec != std::errc{}) { continue; }
            const auto key = L"V" + std::to_wstring(i);
            const std::wstring value(narrow, result.ptr);
            if (!WritePrivateProfileStringW(section.c_str(), key.c_str(), value.c_str(), settingsPath.c_str())) {
                SKSE::log::warn("Persistent target setting write failed: part={} id={}", target.part, target.item);
                return;
            }
        }
        WritePrivateProfileStringW(section.c_str(), L"Saved", L"1", settingsPath.c_str());
    }
    void QueueRestore(const Target& expected, bool undo) {
        std::scoped_lock lock(stateMutex);
        if (Pending(state) || state.beginSession || !state.valid || !state.selected.Same(expected)) { return; }
        auto& history = state.histories[Identity(expected)];
        if (!history.baseline || (undo && history.undo.empty())) { return; }
        const auto before = EffectiveValues(state);
        auto desired = undo ? history.undo.back() : *history.baseline;
        if (!undo && SameValues(before, desired)) { return; }
        for (std::size_t i = 0; i < Rows; ++i) { desired[i] = ClampEdit(desired[i], static_cast<int>(i), expected.part); }
        history.EndGesture();
        state.restoreQueued = State::Restore{desired, before, undo};
    }

    std::string KeyName(std::uint32_t code) {
        if (!code) { return "사용 안 함"; }
        wchar_t wide[64]{};
        LONG scan = static_cast<LONG>((code & 0x7f) << 16);
        if (code & 0x80) { scan |= 1 << 24; }
        if (GetKeyNameTextW(scan, wide, 64) > 0) {
            char utf8[256]{};
            if (WideCharToMultiByte(CP_UTF8, 0, wide, -1, utf8, sizeof(utf8), nullptr, nullptr)) { return utf8; }
        }
        return "키 " + std::to_string(code);
    }

    bool SaveHotkey(std::uint32_t code) {
        std::scoped_lock lock(settingsMutex);
        auto value = std::to_wstring(code);
        if (!WritePrivateProfileStringW(L"Hotkey", L"Key", value.c_str(), settingsPath.c_str())) {
            settingsMessage = "설정 저장 실패: HairPositionerUI.ini의 쓰기 권한을 확인하세요.";
            SKSE::log::error("Hotkey save failed, Windows error {}", GetLastError());
            return false;
        }
        hotkey = code;
        capturingKey = false;
        settingsMessage = "저장됨: " + KeyName(code);
        SKSE::log::info("Hotkey saved: {}", code);
        return true;
    }

    void LoadHotkey() {
        settingsPath = std::filesystem::absolute(L"Data/SKSE/Plugins/HairPositionerUI.ini");
        auto code = GetPrivateProfileIntW(L"Hotkey", L"Key", DefaultHotkey, settingsPath.c_str());
        if (code > 255) { SKSE::log::warn("Invalid hotkey {}, using F3", code); code = DefaultHotkey; }
        hotkey = code;
        SKSE::log::info("Hotkey loaded: {} (0 = disabled)", code);
        auto level = GetPrivateProfileIntW(L"Editing", L"Precision", 1, settingsPath.c_str());
        precision = level <= 2 ? static_cast<int>(level) : 1;
    }

    void SavePrecision(int level) {
        std::scoped_lock lock(settingsMutex);
        auto value = std::to_wstring(level);
        if (WritePrivateProfileStringW(L"Editing", L"Precision", value.c_str(), settingsPath.c_str())) {
            precision = level; settingsMessage.clear();
        } else { settingsMessage = "정밀도 설정 저장 실패: INI 쓰기 권한을 확인하세요."; }
    }

    void RenderHotkeySettings() {
        ImGui::Separator();
        ImGui::Text("조절 창 핫키: %s", KeyName(hotkey.load()).c_str());
        if (capturingKey.load()) {
            ImGui::TextUnformatted("등록할 키를 누르세요. Esc로 취소합니다.");
            if (ImGui::Button("키 등록 취소")) { capturingKey = false; }
        } else if (ImGui::Button("핫키 변경")) {
            capturingKey = true;
            std::scoped_lock lock(settingsMutex); settingsMessage.clear();
        }
        ImGui::SameLine();
        if (ImGui::Button("핫키 끄기")) { SaveHotkey(0); }
        ImGui::SameLine();
        if (ImGui::Button("기본 F3")) { SaveHotkey(DefaultHotkey); }
        std::string message;
        { std::scoped_lock lock(settingsMutex); message = settingsMessage; }
        if (!message.empty()) { ImGui::TextWrapped("%s", message.c_str()); }
    }

    class Reply final : public RE::BSScript::IStackCallbackFunctor {
    public:
        Reply(std::uint64_t epoch, Target target, bool persist, bool autoRestore, EditValues desired = {}) :
            epoch_(epoch), target_(std::move(target)), persist_(persist), autoRestore_(autoRestore), desired_(desired) {}
        void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
        void operator()(RE::BSScript::Variable result) override {
            std::array<float, 15> data{};
            bool good = result.IsArray();
            auto values = good ? result.GetArray() : nullptr;
            good = values && values->size() == data.size();
            if (good) {
                for (std::size_t i = 0; i < data.size(); ++i) {
                    if (!(*values)[static_cast<std::uint32_t>(i)].IsFloat()) { good = false; break; }
                    data[i] = (*values)[static_cast<std::uint32_t>(i)].GetFloat();
                    if (!std::isfinite(data[i])) { good = false; break; }
                }
            }
            bool save = false;
            EditValues toSave{};
            {
                std::scoped_lock lock(stateMutex);
                if (epoch_ != state.generation) { return; }
                const bool stillSelected = state.selected.Same(target_);
                if (good && stillSelected) {
                    state.snapshot = data; state.valid = true; state.error.clear();
                    bool available = data[10] > .5f && data[11] > .5f && data[12] > .5f;
                    auto& history = state.histories[Identity(target_)];
                    if (available && !state.beginSession) { history.Observe(Values(data)); }
                    if (state.restoreActive) {
                        auto request = *state.restoreActive;
                        if (available && SameValues(Values(data), request.desired)) {
                            if (request.undo) { history.UndoSucceeded(); }
                            else { history.Record(request.before, -1, Now()); }
                        } else { state.error = "복원 결과가 요청값과 다릅니다. 현재값과 대상 착용 상태를 확인하세요."; }
                    }
                } else if (!good) {
                    state.error = "브리지 응답 오류: HPUIBridge.pex와 Papyrus 로그를 확인하세요.";
                }
                if (autoRestore_) {
                    state.autoRestoreActive.reset();
                    if (good && data[10] > .5f && data[11] > .5f && data[12] > .5f && SameValues(Values(data), desired_)) {
                        state.restoredThisSession.insert(Identity(target_));
                        save = true;
                    } else {
                        SKSE::log::warn("Persistent settings not applied or target unavailable: part={} id={}", target_.part, target_.item);
                    }
                    if (state.autoRestoreQueue.empty()) { state.refresh = true; }
                }
                state.busy = false;
                state.writeBusy = false;
                state.activeEdit.reset();
                state.restoreActive.reset();
                if (!stillSelected) { state.refresh = true; }
                state.lastReply = Clock::now();
                if (good && persist_) { save = true; }
                if (save && good) { toSave = Values(data); }
            }
            if (save && good) { SavePersistentValues(target_, toSave); }
            Pump(); // Flush accepted edits and queued persistent restores.
        }
    private:
        std::uint64_t epoch_;
        Target target_;
        bool persist_;
        bool autoRestore_;
        EditValues desired_{};
    };

    class CatalogReply final : public RE::BSScript::IStackCallbackFunctor {
    public:
        explicit CatalogReply(std::uint64_t epoch) : epoch_(epoch) {}
        void SetObject(const RE::BSTSmartPointer<RE::BSScript::Object>&) override {}
        void operator()(RE::BSScript::Variable result) override {
            auto values = result.IsArray() ? result.GetArray() : nullptr;
            bool good = values && values->size() >= 6 && values->size() % 3 == 0;
            std::vector<Target> targets;
            if (good) {
                for (std::uint32_t i = 0; i < values->size(); i += 3) {
                    if (!(*values)[i].IsString() || !(*values)[i+1].IsString() || !(*values)[i+2].IsString()) {
                        good = false; break;
                    }
                    auto part = (*values)[i].GetString();
                    if (part.empty()) { continue; }
                    auto id = (*values)[i+1].GetString();
                    Target target;
                    auto p = std::from_chars(part.data(), part.data()+part.size(), target.part);
                    auto n = std::from_chars(id.data(), id.data()+id.size(), target.item);
                    if (p.ec != std::errc{} || p.ptr != part.data()+part.size() ||
                        n.ec != std::errc{} || n.ptr != id.data()+id.size() || target.part < 0 || target.part > 3) {
                        good = false; break;
                    }
                    auto name = std::string((*values)[i+2].GetString());
                    target.name = target.part == 0 ? "헤어" : target.part == 1 ? "수염" :
                        (target.part == 2 ? "착용: " : "추가 파츠: ") + (name.empty() ? std::to_string(target.item) : name);
                    // Part + signed Int ID, rather than list position or name, owns every edit.
                    if (std::none_of(targets.begin(), targets.end(), [&](const auto& t) { return t.Same(target); })) {
                        targets.push_back(std::move(target));
                    }
                }
            }
            std::vector<std::pair<Target, EditValues>> savedTargets;
            if (good) {
                for (const auto& target : targets) {
                    EditValues saved{};
                    if (LoadPersistentValues(target, saved)) { savedTargets.emplace_back(target, saved); }
                }
            }
            {
                std::scoped_lock lock(stateMutex);
                if (epoch_ != state.generation) { return; }
                state.busy = false; state.writeBusy = false; state.catalogBusy = false;
                state.lastCatalog = state.lastReply = Clock::now();
                if (good) {
                    bool changed = state.targets.size() != targets.size() ||
                        !std::equal(state.targets.begin(), state.targets.end(), targets.begin(), targets.end(),
                            [](const auto& a, const auto& b) { return a.Same(b) && a.name == b.name; });
                    if (changed) {
                        SKSE::log::info("Target catalog: {} entries", targets.size());
                        for (const auto& target : targets) {
                            SKSE::log::info("Target part={} id={:08X} name={}", target.part, static_cast<std::uint32_t>(target.item), target.name);
                        }
                    }
                    // Poll the catalog frequently, but only replace the live list when its
                    // contents actually change. Rebuilding an identical list can disturb the
                    // open combo/selection and needlessly trigger another snapshot request.
                    if (changed) {
                        state.targets = std::move(targets);
                        const auto preferredPart = state.selected.part;
                        auto current = std::find_if(state.targets.begin(), state.targets.end(), [](const auto& t) { return t.Same(state.selected); });
                        if (current != state.targets.end()) {
                            state.selected = *current;
                        } else {
                            // If a selected wig disappears briefly, prefer another entry in
                            // the same part/category before falling back to the first entry.
                            auto sameCategory = std::find_if(state.targets.begin(), state.targets.end(), [preferredPart](const auto& t) { return t.part == preferredPart; });
                            state.selected = sameCategory != state.targets.end() ? *sameCategory :
                                (state.targets.empty() ? Target{} : state.targets.front());
                            state.valid = false;
                        }
                        state.refresh = true;
                    }
                    for (auto& saved : savedTargets) {
                        if (!state.restoredThisSession.contains(Identity(saved.first)) &&
                            std::none_of(state.autoRestoreQueue.begin(), state.autoRestoreQueue.end(), [&](const auto& queued) { return queued.first.Same(saved.first); }) &&
                            (!state.autoRestoreActive || !state.autoRestoreActive->Same(saved.first))) {
                            state.autoRestoreQueue.push_back(saved);
                        }
                    }
                    if (changed) { state.error.clear(); }
                } else { state.error = "부위 목록 응답 오류: 새 DLL과 HPUIBridge.pex를 함께 설치하세요."; }
            }
            Pump();
        }
    private:
        std::uint64_t epoch_;
    };

    void QueueValue(int row, float value, const Target& expected, int group = -1) {
        if (row < 0 || row >= static_cast<int>(Rows) || !std::isfinite(value)) { return; }
        std::scoped_lock lock(stateMutex);
        if (!state.valid || state.beginSession || state.writeBusy || state.catalogBusy || state.restoreActive || state.restoreQueued || state.resetQueued || !state.selected.Same(expected)) { return; }
        value = ClampEdit(value, row, expected.part);
        auto before = EffectiveValues(state);
        if (std::abs(before[row] - value) < 0.000001f) { return; }
        state.histories[Identity(expected)].Record(before, group, Now());
        state.queued[row] = value; // Coalesce dragging; never enqueue an unbounded VM stack per frame.
    }

    void Pump() {
        std::uint64_t epoch;
        int row = -1;
        float value = 0;
        bool reset = false;
        bool catalog = false;
        bool autoRestore = false;
        EditValues persistentValues{};
        std::optional<State::Restore> restore;
        Target target;
        {
            std::scoped_lock lock(stateMutex);
            if (state.busy) {
                if (Clock::now() - state.started > 10s) {
                    state.error = "Papyrus 응답 대기 중. 메뉴를 닫고 다시 열거나 로그를 확인하세요.";
                }
                return; // A timeout must not launch a duplicate write while an older VM stack still exists.
            }
            if (!inGame.load()) { return; }
            if (state.beginSession && !Pending(state)) {
                state.histories.clear(); state.valid = false;
                state.beginSession = false; state.refresh = true; state.catalogRefresh = true;
            }
            restore = state.restoreQueued;
            state.restoreQueued.reset();
            state.restoreActive = restore;
            reset = state.resetQueued;
            if (reset) { state.resetQueued = false; }
            else if (!restore) {
                for (std::size_t i = 0; i < Rows; ++i) {
                    if (state.queued[i]) {
                        row = static_cast<int>(i); value = *state.queued[i];
                        state.queued[i].reset(); break;
                    }
                }
            }
            if (!restore && !reset && row < 0 && !state.autoRestoreQueue.empty()) {
                auto pendingRestore = std::move(state.autoRestoreQueue.front());
                state.autoRestoreQueue.pop_front();
                target = pendingRestore.first;
                persistentValues = pendingRestore.second;
                autoRestore = true;
                state.autoRestoreActive = target;
            } else {
                target = state.selected;
            }
            bool visible = window && window->IsOpen.load();
            auto edited = state.histories.find(Identity(state.selected));
            bool recentEdit = edited != state.histories.end() && Now() - edited->second.lastChange < .4;
            catalog = !recentEdit && !manipulating.load() && !restore && !reset && !autoRestore && row < 0 &&
                (state.catalogRefresh || Clock::now() - state.lastCatalog >= 1s);
            if (!restore && !reset && !autoRestore && row < 0 && !catalog && (!visible || !state.refresh)) {
                return;
            }
            if (catalog) { state.catalogRefresh = false; }
            state.catalogBusy = catalog;
            state.refresh = false;
            state.busy = true;
            state.writeBusy = row >= 0 || reset || restore.has_value(); // Background persistent restores never freeze the UI.
            state.started = Clock::now();
            epoch = state.generation;
            if (!autoRestore) { target = state.selected; }
            if (row >= 0) { state.activeEdit = std::pair{row, value}; }
        }
        SKSE::GetTaskInterface()->AddTask([epoch, row, value, reset, catalog, target, restore, autoRestore, persistentValues] {
            {
                std::scoped_lock lock(stateMutex);
                if (epoch != state.generation || !inGame.load()) { return; }
            }
            auto* vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
            RE::BSTSmartPointer<RE::BSScript::IStackCallbackFunctor> callback;
            if (catalog) { callback.reset(new CatalogReply(epoch)); }
            else { callback.reset(new Reply(epoch, target, row >= 0 || reset || restore.has_value(), autoRestore,
                    autoRestore ? persistentValues : restore ? restore->desired : EditValues{})); }
            auto* args = catalog ? RE::MakeFunctionArguments() : (restore || autoRestore) ?
                RE::MakeFunctionArguments(std::int32_t(target.part), std::int32_t(target.item), std::vector<float>((autoRestore ? persistentValues : restore->desired).begin(), (autoRestore ? persistentValues : restore->desired).end())) : row >= 0 ?
                RE::MakeFunctionArguments(std::int32_t(target.part), std::int32_t(target.item), std::int32_t(row), float(value)) :
                RE::MakeFunctionArguments(std::int32_t(target.part), std::int32_t(target.item));
            const char* function = catalog ? "TargetsV2" : (restore || autoRestore) ? "ApplyValuesV3" : reset ? "ResetTargetV2" : row >= 0 ? "SetValueV2" : "SnapshotV2";
            bool ok = vm && vm->DispatchStaticCall(Script, function, args, callback);
            if (!ok) {
                // The VM owns arguments after dispatch; no raw callbacks or DLL offsets are used.
                std::scoped_lock lock(stateMutex);
                if (epoch != state.generation) { return; }
                state.busy = false;
                state.writeBusy = false;
                state.catalogBusy = false;
                if (autoRestore) { state.autoRestoreActive.reset(); }
                if (catalog) { state.lastCatalog = Clock::now(); }
                state.activeEdit.reset();
                state.restoreActive.reset();
                state.lastReply = Clock::now();
                state.error = "브리지를 호출할 수 없습니다. 새 DLL과 HPUIBridge.pex를 함께 설치하세요.";
                SKSE::log::error("Papyrus dispatch failed: {}.{}", Script, function);
            }
        });
    }

    void __stdcall RenderWindow() {
        ImGui::SetNextWindowSize({510.f, 650.f}, ImGui::ImGuiCond_FirstUseEver);
        bool open = window->IsOpen.load();
        if (!ImGui::Begin("부위·파츠 조절 · 1.1.0##HairPositionerUI", &open, ImGui::ImGuiWindowFlags_NoScrollWithMouse)) {
            manipulating = false;
            ImGui::End(); window->IsOpen = open; Pump(); return;
        }
        float wheel = ImGui::GetKeyData(ImGui::ImGuiKey_MouseWheelY)->AnalogValue;
        bool overValue = false;
        bool activeValue = false;
        State view;
        { std::scoped_lock lock(stateMutex); view = state; }
        ImGui::TextUnformatted("플레이어 부위·파츠 · HairPositioner 2.5");
        bool pending = Pending(view);
        ImGui::BeginDisabled(pending || view.targets.empty());
        ImGui::SetNextItemWidth(340.f);
        if (ImGui::BeginCombo("조절 대상", view.targets.empty() ? "조절 대상 없음 / 목록 확인 중" : view.selected.name.c_str())) {
            for (std::size_t index = 0; index < view.targets.size(); ++index) {
                const auto target = view.targets[index];
                ImGui::PushID(static_cast<int>(index));
                if (ImGui::Selectable(target.name.c_str(), target.Same(view.selected))) {
                    std::scoped_lock lock(stateMutex);
                    // A background refresh can finish between the frame snapshot and click.
                    if (!Pending(state) &&
                        std::any_of(state.targets.begin(), state.targets.end(), [&](const auto& t) { return t.Same(target); })) {
                        state.histories[Identity(state.selected)].EndGesture();
                        state.selected = target;
                        state.valid = false; state.refresh = true; state.error.clear();
                        view.selected = target; view.valid = false;
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
        ImGui::EndDisabled();
        if (ImGui::Button("부위 목록 새로 읽기")) {
            std::scoped_lock lock(stateMutex); state.catalogRefresh = true;
        }
        ImGui::TextUnformatted("헤어 · 수염 · 추가 머리 파츠 · 착용 장비/가발");
        if (!inGame.load()) { ImGui::TextUnformatted("세이브를 불러온 뒤 사용하세요."); }
        else if (!view.valid) { ImGui::TextUnformatted("현재값을 읽는 중..."); }
        else if (view.snapshot[10] < .5f) { ImGui::TextUnformatted("HairPositioner가 준비되지 않았습니다. 캐릭터 로딩 후 다시 확인하세요."); }
        else if (view.snapshot[12] < .5f) { ImGui::TextUnformatted("HairPositioner에서 이 부위가 비활성화돼 있습니다."); }
        else if (view.snapshot[11] < .5f) { ImGui::TextUnformatted("현재 캐릭터에 이 부위가 없거나 착용이 해제되었습니다."); }
        if (!view.error.empty()) { ImGui::TextWrapped("%s", view.error.c_str()); }
        int level = precision.load();
        ImGui::SetNextItemWidth(240.f);
        if (ImGui::Combo("정밀도", &level, "미세\0보통\0크게\0")) { SavePrecision(level); }
        level = precision.load();
        ImGui::Text("한 칸: 위치/크기 %.3f · 회전 %.2f도", PrecisionStep(0, level), PrecisionStep(3, level));
        bool enabled = inGame.load() && !view.beginSession && !view.writeBusy && !view.restoreQueued && !view.restoreActive && !view.resetQueued && !view.targets.empty() && view.valid && view.snapshot[10] > .5f &&
                       view.snapshot[11] > .5f && view.snapshot[12] > .5f && view.error.empty();
        ImGui::BeginDisabled(!enabled);
        static constexpr const char* labels[] = {
            "좌우", "앞뒤", "상하", "기울기 X", "기울기 Y", "회전 Z", "너비", "깊이", "높이"
        };
        for (int i = 0; i < 9; ++i) {
            if (i % 3 == 0) { ImGui::Separator(); ImGui::TextUnformatted(i == 0 ? "위치" : i == 3 ? "회전 (도)" : "크기 (1 = 기본)"); }
            float value = view.snapshot[i];
            if (view.activeEdit && view.activeEdit->first == i) { value = view.activeEdit->second; }
            if (view.queued[i]) { value = *view.queued[i]; }
            ImGui::PushID(i);
            ImGui::SetNextItemWidth(230.f);
            float step = PrecisionStep(i, level);
            float move = view.selected.part == 1 ? 10.f : 15.f;
            float lo = i < 3 ? -move : i < 6 ? -90.f : .5f;
            float hi = i < 3 ? move : i < 6 ? 90.f : 2.f;
            if (ImGui::DragFloat(labels[i], &value, step, lo, hi, i < 6 && i >= 3 ? "%.2f" : "%.3f",
                                 ImGui::ImGuiSliderFlags_AlwaysClamp)) { QueueValue(i, value, view.selected, i); }
            bool active = ImGui::IsItemActive();
            activeValue = activeValue || active;
            if (ImGui::IsItemDeactivated()) {
                std::scoped_lock lock(stateMutex); state.histories[Identity(view.selected)].EndGesture(i);
            }
            if (enabled && ImGui::IsItemHovered()) {
                overValue = true;
                if (!active && std::isfinite(wheel) && wheel != 0.f) {
                    // Wheel uses the latest pending value, so fast ticks cannot overwrite each other.
                    { std::scoped_lock lock(stateMutex); value = EffectiveValues(state)[i]; }
                    QueueValue(i, value + wheel * step, view.selected, 32 + i);
                }
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("기본")) { QueueValue(i, i >= 6 ? 1.f : 0.f, view.selected); }
            ImGui::PopID();
        }
        ImGui::TextUnformatted("값 위에서 휠/드래그 · Ctrl+클릭으로 숫자 입력");
        float pivotValue = view.snapshot[9];
        if (view.activeEdit && view.activeEdit->first == 9) { pivotValue = view.activeEdit->second; }
        if (view.queued[9]) { pivotValue = *view.queued[9]; }
        int pivot = std::clamp(static_cast<int>(pivotValue), 0, 2);
        if (ImGui::Combo("기준점", &pivot, "머리 본\0선택 대상 중심\0모델 원점\0")) { QueueValue(9, static_cast<float>(pivot), view.selected); }
        if (view.selected.part == 0 && view.snapshot[13] > .5f) {
            float worn = view.snapshot[14];
            if (view.activeEdit && view.activeEdit->first == 10) { worn = view.activeEdit->second; }
            if (view.queued[10]) { worn = *view.queued[10]; }
            bool follow = worn >= .5f;
            if (ImGui::Checkbox("가발 슬롯 착용품도 헤어와 함께 이동", &follow)) { QueueValue(10, follow ? 1.f : 0.f, view.selected); }
        }
        static Target resetTarget;
        if (ImGui::Button("선택 대상 초기화")) { resetTarget = view.selected; ImGui::OpenPopup("초기화 확인"); }
        if (ImGui::BeginPopupModal("초기화 확인", nullptr, ImGui::ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::TextWrapped("%s의 위치·회전·크기·기준점을 초기화합니다.", resetTarget.name.c_str());
            if (ImGui::Button("초기화")) {
                {
                    std::scoped_lock lock(stateMutex);
                    if (!state.catalogBusy && resetTarget.Same(state.selected)) {
                        state.histories[Identity(state.selected)].Record(EffectiveValues(state), -1, Now());
                        state.queued.fill(std::nullopt); state.resetQueued = true;
                    }
                }
                ImGui::CloseCurrentPopup();
            }
            ImGui::SameLine();
            if (ImGui::Button("취소")) { ImGui::CloseCurrentPopup(); }
            ImGui::EndPopup();
        }
        ImGui::EndDisabled();
        auto history = view.histories.find(Identity(view.selected));
        auto undoCount = history == view.histories.end() ? 0 : history->second.undo.size();
        bool canRecover = enabled && !pending && history != view.histories.end() && history->second.baseline.has_value();
        ImGui::BeginDisabled(!canRecover || undoCount == 0);
        if (ImGui::Button("실행 취소")) { QueueRestore(view.selected, true); }
        ImGui::EndDisabled();
        ImGui::SameLine();
        bool atStart = canRecover && SameValues(EffectiveValues(view), *history->second.baseline);
        ImGui::BeginDisabled(!canRecover || atStart);
        if (ImGui::Button("조절 시작 상태 복원")) { QueueRestore(view.selected, false); }
        ImGui::EndDisabled();
        ImGui::Text("이 부위 실행 취소: %u회 · 창을 다시 열면 새 시작값", static_cast<unsigned>(undoCount));
        if (ImGui::Button("현재값 새로 읽기")) { std::scoped_lock lock(stateMutex); state.refresh = true; }
        ImGui::TextUnformatted(view.writeBusy ? "값 적용 중..." : "변경값은 즉시 적용되고 HairPositionerUI.ini에 저장됩니다. 새 게임에도 자동 적용됩니다.");
        RenderHotkeySettings();
        if (ImGui::Button("닫기")) { open = false; }
        // This window disables automatic wheel scrolling. Manually scroll only
        // away from editable values; never change the framework's global IO.
        if (!overValue && !activeValue && wheel != 0.f && ImGui::IsWindowHovered()) {
            ImGui::SetScrollY(ImGui::GetScrollY() - wheel * 45.f);
        }
        manipulating = open && activeValue;
        ImGui::End();
        window->IsOpen = open;
        Pump();
    }

    void __stdcall RenderPage() {
        ImGui::TextUnformatted("HairPositioner · 플레이어 부위·파츠 조절");
        ImGui::TextWrapped("헤어, 수염, 추가 머리 파츠, 착용 장비/가발을 선택해 조절합니다. 기존 RaceMenu 기능과 값을 공유합니다.");
        if (ImGui::Button("작은 조절 창 열기")) {
            window->IsOpen = true;
            { std::scoped_lock lock(stateMutex); state.refresh = true; state.catalogRefresh = true; state.beginSession = true; state.valid = false; }
            if (auto* main = SKSEMenuFramework::GetMainWindow()) { main->IsOpen = false; }
        }
        ImGui::TextUnformatted("핫키로 조절 창 열기/닫기 (기존 게임 메뉴가 닫힌 상태)");
        RenderHotkeySettings();
    }

    bool __stdcall OnInput(RE::InputEvent* event) {
        if (!window) { return false; }
        auto* button = event ? event->AsButtonEvent() : nullptr;
        auto* main = SKSEMenuFramework::GetMainWindow();
        bool visible = window->IsOpen.load() || (main && main->IsOpen.load());
        if (capturingKey.load() && !visible) { capturingKey = false; }
        if (capturingKey.load() && button && button->GetDevice() == RE::INPUT_DEVICE::kKeyboard) {
            if (button->IsDown()) {
                auto code = button->GetIDCode();
                if (code == RE::BSKeyboardDevice::Key::kEscape) { capturingKey = false; }
                else if (code > 0 && code <= 255) { SaveHotkey(code); }
            }
            return true; // Registration consumes the key; it must not also toggle the window.
        }
        const auto key = hotkey.load();
        if (!inGame.load() || !key) { return false; }
        if (button && button->GetDevice() == RE::INPUT_DEVICE::kKeyboard &&
            button->GetIDCode() == key && button->IsDown()) {
            auto* ui = RE::UI::GetSingleton();
            if (!window->IsOpen.load() && ui && (ui->GameIsPaused() || ui->IsMenuOpen("Console") || ui->IsMenuOpen("RaceSex Menu"))) {
                return false;
            }
            window->IsOpen = !window->IsOpen.load();
            if (window->IsOpen.load()) { std::scoped_lock lock(stateMutex); state.refresh = true; state.catalogRefresh = true; state.beginSession = true; state.valid = false; }
            else { manipulating = false; }
            return true;
        }
        return false; // Framework captures input for our blocking window; do not alter global pause/blur settings.
    }

    void __stdcall AfterRender(SKSEMenuFramework::Model::EventType) { Pump(); }

    void ResetSession(bool game) {
        inGame = game;
        capturingKey = false;
        manipulating = false;
        if (window) { window->IsOpen = false; }
        std::scoped_lock lock(stateMutex);
        auto epoch = state.generation + 1;
        state = State{}; state.generation = epoch;
    }

    void Message(SKSE::MessagingInterface::Message* message) {
        if (!message) { return; }
        switch (message->type) {
        case SKSE::MessagingInterface::kDataLoaded: {
            const auto module = GetModuleHandleW(L"SKSEMenuFramework.dll");
            constexpr const char* required[] = {"AddWindow", "AddSectionItem", "GetMainWindow", "RegisterInpoutEvent",
                "RegisterEventPriority", "igBegin", "igEnd", "igDragFloat", "igCombo_Str", "igBeginDisabled",
                "igBeginCombo", "igEndCombo", "igSelectable_Bool", "igCheckbox",
                "igGetKeyData_Key", "igIsItemHovered", "igIsItemActive", "igIsItemDeactivated",
                "igGetScrollY", "igSetScrollY_Float", "igIsWindowHovered"};
            for (auto name : required) {
                if (!module || !GetProcAddress(module, name)) { SKSE::log::error("Missing framework export: {}", name); return; }
            }
            window = SKSEMenuFramework::AddWindow(RenderWindow, true);
            if (!window) { SKSE::log::error("Failed to register UI window"); return; }
            SKSEMenuFramework::SetSection("HairPositioner");
            SKSEMenuFramework::AddSectionItem("부위·파츠 조절", RenderPage);
            static auto* input = SKSEMenuFramework::AddInputEvent(OnInput);
            static auto* render = SKSEMenuFramework::AddEvent(AfterRender, 0.f);
            (void)input; (void)render;
            SKSE::log::info("Registered HairPositioner UI; hotkey {}", hotkey.load());
            break;
        }
        case SKSE::MessagingInterface::kPreLoadGame: ResetSession(false); break;
        case SKSE::MessagingInterface::kPostLoadGame: ResetSession(true); break;
        case SKSE::MessagingInterface::kNewGame: ResetSession(true); break;
        default: break;
        }
    }
}

SKSEPluginLoad(const SKSE::LoadInterface* skse)
{
    if (skse->IsEditor() || REL::Module::IsVR()) { return false; }
    auto path = SKSE::log::log_directory();
    if (!path) { return false; }
    *path /= "HairPositionerUI.log";
    auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true);
    auto logger = std::make_shared<spdlog::logger>("HPUI", sink);
    spdlog::set_default_logger(logger);
    spdlog::set_level(spdlog::level::info);
    spdlog::flush_on(spdlog::level::info);
    SKSE::Init(skse, false);
    SKSE::log::info("HairPositionerUI 1.1.0: persistent per-target settings, background catalog refresh, non-blocking UI refresh; default F3");
    HPUI::LoadHotkey();
    return SKSE::GetMessagingInterface()->RegisterListener(HPUI::Message);
}
