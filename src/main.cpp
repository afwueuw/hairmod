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
