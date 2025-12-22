#pragma once
#include "engine/scene/scene.h"
#include "engine/component/game_object.h"
#include "engine/component/camera2d.h"
#include <memory>


class Result_Scene : public Scene
{
//プライベート
private:
	//コンポーネント呼び出し
	//カメラ
	std::unique_ptr<Camera2D> camera_;


//パブリック
public:
	//ライフサイクル
	Result_Scene() = default;
	~Result_Scene() override = default;
	//初期化
	void OnEnter() override;
	//破棄
	void OnExit() override;

	//フレームコールバック
	//更新
	void Update() override;
	//描画
	void Render() override;

};