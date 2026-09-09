Feature: Minimal target-local climbing while grabbed
  The player can change the held offset without moving the grabbed boss or enabling normal movement.

  Background:
    Given bUseRouteClimbing is false for the generic Grab fixture
    And the player is within grab distance of Ishibashiri
    And the Grab input binding receives an E press
    And CharacterMovement is disabled

  Scenario: Climb up and down
    When W is held for a fixed duration
    Then RelativeGrabTransform local Z increases
    And the player world transform follows the changed relative transform
    And the grab remains active
    When S is held for the same duration
    Then RelativeGrabTransform local Z changes in the opposite direction

  Scenario: Climb left and right
    When A is held for a fixed duration
    Then RelativeGrabTransform local Y decreases
    When D is held for the same duration
    Then RelativeGrabTransform local Y changes in the opposite direction

  Scenario: Climb after the boss rotates
    When Ishibashiri rotates in world space
    And W is held
    Then RelativeGrabTransform local Z increases
    And the player equals RelativeGrabTransform composed with the boss world transform
    And the player transform contains no NaN values

  Scenario: Clamp the climbing offset
    When climbing input would move the held offset outside its configured local box
    Then local X, Y, and Z remain within their configured minimum and maximum

  Scenario: Climb while the real boss AI moves
    Given the held local X offset is outside Ishibashiri's chase stopping distance
    When Ishibashiri's real Chase Tick runs
    And a climbing direction is held
    Then Ishibashiri changes world location
    And the climbing local offset changes
    And the player follows Ishibashiri without losing the grab or producing NaN values

  Scenario: Release after climbing
    When the Grab input binding receives an E release
    Then the grab target is cleared
    And CharacterMovement resumes in falling or walking mode
    And RelativeGrabTransform stops changing

  Scenario: Retry while climbing
    When the Retry input binding receives an R press
    Then the grab target is cleared
    And climbing stops
    And the player and boss return to their encounter spawn transforms
    And CharacterMovement resumes
    And the old RelativeGrabTransform does not pull the player
