Feature: Generic Nushi encounter lifecycle
  One encounter manager coordinates one Nushi without owning player or stage systems.

  Scenario: Start an encounter
    Given an idle encounter with a dormant Nushi
    When the encounter starts
    Then the encounter should be Running
    And the Nushi should be Active

  Scenario: Ignore a duplicate start
    Given a running encounter
    When the encounter is started again
    Then the encounter should remain Running
    And the started notification should have been emitted once

  Scenario: Safely ignore start without a Nushi
    Given an idle encounter with a nullptr Nushi
    When the encounter starts
    Then the encounter should remain Idle
    And no started notification should be emitted

  Scenario: Keep a partially purified encounter running
    Given a running encounter with three registered Kakon
    When two Kakon are exposed and purified
    Then purification progress should be 2 of 3
    And the Nushi should remain Active
    And the encounter should remain Running

  Scenario: Complete an encounter
    Given a running encounter with three registered Kakon
    When all Kakon are exposed and purified
    Then the Nushi should be Calm
    And the encounter should be Completed

  Scenario: Ignore duplicate completion
    Given a completed encounter with three purified Kakon
    When purification is attempted again on a purified Kakon
    Then the encounter should remain Completed
    And the completed notification should have been emitted once

  Scenario: Reset a completed encounter
    Given a completed encounter
    When the encounter is reset
    Then the encounter should be Idle
    And the Nushi should be Dormant
    And purification progress should be zero
    And every registered Kakon should be Covered

  Scenario: Replay an encounter
    Given a completed encounter has been reset
    When the encounter starts again
    And all Kakon are exposed and purified
    Then the encounter should complete again
    And the completed notification should be emitted once for the new cycle
