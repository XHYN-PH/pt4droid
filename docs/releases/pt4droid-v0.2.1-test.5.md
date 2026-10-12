# pt4droid 0.2.1-test.5

This update adds invisible movement controls, a cleaner launcher and a way to return to the menu without force-closing the app.

## Invisible movement

- Added **Movement control → Invisible left-half movement** in Settings. Touch and drag anywhere in the left half to move, hold to keep walking, and lift to stop. The joystick stays invisible, including while pressed.
- Pair it with **Look control → Swipe to look • no button** for movement on the left and looking on the right without either stick covering the game.
- Movement and look work together. Each finger keeps its original role when dragged across the middle, and extra fingers cannot steal an active stick. Action buttons and Settings take priority.
- Your existing visible movement joystick stays selected until you change the option. Invisible movement works with Floating sticks off; stick size and dead zone still apply. Both guides remain visible in the layout editor so they can be adjusted.

## Menu and logo

- Removed the extra **Game graphics** shortcut from the launcher. All graphics options remain available under **Settings → Game graphics**.
- Removed the logo's black square background, keeping white P.T. lettering on the dark launcher. The README uses transparent white or dark lettering to stay readable in both themes. The adaptive icon's foreground uses the updated logo too.
- Added **Return to menu** to the in-game settings page and Android settings. Confirming it lets the native game shut down, then opens the launcher. You can start again or use the launcher's Quit button. Progress follows the game's normal save points.

## Fixes and checks

- Fixed Settings access when another finger is already moving or looking. Opening it releases the active controls.
- Added a save guard across processes so saves cannot be deleted while the game is running or closing. A stopped or crashed process releases the guard automatically.
- Updated the exit/restart flow to start each game session with fresh SDL and renderer state. Cancelled import leftovers are removed on the next game start; starting during an active import is blocked.
- Reviewed the supplied test.4 log: the imported Turnip driver loaded successfully, and no warning, error or crash messages were recorded. Shadows were the largest measured GPU pass in the heavier scene. This update keeps the existing fixed-resolution renderer and performance options.

Install this signed APK over test.4 to keep your game files, saves, drivers and settings. Android hardware testing is still needed, especially repeated return/start cycles, invisible controls and second-screen devices. Mali/PanVK support remains experimental.
