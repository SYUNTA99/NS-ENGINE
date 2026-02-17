/// @file CVarStub.h
/// @brief CVar 基盤が未実装の場合のスタブ定義
/// CVar 基盤実装後にこのファイルを削除し、本物の CVar に差し替える
#pragma once

#include <cstdint>

// 読み取り専用 CVar スタブ: コンパイル時定数として振る舞う
#define NS_CVAR_INT(Name, Default, Description) inline constexpr int32_t Name = Default;
#define NS_CVAR_FLOAT(Name, Default, Description) inline constexpr float Name = Default;
#define NS_CVAR_BOOL(Name, Default, Description) inline constexpr bool Name = Default;
