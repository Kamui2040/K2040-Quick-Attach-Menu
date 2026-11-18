"""Only owner V2 settings are installed and invoked; no old Builder editor."""
from pathlib import Path
import importlib.util
import unittest

ROOT=Path(__file__).resolve().parents[2]
UI=ROOT/"dist/Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu"

class UnifiedRouting(unittest.TestCase):
    def test_builder_settings_button_requests_existing_unified_hub(self):
        html=(UI/"builder.html").read_text()
        self.assertIn('<button id="settings" class="action"',html)
        self.assertIn('send("open-settings")',html)
        self.assertIn('id="hubSettingsStatus"',html)
        self.assertIn('aria-live="polite"',html)
        self.assertIn("k2040BuilderHubSettingsStatus",html)
        self.assertNotIn("Settings: K2040 Hub (F3)",html)

    def test_request_executes_on_game_thread_and_never_loads_or_bundles_hub(self):
        src=(ROOT/"src/PrismaBridge.cpp").read_text()
        h=(ROOT/"include/K2040ModuleSettingsRequestV1.h").read_text()
        self.assertIn('K2040_RequestModuleSettingsV1',h)
        self.assertIn('K2040_REQUEST_MODULE_SETTINGS_V1_EXPORT',h)
        callbacks=src.split("void OnMenuBuilderChangeRequested(",1)[1].split(
            "void OnMenuSettingsChangeRequested(",1)[0]
        self.assertIn('if (argument && std::string_view(argument) == "open-settings")',callbacks)
        self.assertIn("g_builderHubOpenTaskPending.exchange(true)",callbacks)
        self.assertIn("F4SE::GetTaskInterface()",callbacks)
        self.assertIn("tasks->AddTask([]()",callbacks)
        self.assertIn('OnBuilderChangeRequested("open-settings")',callbacks)
        handler=src.split('if (command == "open-settings")',1)[1].split(
            'if (command == "export-profile")',1)[0]
        self.assertIn('GetModuleHandleW(L"K2040_Unified_Settings.dll")',handler)
        self.assertIn('GetProcAddress(hub, K2040_REQUEST_MODULE_SETTINGS_V1_EXPORT)',handler)
        self.assertIn('request("quick-attach")',handler)
        self.assertIn('"k2040BuilderHubSettingsStatus"',handler)
        self.assertIn('CloseMenuInternal(false);',handler)
        self.assertLess(handler.index('request("quick-attach")'),
                        handler.index('CloseMenuInternal(false);'))
        failed=handler.split('if (!request || !request("quick-attach")) {',1)[1].split(
            "return; // Keep Builder open",1)[0]
        self.assertNotIn("CloseMenuInternal",failed)
        self.assertNotIn("LoadLibrary",handler)
        self.assertNotIn("RequestViewSwitch(ViewMode::Settings)",handler)

    def test_packager_does_not_install_legacy_settings_renderer(self):
        pack=ROOT/"scripts/package-test.py"
        spec=importlib.util.spec_from_file_location("qam_retired_settings_pack",pack)
        module=importlib.util.module_from_spec(spec)
        spec.loader.exec_module(module)
        self.assertEqual(module.RETIRED_SETTINGS_VIEW,
                         "Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html")
        self.assertNotIn(module.RETIRED_SETTINGS_VIEW, module.RUNTIME_FILES)
        if not (ROOT/"dist/Data/F4SE/Plugins/K2040_Quick_Attach_Menu.dll").exists():
            self.skipTest("compiled owner DLL is required for final package verification")
        files=module.payload()
        self.assertIn("Data/K2040/Settings/Providers/quick-attach.json",files)
        self.assertNotIn("Data/PrismaUI_F4/views/K2040_Quick_Attach_Menu/settings.html",files)
        self.assertNotIn("Data/F4SE/Plugins/K2040_Unified_Settings.dll",files)

if __name__=="__main__":
    unittest.main()
