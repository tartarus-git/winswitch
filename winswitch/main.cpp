// Windows includes.
#define WIN32_LEAN_AND_MEAN																							// We don't need to include the whole Windows.h header for this simple program.
#include <Windows.h>

// Other includes.
#include <vector>
#include <cstring>

// Debugging.
#ifdef _DEBUG																										// Define logging code for debug mode.
void log(const TCHAR* message) { OutputDebugString(message); }
#define LOG(message) log(TEXT(message "\n"))
#else																												// When in release mode, don't waste time with logging.
#define LOG(message) ((void)0)																						// This is one way to make the compiler totally remove the logging code.
#endif																												// 0; is a complete statement. The (void) just makes sure you can't assign it to any variable.
																													// ((void)0); just gets optimized out by the compiler.
																													// NOTE: We could have used #undef to achieve this, but I'm keeping ((void)0) just in case
																													// it's better in some strange situations that I can't think of right now.

// Comparing strings.
#ifdef UNICODE
#define TSTRCMP(str1, str2) wcscmp(str1, str2)																		// Switch between the normal and wide version of strcmp based on the UNICODE define.
#else
#define TSTRCMP(str1, str2) strcmp(str1, str2)
#endif

// Hotkey.
#define HOTKEY_ID 0																									// The ID for the hotkey.
#define HOTKEY_MODIFIERS (MOD_NOREPEAT | MOD_CONTROL | MOD_ALT | MOD_SHIFT)											// The modifiers used for the hotkey.
#define KEY_S 0x53																									// The virtual key code for the S key.
#define HOTKEY_KEY KEY_S																							// The actual key used for the hotkey.

// Montors.
struct Monitor {																									// Monitor struct for holding useful data about a single monitor.
	HMONITOR handle;
	RECT rect;																										// Coordinates and size of the monitor within the windows coordinate system.

	Monitor(HMONITOR handle, RECT rect) : handle(handle), rect(rect) { }
};

unsigned int lastMonitorIndex;																						// The last monitor in the list of monitors. Cached value for efficiency in loops.
std::vector<Monitor> monitors;																						// A list of discovered monitors.

BOOL CALLBACK monitorDiscoveryProc(HMONITOR handle, HDC context, LPRECT rect, LPARAM data) {						// Callback function. Gets triggered for every monitor while enumerating.
	monitors.push_back(Monitor(handle, *rect));																		// Add the current monitor to the list of monitors.
	return true;
}

// Window stuff.																									// These windows receive focus when normal windows are minimized.
HWND progman;																										// Handle to the progman window.
HWND tray;																											// Handle to the shell tray window.

