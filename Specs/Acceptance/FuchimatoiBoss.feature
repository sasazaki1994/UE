Feature: Fuchimatoi boss action lifecycle
  Fuchimatoi owns a minimal action state machine while shared Nushi state remains on NushiBase.

  Scenario: Fuchimatoi performs its basic attack cycle
    Given Fuchimatoi is Submerged
    When the bite windup begins
    Then Fuchimatoi should be BiteWindup
    When the bite lunge begins
    Then Fuchimatoi should be BiteLunge
    When its head becomes snagged
    Then Fuchimatoi should be Snagged
    When it begins coiling around terrain
    Then Fuchimatoi should be Coiling
    When the cycle finishes
    Then Fuchimatoi should return to Submerged

  Scenario Outline: Fuchimatoi rejects an illegal transition
    Given Fuchimatoi is <initial state>
    When <transition> is requested
    Then Fuchimatoi should remain <initial state>
    And no action state change should be broadcast

    Examples:
      | initial state | transition             |
      | Submerged     | coiling begins         |
      | Submerged     | head becomes snagged   |
      | BiteWindup    | head becomes snagged   |
      | Snagged       | bite lunge begins      |

  Scenario: Duplicate transitions do not emit duplicate notifications
    Given Fuchimatoi is Submerged
    When the bite windup begins twice
    Then Fuchimatoi should be BiteWindup
    And one action state change should be broadcast

  Scenario: Reset Fuchimatoi during its action cycle
    Given Fuchimatoi is Coiling and its Nushi state is Active
    When Fuchimatoi is reset
    Then Fuchimatoi should be Submerged
    And its Nushi state should be Dormant

  Scenario: Fuchimatoi can replay its action cycle after reset
    Given Fuchimatoi was reset while Coiling
    When the bite windup begins
    And the bite lunge begins
    And its head becomes snagged
    And it begins coiling around terrain
    Then Fuchimatoi should be Coiling
