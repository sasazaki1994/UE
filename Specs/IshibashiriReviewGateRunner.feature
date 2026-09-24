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
    And Fuchimatoi, Minedaki, and Magatsune each run at 60 and 30 FPS
    And each standalone encounter runs its required gamepad, fall recovery, and capture coverage without duplicating equivalent runs
    And Legacy and HighQuality captures are copied rather than moved
    And summary.json contains only PASS, FAIL, BLOCKED, NOT_RUN, or NOT_APPLICABLE gate states
    And package runs only after every major gate passes

  Scenario: Review the complete plan without Unreal Engine
    Given Unreal Engine is unavailable
    When RunIshibashiriReviewGate.ps1 is invoked with DryRun
    Then no build, gameplay, capture, or package process is started
    And every planned command is recorded in execution order with its required inputs and expected artifacts
    And every gate and the overall verdict are NOT_RUN in JSON and Markdown evidence
    And DryRun is never reported as successful Unreal validation

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
