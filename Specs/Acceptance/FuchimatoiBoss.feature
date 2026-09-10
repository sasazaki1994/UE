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

  Scenario: Bite lunge moves the head proxy toward its target
    Given Fuchimatoi has a bite target
    And Fuchimatoi is in BiteLunge
    When bite movement is advanced
    Then the head proxy should move toward the bite target
    And it should not overshoot the target

  Scenario: Reaching the bite target does not automatically snag the head
    Given Fuchimatoi reaches its bite target
    Then Fuchimatoi should remain in BiteLunge
    When a head snag is reported
    Then Fuchimatoi should become Snagged

  Scenario: Bite movement stops after snagging
    Given Fuchimatoi is Snagged
    When bite movement is advanced
    Then the head proxy should not move

  Scenario: Coiling progresses after the head is snagged
    Given Fuchimatoi is Snagged with a coiling duration of 2 seconds
    When it begins coiling
    Then coiling progress should be 0.0
    And coiling should be incomplete
    When coiling is explicitly advanced by 0.5 seconds
    Then coiling progress should be 0.25
    When coiling is explicitly advanced by another 0.5 seconds
    Then coiling progress should be 0.5
    And actor tick should remain disabled

  Scenario: Coiling progress never overshoots
    Given Fuchimatoi is Coiling with a coiling duration of 2 seconds
    When coiling is explicitly advanced by 10 seconds
    Then coiling progress should be exactly 1.0
    And coiling should be complete
    When coiling is explicitly advanced again
    Then coiling progress should remain exactly 1.0

  Scenario: Completing coiling does not automatically change action state
    Given Fuchimatoi is Coiling
    When coiling progress reaches 1.0
    Then Fuchimatoi should remain Coiling
    And no action state change should be broadcast
    When ReturnToSubmerged is explicitly requested
    Then Fuchimatoi should be Submerged

  Scenario Outline: Coiling does not progress outside the Coiling state
    Given Fuchimatoi is <state>
    When coiling is explicitly advanced by 1 second
    Then coiling progress should remain unchanged

    Examples:
      | state      |
      | Submerged  |
      | BiteWindup |
      | BiteLunge  |
      | Snagged    |

  Scenario: Reset clears coiling progress
    Given Fuchimatoi is Coiling with nonzero progress and populated bite data
    And its Nushi state is Active
    When Fuchimatoi is reset
    Then coiling progress should be 0.0
    And coiling should be incomplete
    And Fuchimatoi should be Submerged
    And its Nushi state should be Dormant
    And the head proxy and bite target should be reset to the local origin
    And the bite target should be unset
    When the bite windup begins
    And the bite lunge begins
    And its head becomes snagged
    And it begins coiling
    Then coiling progress should start at 0.0 and be able to advance again

  Scenario: Duplicate coiling requests preserve progress
    Given Fuchimatoi is Coiling with progress of 0.25
    When it begins coiling again
    Then coiling progress should remain 0.25
    And no action state change should be broadcast

  Scenario: A new coiling cycle clears previous progress without reset
    Given Fuchimatoi has completed coiling
    When ReturnToSubmerged is explicitly requested
    And the bite windup begins
    And the bite lunge begins
    And its head becomes snagged
    And it begins coiling
    Then coiling progress should be 0.0
    And coiling should be incomplete

  Scenario Outline: Invalid coiling inputs preserve finite progress
    Given Fuchimatoi is Coiling with progress of <progress>
    When coiling is advanced with <input> set to <value> and the other input positive and finite
    Then coiling progress should remain <progress>
    And Fuchimatoi should remain Coiling

    Examples:
      | progress | input           | value             |
      | 0.25     | DeltaSeconds    | 0                 |
      | 0.25     | DeltaSeconds    | -1                |
      | 0.25     | DeltaSeconds    | NaN               |
      | 0.25     | DeltaSeconds    | positive Infinity |
      | 0.25     | DeltaSeconds    | negative Infinity |
      | 0.25     | CoilingDuration | 0                 |
      | 0.25     | CoilingDuration | -1                |
      | 0.25     | CoilingDuration | NaN               |
      | 0.25     | CoilingDuration | positive Infinity |
      | 0.25     | CoilingDuration | negative Infinity |
      | 1.0      | DeltaSeconds    | NaN               |
      | 1.0      | CoilingDuration | 0                 |
