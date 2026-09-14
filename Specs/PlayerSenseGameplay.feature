Feature: Player-driven sensing
  Scenario Outline: Boundary direction responds to facing
    Given an available unpurified Kakon is <position>
    When the player holds Boundary Sense
    Then the reading strength is <strength>
    Examples:
      | position | strength |
      | ahead    | stronger |
      | right    | medium   |
      | behind   | weak     |

  Scenario: Locked targets do not guide the player
    Given a Kakon or route is unavailable in the current phase
    When the player holds Boundary Sense
    Then that target is excluded from the reading

  Scenario Outline: Corruption warnings retain encounter meaning
    Given the encounter reports <state>
    When the player holds Corruption Sense
    Then the warning is <warning>
    Examples:
      | state      | warning    |
      | Shake      | DANGER     |
      | Pulse      | DANGER     |
      | Transition | TRANSITION |

  Scenario: Corruption Sense has one temporary risk
    When Corruption Sense is held and released
    Then stamina recovery is 50 percent while held and for 2 seconds after release
    And maximum stamina and health are unchanged

  Scenario: Retry clears sensing
    When the encounter is retried
    Then both senses, the risk timer, warning, and cached target are cleared
