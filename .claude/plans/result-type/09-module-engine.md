# 09: Module - Engine Results

## 目的

エンジンレイヤーのエラー定義を実装（Graphics, ECS, Audio, Input, Resource, Scene, Physics）。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/common/result/Module/GraphicsResult.h` | 新規作成 |
| `Source/common/result/Module/EcsResult.h` | 新規作成 |
| `Source/common/result/Module/AudioResult.h` | 新規作成 |
| `Source/common/result/Module/InputResult.h` | 新規作成 |
| `Source/common/result/Module/ResourceResult.h` | 新規作成 |
| `Source/common/result/Module/SceneResult.h` | 新規作成 |
| `Source/common/result/Module/PhysicsResult.h` | 新規作成 |

## 設計

### GraphicsResult

```cpp
// Source/common/result/Module/GraphicsResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// グラフィックスエラー
namespace GraphicsResult {

    //=========================================================================
    // デバイス関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultDeviceError, Result::ModuleId::Graphics, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultDeviceCreationFailed, Result::ModuleId::Graphics, 1);
    NS_DEFINE_ERROR_RESULT(ResultDeviceLost, Result::ModuleId::Graphics, 2);
    NS_DEFINE_ERROR_RESULT(ResultDeviceRemoved, Result::ModuleId::Graphics, 3);
    NS_DEFINE_ERROR_RESULT(ResultDeviceNotSupported, Result::ModuleId::Graphics, 4);
    NS_DEFINE_ERROR_RESULT(ResultFeatureNotSupported, Result::ModuleId::Graphics, 5);
    NS_DEFINE_ERROR_RESULT(ResultDriverVersionMismatch, Result::ModuleId::Graphics, 6);
    NS_DEFINE_ERROR_RESULT(ResultAdapterNotFound, Result::ModuleId::Graphics, 7);
    NS_DEFINE_ERROR_RESULT(ResultDisplayModeNotSupported, Result::ModuleId::Graphics, 8);

    //=========================================================================
    // リソース関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultResourceError, Result::ModuleId::Graphics, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultTextureCreationFailed, Result::ModuleId::Graphics, 100);
    NS_DEFINE_ERROR_RESULT(ResultBufferCreationFailed, Result::ModuleId::Graphics, 101);
    NS_DEFINE_ERROR_RESULT(ResultShaderCreationFailed, Result::ModuleId::Graphics, 102);
    NS_DEFINE_ERROR_RESULT(ResultPipelineCreationFailed, Result::ModuleId::Graphics, 103);
    NS_DEFINE_ERROR_RESULT(ResultRenderTargetCreationFailed, Result::ModuleId::Graphics, 104);
    NS_DEFINE_ERROR_RESULT(ResultSamplerCreationFailed, Result::ModuleId::Graphics, 105);
    NS_DEFINE_ERROR_RESULT(ResultResourceMapFailed, Result::ModuleId::Graphics, 106);
    NS_DEFINE_ERROR_RESULT(ResultResourceUnmapFailed, Result::ModuleId::Graphics, 107);
    NS_DEFINE_ERROR_RESULT(ResultResourceUpdateFailed, Result::ModuleId::Graphics, 108);
    NS_DEFINE_ERROR_RESULT(ResultResourceBindFailed, Result::ModuleId::Graphics, 109);
    NS_DEFINE_ERROR_RESULT(ResultInvalidResourceState, Result::ModuleId::Graphics, 110);
    NS_DEFINE_ERROR_RESULT(ResultResourceNotResident, Result::ModuleId::Graphics, 111);

    //=========================================================================
    // シェーダー関連 (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultShaderError, Result::ModuleId::Graphics, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultShaderCompilationFailed, Result::ModuleId::Graphics, 200);
    NS_DEFINE_ERROR_RESULT(ResultShaderLinkFailed, Result::ModuleId::Graphics, 201);
    NS_DEFINE_ERROR_RESULT(ResultShaderReflectionFailed, Result::ModuleId::Graphics, 202);
    NS_DEFINE_ERROR_RESULT(ResultInvalidShaderBytecode, Result::ModuleId::Graphics, 203);
    NS_DEFINE_ERROR_RESULT(ResultShaderSignatureMismatch, Result::ModuleId::Graphics, 204);
    NS_DEFINE_ERROR_RESULT(ResultConstantBufferMismatch, Result::ModuleId::Graphics, 205);
    NS_DEFINE_ERROR_RESULT(ResultRootSignatureError, Result::ModuleId::Graphics, 206);

