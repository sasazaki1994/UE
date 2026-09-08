Feature: Generic PC gamepad controls
  The existing prototype gameplay remains available from an XInput-compatible
  controller without replacing keyboard and mouse controls.

  Background:
    Given the player is controlled by the local PlayerController
    And generic UE Gamepad_* mappings are active

  Scenario: Analog left stick controls movement and stops at zero
    When Gamepad Left Y is held at 1.0
    Then the player moves forward
    When Gamepad Left X is held at 1.0
    Then the player moves right
    When both axes return to 0.0
    Then no movement input remains
    And an axis value at or below the 0.20 dead zone does not move the player

  Scenario: Analog right stick controls camera at a rate
    When Gamepad Right X is held right
    Then camera yaw increases independently of frame rate
    When Gamepad Right Y is held up
    Then camera pitch looks upward independently of frame rate
    And mouse delta camera input remains mapped

  Scenario Outline: Face buttons share existing gameplay actions
    When I press <button>
    Then the existing <action> starts
    And its combat restrictions remain unchanged

    Examples:
      | button | action |
      | A      | Jump   |
      | B      | Dodge  |
      | X      | Attack |

  Scenario: RB is the same hold-to-grab action as E
    Given the player is within grab distance of Ishibashiri
    When RB is pressed
    Then the existing BeginGrab path starts grab
    When RB is released
    Then the existing ReleaseGrab path immediately restores movement

  Scenario: Left stick climbs through the shared movement axes
    Given RB is held and the player is grabbing Ishibashiri
    When Left Y is held at 0.25, 0.5, and 1.0
    Then RelativeGrabTransform moves upward proportionally to axis strength
    When Left X is held right
    Then RelativeGrabTransform moves in target-local right
    And following remains valid while Ishibashiri moves and rotates
    And no gamepad-only climbing implementation is used

  Scenario: Y retries during gamepad climbing
    Given RB is held and the player is climbing
    When Y is pressed
    Then grab and climbing end
    And player and boss transforms reset
    And CharacterMovement is restored

  Scenario: Keyboard and mouse mappings remain available
    Then WASD and Mouse still control movement and camera
    And Shift and Right Mouse Button still dodge
    And Left Mouse Button still attacks
    And Space still jumps
    And E still holds grab and releases grab
    And R still retries
