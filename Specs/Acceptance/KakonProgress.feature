Feature: Generic Kakon purification progress
  Kakon purification can be reused by every Nushi without boss-specific behavior.

  Scenario: Covered Kakon cannot be purified
    Given a Kakon is Covered
    When purification is attempted
    Then purification should fail
    And the Kakon should remain Covered

  Scenario: Purify three Kakon
    Given three Kakon are registered
    When each Kakon shell is destroyed and each Kakon is purified
    Then the purified count should become 3 of 3
    And all purified should be emitted exactly once

  Scenario: Prevent duplicate purification progress
    Given three registered Kakon have been purified
    When purification is attempted again on a purified Kakon
    Then the purified count should remain 3 of 3
    And all purified should still have been emitted exactly once

  Scenario: Reset purification progress
    Given three registered Kakon have been purified
    When the purification progress is reset
    Then the purified count should become 0 of 3
    And every registered Kakon should be Covered with full shell health

  Scenario: Purify all Kakon again after reset
    Given three purified registered Kakon have been reset
    When each Kakon shell is destroyed and each Kakon is purified again
    Then the purified count should become 3 of 3
    And all purified should be emitted once in the new cycle
