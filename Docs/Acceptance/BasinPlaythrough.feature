Feature: Basin traversal from the normal encounter start
  The playable basin loop is verified without arranging the actors or disabling combat AI.

  Background:
    Given the game starts with BasinPrototype and BasinPlaythroughTest enabled
    And the player and Ishibashiri use their normal basin spawn transforms
    And Ishibashiri's normal AI Tick remains enabled

  Scenario: Approach, climb, land, and retry through game input
    When the test observes the moving foreleg hold and steers with camera and movement input
    And it uses dodge input in response to a telegraph or charge
    And the Grab input binding receives E only inside the normal grab range
    Then route climbing starts at node 0
    When forward input climbs to the first rest ledge
    Then the player remains stopped at the rest node while normal ledge recovery runs
    And stamina increases toward its cap when below full, or remains full when already at its cap
    When the Jump input binding receives Space
    Then the player first enters falling movement
    And the player subsequently enters walking mode on the BasinFloor, with the capsule bottom at its surface
    And ordinary ground movement succeeds before dodge input is sent
    And dodge enters its action state while the player is grounded
    When the Retry input binding receives R
    Then synchronously at input dispatch the player and Ishibashiri have their basin-configured start transforms within 0.1 cm and degree
    And later approach checks allow Ishibashiri's normal AI movement instead of comparing it to the start transform
    And the player has full health and no dodge, attack, Grab, or climbing action
    And Ishibashiri has full health, no purified cores, and the initial chase action
    And the encounter returns to the basin player spawn with full climbing stamina
    And no climbing attachment remains
    And walking movement is enabled
    And no climbing grip or movement input remains
    And terrain, decoration, fog, and lighting component counts are unchanged
    And a second observed approach can attach at the normal range

  Scenario: Scenario diagnostics and cleanup
    When any stage exceeds its own timeout
    Then the failure log includes RunId, player position, boss position, entry distance, boss state, climbing state, node, and movement mode
    And every held simulated input is released
    And fixed-timestep settings are restored
    When the scenario succeeds
    Then every held simulated input is released
    And fixed-timestep settings are restored
    And the process reports a RunId-specific pass marker and exit code 0
