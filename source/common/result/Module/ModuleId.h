/// @file ModuleId.h
/// @brief モジュールID定義
#pragma once


namespace NS::result
{

    /// モジュールID定義
    /// 範囲: 1-511 (9bit, 0は成功用に予約)
    namespace ModuleId
    {
        // Core (1-9)
        constexpr int Common = 1; // 共通エラー

        // System (10-49)
        constexpr int FileSystem = 10; // ファイルシステム
        constexpr int Os = 11;         // OS
        constexpr int Memory = 12;     // メモリ管理
        constexpr int Network = 13;    // ネットワーク
        constexpr int Thread = 14;     // スレッド

        // Engine (50-99)
        constexpr int Graphics = 50; // グラフィックス
        constexpr int Ecs = 51;      // ECS
        constexpr int Audio = 52;    // オーディオ
        constexpr int Input = 53;    // 入力
        constexpr int Resource = 54; // リソース管理
        constexpr int Scene = 55;    // シーン管理
        constexpr int Physics = 56;  // 物理

        // Application (100-149)
        constexpr int Application = 100; // アプリケーション
        constexpr int Ui = 101;          // UI
        constexpr int Script = 102;      // スクリプト

        // User defined (200-511)
        constexpr int UserBegin = 200;
        constexpr int UserEnd = 512;
    } // namespace ModuleId

    /// モジュール名取得
    [[nodiscard]] const char* GetModuleName(int moduleId) noexcept;

} // namespace NS::result
