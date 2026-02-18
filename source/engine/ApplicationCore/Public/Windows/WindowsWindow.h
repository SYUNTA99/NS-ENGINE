/// @file WindowsWindow.h
/// @brief Windows固有ウィンドウ実装（GenericWindow + IDropTarget）
#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h> // must precede oleidl.h

#include <oleidl.h>

// Windows API マクロがメソッド名と衝突するため除去
#undef IsMaximized
#undef IsMinimized
#endif

#include "GenericPlatform/GenericWindow.h"
#include <memory>
#include <string>
#include <vector>

namespace NS
{
    class GenericApplication;
    struct DragDropOLEData;

    /// Win32 ウィンドウラッパー
    class WindowsWindow : public GenericWindow, public IDropTarget, public std::enable_shared_from_this<WindowsWindow>
    {
    public:
        /// ウィンドウクラス名
        static const NS::TCHAR kAppWindowClass[];

        /// WndProc コールバック設定（WindowsApplication から呼び出す）
        static void SetWndProcCallback(WNDPROC proc);

        /// ウィンドウクラス登録
        static void Initialize(HINSTANCE hInstance, HICON hIcon);

        /// ファクトリ
        static std::shared_ptr<WindowsWindow> MakeWindow();

        WindowsWindow();
        ~WindowsWindow() override;
        NS_DISALLOW_COPY_AND_MOVE(WindowsWindow);
    public:

        /// インスタンス初期化（OS ウィンドウ生成）
        void Initialize(GenericApplication* kApplication,
                        const GenericWindowDefinition& inDefinition,
                        HINSTANCE hInstance,
                        const std::shared_ptr<WindowsWindow>& inParent,
                        bool bShowImmediately);

        /// HWND アクセサ
        HWND GetHWnd() const { return m_hwnd; }

        /// 親ウィンドウ最小化・復元通知
        void OnParentWindowMinimized();
        void OnParentWindowRestored();

        /// 透過サポート動的変更
        void OnTransparencySupportChanged(WindowTransparency newTransparency);

        /// ウィンドウ領域調整（仮想サイズ最適化）
        void AdjustWindowRegion(int32_t width, int32_t height);

        /// WindowZone → Win32 ヒットテストコード変換
        static int32_t WindowZoneToHitTest(WindowZone::Type zone);

        // =====================================================================
        // GenericWindow overrides
        // =====================================================================
        void ReshapeWindow(int32_t x, int32_t y, int32_t width, int32_t height) override;
        void MoveWindowTo(int32_t x, int32_t y) override;
        bool GetFullScreenInfo(int32_t& x, int32_t& y, int32_t& width, int32_t& height) const override;
        bool GetRestoredDimensions(int32_t& x, int32_t& y, int32_t& width, int32_t& height) const override;
        void AdjustCachedSize(PlatformRect& size) const override;
        void Destroy() override;
        void SetWindowMode(WindowMode::Type newWindowMode) override;
        WindowMode::Type GetWindowMode() const override;
        void Show() override;
        void Hide() override;
        void Minimize() override;
        void Maximize() override;
        void Restore() override;
        void BringToFront(bool bForce = false) override;
        void HackForceToFront() override ;
        void SetWindowFocus() override;
        void Enable(bool bEnable) override;
        bool IsEnabled() const override;
        void SetOpacity(float inOpacity) override;
        void SetText(const NS::TCHAR* inText) override;
        float GetDPIScaleFactor() const override;
        void SetDPIScaleFactor(float value) override;
        bool IsManualManageDPIChanges() const override;
        void SetManualManageDPIChanges(bool bManual) override;
        int32_t GetWindowBorderSize() const override;
        int32_t GetWindowTitleBarSize() const override;
        void* GetOSWindowHandle() const override;
        void DrawAttention(const WindowDrawAttentionParameters& parameters) override;
        bool IsMaximized() const override;
        bool IsMinimized() const override;
        bool IsVisible() const override;
        bool IsForegroundWindow() const override;
        bool IsPointInWindow(int32_t x, int32_t y) const override;

        // =====================================================================
        // IDropTarget
        // =====================================================================
        HRESULT STDMETHODCALLTYPE DragEnter(IDataObject* pDataObj,
                                            DWORD grfKeyState,
                                            POINTL pt,
                                            DWORD* pdwEffect) override;
        HRESULT STDMETHODCALLTYPE DragOver(DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;
        HRESULT STDMETHODCALLTYPE DragLeave() override;
        HRESULT STDMETHODCALLTYPE Drop(IDataObject* pDataObj, DWORD grfKeyState, POINTL pt, DWORD* pdwEffect) override;

        // =====================================================================
        // IUnknown
        // =====================================================================
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;
        ULONG STDMETHODCALLTYPE AddRef() override;
        ULONG STDMETHODCALLTYPE Release() override;

    private:
        HWND m_hwnd = nullptr;
        int32_t m_virtualWidth = 0;
        int32_t m_virtualHeight = 0;
        float m_aspectRatio = 0.0F;
        bool m_bIsVisible = false;
        bool m_bIsFirstTimeVisible = true;
        LONG m_oleRefCount = 1; // COM規約: オブジェクト生成時に1
        WINDOWPLACEMENT m_preFullscreenWindowPlacement = {};
        HANDLE m_waitableTimer = nullptr;
        GenericApplication* m_owningApplication = nullptr;

        // OLE D&D (13-04)
        std::unique_ptr<DragDropOLEData> m_dragDropData;
    };

} // namespace NS
