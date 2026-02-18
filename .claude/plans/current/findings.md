# Findings

## branch-clone 対象 (8件)
1. WindowsApplication.cpp:779 — switch 連続同一分岐
2. WindowsTextInputMethodSystem.cpp:194 — switch 連続同一分岐
3. WindowsWindow.cpp:309 — if/else 同一本体
4. ConsoleManager.cpp:188 — if/else 同一本体
5. ConsoleManager.cpp:352 — if/else 同一本体
6. ConsoleManager.h:175 — if/else 同一本体
7. WindowsPlatformAffinity.cpp:188 — switch 連続同一分岐
8. WindowsPlatformAffinity.cpp:194 — switch 連続同一分岐

## special-member-functions 対象クラス (~30)
### ApplicationCore
GenericApplication, GenericApplicationMessageHandler, GenericPlatformSoftwareCursor,
GenericPlatformSplash, GenericWindow, ICursor, IPlatformInputDeviceMapper,
ITextInputMethodChangeNotifier, ITextInputMethodContext, ITextInputMethodSystem,
IWindowsMessageHandler, WindowsApplication, WindowsCursor,
WindowsTextInputMethodSystem, WindowsWindow, WindowsSplash

### HAL
GenericPlatformCrashContext, IFileHandle, IPlatformFile,
IConsoleManager, TAutoConsoleVariable, IConsoleObject,
MallocAnsi, Malloc, OutputDevice, OutputDeviceDebug, OutputDeviceConsole,
ConsoleManagerImpl

## owning-memory 対象 (18件)
- ConsoleManager.cpp: Register系7種 (new/delete → unique_ptr)
- ConsoleManager.cpp: UnregisterConsoleObject (delete)
- WindowsPlatformFile.cpp:251,276 (new返却)
