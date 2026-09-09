Feature: Generic stamina resource
  A reusable player component manages only a bounded numeric stamina resource.

  Scenario: Consume stamina
    Given stamina is full at 100
    When 25 stamina is consumed
    Then current stamina should be 75
    And stamina changed should be emitted exactly once

  Scenario: Clamp stamina at zero
    Given current stamina is 10
    When 30 stamina is consumed
    Then current stamina should be 0

  Scenario: Ignore a negative consumption
    Given current stamina is 50
    When -10 stamina is consumed
    Then current stamina should remain 50
    And stamina changed should not be emitted

  Scenario: Restore stamina without exceeding the maximum
    Given current stamina is 90
    When 30 stamina is restored
    Then current stamina should be 100

  Scenario: Ignore a negative restoration
    Given current stamina is 40
    When -10 stamina is restored
    Then current stamina should remain 40
    And stamina changed should not be emitted

  Scenario: Do not notify when restoration is clamped to the current value
    Given stamina is full at 100
    When 20 stamina is restored
    Then current stamina should remain 100
    And stamina changed should not be emitted

  Scenario: Stamina reaches zero
    Given current stamina is 20
    When 30 stamina is consumed
    Then current stamina should be 0
    And stamina depleted should be emitted exactly once

  Scenario: Prevent duplicate depleted notifications
    Given stamina is depleted
    When 10 stamina is consumed
    Then current stamina should remain 0
    And stamina depleted should not be emitted again

  Scenario: Recover after depletion
    Given stamina is depleted
    When 50 stamina is restored
    And 50 stamina is consumed
    Then current stamina should be 0
    And stamina depleted should be emitted again

  Scenario: Refill stamina
    Given current stamina is 30
    When stamina is refilled
    Then current stamina should equal maximum stamina

  Scenario: Reset stamina
    Given current stamina is 15
    When stamina is reset
    Then current stamina should equal maximum stamina
