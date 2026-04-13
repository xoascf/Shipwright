#include "SohMenu.h"
#include "SohGui.hpp"

extern "C" {
extern PlayState* gPlayState;
}

void WarpPointsWidget(WidgetInfo& info);

namespace SohGui {

extern std::shared_ptr<SohMenu> mSohMenu;
using namespace UIWidgets;

static const std::map<int32_t, const char*> logLevels = {
    { DEBUG_LOG_TRACE, "Trace" }, { DEBUG_LOG_DEBUG, "Debug" }, { DEBUG_LOG_INFO, "Info" },
    { DEBUG_LOG_WARN, "Warn" },   { DEBUG_LOG_ERROR, "Error" }, { DEBUG_LOG_CRITICAL, "Critical" },
    { DEBUG_LOG_OFF, "Off" },
};

#ifdef _DEBUG
DebugLogOption defaultLogLevel = DEBUG_LOG_TRACE;
#else
DebugLogOption defaultLogLevel = DEBUG_LOG_INFO;
#endif

static const std::map<int32_t, const char*> debugSaveFileModes = {
    { 0, "Off" },
    { 1, "Vanilla" },
    { 2, "Maxed" },
};

void SohMenu::AddMenuDevTools() {
    // Add Dev Tools Menu
    AddMenuEntry(SohGuiStrings::SidebarSections::DevTools, CVAR_SETTING("Menu.DevToolsSidebarSection"));

    // General
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, SohGuiStrings::SidebarEntryNames::Common::General, 3);
    WidgetPath path = { SohGuiStrings::SidebarSections::DevTools, SohGuiStrings::SidebarEntryNames::Common::General, SECTION_COLUMN_1 };

