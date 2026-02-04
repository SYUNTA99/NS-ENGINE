# 13: Integration - Tests

## 目的

Result型システム全体の統合テストを実装。

## ファイル

| ファイル | 操作 |
|----------|------|
| `Source/Tests/ResultTest.cpp` | 新規作成 |

## 設計

### テスト項目

#### 1. Core Tests

```cpp
// ResultTraitsテスト
TEST_CASE("ResultTraits bit layout") {
    using Traits = NS::Result::Detail::ResultTraits;

    // レイアウト検証
    STATIC_REQUIRE(Traits::kModuleBitsCount == 9);
    STATIC_REQUIRE(Traits::kDescriptionBitsCount == 13);
    STATIC_REQUIRE(Traits::kReservedBitsCount == 10);
    STATIC_REQUIRE(Traits::kModuleBitsCount + Traits::kDescriptionBitsCount +
                   Traits::kReservedBitsCount == 32);

    // 値生成・抽出
    constexpr auto value = Traits::MakeInnerValue(2, 100);
    STATIC_REQUIRE(Traits::GetModuleFromValue(value) == 2);
    STATIC_REQUIRE(Traits::GetDescriptionFromValue(value) == 100);

    // Reserved操作
    constexpr auto valueWithReserved = Traits::SetReserved(value, 5);
    STATIC_REQUIRE(Traits::GetReservedFromValue(valueWithReserved) == 5);
    STATIC_REQUIRE(Traits::MaskReservedFromValue(valueWithReserved) == value);
}

// Resultテスト
TEST_CASE("Result basic operations") {
    NS::Result success;
    REQUIRE(success.IsSuccess());
    REQUIRE_FALSE(success.IsFailure());

    NS::Result failure = NS::CommonResult::ResultOperationFailed();
    REQUIRE(failure.IsFailure());
    REQUIRE_FALSE(failure.IsSuccess());

    // 比較
    REQUIRE(success != failure);
    REQUIRE(failure == NS::CommonResult::ResultOperationFailed());
}

// ResultSuccessテスト
TEST_CASE("ResultSuccess conversion") {
    NS::ResultSuccess success;
    REQUIRE(success.IsSuccess());

    NS::Result result = success;
    REQUIRE(result.IsSuccess());

    // 失敗をResultSuccessに変換しようとするとアボート
    // （これはテストできないが、動作確認は手動で）
}

// Trivial性テスト
TEST_CASE("Result trivial copyability") {
    STATIC_REQUIRE(std::is_trivially_copyable_v<NS::Result>);
    STATIC_REQUIRE(std::is_trivially_copyable_v<NS::ResultSuccess>);
    STATIC_REQUIRE(sizeof(NS::Result) == sizeof(std::uint32_t));
}
```

#### 2. Error Tests

```cpp
// エラー定義テスト
TEST_CASE("Error result definitions") {
    auto pathNotFound = NS::FileSystemResult::ResultPathNotFound();
    REQUIRE(pathNotFound.GetModule() == NS::Result::ModuleId::FileSystem);
    REQUIRE(pathNotFound.GetDescription() == 1);

    // Result変換
    NS::Result result = pathNotFound;
    REQUIRE(result == pathNotFound);
    REQUIRE(NS::FileSystemResult::ResultPathNotFound::CanAccept(result));
}

// 範囲マッチングテスト
TEST_CASE("Error range matching") {
    // ResultDataCorruptedは4000-5000の範囲
    auto dataError = NS::Result::Detail::ConstructResult(
        NS::Result::Detail::ResultTraits::MakeInnerValue(
            NS::Result::ModuleId::FileSystem, 4500));

    REQUIRE(NS::FileSystemResult::ResultDataCorrupted::Includes(dataError));
    REQUIRE_FALSE(NS::FileSystemResult::ResultPathError::Includes(dataError));
}

// カテゴリテスト
TEST_CASE("Error category") {
    auto timeout = NS::CommonResult::ResultTimeout();
    auto category = NS::Result::GetErrorCategory(timeout);

    REQUIRE(category.persistence == NS::Result::ErrorPersistence::Transient);
    REQUIRE(category.IsRetriable());

    auto corruption = NS::CommonResult::ResultDataCorrupted();
    auto corruptionCategory = NS::Result::GetErrorCategory(corruption);

    REQUIRE(corruptionCategory.persistence == NS::Result::ErrorPersistence::Permanent);
    REQUIRE_FALSE(corruptionCategory.IsRetriable());
}
```

