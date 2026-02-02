// =============================================================================
// File: com_ptr.h
// Description: ComPtr alias for COM object management
// =============================================================================

#pragma once

#include <wrl/client.h>

// Convenient alias for Microsoft::WRL::ComPtr
template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
