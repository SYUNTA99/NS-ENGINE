//----------------------------------------------------------------------------
//! @file   generate_test_textures.cpp
//! @brief  テスト用テクスチャ生成ユーティリティ
//!
//! @details
//! このツールは以下のテストテクスチャを生成します：
//!
//! PNG形式（256x256）:
//! - checkerboard_256.png : チェッカーボードパターン（UV確認、タイリングテスト用）
//! - gradient_256.png     : 赤緑グラデーション（UV方向確認用）
//! - uv_test_256.png      : UV座標テストパターン（R=U, G=V）
//! - noise_256.png        : 擬似乱数ノイズ（フィルタリングテスト用）
//! - normal_flat_256.png  : フラット法線マップ（法線マップ読み込みテスト用）
//! - circle_256.png       : 円形パターン（アルファブレンディングテスト用）
//!
//! PNG形式（64x64）:
//! - white_64.png  : 白単色（乗算テスト用）
//! - black_64.png  : 黒単色（加算テスト用）
//! - red_64.png    : 赤単色（チャンネル確認用）
//! - green_64.png  : 緑単色（チャンネル確認用）
//! - blue_64.png   : 青単色（チャンネル確認用）
//!
//! DDS形式:
//! - checkerboard_256.dds : DDSローダーテスト用
//! - gradient_128.dds     : 小サイズDDSテスト用
//!
//! ビルド方法:
//!   cmake .. -DBUILD_TESTS=ON
//!   cmake --build . --config Debug
//!
//! 実行方法:
//!   ./Debug/generate_test_textures.exe
//!   出力先: tests/assets/textures/
//----------------------------------------------------------------------------

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <wincodec.h>
#include <wrl/client.h>
#include <vector>
#include <cstdint>
#include <cmath>
#include <iostream>
#include <filesystem>

#pragma comment(lib, "ole32.lib")
#pragma comment(lib, "windowscodecs.lib")

using Microsoft::WRL::ComPtr;
namespace fs = std::filesystem;

//----------------------------------------------------------------------------
// 画像生成関数
//----------------------------------------------------------------------------

//! チェッカーボードパターンを生成
//! @param [in] width 画像幅（ピクセル）
//! @param [in] height 画像高さ（ピクセル）
//! @param [in] cellSize 1マスのサイズ（ピクセル、デフォルト32）
//! @return RGBA形式のピクセルデータ
//!
//! @details
//! テスト用途：
//! - テクスチャのUV座標が正しく設定されているか確認
//! - テクスチャリピート/クランプの動作確認
//! - ミップマップ生成の品質確認
std::vector<uint8_t> GenerateCheckerboard(int width, int height, int cellSize = 32)
{
    std::vector<uint8_t> data(width * height * 4);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            // セル位置に基づいて白/グレーを決定
            bool white = ((x / cellSize) + (y / cellSize)) % 2 == 0;
            uint8_t color = white ? 255 : 64;

            data[idx + 0] = color; // R
            data[idx + 1] = color; // G
            data[idx + 2] = color; // B
            data[idx + 3] = 255;   // A（完全不透明）
        }
    }

    return data;
}

//! 赤緑グラデーションパターンを生成
//! @param [in] width 画像幅（ピクセル）
//! @param [in] height 画像高さ（ピクセル）
//! @return RGBA形式のピクセルデータ
//!
//! @details
//! テスト用途：
//! - テクスチャ座標の方向確認（左→右で赤増加、上→下で緑増加）
//! - バイリニアフィルタリングの動作確認
//! - ガンマ補正の確認
std::vector<uint8_t> GenerateGradient(int width, int height)
{
    std::vector<uint8_t> data(width * height * 4);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            data[idx + 0] = static_cast<uint8_t>(x * 255 / (width - 1));   // R: 水平方向
            data[idx + 1] = static_cast<uint8_t>(y * 255 / (height - 1)); // G: 垂直方向
            data[idx + 2] = 128;  // B: 固定値
            data[idx + 3] = 255;  // A
        }
    }

    return data;
}