#### 3. Context Tests

```cpp
// SourceLocationテスト
TEST_CASE("SourceLocation capture") {
    auto loc = NS::Result::SourceLocation::FromStd();
    REQUIRE(loc.IsValid());
    REQUIRE(loc.line > 0);
    REQUIRE_FALSE(loc.file.empty());
}

// ResultContextテスト
TEST_CASE("ResultContext with location") {
    auto error = NS::FileSystemResult::ResultPathNotFound();
    NS::Result::RecordContext(error, NS_CURRENT_SOURCE_LOCATION(), "test message");

    auto ctx = NS::Result::GetResultContext(error);
    REQUIRE(ctx.has_value());
    REQUIRE(ctx->location.IsValid());
    REQUIRE(ctx->message == "test message");
}

// マクロテスト
TEST_CASE("Context macros") {
    auto testFunc = []() -> NS::Result {
        NS_RETURN_IF_FAILED_CTX(NS::FileSystemResult::ResultPathNotFound());
        return NS::ResultSuccess();
    };

    NS::Result result = testFunc();
    REQUIRE(result.IsFailure());

    auto ctx = NS::Result::GetResultContext(result);
    REQUIRE(ctx.has_value());
}
```

#### 4. ErrorChain Tests

```cpp
// エラーチェーンテスト
TEST_CASE("Error chain building") {
    auto innerError = NS::FileSystemResult::ResultPathNotFound();
    auto outerError = NS::Result::MakeChainedResult(
        NS::CommonResult::ResultLoadFailed(),
        innerError,
        "Failed to load config"
    );

    REQUIRE(outerError == NS::CommonResult::ResultLoadFailed());

    const auto* chain = NS::Result::GetErrorChain(outerError);
    REQUIRE(chain != nullptr);
    REQUIRE(chain->GetDepth() == 2);
    REQUIRE(chain->GetResult() == NS::CommonResult::ResultLoadFailed());
    REQUIRE(chain->GetRootCause() == innerError);
}

// チェーンビルダーテスト
TEST_CASE("ErrorChainBuilder") {
    NS::Result::ErrorChainBuilder builder(NS::CommonResult::ResultOperationFailed());
    builder.CausedBy(NS::FileSystemResult::ResultAccessDenied())
           .CausedBy(NS::OsResult::ResultNotEnoughPrivilege());

    auto chain = builder.Build();
    REQUIRE(chain.GetDepth() == 3);
}
```

#### 5. Formatter Tests

```cpp
// フォーマットテスト
TEST_CASE("Result formatting") {
    auto error = NS::FileSystemResult::ResultPathNotFound();

    auto compact = NS::Result::FormatResultCompact(error);
    REQUIRE(compact.find("FileSystem") != std::string::npos);
    REQUIRE(compact.find("PathNotFound") != std::string::npos);

    auto verbose = NS::Result::FormatResultVerbose(error);
    REQUIRE(verbose.find("Module=") != std::string::npos);
}

// std::formatテスト
TEST_CASE("std::format support") {
    auto error = NS::FileSystemResult::ResultPathNotFound();
    auto str = std::format("Error: {}", error);
    REQUIRE_FALSE(str.empty());
}
```

#### 6. Registry Tests