LRESULT CALLBACK windowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY:																								// Handle WM_DESTROY message.
		PostQuitMessage(0);
		return 0;

	case WM_HOTKEY:																									// Handle WM_HOTKEY message.
		if (wParam == HOTKEY_ID) {																					// Only continue if the id matches our hotkey. This isn't necessary, but for completeness.
			LOG("Hotkey pressed, getting foreground window...");
			HWND target = GetForegroundWindow();																	// Get the handle to the window which currently has user's focus.

			// Validation
			if (target == progman || target == tray) {																// If all windows are out of focus, skip move algorithm.
				LOG("All normal windows are out of focus and either progman or tray window has focus. Skipping move algorithm...");
				return 0;
			}
			if (target) {																							// If not even one of the system windows has focus, also skip the move algorithm.
				TCHAR classNameBuffer[8];					// TODO: Maybe you could get the handle of the last WorkerW window and use it here instead. What significance does the order have in spy++ and windows?
				classNameBuffer[7] = TEXT('\0');																	// Trust me, this is necessary to make sure that the string cmp works. Read the GetClassName docs.
				if (GetClassName(target, classNameBuffer, 8)) {														// If the target window is a WorkerW window, all normal windows don't have focus, so skip move.
					if (!TSTRCMP(classNameBuffer, TEXT("WorkerW"))) {
						LOG("All normal windows are out of focus and a WorkerW window has focus. Skipping move algorithm...");
						return 0;
					}
				}
				else {
					LOG("Couldn't get the class name of the target window for validation purposes. Terminating...");
					PostQuitMessage(0);
					return 0;
				}

				// Move algorithm.
				LOG("Foreground window found, getting the monitor in which the window is located...");
				HMONITOR currentMonitorHandle = MonitorFromWindow(target, MONITOR_DEFAULTTONULL);					// Get the handle to the monitor with the greatest area of intersection with the target window.

				// Out-of-bounds windows.
				if (!currentMonitorHandle) {																		// If the window is outside of the bounds of all monitors, move window into first monitor.
					LOG("Window isn't in the bounds of any monitor. Moving window to the first monitor...");
					RECT targetRect;
					if (GetWindowRect(target, &targetRect)) {
						SIZE firstMonitorSize = { monitors[0].rect.right - monitors[0].rect.left, monitors[0].rect.bottom - monitors[0].rect.top };

						// If the window doesn't need to be resized to fit inside the first monitor, just change the position and be done with it. No need to repaint here.
						if (targetRect.right - targetRect.left <= firstMonitorSize.cx && targetRect.bottom - targetRect.top <= firstMonitorSize.cy) {
							// The second param is NULL because it doesn't matter because we put SWP_NOZORDER in the last param.
							if (SetWindowPos(target, NULL, monitors[0].rect.left, monitors[0].rect.top, 0, 0, SWP_NOZORDER | SWP_NOSIZE)) {
								LOG("Sucessfully moved the out-of-bounds window into the first monitor. Didn't need to change the size of the window.");
								return 0;
							}
							LOG("Couldn't move the window into the first monitor. Quitting move algorithm...");
							return 0;
						}

						// If the window does need to be resized to fit inside the first monitor, do that. Repaint window because size changed.
						if (MoveWindow(target, monitors[0].rect.left, monitors[0].rect.top, firstMonitorSize.cx, firstMonitorSize.cy, true)) {
							LOG("Sucessfully moved the out-of-bounds window into the first monitor. Resized the window so that it fits into the monitor.");
							return 0;
						}
						LOG("Couldn't move the window into the first monitor. Quitting move algorithm...");
						return 0;
					}
					LOG("Couldn't retrieve size of target window. Exiting move algorithm...");
					return 0;
				}

				// In-bounds windows.
				LOG("Monitor found. Finding the same monitor in the monitor list...");
				int nextMonitor = -1;
				for (unsigned int i = 0; i < lastMonitorIndex; i++) {												// Loop through the list of monitors and find the target monitor. Find the next monitor as well.
					if (monitors[i].handle == currentMonitorHandle) {
						nextMonitor = i + 1;
						break;
					}
				}
				if (nextMonitor == -1) {
					if (monitors[lastMonitorIndex].handle == currentMonitorHandle) {								// Check if the monitor is the last one in the list.
						nextMonitor = 0;																			// If it is, loop around and set the next monitor to the first monitor in the list.
					}
					else {																							// If the target monitor doesn't exist in the list, we know we have rediscover monitors.
						LOG("The target monitor could not be found in the monitor list. Rediscovering monitors and trying again...");
						monitors.clear();																			// Clear the monitors list before we start adding stuff to it again.
						if (EnumDisplayMonitors(NULL, NULL, monitorDiscoveryProc, NULL)) {							// Enumerate all monitors to to update the monitors list.
							lastMonitorIndex = monitors.size() - 1;													// Update the index of the last monitor.

							for (unsigned int i = 0; i < lastMonitorIndex; i++) {									// Try finding the monitor again.
								if (monitors[i].handle == currentMonitorHandle) {
									nextMonitor = i + 1;
									break;
								}
							}
							if (nextMonitor == -1) {
								if (monitors[lastMonitorIndex].handle == currentMonitorHandle) {					// Check if the target monitor is the last one in the list.
									nextMonitor = 0;																// If it is, loop around just like before.
								}
								else {																				// If the target monitor still doesn't exist in the list, terminate program.
									LOG("Monitor still could not be found. Terminating program...");
									PostQuitMessage(0);
									return 0;
								}
							}																						// If the target monitor does exist now, fall through the if statements and continue normally.
						}
						else {
							LOG("Could not enumerate monitors. Terminating...");
							PostQuitMessage(0);
							return 0;
						}
					}
				}

				// Actual move code for in-bound windows (the main application of this tool).
				LOG("Found target monitor in the monitor list. Moving window to the next monitor in the monitor list...");
				// Restore the window to it's original size before moving. This is because restoring it after the move will bring it back to where it was before it was maximized.
				// It needs to be unmaximized after the move so that the OS can update the position to which it will be sent back to if it is restored.
				// NOTE: We only restore the window BEFORE the move because moving an unmaximized window makes more sense. Technically, you could unmaximize and then maximize it again after the move.
				if (ShowWindow(target, SW_RESTORE)) {
					// Move the target window to new monitor and resize it accordingly so that the intersection is large enough for window to maximize to the right monitor. Also repaint the window.
					if (MoveWindow(target, monitors[nextMonitor].rect.left, monitors[nextMonitor].rect.top,
						monitors[nextMonitor].rect.right - monitors[nextMonitor].rect.left, monitors[nextMonitor].rect.bottom - monitors[nextMonitor].rect.top, true)) {
						if (ShowWindow(target, SW_MAXIMIZE)) { return 0; }											// Maximize window.
						LOG("Couldn't maximize the target window.");
						return 0;																					// This and the following errors shouldn't terminate program, so we're not calling PostQuitMessage().
					}
					LOG("Couldn't move target window to new monitor.");
					return 0;
				}
				LOG("Couldn't restore target window before moving it to new monitor.");
				return 0;
			}
			LOG("No foreground window found.");
			return 0;
		}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);																// Pass unhandled messages to default message handler, which might use messages that we ignore.
}

