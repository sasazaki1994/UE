Feature: Selectable high quality rendering
  The proven lightweight gameplay path must remain available while visual work is validated.

  Scenario: Legacy is the default profile
    Given the project is launched without "-HighQuality"
    Then the default Windows RHI is DX11
    And Lumen and Virtual Shadow Maps are disabled

  Scenario: High Quality is explicitly selected
    Given the project is launched with "-HighQuality"
    Then the launcher requests DX12 and Shader Model 6
    And Lumen GI, Lumen Reflections, Virtual Shadow Maps, volumetric fog, bloom and auto exposure are requested

  Scenario: Gameplay validation can use either profile
    Given a route climbing, camera, Grab, Retry, gamepad or Basin validation command
    When "-HighQuality" is omitted or supplied
    Then the same gameplay test implementation and map are used
