Feature: STEP 4C Windows review-gate evidence runner
  The STEP 4B criteria remain unchanged; the runner only automates their collection.

  Scenario: Build failure stops gameplay
    Given Windows and exactly Unreal Engine 5.6.1 pass preflight
    When the Prototype Build action fails
    Then stdout, stderr, command, exit code, and Unreal logs are retained
    And every gameplay result remains NOT_RUN
    And package is not run

  Scenario: One command gathers review evidence
    Given Windows, Unreal Engine 5.6.1, MSVC, Windows SDK, the project, and the map pass preflight
    When RunIshibashiriReviewGate.ps1 is invoked
    Then Approach and Ishibashiri run at 60 and 30 FPS
    And keyboard and gamepad campaign paths run
    And Legacy and HighQuality captures are copied rather than moved
    And summary.json contains only PASS, FAIL, BLOCKED, NOT_RUN, or NOT_APPLICABLE gate states
    And package runs only after every major gate passes

  Scenario: Control Rig is unavailable
    Given CR_Shirotsura_Climbing.uasset is absent
    When the IK gate is reached
    Then controlRig is BLOCKED
    And no replacement uasset is generated

  Scenario: Runtime prerequisites are unavailable
    Given the host is not Windows or its Unreal version is not exactly 5.6.1
    When preflight runs
    Then the verdict is UNVERIFIED — WINDOWS UE 5.6.1 NOT AVAILABLE
    And no gameplay command runs