//! 単色画像を生成
//! @param [in] width 画像幅（ピクセル）
//! @param [in] height 画像高さ（ピクセル）
//! @param [in] r 赤成分（0-255）
//! @param [in] g 緑成分（0-255）
//! @param [in] b 青成分（0-255）
//! @param [in] a アルファ成分（0-255、デフォルト255）
//! @return RGBA形式のピクセルデータ
//!
//! @details
//! テスト用途：
//! - テクスチャ乗算/加算の動作確認
//! - カラーチャンネルの正しい読み込み確認
//! - デフォルトテクスチャとしての使用
std::vector<uint8_t> GenerateSolidColor(int width, int height, uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255)
{
    std::vector<uint8_t> data(width * height * 4);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            data[idx + 0] = r;
            data[idx + 1] = g;
            data[idx + 2] = b;
            data[idx + 3] = a;
        }
    }

    return data;
}

//! フラット法線マップを生成（真上を向いた法線）
//! @param [in] width 画像幅（ピクセル）
//! @param [in] height 画像高さ（ピクセル）
//! @return RGBA形式のピクセルデータ
//!
//! @details
//! 法線エンコーディング：
//! - R = (Nx + 1) / 2 * 255  → 128 (Nx = 0)
//! - G = (Ny + 1) / 2 * 255  → 128 (Ny = 0)
//! - B = (Nz + 1) / 2 * 255  → 255 (Nz = 1)
//!
//! テスト用途：
//! - 法線マップの読み込みと展開の確認
//! - 法線マップなしメッシュのデフォルトテクスチャ
std::vector<uint8_t> GenerateFlatNormalMap(int width, int height)
{
    std::vector<uint8_t> data(width * height * 4);

    // フラット法線: (0, 0, 1) → RGB(128, 128, 255)
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            data[idx + 0] = 128; // X = 0 → 128
            data[idx + 1] = 128; // Y = 0 → 128
            data[idx + 2] = 255; // Z = 1 → 255
            data[idx + 3] = 255; // A
        }
    }

    return data;
}

//! 擬似乱数ノイズパターンを生成
//! @param [in] width 画像幅（ピクセル）
//! @param [in] height 画像高さ（ピクセル）
//! @param [in] seed 乱数シード（デフォルト12345）
//! @return RGBA形式のピクセルデータ
//!
//! @details
//! 線形合同法（LCG）による擬似乱数を使用。
//! 同じシードで同じパターンが生成されるため再現性がある。
//!
//! テスト用途：
//! - テクスチャフィルタリングの動作確認
//! - ミップマップ生成アルゴリズムの品質確認
//! - ノイズベースのエフェクト用
std::vector<uint8_t> GenerateNoise(int width, int height, uint32_t seed = 12345)
{
    std::vector<uint8_t> data(width * height * 4);

    // 線形合同法による簡易乱数生成
    uint32_t state = seed;
    auto nextRand = [&state]() -> uint8_t {
        state = state * 1103515245 + 12345;
        return static_cast<uint8_t>((state >> 16) & 0xFF);
    };

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;
            uint8_t value = nextRand();
            data[idx + 0] = value; // R
            data[idx + 1] = value; // G（グレースケール）
            data[idx + 2] = value; // B
            data[idx + 3] = 255;   // A
        }
    }

    return data;
}

//! UV座標テストパターンを生成
//! @param [in] width 画像幅（ピクセル）
//! @param [in] height 画像高さ（ピクセル）
//! @return RGBA形式のピクセルデータ
//!
//! @details
//! R = U座標（0.0→0, 1.0→255）
//! G = V座標（0.0→0, 1.0→255）
//! B = 0（未使用）
//!
//! テスト用途：
//! - メッシュのUV座標が正しく設定されているか視覚的に確認
//! - UV座標の歪みやストレッチの検出
std::vector<uint8_t> GenerateUVTestPattern(int width, int height)
{
    std::vector<uint8_t> data(width * height * 4);

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;

            // U座標 → 赤チャンネル
            data[idx + 0] = static_cast<uint8_t>(x * 255 / (width - 1));
            // V座標 → 緑チャンネル
            data[idx + 1] = static_cast<uint8_t>(y * 255 / (height - 1));
            // 青は0
            data[idx + 2] = 0;
            data[idx + 3] = 255;
        }
    }

    return data;
}

