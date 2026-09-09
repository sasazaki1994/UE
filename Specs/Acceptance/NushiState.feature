Feature: Generic Nushi state progression
  A reusable Nushi state remains separate from boss AI and Kakon progress tracking.

  Scenario: Calm a Nushi by purifying all Kakon
    Given a dormant Nushi with three registered Kakon
    When the encounter starts
    Then the Nushi should become active
    When the three Kakon are exposed and purified
    Then the Nushi should become calm

  Scenario: Prevent starting an encounter twice
    Given a dormant Nushi with three registered Kakon
    When the encounter is started twice
    Then the Nushi should be active
    And the state changed notification should have been emitted once

  Scenario: Ignore full purification before the encounter
    Given a dormant Nushi with three registered Kakon
    When the three Kakon are exposed and purified
    Then the Nushi should remain dormant
    And no calm state changed notification should be emitted

  Scenario: Keep the Nushi active while purification is incomplete
    Given an active Nushi with three registered Kakon
    When two Kakon are exposed and purified
    Then the purification progress should be 2 of 3
    And the Nushi should remain active

  Scenario: Become calm only when every Kakon is purified
    Given an active Nushi with two of three Kakon purified
    When the final Kakon is exposed and purified
    Then the purification progress should be 3 of 3
    And the Nushi should become calm
    And the calm state changed notification should be emitted once

  Scenario: Ignore duplicate purification after becoming calm
    Given a calm Nushi with three purified Kakon
    When purification is attempted again on a purified Kakon
    Then the Nushi should remain calm
    And the calm state changed notification should still have been emitted once

  Scenario: Reset a calm Nushi
    Given a calm Nushi with three purified Kakon
    When the Nushi is reset
    Then the Nushi should become dormant
    And the purification progress should be 0 of 3
    And every registered Kakon should be Covered

  Scenario: Complete another encounter after reset
    Given a calm Nushi with three purified Kakon
    When the Nushi is reset
    And the encounter starts
    And the three Kakon are exposed and purified again
    Then the Nushi should become calm
    And the purification progress should be 3 of 3
