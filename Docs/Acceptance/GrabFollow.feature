Feature: Moving actor grab prototype
  The player can hold onto the current encounter target without changing the existing combat balance.

  Background:
    Given bUseRouteClimbing is false for the generic Grab fixture

  Scenario: Hold E while the boss translates and rotates
    Given the player is within grab distance of Ishibashiri
    When the Grab input binding receives an E press
    Then the player is grabbing Ishibashiri
    And normal CharacterMovement is disabled
    When Ishibashiri translates and rotates
    Then the player's transform relative to Ishibashiri remains stable

  Scenario: Follow movement produced by the real boss AI
    Given the player is grabbing Ishibashiri
    And the held offset is outside Ishibashiri's chase stopping distance
    When Ishibashiri's Chase Tick moves the boss
    Then the boss changes world location
    And the player's transform relative to Ishibashiri remains stable
    And the player's world location contains no NaN values

  Scenario: Release a grab
    Given the player is grabbing Ishibashiri
    When the Grab input binding receives an E release
    Then the grab target is cleared
    And CharacterMovement resumes in falling or walking mode

  Scenario: Retry while grabbing
    Given the player is grabbing Ishibashiri
    When the Retry input binding receives an R press
    Then the grab target is cleared
    And movement and gravity are restored
    And the player is not pulled toward the old boss transform
