Feature: 石走り編デモの通常プレイ HUD
  通常プレイでは身体表現を優先し、必要な場面にだけ操作と生存情報を示す。

  Scenario: 地上の通常状態では常設パネルを表示しない
    Given 石走りとの戦闘が進行中である
    And プレイヤーは地上にいて Sense を使用していない
    When HUD を描画する
    Then タイトル、数値 STAMINA、常設 KAKON 行を表示しない

  Scenario Outline: 初回操作案内は該当場面に一枚だけ表示する
    Given Button prompts が Contextual である
    And <action> をまだ成功させていない
    When <context> になる
    Then <prompt> を表示する
    And 他の操作案内を同時に表示しない
    And ゲーム進行と入力を停止しない

    Examples:
      | action     | context                      | prompt          |
      | 回避       | 最初の Telegraph             | Shift / B 回避  |
      | 反撃       | 最初の Recover               | LMB / X 反撃    |
      | 取り付き   | Kneel 中かつ 4m 以内         | E / RB 取り付く |
      | しがみつき | 最初の shake warning         | E / RB 長押し   |
      | 浄化       | 未浄化の禍根が届く安全な足場 | LMB / X 浄化    |

  Scenario: 浄化直後だけ進捗を表示する
    Given 常設の禍根カウンターは表示されていない
    When 禍根を一つ浄化する
    Then 「禍根 1/3」を 1.5 秒表示する
    And 次の浄化までカウンターを隠す

  Scenario: 登攀中のスタミナは量として表示する
    Given プレイヤーが石走りに取り付いている
    When スタミナが 90 未満または shake の危険がある
    Then 数字を使わないスタミナバーを表示する
    And 地上の安全な全快状態ではスタミナバーを表示しない

  Scenario: 鎮静と敗北を区別する
    When 石走りが生きたまま鎮まる
    Then 「石走りは鎮まった / Encounter Completed」を表示する
    And 「VICTORY」と Retry 案内を表示しない
    When プレイヤーが敗北する
    Then 「DEFEAT」と Retry 案内を表示する

  Scenario: DebugGuidance は内部値を独立して表示する
    Given -DebugGuidance を指定している
    When HUD を描画する
    Then DEBUG と明示したパネルに HP、体勢、状態時間、スタミナ数値、ルート、shake、浄化数を表示する
    And Debug 表示は入力可否、戦闘進行、保存を変更しない
