Feature: Common Nushi base actor
  Every Nushi shares the same minimal progression and state facade without boss-specific behavior.

  Scenario: A Nushi contains the common progression components
    Given a NushiBase actor exists
    Then it should contain a NushiProgressComponent
    And it should contain a NushiStateComponent
    And its initial state should be Dormant

  Scenario: Register unique Kakon through the common base
    Given a NushiBase actor exists
    When three different Kakon are registered through the NushiBase
    And one of those Kakon is registered again
    Then the registered Kakon count should remain 3

  Scenario: Keep an active Nushi active during partial purification
    Given a NushiBase with three registered Kakon
    When the encounter starts
    And two Kakon are exposed and purified
    Then its purification progress should be 2 of 3
    And the Nushi should remain Active

  Scenario: Calm a Nushi through its common base
    Given a NushiBase with three registered Kakon
    When the encounter starts
    And all three Kakon are exposed and purified
    Then its purification progress should be 3 of 3
    And the Nushi should become Calm

  Scenario: Reset a calmed Nushi
    Given a NushiBase is Calm with three purified Kakon
    When the Nushi is reset
    Then the Nushi should become Dormant
    And its purification progress should be zero
    And its Kakon should return to Covered

  Scenario: Complete another encounter after reset
    Given a calmed NushiBase with three purified Kakon is reset
    When the encounter starts again
    And all three Kakon are exposed and purified again
    Then its purification progress should be 3 of 3
    And the Nushi should become Calm again
