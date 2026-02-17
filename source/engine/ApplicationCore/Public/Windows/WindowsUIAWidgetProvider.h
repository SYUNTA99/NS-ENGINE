/// @file WindowsUIAWidgetProvider.h
/// @brief Windows UI Automation プロバイダー階層 (14-03)
#pragma once

#ifndef UE_WINDOWS_USING_UIA
#define UE_WINDOWS_USING_UIA 0
#endif

#if UE_WINDOWS_USING_UIA

#if defined(_WIN32) || defined(_WIN64)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <uiautomation.h>
#endif

#include "ApplicationCore/ApplicationCoreTypes.h"
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>

#pragma comment(lib, "uiautomationcore.lib")

namespace NS
{
    class IAccessibleWidget;
    class GenericWindow;

    // =========================================================================
    // UIA ControlTypeId マッピング
    // =========================================================================

    /// AccessibleWidgetType → UIA ControlTypeId 変換
    inline CONTROLTYPEID AccessibleWidgetTypeToUIA(AccessibleWidgetType Type)
    {
        switch (Type)
        {
        case AccessibleWidgetType::Button:
            return UIA_ButtonControlTypeId;
        case AccessibleWidgetType::CheckBox:
            return UIA_CheckBoxControlTypeId;
        case AccessibleWidgetType::ComboBox:
            return UIA_ComboBoxControlTypeId;
        case AccessibleWidgetType::Hyperlink:
            return UIA_HyperlinkControlTypeId;
        case AccessibleWidgetType::Image:
            return UIA_ImageControlTypeId;
        case AccessibleWidgetType::Layout:
            return UIA_GroupControlTypeId;
        case AccessibleWidgetType::ScrollBar:
            return UIA_ScrollBarControlTypeId;
        case AccessibleWidgetType::Slider:
            return UIA_SliderControlTypeId;
        case AccessibleWidgetType::Text:
            return UIA_TextControlTypeId;
        case AccessibleWidgetType::TextEdit:
            return UIA_EditControlTypeId;
        case AccessibleWidgetType::Window:
            return UIA_WindowControlTypeId;
        case AccessibleWidgetType::List:
            return UIA_ListControlTypeId;
        case AccessibleWidgetType::ListItem:
            return UIA_ListItemControlTypeId;
        default:
            return UIA_CustomControlTypeId;
        }
    }

    // =========================================================================
    // WindowsUIABaseProvider
    // =========================================================================

    /// UIA プロバイダー基底クラス (COM参照カウント)
    class WindowsUIABaseProvider : public IRawElementProviderSimple
    {
    public:
        WindowsUIABaseProvider() = default;
        virtual ~WindowsUIABaseProvider() = default;

        // IUnknown
        ULONG STDMETHODCALLTYPE AddRef() override { return ::InterlockedIncrement(&m_refCount); }

        ULONG STDMETHODCALLTYPE Release() override
        {
            LONG count = ::InterlockedDecrement(&m_refCount);
            if (count <= 0)
            {
                delete this;
                return 0;
            }
            return static_cast<ULONG>(count);
        }

        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override
        {
            if (!ppvObject)
                return E_INVALIDARG;

            if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, __uuidof(IRawElementProviderSimple)))
            {
                *ppvObject = static_cast<IRawElementProviderSimple*>(this);
                AddRef();
                return S_OK;
            }