//! 円形パターンを生成（アンチエイリアス付き）
//! @param [in] width 画像幅（ピクセル）
//! @param [in] height 画像高さ（ピクセル）
//! @return RGBA形式のピクセルデータ
//!
//! @details
//! 中心に白い円、外側は透明。エッジは2ピクセルで滑らかに。
//!
//! テスト用途：
//! - アルファブレンディングの動作確認
//! - アルファテストの動作確認
//! - プリマルチプライドアルファの確認
std::vector<uint8_t> GenerateCircle(int width, int height)
{
    std::vector<uint8_t> data(width * height * 4);

    float cx = width / 2.0f;   // 円の中心X
    float cy = height / 2.0f;  // 円の中心Y
    float radius = std::min(cx, cy) * 0.9f;  // 半径（画像の90%）

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = (y * width + x) * 4;

            // 中心からの距離を計算
            float dx = x - cx;
            float dy = y - cy;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist < radius) {
                // 円の内側 - 白色、エッジでアンチエイリアス
                float alpha = std::min(1.0f, (radius - dist) / 2.0f);
                data[idx + 0] = 255;  // R
                data[idx + 1] = 255;  // G
                data[idx + 2] = 255;  // B
                data[idx + 3] = static_cast<uint8_t>(alpha * 255);  // A
            } else {
                // 円の外側 - 完全透明
                data[idx + 0] = 0;
                data[idx + 1] = 0;
                data[idx + 2] = 0;
                data[idx + 3] = 0;
            }
        }
    }

    return data;
}

//----------------------------------------------------------------------------
// 関数ポインタ互換用ラッパー関数
// （デフォルト引数を持つ関数は関数ポインタとして使えないため）
//----------------------------------------------------------------------------
std::vector<uint8_t> GenerateCheckerboard2(int w, int h) { return GenerateCheckerboard(w, h, 32); }
std::vector<uint8_t> GenerateNoise2(int w, int h) { return GenerateNoise(w, h, 12345); }
std::vector<uint8_t> GenerateWhite(int w, int h) { return GenerateSolidColor(w, h, 255, 255, 255); }
std::vector<uint8_t> GenerateBlack(int w, int h) { return GenerateSolidColor(w, h, 0, 0, 0); }
std::vector<uint8_t> GenerateRed(int w, int h) { return GenerateSolidColor(w, h, 255, 0, 0); }
std::vector<uint8_t> GenerateGreen(int w, int h) { return GenerateSolidColor(w, h, 0, 255, 0); }
std::vector<uint8_t> GenerateBlue(int w, int h) { return GenerateSolidColor(w, h, 0, 0, 255); }

//----------------------------------------------------------------------------
// WIC (Windows Imaging Component) を使用したPNG保存
//----------------------------------------------------------------------------

//! PNG形式で画像を保存
//! @param [in] filename 出力ファイルパス
//! @param [in] width 画像幅
//! @param [in] height 画像高さ
//! @param [in] data RGBA形式のピクセルデータ
//! @return 成功時true
bool SavePNG(const wchar_t* filename, int width, int height, const uint8_t* data)
{
    // WICファクトリの作成
    ComPtr<IWICImagingFactory> factory;
    HRESULT hr = CoCreateInstance(
        CLSID_WICImagingFactory,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&factory)
    );
    if (FAILED(hr)) {
        std::cerr << "Failed to create WIC factory" << std::endl;
        return false;
    }

    // ストリームの作成
    ComPtr<IWICStream> stream;
    hr = factory->CreateStream(&stream);
    if (FAILED(hr)) return false;

    hr = stream->InitializeFromFilename(filename, GENERIC_WRITE);
    if (FAILED(hr)) return false;

    // PNGエンコーダーの作成
    ComPtr<IWICBitmapEncoder> encoder;
    hr = factory->CreateEncoder(GUID_ContainerFormatPng, nullptr, &encoder);
    if (FAILED(hr)) return false;

    hr = encoder->Initialize(stream.Get(), WICBitmapEncoderNoCache);
    if (FAILED(hr)) return false;

    // フレームの作成（PNGは1フレームのみ）
    ComPtr<IWICBitmapFrameEncode> frame;
    hr = encoder->CreateNewFrame(&frame, nullptr);
    if (FAILED(hr)) return false;

    hr = frame->Initialize(nullptr);
    if (FAILED(hr)) return false;

    hr = frame->SetSize(width, height);
    if (FAILED(hr)) return false;

    // ピクセルフォーマットの設定（32bit RGBA）
    WICPixelFormatGUID format = GUID_WICPixelFormat32bppRGBA;
    hr = frame->SetPixelFormat(&format);
    if (FAILED(hr)) return false;

    // ピクセルデータの書き込み
    hr = frame->WritePixels(height, width * 4, width * height * 4, const_cast<BYTE*>(data));
    if (FAILED(hr)) return false;

    // コミット
    hr = frame->Commit();
    if (FAILED(hr)) return false;

    hr = encoder->Commit();
    if (FAILED(hr)) return false;

    return true;
}

