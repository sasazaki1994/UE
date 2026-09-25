# language: en
Feature: Dedicated review gate for the Ishibashiri playable demo
  The short demo is judged independently from the full four-encounter campaign gate.

  Scenario: Ishibashiri demo is the primary playable gate
    Given the editor target has built successfully
    When the demo runs at 60 FPS, 30 FPS, with gamepad input, and with capture
    Then every run requires ISHIBASHIRI_DEMO_E2E_PASS from its own log
    And the completion, retry, Sense reset, and save-isolation evidence is evaluated

  Scenario: A build failure stops all gameplay validation
    When the build command fails
    Then no demo, campaign regression, capture, or package command is started

  Scenario: Demo persistence remains isolated
    When Ishibashiri Demo runs
    Then the source contract disables MagabaraiCampaign load, write, and delete before any load
    And no normal campaign persistence marker occurs in the completed Demo E2E log

  Scenario: High Quality requires actual renderer evidence
    When the High Quality demo process exits successfully
    Then D3D12, SM6, Lumen GI, Lumen Reflections, and Virtual Shadow Maps must all be evidenced in its log

  Scenario: Full campaign receives a minimal regression run
    Then Campaign runs once at 60 FPS
    And the dedicated gate does not run the individual Fuchimatoi, Minedaki, or Magatsune matrix

  Scenario: Package is produced only after required demo gates pass
    Given every required demo and minimal campaign regression gate is PASS
    When Package is invoked
    Then its output and logs are copied into the evidence directory

  Scenario: DryRun never claims runtime success
    When the review gate is invoked with DryRun
    Then it prints every planned command in order without starting a process
    And every result and the verdict are NOT_RUN
