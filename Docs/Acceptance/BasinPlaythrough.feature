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
    Then stamina increases while the player remains stopped on that ledge
    When the Jump input binding receives Space
    Then the player first enters falling movement
    And the player subsequently lands on the basin floor
    And ground movement and dodge input work after landing
    When the Retry input binding receives R
    Then the encounter returns to the basin player spawn with full climbing stamina
    And no climbing attachment remains
    And walking movement is enabled
    And a second observed approach can attach at the normal range

  Scenario: Scenario diagnostics and cleanup
    When any stage exceeds its own timeout
    Then the failure log includes RunId, player position, boss position, entry distance, boss state, climbing state, node, and movement mode
    And every held simulated input is released
    When the scenario succeeds
    Then every held simulated input is released
    And the process reports a RunId-specific pass marker and exit code 0
