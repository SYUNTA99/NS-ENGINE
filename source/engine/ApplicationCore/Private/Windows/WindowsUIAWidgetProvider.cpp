/// @file WindowsUIAWidgetProvider.cpp
/// @brief Windows UI Automation プロバイダー実装 (14-03)

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

#include "GenericPlatform/IAccessibleWidget.h"
#include "Windows/WindowsUIAWidgetProvider.h"

namespace NS
{
    // =========================================================================
    // WindowsUIAWidgetProvider
    // =========================================================================

    WindowsUIAWidgetProvider::WindowsUIAWidgetProvider(std::shared_ptr<IAccessibleWidget> InWidget)
        : m_widget(std::move(InWidget))
    {}

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::QueryInterface(REFIID riid, void** ppvObject)
    {
        if (!ppvObject)
            return E_INVALIDARG;

        if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, __uuidof(IRawElementProviderSimple)))
        {
            *ppvObject = static_cast<IRawElementProviderSimple*>(this);
            AddRef();
            return S_OK;
        }
        if (IsEqualIID(riid, __uuidof(IRawElementProviderFragment)))
        {
            *ppvObject = static_cast<IRawElementProviderFragment*>(this);
            AddRef();
            return S_OK;
        }

        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::get_ProviderOptions(ProviderOptions* pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;
        *pRetVal = ProviderOptions_ServerSideProvider;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::GetPatternProvider(PATTERNID /*patternId*/, IUnknown** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;
        *pRetVal = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::GetPropertyValue(PROPERTYID propertyId, VARIANT* pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;

        ::VariantInit(pRetVal);

        if (!m_widget || !m_widget->IsValid())
            return S_OK;

        switch (propertyId)
        {
        case UIA_ControlTypePropertyId:
            pRetVal->vt = VT_I4;
            pRetVal->lVal = static_cast<LONG>(AccessibleWidgetTypeToUIA(m_widget->GetWidgetType()));
            break;

        case UIA_NamePropertyId:
        {
            std::wstring name = m_widget->GetWidgetName();
            pRetVal->vt = VT_BSTR;
            pRetVal->bstrVal = ::SysAllocString(name.c_str());
            break;
        }

        case UIA_HelpTextPropertyId:
        {
            std::wstring help = m_widget->GetHelpText();
            if (!help.empty())
            {
                pRetVal->vt = VT_BSTR;
                pRetVal->bstrVal = ::SysAllocString(help.c_str());
            }
            break;
        }

        case UIA_IsEnabledPropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = m_widget->IsEnabled() ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        case UIA_IsKeyboardFocusablePropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = m_widget->SupportsFocus() ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        case UIA_IsOffscreenPropertyId:
            pRetVal->vt = VT_BOOL;
            pRetVal->boolVal = m_widget->IsHidden() ? VARIANT_TRUE : VARIANT_FALSE;
            break;

        case UIA_ProcessIdPropertyId:
            pRetVal->vt = VT_I4;
            pRetVal->lVal = static_cast<LONG>(::GetCurrentProcessId());
            break;

        default:
            break;
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;
        *pRetVal = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::Navigate(NavigateDirection direction,
                                                                 IRawElementProviderFragment** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;
        *pRetVal = nullptr;

        if (!m_widget || !m_widget->IsValid())
            return S_OK;

        IAccessibleWidget* target = nullptr;

        switch (direction)
        {
        case NavigateDirection_Parent:
            target = m_widget->GetParent();
            break;
        case NavigateDirection_NextSibling:
            target = m_widget->GetNextSibling();
            break;
        case NavigateDirection_PreviousSibling:
            target = m_widget->GetPreviousSibling();
            break;
        case NavigateDirection_FirstChild:
            if (m_widget->GetNumberOfChildren() > 0)
                target = m_widget->GetChildAt(0);
            break;
        case NavigateDirection_LastChild:
        {
            int32_t count = m_widget->GetNumberOfChildren();
            if (count > 0)
                target = m_widget->GetChildAt(count - 1);
            break;
        }
        }

        if (target)
        {
            auto* provider = WindowsUIAManager::Get().FindProvider(target);
            if (provider)
            {
                *pRetVal = static_cast<IRawElementProviderFragment*>(provider);
                provider->AddRef();
            }
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::GetRuntimeId(SAFEARRAY** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;

        int runtimeId[2] = {UiaAppendRuntimeId, m_widget ? m_widget->GetId() : 0};
        *pRetVal = ::SafeArrayCreateVector(VT_I4, 0, 2);
        if (*pRetVal)
        {
            for (LONG i = 0; i < 2; ++i)
            {
                ::SafeArrayPutElement(*pRetVal, &i, &runtimeId[i]);
            }
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::get_BoundingRectangle(UiaRect* pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;

        pRetVal->left = 0;
        pRetVal->top = 0;
        pRetVal->width = 0;
        pRetVal->height = 0;

        if (m_widget && m_widget->IsValid())
        {
            int32_t x, y, w, h;
            m_widget->GetBounds(x, y, w, h);
            pRetVal->left = static_cast<double>(x);
            pRetVal->top = static_cast<double>(y);
            pRetVal->width = static_cast<double>(w);
            pRetVal->height = static_cast<double>(h);
        }

        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::GetEmbeddedFragmentRoots(SAFEARRAY** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;
        *pRetVal = nullptr;
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::SetFocus()
    {
        if (m_widget && m_widget->SupportsAccessibleFocus())
        {
            m_widget->SetUserFocus(0);
        }
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWidgetProvider::get_FragmentRoot(IRawElementProviderFragmentRoot** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;
        // ルートウィジェットの親をたどってウィンドウプロバイダーを探す
        IAccessibleWidget* root = m_widget.get();
        while (root && root->GetParent())
        {
            root = root->GetParent();
        }
        if (root)
        {
            auto* provider = WindowsUIAManager::Get().FindProvider(root);
            if (provider)
            {
                HRESULT hr = provider->QueryInterface(__uuidof(IRawElementProviderFragmentRoot),
                                                      reinterpret_cast<void**>(pRetVal));
                if (SUCCEEDED(hr))
                    return S_OK;
            }
        }
        *pRetVal = nullptr;
        return S_OK;
    }

    // =========================================================================
    // WindowsUIAWindowProvider
    // =========================================================================

    WindowsUIAWindowProvider::WindowsUIAWindowProvider(std::shared_ptr<IAccessibleWidget> InWidget, HWND InHWnd)
        : WindowsUIAWidgetProvider(std::move(InWidget)), m_hwnd(InHWnd)
    {}

    HRESULT STDMETHODCALLTYPE WindowsUIAWindowProvider::QueryInterface(REFIID riid, void** ppvObject)
    {
        if (!ppvObject)
            return E_INVALIDARG;

        if (IsEqualIID(riid, __uuidof(IRawElementProviderFragmentRoot)))
        {
            *ppvObject = static_cast<IRawElementProviderFragmentRoot*>(this);
            AddRef();
            return S_OK;
        }

        return WindowsUIAWidgetProvider::QueryInterface(riid, ppvObject);
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWindowProvider::ElementProviderFromPoint(double /*x*/,
                                                                                 double /*y*/,
                                                                                 IRawElementProviderFragment** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;
        *pRetVal = nullptr;
        // TODO: 座標からウィジェットを検索してプロバイダーを返す
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWindowProvider::GetFocus(IRawElementProviderFragment** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;
        *pRetVal = nullptr;
        // TODO: フォーカスされたウィジェットのプロバイダーを返す
        return S_OK;
    }

    HRESULT STDMETHODCALLTYPE WindowsUIAWindowProvider::get_HostRawElementProvider(IRawElementProviderSimple** pRetVal)
    {
        if (!pRetVal)
            return E_INVALIDARG;

        if (m_hwnd)
        {
            return ::UiaHostProviderFromHwnd(m_hwnd, pRetVal);
        }

        *pRetVal = nullptr;
        return S_OK;
    }

    // =========================================================================
    // WindowsUIAManager
    // =========================================================================

    WindowsUIAManager& WindowsUIAManager::Get()
    {
        static WindowsUIAManager instance;
        return instance;
    }

    WindowsUIAWidgetProvider* WindowsUIAManager::FindProvider(IAccessibleWidget* Widget)
    {
        if (!Widget)
            return nullptr;

        auto it = m_providerCache.find(Widget);
        if (it != m_providerCache.end())
        {
            return it->second;
        }
        return nullptr;
    }

    WindowsUIAWidgetProvider* WindowsUIAManager::GetOrCreateProvider(std::shared_ptr<IAccessibleWidget> Widget)
    {
        if (!Widget)
            return nullptr;

        auto it = m_providerCache.find(Widget.get());
        if (it != m_providerCache.end())
        {
            return it->second;
        }

        auto* provider = new WindowsUIAWidgetProvider(Widget);
        m_providerCache[Widget.get()] = provider;
        return provider;
    }

    void WindowsUIAManager::RemoveProvider(const std::shared_ptr<IAccessibleWidget>& Widget)
    {
        if (!Widget)
            return;

        auto it = m_providerCache.find(Widget.get());
        if (it != m_providerCache.end())
        {
            it->second->Release();
            m_providerCache.erase(it);
        }
    }

    void WindowsUIAManager::Clear()
    {
        for (auto& pair : m_providerCache)
        {
            if (pair.second)
                pair.second->Release();
        }
        m_providerCache.clear();
    }

} // namespace NS

#endif // UE_WINDOWS_USING_UIA
