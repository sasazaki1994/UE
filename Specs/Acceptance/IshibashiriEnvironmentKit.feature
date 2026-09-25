# language: ja
機能: 石走り編の最小Production Environment Kit
  Primitive置換は既存Gameplayを動かさず、最初の3点を実機評価してから量産する。

  シナリオ: First Adoption Batchだけを先に評価する
    前提 8点すべてのTripo制作仕様がspec_onlyとして登録されている
    もし OldCedar_A、Rock_A、BoundaryStone_Aを順に生成する
    ならば Blender cleanupとUE Importを行う
    かつ Approachへ仮配置して同一カメラのBeforeとAfterを比較する
    かつ 3点が受入条件を満たすまで残り5点を量産しない

  シナリオ: 環境置換はGameplay契約を維持する
    前提 ApproachとBasinの既存Navigation、Grab、Climbing、回避空間が成立している
    もし Production Environment Assetを仮配置する
    ならば Approach距離とPath pointを変更しない
    かつ Grab入口とClimbing routeを塞がない
    かつ Ishibashiri戦の回避空間を狭めない
    かつ Collision gameplay contractを変更しない

  シナリオ: 痕跡は3D生成対象にしない
    ならば 巨大足跡、穢れの亀裂、泥、苔変化、擦過痕をDecalまたはMaterial variationとして扱う
    かつ 存在しないPNG、FBX、GLBを登録しない

  シナリオ: 実機証拠がない状態を成功扱いしない
    前提 Windows UE 5.6.1、Blender、Tripoをこの工程で実行していない
    ならば UE Import、Visual Review、Before/After CaptureをNOT_RUNとして記録する

  シナリオ: Tripo未契約でもFirst Adoption Batchだけを生成できる
    前提 manifestのFirst Adoption Batchが3点に限定されている
    もし Blender procedural fallbackを同じversionとcommitで実行する
    ならば 固定seedでOldCedar_A、Rock_A、BoundaryStone_Aだけを生成する
    かつ 外部texture、network、有料AI、外部3D API、Marketplace素材を使用しない
    かつ Tripo generation statusをNOT_RUNのまま維持する

  シナリオ: 未生成とUE未検証を成功扱いしない
    前提 Blender executableが利用できず生成物が存在しない
    ならば validatorはNOT_GENERATEDを返す
    かつ 空のFBX、GLB、偽previewを作らない
    かつ UE ImportとUE Visual ReviewをNOT_RUNとして記録する
