Feature: Ishibashiri production visual slice
  The proven encounter remains playable while presentation communicates an old mountain sanctuary.

  Scenario: Normal play uses diegetic sense feedback
    Given DebugGuidance is disabled
    When Boundary Sense is held toward the current Kakon
    Then the boundary blade response strengthens without drawing a target laser
    And the corrupted left arm remains a separate danger response

  Scenario Outline: Kakon presentation follows lifecycle state
    Given a Kakon is <state>
    Then its presentation is <appearance>
    Examples:
      | state     | appearance                         |
      | Covered   | dark veining without a marker glow |
      | Exposed   | restrained red-black pulse         |
      | Purified  | unlit and returned to the body      |

  Scenario: Calm is not death
    Given all three Ishibashiri Kakon are purified
    Then Ishibashiri uses the Calmed animation
    And every Kakon marker is extinguished
    And the basin fog eases without an explosion or disappearance

  Scenario: Debug authoring remains available
    Given DebugGuidance is enabled
    Then route and Kakon authoring guidance retains high contrast
    And normal play omits internal state and telemetry text

  Scenario: Gameplay contract is unchanged
    Then the eleven route coordinates and neighbors are unchanged
    And Grab, Climb, Cling, Stamina, three Kakon, Retry and camera behavior remain gameplay-owned
