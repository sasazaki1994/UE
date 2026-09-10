# 白面・石走り 外観改良

## 造形方針

参考画像は方向性として扱い、既存の寸法、骨格、基準姿勢、15クリップ、石走りの3禍根座標とゲーム側の11登攀地点を優先する。白面は「祭祀を背負って歩く祓い手」として、古い白木、藍染、未漂白麻、短い藁と少量の朱へ整理した。石走りは「長く祀られてきた、歩く山」として、皮膚と肩岩を根・遷移石で接続し、岩棚を塞がず、注連縄の紙垂に退色と不揃いな垂れを与える。

白面には木面の浅い手斧跡と小欠け、藍布の当て布と縫い目、肘より上で終わる短い肩掛けを追加した。左腕の寸法は変更せず、黒根を主体に発光線を細く限定した。石走りの禍根は座標と個数を固定し、7本の針状結晶を3本の欠けた鉱物歯へ減らし、黒根の瘤を主役にした。顔、蹄、牙、棚の主要寸法は変更していない。

## 変更前の基準値

コミット済み `model-info.json` を改修前基準として使用する。白面は174メッシュ / 74,626三角形 / 1.306 × 0.331 × 1.717 m、石走りは537メッシュ / 205,172三角形 / 7.063 × 12.179 × 9.398 m。リグ済み検証記録は白面10クリップ、石走り5クリップで合計15クリップを示す。テクスチャは既存どおり BaseColor / Normal 各2048 pxである。

## 再生成とUE取り込み

Blender 3.6系とUE 5.6.1を使う（既存検証環境と同じ）。作業前のファイルを別名で複製して比較用に保持し、リポジトリルートで次を順に実行する。

```bash
blender --background --factory-startup --python Tools/CreateCharacterModels.py
blender --background --factory-startup --python Tools/RefineCharacterModels.py
blender --background --factory-startup --python Tools/RigCharacterModels.py
blender --background --factory-startup --python Tools/VerifyRiggedCharacters.py
UnrealEditor-Cmd IshibashiriPrototype.uproject -run=pythonscript -script=Tools/ImportRiggedCharacters.py -unattended
UnrealEditor-Cmd IshibashiriPrototype.uproject -run=pythonscript -script=Tools/ApplyRiggedMaterials.py -unattended
UnrealEditor-Cmd IshibashiriPrototype.uproject -run=pythonscript -script=Tools/VerifyRiggedCharacters.py -unattended
```

生成は既存 `.blend` / `.fbx` / `.glb`、リグ済みFBX、2048 px atlas、previewを更新する。UE取り込みは実プレイ参照先 `/Game/Characters/Rigged/{Shirotsura,Ishibashiri}` を置換保存する。マテリアル適用は既存式を型と序数で再利用し、素材名から布・縄 `.96`、岩 `.91`、生体 `.78`、刃 `.28` の粗さを設定するため、再実行でノードを増やさない。

## 描画確認

変更前と変更後で `STUDIO_Camera` と3灯を変えず、白面の正面・背面・Climb、石走りの斜め前・背面・背中近景を保存する。肩掛けと上腕の食い込み、岩棚上面、3禍根の通常距離での判別、紙垂の不均一さを確認する。問題があれば造形生成値だけを直し、骨・座標・ゲーム側コリジョンは動かさない。

## 今回の検証結果と未確認事項

このLinuxコンテナにはBlenderとUnrealEditorが無いため、今回は制作・リグ・取り込みスクリプトの構文およびソース契約検査までを実施した。したがって、生成済みバイナリアセットやpreviewを更新しておらず、変更後比較画像も作成していない。既存の手修正バイナリを保護するため、利用不能なツールの生成結果を模造していない。

Blender環境では、再生成後の三角形数・寸法を上記基準と比較し、非多様体、反転法線、未ウェイト頂点、15クリップの変形を確認する。UE環境では上記取り込み後、`Docs/ClimbingValidation.md` の描画あり60 FPS試験を再実行し、取り付き、分岐、3浄化、勝利、再挑戦に加え、肩掛け干渉、足場ずれ、禍根視認性を画像で判定する。これらは未実施であり、PRはDraftとして扱う。
