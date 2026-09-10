Feature: Authored colossus route with keyboard and gamepad input
  The default encounter uses the rigged Shirotsura and Ishibashiri models.

  Background:
    Given bUseRouteClimbing is true
    And both deformation skeletons and animations are loaded

  Scenario Outline: Purify the three distinct cores
    Given the player approaches the gold foreleg hold through movement input
    When the player presses <grab>
    Then the player mounts the route and normal CharacterMovement is disabled
    When the player climbs using <move> and braces with <grab> during shaking
    Then the player follows boss translation and rotation
    And resting on a ledge restores stamina
    When the player traverses the shoulder branch, summit and back
    And attacks each core with <attack>
    Then all three distinct cores are purified and the encounter ends in victory
    And attacking an already purified core does not add damage
    When the player presses <retry>
    Then attachment, stamina, core state and encounter state reset

    Examples:
      | grab | move       | attack | retry |
      | E    | WASD       | LMB    | R     |
      | RB   | Left Stick | X      | Y     |

  Scenario: Release brace without detaching
    Given the player is on the route
    When E or RB is released
    Then the player remains attached but no longer braces
    When the boss shakes without bracing
    Then the player falls before stamina is exhausted

  Scenario Outline: Detach and restore falling movement
    Given the player is on the route
    When <trigger>
    Then the player leaves the route and CharacterMovement resumes falling

    Examples:
      | trigger              |
      | Space or A is pressed |
      | stamina reaches zero |