    //=========================================================================
    // レンダリング関連 (300-399)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultRenderingError, Result::ModuleId::Graphics, 300, 400);
    NS_DEFINE_ERROR_RESULT(ResultSwapChainCreationFailed, Result::ModuleId::Graphics, 300);
    NS_DEFINE_ERROR_RESULT(ResultSwapChainResizeFailed, Result::ModuleId::Graphics, 301);
    NS_DEFINE_ERROR_RESULT(ResultPresentFailed, Result::ModuleId::Graphics, 302);
    NS_DEFINE_ERROR_RESULT(ResultFrameSkipped, Result::ModuleId::Graphics, 303);
    NS_DEFINE_ERROR_RESULT(ResultCommandListError, Result::ModuleId::Graphics, 304);
    NS_DEFINE_ERROR_RESULT(ResultCommandQueueError, Result::ModuleId::Graphics, 305);
    NS_DEFINE_ERROR_RESULT(ResultFenceError, Result::ModuleId::Graphics, 306);
    NS_DEFINE_ERROR_RESULT(ResultGpuTimeout, Result::ModuleId::Graphics, 307);
    NS_DEFINE_ERROR_RESULT(ResultGpuHang, Result::ModuleId::Graphics, 308);

    //=========================================================================
    // 検証関連 (400-499)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultValidationError, Result::ModuleId::Graphics, 400, 500);
    NS_DEFINE_ERROR_RESULT(ResultInvalidPipelineState, Result::ModuleId::Graphics, 400);
    NS_DEFINE_ERROR_RESULT(ResultInvalidRenderState, Result::ModuleId::Graphics, 401);
    NS_DEFINE_ERROR_RESULT(ResultInvalidDescriptor, Result::ModuleId::Graphics, 402);
    NS_DEFINE_ERROR_RESULT(ResultDescriptorHeapFull, Result::ModuleId::Graphics, 403);
    NS_DEFINE_ERROR_RESULT(ResultInvalidFormat, Result::ModuleId::Graphics, 404);
    NS_DEFINE_ERROR_RESULT(ResultInvalidDimension, Result::ModuleId::Graphics, 405);

} // namespace GraphicsResult

} // namespace NS
```

### EcsResult

```cpp
// Source/common/result/Module/EcsResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// ECSエラー
namespace EcsResult {

    //=========================================================================
    // エンティティ関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultEntityError, Result::ModuleId::Ecs, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultEntityNotFound, Result::ModuleId::Ecs, 1);
    NS_DEFINE_ERROR_RESULT(ResultEntityAlreadyExists, Result::ModuleId::Ecs, 2);
    NS_DEFINE_ERROR_RESULT(ResultEntityDestroyed, Result::ModuleId::Ecs, 3);
    NS_DEFINE_ERROR_RESULT(ResultEntityVersionMismatch, Result::ModuleId::Ecs, 4);
    NS_DEFINE_ERROR_RESULT(ResultMaxEntitiesReached, Result::ModuleId::Ecs, 5);
    NS_DEFINE_ERROR_RESULT(ResultInvalidEntityId, Result::ModuleId::Ecs, 6);

    //=========================================================================
    // コンポーネント関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultComponentError, Result::ModuleId::Ecs, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultComponentNotFound, Result::ModuleId::Ecs, 100);
    NS_DEFINE_ERROR_RESULT(ResultComponentAlreadyExists, Result::ModuleId::Ecs, 101);
    NS_DEFINE_ERROR_RESULT(ResultComponentTypeMismatch, Result::ModuleId::Ecs, 102);
    NS_DEFINE_ERROR_RESULT(ResultMaxComponentsReached, Result::ModuleId::Ecs, 103);
    NS_DEFINE_ERROR_RESULT(ResultComponentNotRegistered, Result::ModuleId::Ecs, 104);
    NS_DEFINE_ERROR_RESULT(ResultInvalidComponentId, Result::ModuleId::Ecs, 105);
    NS_DEFINE_ERROR_RESULT(ResultComponentSizeExceeded, Result::ModuleId::Ecs, 106);