//----------------------------------------------------------------------------
// DDS (DirectDraw Surface) 形式での保存
// 非圧縮RGBA形式のみサポート
//----------------------------------------------------------------------------

#pragma pack(push, 1)
//! DDSピクセルフォーマット構造体
struct DDS_PIXELFORMAT
{
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwFourCC;
    uint32_t dwRGBBitCount;
    uint32_t dwRBitMask;
    uint32_t dwGBitMask;
    uint32_t dwBBitMask;
    uint32_t dwABitMask;
};

//! DDSヘッダー構造体
struct DDS_HEADER
{
    uint32_t dwSize;
    uint32_t dwFlags;
    uint32_t dwHeight;
    uint32_t dwWidth;
    uint32_t dwPitchOrLinearSize;
    uint32_t dwDepth;
    uint32_t dwMipMapCount;
    uint32_t dwReserved1[11];
    DDS_PIXELFORMAT ddspf;
    uint32_t dwCaps;
    uint32_t dwCaps2;
    uint32_t dwCaps3;
    uint32_t dwCaps4;
    uint32_t dwReserved2;
};
#pragma pack(pop)

// DDSフォーマット定数
constexpr uint32_t DDS_MAGIC = 0x20534444;        // "DDS " (リトルエンディアン)
constexpr uint32_t DDSD_CAPS = 0x1;
constexpr uint32_t DDSD_HEIGHT = 0x2;
constexpr uint32_t DDSD_WIDTH = 0x4;
constexpr uint32_t DDSD_PITCH = 0x8;
constexpr uint32_t DDSD_PIXELFORMAT = 0x1000;
constexpr uint32_t DDPF_ALPHAPIXELS = 0x1;
constexpr uint32_t DDPF_RGB = 0x40;
constexpr uint32_t DDSCAPS_TEXTURE = 0x1000;

//! DDS形式で画像を保存（非圧縮BGRA）
//! @param [in] filename 出力ファイルパス
//! @param [in] width 画像幅
//! @param [in] height 画像高さ
//! @param [in] data RGBA形式のピクセルデータ
//! @return 成功時true
//!
//! @note DDSはBGRA順序のため、保存時にチャンネルを入れ替える
bool SaveDDS(const wchar_t* filename, int width, int height, const uint8_t* data)
{
    FILE* file = nullptr;
    _wfopen_s(&file, filename, L"wb");
    if (!file) return false;

    // マジックナンバー "DDS " の書き込み
    fwrite(&DDS_MAGIC, sizeof(uint32_t), 1, file);

    // ヘッダーの設定
    DDS_HEADER header = {};
    header.dwSize = sizeof(DDS_HEADER);
    header.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH | DDSD_PITCH | DDSD_PIXELFORMAT;
    header.dwHeight = height;
    header.dwWidth = width;
    header.dwPitchOrLinearSize = width * 4;  // 1行あたりのバイト数
    header.dwDepth = 1;
    header.dwMipMapCount = 1;

    // ピクセルフォーマット（非圧縮32bit BGRA）
    header.ddspf.dwSize = sizeof(DDS_PIXELFORMAT);
    header.ddspf.dwFlags = DDPF_ALPHAPIXELS | DDPF_RGB;
    header.ddspf.dwRGBBitCount = 32;
    header.ddspf.dwRBitMask = 0x000000FF;  // R
    header.ddspf.dwGBitMask = 0x0000FF00;  // G
    header.ddspf.dwBBitMask = 0x00FF0000;  // B
    header.ddspf.dwABitMask = 0xFF000000;  // A

    header.dwCaps = DDSCAPS_TEXTURE;

    fwrite(&header, sizeof(DDS_HEADER), 1, file);

    // RGBAからBGRAへ変換して書き込み
    std::vector<uint8_t> bgraData(width * height * 4);
    for (int i = 0; i < width * height; i++) {
        bgraData[i * 4 + 0] = data[i * 4 + 2]; // B ← R位置からBを取得
        bgraData[i * 4 + 1] = data[i * 4 + 1]; // G
        bgraData[i * 4 + 2] = data[i * 4 + 0]; // R ← B位置からRを取得
        bgraData[i * 4 + 3] = data[i * 4 + 3]; // A
    }

    fwrite(bgraData.data(), 1, bgraData.size(), file);

    fclose(file);
    return true;
}

//----------------------------------------------------------------------------
// メイン関数
//----------------------------------------------------------------------------

