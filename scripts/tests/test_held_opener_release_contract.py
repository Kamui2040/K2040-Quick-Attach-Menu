"""Guard that closes never leak a still-held opener into vanilla input."""
from pathlib import Path
import unittest

ROOT=Path(__file__).resolve().parents[2]
HOTKEY=(ROOT/'src/Hotkey.cpp').read_text()
BRIDGE=(ROOT/'src/PrismaBridge.cpp').read_text()

class HeldOpenerReleaseContract(unittest.TestCase):
    def test_remapped_configured_virtual_keys_drive_close_guard(self):
        self.assertIn('g_quickOpenerVirtualKey.store(g_openMenuHotkey.virtualKey',HOTKEY)
        self.assertIn('g_builderOpenerVirtualKey.store(g_openMenuBuilderHotkey.virtualKey',HOTKEY)
        self.assertIn('IsPhysicalKeyDown(vk)',HOTKEY)
        self.assertIn('for (const auto* key : {&g_quickOpenerVirtualKey, &g_builderOpenerVirtualKey})',HOTKEY)
        self.assertIn('PollDeferredQuickMenuGameplayIsolationRelease();',HOTKEY)

    def test_gameplay_guard_is_retained_on_held_close(self):
        part=BRIDGE.split('void ScheduleQuickMenuGameplayIsolationRelease()',1)[1].split('bool ActivateBuilderMenuModeGuard()',1)[0]
        self.assertIn('k2040::HeldMenuOpenerVirtualKey()',part)
        self.assertIn('g_deferredQuickMenuReleaseKey.store(heldKey',part)
        self.assertIn('if (heldKey != 0)',part)
        self.assertIn('return;',part)
        self.assertIn('if (!k2040::GetPrismaBridge().IsMenuFocused())',part)

    def test_poller_queues_game_thread_cleanup_with_timeout(self):
        part=BRIDGE.split('void PollDeferredQuickMenuGameplayIsolationRelease()',1)[1].split('bool PrismaBridge::Initialize()',1)[0]
        self.assertIn('GetAsyncKeyState',part)
        self.assertIn('GetTickCount64() - started < 5000',part)
        self.assertIn('g_deferredQuickMenuReleaseQueued.exchange(true',part)
        self.assertIn('tasks->AddTask([key]() { FinishDeferredQuickMenuGameplayIsolationRelease(key); });',part)
        self.assertNotIn('ReleaseQuickMenuGameplayIsolation();',part)

    def test_guard_cannot_release_new_menu_or_leak_across_load(self):
        release=BRIDGE.split('void FinishDeferredQuickMenuGameplayIsolationRelease(',1)[1].split('void ScheduleQuickMenuGameplayIsolationRelease()',1)[0]
        self.assertIn('GetPrismaBridge().IsMenuFocused()',release)
        self.assertIn('g_deferredQuickMenuReleaseKey.load',release)
        self.assertIn('ReleaseQuickMenuGameplayIsolation();',release)
        reopen=BRIDGE.split('bool PrismaBridge::BeginOpenFromHotkey()',1)[1].split('std::string PrismaBridge::BuildMenuPayload',1)[0]
        self.assertIn('g_deferredQuickMenuReleaseKey.store(0',reopen)
        transition=BRIDGE.split('void PrismaBridge::ResetForGameTransition(',1)[1].split('void PrismaBridge::OnDomReady(',1)[0]
        self.assertIn('ReleaseQuickMenuGameplayIsolation();',transition)

    def test_close_and_switch_still_keep_distinct_lifecycles(self):
        self.assertIn('void PrismaBridge::CloseMenuForSwitch()',BRIDGE)
        self.assertIn('CloseMenuInternal(true);',BRIDGE)
        self.assertIn('SetMenuHotkeyUiForwardingActive(false);',BRIDGE)

if __name__ == '__main__':unittest.main()