    //=========================================================================
    // アーキタイプ関連 (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultArchetypeError, Result::ModuleId::Ecs, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultArchetypeNotFound, Result::ModuleId::Ecs, 200);
    NS_DEFINE_ERROR_RESULT(ResultArchetypeMismatch, Result::ModuleId::Ecs, 201);
    NS_DEFINE_ERROR_RESULT(ResultChunkAllocationFailed, Result::ModuleId::Ecs, 202);
    NS_DEFINE_ERROR_RESULT(ResultChunkCapacityExceeded, Result::ModuleId::Ecs, 203);
    NS_DEFINE_ERROR_RESULT(ResultArchetypeLayoutInvalid, Result::ModuleId::Ecs, 204);

    //=========================================================================
    // システム関連 (300-399)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultSystemError, Result::ModuleId::Ecs, 300, 400);
    NS_DEFINE_ERROR_RESULT(ResultSystemNotFound, Result::ModuleId::Ecs, 300);
    NS_DEFINE_ERROR_RESULT(ResultSystemAlreadyExists, Result::ModuleId::Ecs, 301);
    NS_DEFINE_ERROR_RESULT(ResultSystemDependencyCycle, Result::ModuleId::Ecs, 302);
    NS_DEFINE_ERROR_RESULT(ResultSystemExecutionFailed, Result::ModuleId::Ecs, 303);
    NS_DEFINE_ERROR_RESULT(ResultSystemDisabled, Result::ModuleId::Ecs, 304);

    //=========================================================================
    // クエリ関連 (400-499)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultQueryError, Result::ModuleId::Ecs, 400, 500);
    NS_DEFINE_ERROR_RESULT(ResultQueryCreationFailed, Result::ModuleId::Ecs, 400);
    NS_DEFINE_ERROR_RESULT(ResultQueryInvalidFilter, Result::ModuleId::Ecs, 401);
    NS_DEFINE_ERROR_RESULT(ResultQueryNoMatch, Result::ModuleId::Ecs, 402);
    NS_DEFINE_ERROR_RESULT(ResultQueryIteratorInvalid, Result::ModuleId::Ecs, 403);

} // namespace EcsResult

} // namespace NS
```

### AudioResult

```cpp
// Source/common/result/Module/AudioResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// オーディオエラー
namespace AudioResult {

    //=========================================================================
    // デバイス関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultDeviceError, Result::ModuleId::Audio, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultDeviceCreationFailed, Result::ModuleId::Audio, 1);
    NS_DEFINE_ERROR_RESULT(ResultDeviceNotAvailable, Result::ModuleId::Audio, 2);
    NS_DEFINE_ERROR_RESULT(ResultDeviceLost, Result::ModuleId::Audio, 3);
    NS_DEFINE_ERROR_RESULT(ResultFormatNotSupported, Result::ModuleId::Audio, 4);
    NS_DEFINE_ERROR_RESULT(ResultDriverError, Result::ModuleId::Audio, 5);

    //=========================================================================
    // 再生関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultPlaybackError, Result::ModuleId::Audio, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultPlaybackFailed, Result::ModuleId::Audio, 100);
    NS_DEFINE_ERROR_RESULT(ResultBufferUnderrun, Result::ModuleId::Audio, 101);
    NS_DEFINE_ERROR_RESULT(ResultBufferOverrun, Result::ModuleId::Audio, 102);
    NS_DEFINE_ERROR_RESULT(ResultSourceNotFound, Result::ModuleId::Audio, 103);
    NS_DEFINE_ERROR_RESULT(ResultVoiceLimitReached, Result::ModuleId::Audio, 104);
    NS_DEFINE_ERROR_RESULT(ResultMixerError, Result::ModuleId::Audio, 105);

