#include "result_scene.h"
#include "title_scene.h"
#include "engine/scene/scene_manager.h"
#include "engine/input/input_manager.h"
#include "engine/input/key.h"
#include "engine/c_systems/sprite_batch.h"
#include "engine/debug/debug_draw.h"
#include "common/logging/logging.h"


//ライフサイクル
//初期化
void Result_Scene::OnEnter()
{
	LOG_INFO("現在のシーン : リザルト");
	camera_ = std::make_unique<Camera2D>();
	//画面中央
	camera_->SetPosition(Vector2(640.0f, 360.0f));
}
//破棄
void Result_Scene::OnExit()
{
	camera_.reset();
}

//フレームコールバック
//更新
void Result_Scene::Update()
{
	auto& input = *InputManager::GetInstance();

	//space or enterがおされたら
	if (input.GetKeyboard().IsKeyDown(Key::Enter))
	{
		//Titleシーンへの画面遷移
		SceneManager::Get().Load<Title_Scene>();
	}
}

//描画
void Result_Scene::Render()
{

}