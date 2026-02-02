//----------------------------------------------------------------------------
//! @file   typed_foreach.h
//! @brief  ECS TypedForEach - In/Out/InOut対応の型安全なForEach
//!
//! 可変長テンプレートを使用し、1〜8コンポーネントに対応。
//! コンパイル時にアクセスモードとラムダ引数の整合性を検証する。
//----------------------------------------------------------------------------
#pragma once

#include "access_mode.h"
#include "archetype_storage.h"
#include <type_traits>
#include <tuple>
#include <array>

namespace ECS {

// 前方宣言
class World;

namespace detail {

//============================================================================
// ラムダ引数検証ヘルパー
//============================================================================

// ラムダの引数型を取得するためのヘルパー
template<typename T>
struct function_traits;

// ラムダ（operator()を持つ）の特殊化
template<typename T>
struct function_traits : function_traits<decltype(&T::operator())> {};

// const メンバ関数ポインタ
template<typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const> {
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);

    template<size_t N>
    using arg_type = std::tuple_element_t<N, args_tuple>;
};

// 非const メンバ関数ポインタ
template<typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...)> {
    using args_tuple = std::tuple<Args...>;
    static constexpr size_t arity = sizeof...(Args);

    template<size_t N>
    using arg_type = std::tuple_element_t<N, args_tuple>;
};

//============================================================================
// アクセスモードと引数型の整合性検証
//============================================================================

// 基本テンプレート
template<typename AccessMode, typename LambdaArg>
struct validate_arg_type : std::false_type {};

// In<T> → const T& または const T
template<typename T, typename LambdaArg>
struct validate_arg_type<In<T>, LambdaArg> {
    // const参照 または const値で受け取る
    static constexpr bool value =
        std::is_same_v<std::remove_cvref_t<LambdaArg>, T> &&
        (std::is_const_v<std::remove_reference_t<LambdaArg>> ||
         !std::is_reference_v<LambdaArg>);
};

// InOut<T> → T& （非const参照）
template<typename T, typename LambdaArg>
struct validate_arg_type<InOut<T>, LambdaArg> {
    static constexpr bool value =
        std::is_same_v<std::decay_t<LambdaArg>, T> &&
        std::is_lvalue_reference_v<LambdaArg> &&
        !std::is_const_v<std::remove_reference_t<LambdaArg>>;
};

template<typename AccessMode, typename LambdaArg>
inline constexpr bool validate_arg_type_v = validate_arg_type<AccessMode, LambdaArg>::value;

//============================================================================
// アクセスモードシーケンスの検証
//============================================================================

template<typename Func, typename... AccessModes>
struct validate_lambda_args {
private:
    using traits = function_traits<std::decay_t<Func>>;

    // 引数数チェック: Actor + コンポーネント数
    static_assert(traits::arity == 1 + sizeof...(AccessModes),
        "Lambda must have Actor as first argument, followed by one argument per access mode");

    // 最初の引数がActorであることを確認
    static_assert(std::is_same_v<std::decay_t<typename traits::template arg_type<0>>, Actor>,
        "First lambda argument must be Actor");

    // 各アクセスモードと対応するラムダ引数の検証
    template<size_t... Is>
    static constexpr bool validate_all(std::index_sequence<Is...>) {
        return (validate_arg_type_v<
            std::tuple_element_t<Is, std::tuple<AccessModes...>>,
            typename traits::template arg_type<Is + 1>
        > && ...);
    }

public:
    static constexpr bool value = validate_all(std::index_sequence_for<AccessModes...>{});
};

template<typename Func, typename... AccessModes>
inline constexpr bool validate_lambda_args_v = validate_lambda_args<Func, AccessModes...>::value;

//============================================================================
// アクセスモード判定（全てがアクセスモードかどうか）
// ※ InvokeWithComponentsSoAのSFINAE判定で使用するため、先に定義
//============================================================================

template<typename... Ts>
struct all_are_access_modes : std::bool_constant<(is_access_mode_v<Ts> && ...)> {};

template<typename... Ts>
inline constexpr bool all_are_access_modes_v = all_are_access_modes<Ts...>::value;

//============================================================================
// コンポーネント参照取得ヘルパー（SoA対応）
//============================================================================

//! アクセスモードに応じたコンポーネント参照を取得（SoA）
//! @tparam AccessMode In<T>, Out<T>, InOut<T>のいずれか
//! @param arrayBase コンポーネント配列の先頭ポインタ
//! @param index エンティティインデックス
//! @return const T& (In) または T& (Out/InOut)
template<typename AccessMode>
[[nodiscard]] inline auto GetComponentRefSoA(
    std::byte* arrayBase,
    uint16_t index) noexcept -> decltype(auto)
{
    using T = unwrap_access_t<AccessMode>;
    T* array = reinterpret_cast<T*>(arrayBase);
    if constexpr (is_in_v<AccessMode>) {
        return static_cast<const T&>(array[index]);
    } else {
        return array[index];
    }
}

//============================================================================
// 可変長コンポーネント呼び出しヘルパー（SoA対応）
//============================================================================

//! 純粋型用のコンポーネント参照取得（SoA、レガシー互換）
template<typename T>
[[nodiscard]] inline T& GetComponentRefSoALegacy(
    std::byte* arrayBase,
    uint16_t index) noexcept
{
    T* array = reinterpret_cast<T*>(arrayBase);
    return array[index];
}

//! 可変長のコンポーネント参照でラムダを呼び出す（SoA、アクセスモード版）
//! @tparam AccessModes アクセスモード群
//! @tparam Func ラムダ型
//! @tparam Is インデックスシーケンス
template<typename... AccessModes, typename Func, size_t... Is,
         std::enable_if_t<all_are_access_modes_v<AccessModes...>, int> = 0>
