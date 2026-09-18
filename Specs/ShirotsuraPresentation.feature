Feature: Shared Shirotsura presentation

  Scenario: Story card text respects its viewport-relative region
    Given a story card has Japanese, short English, or empty body text
    When its lines are measured at the current draw scale
    Then measuring sample text does not replace the viewport-relative wrap width
    And the body region ends above the continue prompt

  Scenario: Every encounter presents the existing Shirotsura rig
    Given an encounter player retains its own movement and combat rules
    When its shared presentation is initialized
    Then the rigged Shirotsura mesh and matching animation clips are selected
    And the primitive fallback is visible only when the rig is unavailable
    And retry returns presentation to an idle state