    //=========================================================================
    // フォーマット関連 (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultFormatError, Result::ModuleId::Audio, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultInvalidFormat, Result::ModuleId::Audio, 200);
    NS_DEFINE_ERROR_RESULT(ResultDecodeFailed, Result::ModuleId::Audio, 201);
    NS_DEFINE_ERROR_RESULT(ResultEncodeFailed, Result::ModuleId::Audio, 202);
    NS_DEFINE_ERROR_RESULT(ResultStreamError, Result::ModuleId::Audio, 203);
    NS_DEFINE_ERROR_RESULT(ResultCodecNotFound, Result::ModuleId::Audio, 204);

} // namespace AudioResult

} // namespace NS
```

### InputResult

```cpp
// Source/common/result/Module/InputResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// 入力エラー
namespace InputResult {

    //=========================================================================
    // デバイス関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultDeviceError, Result::ModuleId::Input, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultDeviceNotFound, Result::ModuleId::Input, 1);
    NS_DEFINE_ERROR_RESULT(ResultDeviceDisconnected, Result::ModuleId::Input, 2);
    NS_DEFINE_ERROR_RESULT(ResultDeviceNotSupported, Result::ModuleId::Input, 3);
    NS_DEFINE_ERROR_RESULT(ResultDeviceInitFailed, Result::ModuleId::Input, 4);
    NS_DEFINE_ERROR_RESULT(ResultDeviceBusy, Result::ModuleId::Input, 5);

    //=========================================================================
    // バインディング関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultBindingError, Result::ModuleId::Input, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultBindingNotFound, Result::ModuleId::Input, 100);
    NS_DEFINE_ERROR_RESULT(ResultBindingConflict, Result::ModuleId::Input, 101);
    NS_DEFINE_ERROR_RESULT(ResultInvalidBinding, Result::ModuleId::Input, 102);
    NS_DEFINE_ERROR_RESULT(ResultActionNotFound, Result::ModuleId::Input, 103);

} // namespace InputResult

} // namespace NS
```

### ResourceResult

```cpp
// Source/common/result/Module/ResourceResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// リソース管理エラー
namespace ResourceResult {

    //=========================================================================
    // ロード関連 (1-99)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultLoadError, Result::ModuleId::Resource, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultLoadFailed, Result::ModuleId::Resource, 1);
    NS_DEFINE_ERROR_RESULT(ResultResourceNotFound, Result::ModuleId::Resource, 2);
    NS_DEFINE_ERROR_RESULT(ResultInvalidResourceType, Result::ModuleId::Resource, 3);
    NS_DEFINE_ERROR_RESULT(ResultLoaderNotFound, Result::ModuleId::Resource, 4);
    NS_DEFINE_ERROR_RESULT(ResultDependencyMissing, Result::ModuleId::Resource, 5);
    NS_DEFINE_ERROR_RESULT(ResultCircularDependency, Result::ModuleId::Resource, 6);
    NS_DEFINE_ERROR_RESULT(ResultVersionMismatch, Result::ModuleId::Resource, 7);

    //=========================================================================
    // キャッシュ関連 (100-199)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultCacheError, Result::ModuleId::Resource, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultCacheMiss, Result::ModuleId::Resource, 100);
    NS_DEFINE_ERROR_RESULT(ResultCacheCorrupted, Result::ModuleId::Resource, 101);
    NS_DEFINE_ERROR_RESULT(ResultCacheFull, Result::ModuleId::Resource, 102);
    NS_DEFINE_ERROR_RESULT(ResultCacheEvictionFailed, Result::ModuleId::Resource, 103);

    //=========================================================================
    // ハンドル関連 (200-299)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultHandleError, Result::ModuleId::Resource, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultInvalidHandle, Result::ModuleId::Resource, 200);
    NS_DEFINE_ERROR_RESULT(ResultHandleExpired, Result::ModuleId::Resource, 201);
    NS_DEFINE_ERROR_RESULT(ResultHandleTypeMismatch, Result::ModuleId::Resource, 202);
    NS_DEFINE_ERROR_RESULT(ResultResourceNotReady, Result::ModuleId::Resource, 203);
    NS_DEFINE_ERROR_RESULT(ResultResourceStillReferenced, Result::ModuleId::Resource, 204);

    //=========================================================================
    // パッケージ関連 (300-399)
    //=========================================================================

