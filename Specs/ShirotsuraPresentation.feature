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

  Scenario Outline: The campaign chapter is the authority for the two appearances
    Given the campaign is in <chapter>
    When a Shirotsura player is spawned after ProductionVisuals selection
    Then the requested corruption appearance is <appearance>
    And retry reuses that appearance without creating another material instance

    Examples:
      | chapter               | appearance |
      | Prologue              | Early      |
      | IshibashiriApproach   | Early      |
      | Fuchimatoi            | Early      |
      | Interlude2            | Advanced   |
      | Minedaki              | Advanced   |
      | Ending                | Advanced   |

  Scenario: An unsupported mesh keeps its existing presentation
    Given the selected mesh lacks the audited corruption slot or scalar parameter
    When the visual component resolves the current appearance
    Then stage resolution and material application are reported separately
    And no shared material or unrelated slot is changed

  Scenario: Only the verified left-arm section is changed
    Given face and neck skin share an atlas section with the right hand
    When the corruption material setup is run twice
    Then the dedicated left-arm section has one idempotent two-stage blend graph
    And face, neck, and left-eye completion remains explicitly unsupported
    And mask, clothing, right arm, sword, gameplay stats, Sense, and input are unchanged