            *ppvObject = nullptr;
            return E_NOINTERFACE;
        }

    protected:
        LONG m_refCount = 1;
    };

    // =========================================================================
    // WindowsUIAWidgetProvider
    // =========================================================================

    /// ウィジェット用UIAプロバイダー
    /// IRawElementProviderSimple + IRawElementProviderFragment を実装
    class WindowsUIAWidgetProvider : public WindowsUIABaseProvider, public IRawElementProviderFragment
    {
    public:
        explicit WindowsUIAWidgetProvider(std::shared_ptr<IAccessibleWidget> InWidget);
        ~WindowsUIAWidgetProvider() override = default;

        // IUnknown (IRawElementProviderFragment 経由)
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

        // IRawElementProviderSimple
        HRESULT STDMETHODCALLTYPE get_ProviderOptions(ProviderOptions* pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetPatternProvider(PATTERNID patternId, IUnknown** pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal) override;
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) override;

        // IRawElementProviderFragment
        HRESULT STDMETHODCALLTYPE Navigate(NavigateDirection direction, IRawElementProviderFragment** pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetRuntimeId(SAFEARRAY** pRetVal) override;
        HRESULT STDMETHODCALLTYPE get_BoundingRectangle(UiaRect* pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal) override;
        HRESULT STDMETHODCALLTYPE SetFocus() override;
        HRESULT STDMETHODCALLTYPE get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal) override;

        /// 関連ウィジェットを取得
        std::shared_ptr<IAccessibleWidget> GetWidget() const { return m_widget; }

    protected:
        std::shared_ptr<IAccessibleWidget> m_widget;
    };

    // =========================================================================
    // WindowsUIAWindowProvider
    // =========================================================================

    /// ウィンドウ用UIAプロバイダー (FragmentRoot)
    class WindowsUIAWindowProvider : public WindowsUIAWidgetProvider, public IRawElementProviderFragmentRoot
    {
    public:
        WindowsUIAWindowProvider(std::shared_ptr<IAccessibleWidget> InWidget, HWND InHWnd);
        ~WindowsUIAWindowProvider() override = default;

        // IUnknown
        HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** ppvObject) override;

        // IRawElementProviderFragmentRoot
        HRESULT STDMETHODCALLTYPE ElementProviderFromPoint(double x,
                                                           double y,
                                                           IRawElementProviderFragment** pRetVal) override;
        HRESULT STDMETHODCALLTYPE GetFocus(IRawElementProviderFragment** pRetVal) override;

        // IRawElementProviderSimple override
        HRESULT STDMETHODCALLTYPE get_HostRawElementProvider(IRawElementProviderSimple** pRetVal) override;

    private:
        HWND m_hwnd = nullptr;
    };

    // =========================================================================
    // ScopedWidgetProvider (RAII)
    // =========================================================================

    /// スコープベースのプロバイダー参照管理
    class ScopedWidgetProvider
    {
    public:
        explicit ScopedWidgetProvider(WindowsUIAWidgetProvider* InProvider) : m_provider(InProvider)
        {
            if (m_provider)
                m_provider->AddRef();
        }

        ~ScopedWidgetProvider()
        {
            if (m_provider)
                m_provider->Release();
        }

        ScopedWidgetProvider(const ScopedWidgetProvider&) = delete;
        ScopedWidgetProvider& operator=(const ScopedWidgetProvider&) = delete;

        WindowsUIAWidgetProvider* Get() const { return m_provider; }

    private:
        WindowsUIAWidgetProvider* m_provider = nullptr;
    };

    // =========================================================================
    // WindowsUIAManager
    // =========================================================================

    /// UIA プロバイダーキャッシュ・管理
    class WindowsUIAManager
    {
    public:
        /// シングルトン取得
        static WindowsUIAManager& Get();

        /// プロバイダー取得 (なければ作成)
        WindowsUIAWidgetProvider* GetOrCreateProvider(std::shared_ptr<IAccessibleWidget> Widget);

        /// 生ポインタからプロバイダー検索（キャッシュのみ）
        WindowsUIAWidgetProvider* FindProvider(IAccessibleWidget* Widget);

        /// プロバイダー削除
        void RemoveProvider(const std::shared_ptr<IAccessibleWidget>& Widget);

        /// 全プロバイダークリア
        void Clear();

    private:
        WindowsUIAManager() = default;
        std::unordered_map<IAccessibleWidget*, WindowsUIAWidgetProvider*> m_providerCache;
    };

} // namespace NS

#endif // UE_WINDOWS_USING_UIA