    NS_DEFINE_ERROR_RANGE_RESULT(ResultPackageError, Result::ModuleId::Resource, 300, 400);
    NS_DEFINE_ERROR_RESULT(ResultPackageNotFound, Result::ModuleId::Resource, 300);
    NS_DEFINE_ERROR_RESULT(ResultPackageCorrupted, Result::ModuleId::Resource, 301);
    NS_DEFINE_ERROR_RESULT(ResultPackageVersionMismatch, Result::ModuleId::Resource, 302);
    NS_DEFINE_ERROR_RESULT(ResultPackageMountFailed, Result::ModuleId::Resource, 303);
    NS_DEFINE_ERROR_RESULT(ResultPackageUnmountFailed, Result::ModuleId::Resource, 304);

} // namespace ResourceResult

} // namespace NS
```

### SceneResult / PhysicsResult

```cpp
// Source/common/result/Module/SceneResult.h
#pragma once

#include "common/result/Error/ErrorDefines.h"
#include "common/result/Module/ModuleId.h"

namespace NS {

/// シーン管理エラー
namespace SceneResult {

    NS_DEFINE_ERROR_RANGE_RESULT(ResultSceneError, Result::ModuleId::Scene, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultSceneNotFound, Result::ModuleId::Scene, 1);
    NS_DEFINE_ERROR_RESULT(ResultSceneAlreadyLoaded, Result::ModuleId::Scene, 2);
    NS_DEFINE_ERROR_RESULT(ResultSceneLoadFailed, Result::ModuleId::Scene, 3);
    NS_DEFINE_ERROR_RESULT(ResultSceneUnloadFailed, Result::ModuleId::Scene, 4);
    NS_DEFINE_ERROR_RESULT(ResultSceneTransitionFailed, Result::ModuleId::Scene, 5);

    NS_DEFINE_ERROR_RANGE_RESULT(ResultObjectError, Result::ModuleId::Scene, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultObjectNotFound, Result::ModuleId::Scene, 100);
    NS_DEFINE_ERROR_RESULT(ResultObjectNameConflict, Result::ModuleId::Scene, 101);
    NS_DEFINE_ERROR_RESULT(ResultInvalidHierarchy, Result::ModuleId::Scene, 102);
    NS_DEFINE_ERROR_RESULT(ResultCircularParent, Result::ModuleId::Scene, 103);

} // namespace SceneResult

/// 物理エラー
namespace PhysicsResult {

    NS_DEFINE_ERROR_RANGE_RESULT(ResultWorldError, Result::ModuleId::Physics, 1, 100);
    NS_DEFINE_ERROR_RESULT(ResultWorldCreationFailed, Result::ModuleId::Physics, 1);
    NS_DEFINE_ERROR_RESULT(ResultSimulationFailed, Result::ModuleId::Physics, 2);
    NS_DEFINE_ERROR_RESULT(ResultStepFailed, Result::ModuleId::Physics, 3);

    NS_DEFINE_ERROR_RANGE_RESULT(ResultBodyError, Result::ModuleId::Physics, 100, 200);
    NS_DEFINE_ERROR_RESULT(ResultBodyCreationFailed, Result::ModuleId::Physics, 100);
    NS_DEFINE_ERROR_RESULT(ResultBodyNotFound, Result::ModuleId::Physics, 101);
    NS_DEFINE_ERROR_RESULT(ResultInvalidBodyType, Result::ModuleId::Physics, 102);

    NS_DEFINE_ERROR_RANGE_RESULT(ResultColliderError, Result::ModuleId::Physics, 200, 300);
    NS_DEFINE_ERROR_RESULT(ResultColliderCreationFailed, Result::ModuleId::Physics, 200);
    NS_DEFINE_ERROR_RESULT(ResultInvalidColliderShape, Result::ModuleId::Physics, 201);
    NS_DEFINE_ERROR_RESULT(ResultCollisionDetectionFailed, Result::ModuleId::Physics, 202);

} // namespace PhysicsResult

} // namespace NS
```

## TODO

- [ ] `GraphicsResult.h` 作成
- [ ] `EcsResult.h` 作成
- [ ] `AudioResult.h` 作成
- [ ] `InputResult.h` 作成
- [ ] `ResourceResult.h` 作成
- [ ] `SceneResult.h` 作成
- [ ] `PhysicsResult.h` 作成
- [ ] 各.cpp実装（カテゴリ/情報解決）
- [ ] ビルド確認