    AddWidget(path, "Popout Menu", WIDGET_CVAR_CHECKBOX)
        .CVar("gSettings.Menu.Popout")
        .Options(CheckboxOptions().Tooltip("Cambia la visualización del menú de superposición a ventana."));
    AddWidget(path, "Habilitar Debug Mode", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("DebugEnabled"))
        .Options(
            CheckboxOptions().Tooltip("Habilita el modo de depuración (Debug Mode), permitiendo seleccionar mapas con L + R + Z, "
                                      "noclip con L + D-pad Right, y abrir el menú de depuración con L en la pantalla de pausa."));
    AddWidget(path, "Map Select Button Combination:", WIDGET_CVAR_BTN_SELECTOR)
        .CVar("gDeveloperTools.MapSelectBtn")
        .Options(BtnSelectorOptions().DefaultValue(BTN_R | BTN_L | BTN_Z))
        .PreFunc([](WidgetInfo& info) { info.isHidden = !CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0); });
    AddWidget(path, "No Clip Button Combination:", WIDGET_CVAR_BTN_SELECTOR)
        .CVar("gDeveloperTools.NoClipBtn")
        .PreFunc([](WidgetInfo& info) { info.isHidden = !CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0); })
        .Options(BtnSelectorOptions().DefaultValue(BTN_L | BTN_DRIGHT));
    AddWidget(path, "OoT Registry Editor", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("RegEditEnabled"))
        .PreFunc([](WidgetInfo& info) { info.isHidden = !CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0); })
        .Options(CheckboxOptions().Tooltip("Enables the registry editor."));
    AddWidget(path, "Debug Save File Mode", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("DebugSaveFileMode"))
        .PreFunc([](WidgetInfo& info) { info.isHidden = !CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0); })
        .Options(ComboboxOptions()
                     .Tooltip("Changes the behavior of debug file select creation (creating a save file on slot 1 "
                              "with debug mode on):\n"
                              "- Off: The debug save file will be a normal savefile.\n"
                              "- Vanilla: The debug save file will be the debug save file from the original game.\n"
                              "- Maxed: The debug save file will be a save file with all of the items & upgrades.")
                     .ComboMap(debugSaveFileModes)
                     .DefaultIndex(1));
    AddWidget(path, "OoT Skulltula Debug", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("SkulltulaDebugEnabled"))
        .PreFunc([](WidgetInfo& info) { info.isHidden = !CVarGetInteger(CVAR_DEVELOPER_TOOLS("DebugEnabled"), 0); })
        .Options(CheckboxOptions().Tooltip("Enables Skulltula Debug, when moving the cursor in the menu above various "
                                           "map icons (boss key, compass, map screen locations, etc.) will set the GS "
                                           "bits in that area.\nUSE WITH CAUTION AS IT DOES NOT UPDATE THE GS COUNT!"));
    AddWidget(path, "Resource logging", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("ResourceLogging"))
        .Options(CheckboxOptions().Tooltip("Logs some resources as XML when they're loaded in binary format."));

    AddWidget(path, "Frame Advance", WIDGET_CHECKBOX)
        .Options(CheckboxOptions().Tooltip(
            "This allows you to advance through the game one frame at a time on command. "
            "To advance a frame, hold Z and tap R on the second controller. Holding Z "
            "and R will advance a frame every half second. You can also use the buttons below."))
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = mSohMenu->disabledMap.at(DISABLE_FOR_NULL_PLAY_STATE).active ||
                            mSohMenu->disabledMap.at(DISABLE_FOR_DEBUG_MODE_OFF).active;
            if (gPlayState != nullptr) {
                info.valuePointer = (bool*)&gPlayState->frameAdvCtx.enabled;
            } else {
                info.valuePointer = (bool*)nullptr;
            }
        });
    AddWidget(path, "Advance 1", WIDGET_BUTTON)
        .Options(ButtonOptions().Tooltip("Advance 1 frame.").Size(Sizes::Inline))
        .Callback([](WidgetInfo& info) { CVarSetInteger(CVAR_DEVELOPER_TOOLS("FrameAdvanceTick"), 1); })
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = mSohMenu->disabledMap.at(DISABLE_FOR_FRAME_ADVANCE_OFF).active ||
                            mSohMenu->disabledMap.at(DISABLE_FOR_DEBUG_MODE_OFF).active;
        });
    AddWidget(path, "Advance (Hold)", WIDGET_BUTTON)
        .Options(ButtonOptions().Tooltip("Advance frames while the button is held.").Size(Sizes::Inline))
        .PreFunc([](WidgetInfo& info) {
            info.isHidden = mSohMenu->disabledMap.at(DISABLE_FOR_FRAME_ADVANCE_OFF).active ||
                            mSohMenu->disabledMap.at(DISABLE_FOR_DEBUG_MODE_OFF).active;
        })
        .PostFunc([](WidgetInfo& info) {
            if (ImGui::IsItemActive()) {
                CVarSetInteger(CVAR_DEVELOPER_TOOLS("FrameAdvanceTick"), 1);
            }
        })
        .SameLine(true);
    AddWidget(path, "Log Level", WIDGET_CVAR_COMBOBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("LogLevel"))
        .Options(ComboboxOptions()
                     .Tooltip("The log level determines which messages are printed to the console."
                              " This does not affect the log file output")
                     .ComboMap(logLevels)
                     .DefaultIndex(defaultLogLevel))
        .Callback([](WidgetInfo& info) {
            Ship::Context::GetInstance()->GetLogger()->set_level(
                (spdlog::level::level_enum)CVarGetInteger(CVAR_DEVELOPER_TOOLS("LogLevel"), defaultLogLevel));
        });

    path.column = SECTION_COLUMN_2;
    AddWidget(path, "Warping", WIDGET_SEPARATOR_TEXT);
    AddWidget(path, "Better Debug Warp Screen", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("BetterDebugWarpScreen"))
        .Options(CheckboxOptions()
                     .Tooltip("Optimized Debug Warp Screen, with the added ability to chose entrances and time of day.")
                     .DefaultValue(true));
    AddWidget(path, "Debug Warp Screen Translation", WIDGET_CVAR_CHECKBOX)
        .CVar(CVAR_DEVELOPER_TOOLS("DebugWarpScreenTranslation"))
        .Options(CheckboxOptions()
                     .Tooltip("Translate the Debug Warp Screen based on the game language.")
                     .DefaultValue(true));
    AddWidget(path, "Warp Points", WIDGET_CUSTOM).CustomFunction(WarpPointsWidget).HideInSearch(true);

    // Stats
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::Stats;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 1);
    AddWidget(path, "Abrir en ventana: Estadísticas", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("SohStats"))
        .RaceDisable(false)
        .WindowName(SohGuiStrings::WindowNames::Stats)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre la ventana de Estadísticas de forma independiente."));

    // Console
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::Console;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 1);
    AddWidget(path, "Abrir en ventana: Consola", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("SohConsole"))
        .WindowName(SohGuiStrings::WindowNames::Console)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre la Consola de forma independiente."));

    // Save Editor
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::SaveEditor;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 1);
    AddWidget(path, "Abrir en ventana: Editor de guardado", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("SaveEditor"))
        .WindowName(SohGuiStrings::WindowNames::SaveEditor)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre el Editor de guardado de forma independiente."));

    // Hook Debugger
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::HookDebugger;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 1);
    AddWidget(path, "Abrir en ventana: Depurador de hooks", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("HookDebugger"))
        .WindowName(SohGuiStrings::WindowNames::HookDebugger)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre el Depurador de hooks de forma independiente."));

    // Collision Viewer
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::CollisionViewer;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 2);
    AddWidget(path, "Abrir en ventana: Visor de colisiones", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("CollisionViewer"))
        .WindowName(SohGuiStrings::WindowNames::CollisionViewer)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre el Visor de colisiones de forma independiente."));

    // Actor Viewer
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::ActorViewer;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 2);
    AddWidget(path, "Abrir en ventana: Visor de actores", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ActorViewer"))
        .WindowName(SohGuiStrings::WindowNames::ActorViewer)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre el Visor de actores de forma independiente."));

    // Display List Viewer
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::DListViewer;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 2);
    AddWidget(path, "Abrir en ventana: Visor de Display Lists", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("DisplayListViewer"))
        .WindowName(SohGuiStrings::WindowNames::DisplayListViewer)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre el Visor de Display Lists de forma independiente."));

    // Value Viewer
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::ValueViewer;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 2);
    AddWidget(path, "Abrir en ventana: Visor de valores", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("ValueViewer"))
        .WindowName(SohGuiStrings::WindowNames::ValueViewer)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre el Visor de valores de forma independiente."));

    // Message Viewer
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::MessageViewer;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 2);
    AddWidget(path, "Abrir en ventana: Visor de mensajes", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("MessageViewer"))
        .WindowName(SohGuiStrings::WindowNames::MessageViewer)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre el Visor de mensajes de forma independiente."));

    // Gfx Debugger
    path.sidebarName = SohGuiStrings::SidebarEntryNames::DevTools::GfxDebugger;
    AddSidebarEntry(SohGuiStrings::SidebarSections::DevTools, path.sidebarName, 1);
    AddWidget(path, "Abrir en ventana: Depurador GFX", WIDGET_WINDOW_BUTTON)
        .CVar(CVAR_WINDOW("SohGfxDebugger"))
        .WindowName(SohGuiStrings::WindowNames::GfxDebugger)
        .HideInSearch(true)
        .Options(WindowButtonOptions().Tooltip("Abre el Depurador GFX de forma independiente."));
}

} // namespace SohGui
