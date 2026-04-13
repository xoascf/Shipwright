#ifndef SohGuiStrings_hpp
#define SohGuiStrings_hpp

namespace SohGui {
namespace SohGuiStrings {

namespace SidebarSections {
inline constexpr const char* Settings = "Configuración";
inline constexpr const char* DevTools = "Para desarrolladores";
inline constexpr const char* Enhancements = "Mejoras";
inline constexpr const char* Randomizer = "Randomizer";
inline constexpr const char* Network = "Red";
} // namespace SidebarSections

namespace WindowNames {
inline constexpr const char* ModalWindow = "Ventana modal##ModalWindow";
inline constexpr const char* ControllerConfig = "Mapeo de mando##ControllerConfig";
inline constexpr const char* Console = "Consola##Console";
inline constexpr const char* Stats = "Estadísticas##Stats";
inline constexpr const char* GfxDebugger = "DepuradorGfx##GfxDebuggerWindow";
inline constexpr const char* SaveEditor = "Editor de guardado##SaveEditor";
inline constexpr const char* ActorViewer = "Visor de actores##ActorViewer";
inline constexpr const char* CollisionViewer = "Visor de colisiones##CollisionViewer";
inline constexpr const char* DisplayListViewer = "Visor de Display Lists##DisplayListViewer";
inline constexpr const char* ValueViewer = "Visor de valores##ValueViewer";
inline constexpr const char* MessageViewer = "Visor de mensajes##MessageViewer";
inline constexpr const char* HookDebugger = "Depurador de hooks##HookDebugger";
inline constexpr const char* InputViewer = "Visor de entradas##InputViewer";
inline constexpr const char* InputViewerSettings = "Ajustes del Visor de entradas##InputViewerSettings";
inline constexpr const char* ModMenu = "Menú de mods##ModMenu";
inline constexpr const char* AudioEditor = "Editor de audio##AudioEditor";
inline constexpr const char* CosmeticsEditor = "Editor de cosméticos##CosmeticsEditor";
inline constexpr const char* GameplayStats = "Estadísticas de juego##GameplayStats";
inline constexpr const char* TimeSplits = "Splits de tiempo##TimeSplits";
inline constexpr const char* Plandomizer = "Editor de plandomizer##PlandomizerEditor";
inline constexpr const char* ItemTracker = "Rastreador de objetos##ItemTracker";
inline constexpr const char* ItemTrackerSettings = "Ajustes del Rastreador de objetos##ItemTrackerSettings";
inline constexpr const char* EntranceTracker = "Rastreador de entradas##EntranceTracker";
inline constexpr const char* EntranceTrackerSettings = "Ajustes del Rastreador de entradas##EntranceTrackerSettings";
inline constexpr const char* CheckTracker = "Rastreador de chequeos##CheckTracker";
inline constexpr const char* CheckTrackerSettings = "Ajustes del Rastreador de chequeos##CheckTrackerSettings";
inline constexpr const char* Notifications = "Ventana Notificaciones##Notifications";
inline constexpr const char* AdditionalTimers = "Temporizadores adicionales##AdditionalTimers";
inline constexpr const char* AnchorRoom = "Sala Anchor##AnchorRoom";
} // namespace WindowNames

namespace SidebarEntryNames {
namespace Common {
inline constexpr const char* General = "General";
} // namespace Common

namespace Settings {
inline constexpr const char* Audio = "Sonido";
inline constexpr const char* Graphics = "Gráficos";
inline constexpr const char* Controls = "Mando";
inline constexpr const char* InputViewer = "Visor de entradas";
inline constexpr const char* Notifications = "Notificaciones";
inline constexpr const char* ModMenu = "Menú de mods";
inline constexpr const char* Presets = "Presets";
} // namespace Settings

namespace DevTools {
inline constexpr const char* Stats = "Estadísticas";
inline constexpr const char* Console = "Consola";
inline constexpr const char* SaveEditor = "Editor de guardado";
inline constexpr const char* HookDebugger = "Depurador de hooks";
inline constexpr const char* CollisionViewer = "Visor de colisiones";
inline constexpr const char* ActorViewer = "Visor de actores";
inline constexpr const char* DListViewer = "Visor de Display Lists";
inline constexpr const char* ValueViewer = "Visor de valores";
inline constexpr const char* MessageViewer = "Visor de mensajes";
inline constexpr const char* GfxDebugger = "Depurador GFX";
} // namespace DevTools

namespace Enhancements {
inline constexpr const char* QualityOfLife = "Jugabilidad";
inline constexpr const char* SkipsAndSpeedups = "Ahorradores de tiempo";
inline constexpr const char* Graphics = "Gráficos";
inline constexpr const char* Items = "Objetos";
inline constexpr const char* Fixes = "Correcciones";
inline constexpr const char* Difficulty = "Opciones de dificultad";
inline constexpr const char* Minigames = "Minijuegos";
inline constexpr const char* ExtraModes = "Modos adicionales";
inline constexpr const char* Cheats = "Trucos";
inline constexpr const char* CosmeticsEditor = "Editor de cosméticos";
inline constexpr const char* AudioEditor = "Editor de audio";
inline constexpr const char* GameplayStats = "Estadísticas de juego";
inline constexpr const char* TimeSplits = "Splits de tiempo";
inline constexpr const char* Timers = "Temporizadores";
} // namespace Enhancements

namespace Randomizer {
inline constexpr const char* Locations = "Ubicaciones";
inline constexpr const char* TricksGlitches = "Trucos y glitches";
inline constexpr const char* Plandomizer = "Plandomizer";
inline constexpr const char* ItemTracker = "Rastreador de objetos";
inline constexpr const char* EntranceTracker = "Rastreador de entradas";
inline constexpr const char* CheckTracker = "Rastreador de chequeos";
} // namespace Randomizer

namespace Network {
inline constexpr const char* Info = "Información";
inline constexpr const char* Sail = "Sail";
inline constexpr const char* CrowdControl = "Crowd Control";
inline constexpr const char* Anchor = "Anchor";
} // namespace Network
} // namespace SidebarEntryNames

} // namespace SohGuiStrings

namespace GuiWindowNames = SohGuiStrings::WindowNames;
} // namespace SohGui

#endif // SohGuiStrings_hpp