inline void InvokeWithComponentsSoA(
    Func&& func,
    Actor actor,
    uint16_t index,
    const std::array<std::byte*, sizeof...(AccessModes)>& arrayBases,
    std::index_sequence<Is...>) noexcept(noexcept(
        func(actor, GetComponentRefSoA<std::tuple_element_t<Is, std::tuple<AccessModes...>>>(
            arrayBases[Is], index)...)))
{
    func(actor, GetComponentRefSoA<std::tuple_element_t<Is, std::tuple<AccessModes...>>>(
        arrayBases[Is], index)...);
}

//! 可変長のコンポーネント参照でラムダを呼び出す（SoA、レガシー版）
//! @tparam Ts コンポーネント型群
//! @tparam Func ラムダ型
//! @tparam Is インデックスシーケンス
template<typename... Ts, typename Func, size_t... Is,
         std::enable_if_t<!all_are_access_modes_v<Ts...>, int> = 0>
inline void InvokeWithComponentsSoA(
    Func&& func,
    Actor actor,
    uint16_t index,
    const std::array<std::byte*, sizeof...(Ts)>& arrayBases,
    std::index_sequence<Is...>) noexcept(noexcept(
        func(actor, GetComponentRefSoALegacy<std::tuple_element_t<Is, std::tuple<Ts...>>>(
            arrayBases[Is], index)...)))
{
    func(actor, GetComponentRefSoALegacy<std::tuple_element_t<Is, std::tuple<Ts...>>>(
        arrayBases[Is], index)...);
}

//============================================================================
// Write判定ヘルパー（変更追跡用）
//============================================================================

//! アクセスモード群にWrite（Out/InOut）が含まれるか
template<typename... AccessModes>
struct has_any_write : std::bool_constant<(is_write_access_v<AccessModes> || ...)> {};

template<typename... AccessModes>
inline constexpr bool has_any_write_v = has_any_write<AccessModes...>::value;

//! 指定インデックスのアクセスモードがWrite（Out/InOut）か
template<size_t I, typename... AccessModes>
inline constexpr bool is_write_at_v = is_write_access_v<
    std::tuple_element_t<I, std::tuple<AccessModes...>>
>;

//============================================================================
// Chunk変更マーキングヘルパー
//============================================================================

//! Write対象のコンポーネントのバージョンを更新
template<typename... AccessModes, size_t... Is>
void MarkWrittenComponents(
    Archetype& arch,
    size_t chunkIndex,
    uint32_t version,
    const std::array<size_t, sizeof...(AccessModes)>& /*offsets*/,
    std::index_sequence<Is...>) noexcept
{
    // 各アクセスモードがWriteかどうかをチェックし、Writeならバージョン更新
    (void)std::initializer_list<int>{
        (is_write_at_v<Is, AccessModes...>
            ? (arch.MarkComponentWritten<unwrap_access_t<
                std::tuple_element_t<Is, std::tuple<AccessModes...>>>>(
                    chunkIndex, version), 0)
            : 0)...
    };
}

//============================================================================
// 可変長 TypedForEach 実装（SoA対応）
//============================================================================

//! 可変長テンプレートによるTypedForEach実装（SoA）
//! 1〜8コンポーネントに対応
//!
//! @tparam AccessModes In<T>, Out<T>, InOut<T>の組み合わせ
//! @tparam Func ラムダ型 (Actor, T1&, T2&, ...) -> void
//! @param archetypes ArchetypeStorage参照
//! @param func 各エンティティに対して呼び出す関数
template<typename... AccessModes, typename Func>
void TypedForEachImpl(ArchetypeStorage& archetypes, Func&& func) {
    static_assert(sizeof...(AccessModes) >= 1,
        "At least one access mode required");
    static_assert(sizeof...(AccessModes) <= 8,
        "Maximum 8 components supported");

    // Write操作があるかどうかをコンパイル時に判定
    constexpr bool hasWrite = has_any_write_v<AccessModes...>;
    const uint32_t writeVersion = archetypes.GetWriteVersion();

    // 各アクセスモードから純粋な型を抽出
    archetypes.ForEachMatching<unwrap_access_t<AccessModes>...>(
        [&func, writeVersion](Archetype& arch) {
            auto& metas = arch.GetChunkMetas();

            // 各Chunkをイテレーション
            for (size_t ci = 0; ci < metas.size(); ++ci) {
                const uint16_t count = metas[ci].count;
                if (count == 0) continue;

                // Write操作がある場合、Chunk処理前にバージョンを更新
                if constexpr (hasWrite) {
                    // コンポーネントオフセットを取得（変更追跡用）
                    std::array<size_t, sizeof...(AccessModes)> offsets = {
                        arch.GetComponentOffset<unwrap_access_t<AccessModes>>()...
                    };
                    MarkWrittenComponents<AccessModes...>(
                        arch, ci, writeVersion, offsets,
                        std::index_sequence_for<AccessModes...>{}
                    );
                }

                // SoA: 各コンポーネント配列の先頭アドレスを取得
                std::array<std::byte*, sizeof...(AccessModes)> arrayBases = {
                    reinterpret_cast<std::byte*>(arch.GetComponentArray<unwrap_access_t<AccessModes>>(ci))...
                };

                const Actor* actors = arch.GetActorArray(ci);

                // 各エンティティをイテレーション（連続メモリアクセス）
                for (uint16_t i = 0; i < count; ++i) {
                    InvokeWithComponentsSoA<AccessModes...>(
                        func,
                        actors[i],
                        i,
                        arrayBases,
                        std::index_sequence_for<AccessModes...>{}
                    );
                }
            }
        }
    );
}

} // namespace detail

} // namespace ECS