int main()
{
    // コンソール出力をUTF-8に設定
    SetConsoleOutputCP(CP_UTF8);

    std::cout << "=== テストテクスチャ生成ツール ===" << std::endl;

    // COMの初期化（WIC使用のため）
    CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);

    // 出力ディレクトリの決定
    fs::path outputDir = fs::current_path() / "tests" / "assets" / "textures";

    // ビルドディレクトリから実行された場合の代替パス
    if (!fs::exists(outputDir.parent_path())) {
        outputDir = fs::current_path().parent_path() / "tests" / "assets" / "textures";
    }

    // ディレクトリが存在しない場合は作成
    if (!fs::exists(outputDir)) {
        std::cout << "ディレクトリ作成: " << outputDir << std::endl;
        fs::create_directories(outputDir);
    }

    std::cout << "出力先: " << outputDir << std::endl;

    //----------------------------------------------------------------------
    // テクスチャ生成定義
    //----------------------------------------------------------------------
    struct TextureInfo {
        const wchar_t* name;  //!< ファイル名
        int width;            //!< 幅
        int height;           //!< 高さ
        std::vector<uint8_t>(*generator)(int, int);  //!< 生成関数
    };

    std::vector<TextureInfo> textures = {
        // 256x256テクスチャ（主要なテスト用）
        { L"checkerboard_256.png", 256, 256, GenerateCheckerboard2 },  // チェッカーボード
        { L"gradient_256.png", 256, 256, GenerateGradient },           // グラデーション
        { L"uv_test_256.png", 256, 256, GenerateUVTestPattern },       // UV座標テスト
        { L"noise_256.png", 256, 256, GenerateNoise2 },                // ノイズ
        { L"normal_flat_256.png", 256, 256, GenerateFlatNormalMap },   // フラット法線
        { L"circle_256.png", 256, 256, GenerateCircle },               // 円形（アルファ）

        // 64x64テクスチャ（単色、小サイズテスト用）
        { L"white_64.png", 64, 64, GenerateWhite },   // 白
        { L"black_64.png", 64, 64, GenerateBlack },   // 黒
        { L"red_64.png", 64, 64, GenerateRed },       // 赤
        { L"green_64.png", 64, 64, GenerateGreen },   // 緑
        { L"blue_64.png", 64, 64, GenerateBlue },     // 青
    };

    int successCount = 0;

    //----------------------------------------------------------------------
    // PNG形式で保存
    //----------------------------------------------------------------------
    std::cout << "\nPNGテクスチャを生成中..." << std::endl;

    for (const auto& tex : textures) {
        auto data = tex.generator(tex.width, tex.height);
        fs::path filepath = outputDir / tex.name;

        std::cout << "生成中: " << filepath.filename().string()
                  << " (" << tex.width << "x" << tex.height << ")... ";

        if (SavePNG(filepath.wstring().c_str(), tex.width, tex.height, data.data())) {
            std::cout << "OK" << std::endl;
            successCount++;
        } else {
            std::cout << "失敗" << std::endl;
        }
    }

    //----------------------------------------------------------------------
    // DDS形式で保存（DDSローダーテスト用）
    //----------------------------------------------------------------------
    std::cout << "\nDDSテクスチャを生成中..." << std::endl;

    // チェッカーボード 256x256 DDS
    auto checkerData = GenerateCheckerboard(256, 256);
    fs::path ddsPath = outputDir / L"checkerboard_256.dds";
    std::cout << "生成中: checkerboard_256.dds... ";
    if (SaveDDS(ddsPath.wstring().c_str(), 256, 256, checkerData.data())) {
        std::cout << "OK" << std::endl;
        successCount++;
    } else {
        std::cout << "失敗" << std::endl;
    }

    // グラデーション 128x128 DDS（小サイズテスト）
    auto gradientData = GenerateGradient(128, 128);
    ddsPath = outputDir / L"gradient_128.dds";
    std::cout << "生成中: gradient_128.dds... ";
    if (SaveDDS(ddsPath.wstring().c_str(), 128, 128, gradientData.data())) {
        std::cout << "OK" << std::endl;
        successCount++;
    } else {
        std::cout << "失敗" << std::endl;
    }

    // COM終了処理
    CoUninitialize();

    //----------------------------------------------------------------------
    // 完了メッセージ
    //----------------------------------------------------------------------
    std::cout << "\n=== 完了 ===" << std::endl;
    std::cout << "生成したテクスチャ数: " << successCount << std::endl;

    return 0;
}
