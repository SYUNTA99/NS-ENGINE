#include "title_scene.h"
#include "test_scene.h"
#include "engine/scene/scene_manager.h"
#include "engine/input/input_manager.h"
#include "engine/input/key.h"
#include "engine/c_systems/sprite_batch.h"
#include "engine/debug/debug_draw.h"
#include "common/logging/logging.h"


//ライフサイクル
//初期化
void Title_Scene::OnEnter()
{
	LOG_INFO("現在のシーン : タイトル");
	camera_ = std::make_unique<Camera2D>();
	//画面中央
	camera_->SetPosition(Vector2(640.0f, 360.0f));
}
//破棄
void Title_Scene::OnExit()
{
	camera_.reset();
}

//フレームコールバック
//更新
void Title_Scene::Update()
{
	auto& input = *InputManager::GetInstance();

	//space or enterがおされたら
	if (input.GetKeyboard().IsKeyDown(Key::Enter) || input.GetKeyboard().IsKeyDown(Key::Space))
	{
		//Gameシーンへの画面遷移
		SceneManager::Get().Load<TestScene>();
	}

	//escで終了
	if (input.GetKeyboard().IsKeyDown(Key::Escape))
	{
		PostQuitMessage(0);
	}
}

//描画
void Title_Scene::Render()
{

}