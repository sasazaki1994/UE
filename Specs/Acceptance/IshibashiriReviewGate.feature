# language: ja
機能: STEP 4B 石走り First Playable Review Gate
  石走りを Vertical Slice として提示可能と判定するには、source contract ではなく
  Windows Unreal Engine 5.6.1 の実行証跡が必要である。

  シナリオ: Windows runtime がない環境では PASS にしない
    前提 Windows Unreal Engine 5.6.1 を実行できない
    もし source test だけが成功した
    なら Review Gate の判定は "UNVERIFIED — WINDOWS UE RUNTIME NOT AVAILABLE" である
    かつ Build、実プレイ、描画、IK、性能、Package を PASS と記録しない

  シナリオ: PASS 判定には一連の実プレイ証跡が必要である
    前提 Windows Unreal Engine 5.6.1 の Development Editor Build が成功している
    かつ 30 FPS と 60 FPS で Player 入力により Approach を完走している
    かつ Approach から石走り Encounter へ遷移している
    かつ Player 入力により Kakon 3/3、Calm、Victory へ到達している
    かつ Retry が Approach を再生せず Encounter を初期化している
    かつ Approach の Sense state、target、timer、world pointer が Encounter に残っていない
    かつ Climbing Control Rig asset が保存され node 0 から node 3 で動作している
    かつ Legacy と HighQuality の必須画像を実描画で確認している
    かつ HighQuality log で D3D12、SM6、Lumen GI、Lumen Reflections、VSM を確認している
    なら Review Gate の判定を "PASS — ISHIBASHIRI FIRST PLAYABLE VALIDATED" にできる

  シナリオ: Control Rig の数値だけでは見た目を合格にしない
    前提 同じ条件の IK 無効時と IK 有効時の画像がある
    もし Grab、前脚、手足接触、肩、Boss 移動、Shake と Cling を比較する
    なら通常接触で 20 cm 超の誤差が長時間継続していない
    かつ画像上で大きな浮き、貫通、関節反転がない

  シナリオ: Package は主要 Gate の後だけ実行する
    前提 Build、Approach、Encounter、Retry、Sense、IK、Legacy、HighQuality の主要 Gate が成功している
    なら Package を作成する
    かつ Package 版で Title、Prologue、Approach 開始、Player 移動、Encounter 開始を確認する