#ifdef UNICODE
int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, wchar_t* lpCmdLine, int nCmdShow) {
	LOG("Started program from wWinMain entry point (UNICODE).");
#else
int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, char* lpCmdLine, int nCmdShow) {
	LOG("Started program from WinMain entry point (ANSI).");
#endif

	const TCHAR CLASS_NAME[] = TEXT("winswitch_window");															// TCHAR conditionally becomes either ANSI or wide based on the UNICODE define.

	WNDCLASS windowClass = { };																						// Create the WNDCLASS structure for the window.
	windowClass.lpfnWndProc = windowProc;
	windowClass.hInstance = hInstance;
	windowClass.lpszClassName = CLASS_NAME;

	LOG("Registering the window class...");
	RegisterClass(&windowClass);																					// Register window class with the operating system.

	LOG("Creating the window...");
	HWND hWnd = CreateWindowEx(0, CLASS_NAME, NULL, 0, 0, 0, 0, 0, HWND_MESSAGE, NULL, hInstance, NULL);			// Create a light-weight, bare-minimum, message-only, invisible window.

	if (!hWnd) {																									// If window creation not successful, terminate program.
		LOG("Error encountered while creating the window. Terminating...");											// No need to unregister WNDCLASS, OS does it for us on program termination.
		return 0;
	}

	LOG("Finding relevant system windows...");
	progman = FindWindow(TEXT("Progman"), TEXT("Program Manager"));													// Find the program manager window.
	if (!progman) {																									// If the program manager window couldn't be found, terminate.
		LOG("Error encountered while finding program manager. Terminating...");
		return 0;
	}
	tray = FindWindow(TEXT("Shell_TrayWnd"), TEXT(""));																// Find the shell tray window.
	if (!tray) {																									// If the window couldn't be found, terminate.
		LOG("Error encountered while finding the shell tray window. Terminating...");
		return 0;
	}

	LOG("Discovering monitors...");
	// TODO: Technically these if's should be inverted because I'm pretty sure the x86 branch predictor's first guess is NOT taken.
	// Also because I've heard compilers for x86 make the taken branch require the jump, possibly to optimize for the above fact.
	// TODO: Research about this topic.
	if (EnumDisplayMonitors(NULL, NULL, monitorDiscoveryProc, NULL)) {												// Enumerate the display monitors and add them to a list so that we can switch between them later.
		lastMonitorIndex = monitors.size() - 1;																		// Store the index of the last monitor for later use in the move algorithm.

		LOG("Registering hotkey...");
		if (RegisterHotKey(hWnd, 0, HOTKEY_MODIFIERS, HOTKEY_KEY)) {												// Register the hotkey with the OS.

			LOG("Running the message loop...");
			MSG msg = { };
			while (GetMessage(&msg, NULL, 0, 0)) {																	// Loop and keep receiving messages.
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
			return 0;

		}
		LOG("Failed to register hotkey with the OS. Terminating...");
		return 0;																									// PostQuitMessage is only required (and only possible) if you have a message loop.

	}
	LOG("Failed to enumerate the display monitors. Terminating...");
	return 0;
}