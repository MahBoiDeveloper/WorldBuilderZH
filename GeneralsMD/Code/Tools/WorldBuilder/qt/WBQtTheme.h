// WBQtTheme.h -- selectable dark/light theming for the Qt side of WorldBuilder.
//
// The Qt UI theme is user-selectable and persisted (HKCU\Software\WorldBuilderZH\Qt,
// key "Theme"):
//   System -- follow the Windows "apps" colour setting (Settings > Personalization)
//   Dark   -- force a Fusion dark palette + dark title bars
//   Light  -- force the native light look
// Applied once at startup and re-applied live by setMode(); a global filter darkens
// (or lightens) the title bar of every top-level Qt window as it appears, so panels
// migrated from MFC later inherit the chosen theme with no per-window code. (The
// legacy MFC UI is not themed; it stays light until each piece is migrated.)
#ifndef WB_QT_THEME_H
#define WB_QT_THEME_H

namespace WBQtTheme
{
	enum Mode
	{
		ModeSystem = 0,		// follow the Windows app theme
		ModeDark   = 1,		// always dark
		ModeLight  = 2		// always light (native)
	};

	// Persisted selection (default ModeSystem).
	Mode mode();
	void setMode(Mode m);	// persist the choice and re-apply the theme live

	bool osPrefersDark();	// current Windows app theme (dark?)
	bool effectiveDark();	// mode() resolved against the OS -> should we render dark?

	// Apply the persisted theme to the QApplication and install the global title-bar
	// filter. Call once, right after the QApplication exists and before showing windows.
	void applyApplicationTheme();
}

#endif // WB_QT_THEME_H
