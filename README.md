# winswitch
Very simple, light-weight program which runs in the background and registers a hotkey for moving windows across monitors with the keyboard.

Only works in Windows!

# How To Build
Clone the repo (non-recursively, it does not and will never have submodules) and open it up in Visual Studio, it's a Visual Studio solution. Build/run it however you want, but I recommend Release|x86, since that's got all the correct settings set.

# How To Install
The final binary is independent, just move it where ever you want it and run it. I recommend putting it in some sort of startup folder, so that it runs everytime your computer boots.

# How To Debug
The program itself doesn't say anything whatsoever. It doesn't have any visible attributes, it simply runs silently in the background and quits silently if it encounters an error.

If you experience the hotkey not working, the program has almost definitely quit. If the program quits unexpectedly and you can't figure out why, just open up ths Visual Studio solution in this repo, set to Debug instead of Release and then debug it inside of Visual Studio. If you do this, the program will output diagnostics to the debug console in Visual Studio, which you can then read through.

# How To Use The Hotkey
Currently, I've baked the hotkey into the binary. It's easy enough to change it and recompile, but if you need a way to change it at runtime, post an issue and I'll try to get on that when I have time.

Anyway, the hotkey is Ctrl+Shift+Alt+S (the S stands for Switch). When you press it, it'll take whichever window is in focus at the moment and move it to the next monitor ("next" is determined by the programs internal monitor identifier buffer, which is determined by the system. I haven't found out if the order can be relied upon yet, so you should assume that the order is random for now). The window will be maximized after the move is complete.

Another useful feature is that you can use the program to retrieve windows that have gotten lost outside the bounds of the visible area in Windows. If a window is outside the bounds and belongs to no monitor, it'll be moved (just as above) to the first monitor in the monitor identifier buffer.