```cpp
// レジストリテスト
TEST_CASE("Result registry") {
    auto& registry = NS::Result::ResultRegistry::Instance();

    // 型を検索
    auto error = NS::FileSystemResult::ResultPathNotFound();
    auto* info = registry.Find(error);

    REQUIRE(info != nullptr);
    REQUIRE(info->module == NS::Result::ModuleId::FileSystem);
}
```

#### 7. Statistics Tests

```cpp
// 統計テスト
TEST_CASE("Result statistics") {
    auto& stats = NS::Result::ResultStatistics::Instance();
    stats.Reset();

    // エラーを記録
    for (int i = 0; i < 100; ++i) {
        NS::Result::RecordError(NS::CommonResult::ResultTimeout());
    }
    for (int i = 0; i < 50; ++i) {
        NS::Result::RecordSuccess();
    }

    REQUIRE(stats.GetTotalErrors() == 100);
    REQUIRE(stats.GetTotalSuccess() == 50);

    // 頻出エラー
    auto topErrors = stats.GetTopErrors(5);
    REQUIRE_FALSE(topErrors.empty());
    REQUIRE(topErrors[0].result == NS::CommonResult::ResultTimeout());
    REQUIRE(topErrors[0].count == 100);

    // エラー率
    double rate = stats.GetErrorRate();
    REQUIRE(rate > 0.6);  // 100 / 150 ≈ 0.667
}
```

#### 8. Integration Tests

```cpp
// 統合テスト：実際の使用パターン
TEST_CASE("Integration: file loading pattern") {
    // ファイルロードのシミュレーション
    auto loadFile = [](const char* path) -> NS::Result {
        if (std::string_view(path).empty()) {
            return NS::CommonResult::ResultNullPointer();
        }
        if (std::string_view(path) == "missing.txt") {
            return NS::FileSystemResult::ResultPathNotFound();
        }
        return NS::ResultSuccess();
    };

    auto loadConfig = [&loadFile](const char* path) -> NS::Result {
        NS::Result fileResult = loadFile(path);
        if (fileResult.IsFailure()) {
            return NS::Result::MakeChainedResult(
                NS::CommonResult::ResultLoadFailed(),
                fileResult,
                "Config load failed"
            );
        }
        return NS::ResultSuccess();
    };

    // テスト
    NS::Result result = loadConfig("missing.txt");
    REQUIRE(result.IsFailure());
    REQUIRE(result == NS::CommonResult::ResultLoadFailed());

    // チェーン検証
    const auto* chain = NS::Result::GetErrorChain(result);
    REQUIRE(chain != nullptr);
    REQUIRE(chain->GetRootCause() == NS::FileSystemResult::ResultPathNotFound());
}

// パフォーマンステスト
TEST_CASE("Performance: Result creation overhead") {
    constexpr int kIterations = 1000000;

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < kIterations; ++i) {
        NS::Result r = NS::CommonResult::ResultOperationFailed();
        (void)r.IsFailure();
    }
    auto end = std::chrono::high_resolution_clock::now();

    auto duration = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
    auto perOp = duration.count() / kIterations;

    // 1操作あたり100ns以下であること
    REQUIRE(perOp < 100);
    INFO("Result creation: " << perOp << "ns per operation");
}
```

## 検証チェックリスト

- [ ] 全static_assertがパス
- [ ] 全単体テストがパス
- [ ] Debug/Releaseビルド成功
- [ ] メモリリークなし（MSVCのCRTデバッグ）
- [ ] スレッドセーフティ（TSan）
- [ ] パフォーマンス基準達成

## TODO

- [ ] `Source/Tests/` ディレクトリ確認
- [ ] `ResultTest.cpp` 作成
- [ ] Coreテスト実装
- [ ] Errorテスト実装
- [ ] Contextテスト実装
- [ ] Formatterテスト実装
- [ ] Statisticsテスト実装
- [ ] 統合テスト実装
- [ ] パフォーマンステスト実装
- [ ] 全テスト実行・パス確認
