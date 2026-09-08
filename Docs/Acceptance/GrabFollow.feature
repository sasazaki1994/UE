Feature: Moving actor grab prototype
  The player can hold onto the current encounter target without changing the existing combat balance.

  Scenario: Hold E while the boss translates and rotates
    Given the player is within grab distance of Ishibashiri
    When the Grab input binding receives an E press
    Then the player is grabbing Ishibashiri
    And normal CharacterMovement is disabled
    When Ishibashiri translates and rotates
    Then the player's transform relative to Ishibashiri remains stable

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
