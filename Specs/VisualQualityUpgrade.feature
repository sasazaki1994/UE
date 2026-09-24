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

  Scenario: Ishibashiri grab has no authored montage
    Given the optional Ishibashiri grab Montage and Anim Blueprint are absent
    When Shirotsura grabs the foreleg during Kneel
    Then the capsule approaches the moving route-zero target over a visible interval
    And facing alignment completes before climbing control is handed off
    And no missing binary asset is reported as complete

  Scenario: Ishibashiri is moving terrain
    Given Shirotsura is attached to Ishibashiri
    When Ishibashiri walks, warns, or shakes
    Then route placement and four-limb contact targets remain in the creature frame
    And the climbing camera widens and follows without high-frequency camera shake

  Scenario: The third Kakon is purified
    Given all three Kakon use the existing purification lifecycle
    When the last Kakon transitions to Purified
    Then corruption presentation converges instead of exploding
    And Ishibashiri enters Calm without an HP or death state
    And Encounter Completed, Victory, and Retry keep their existing authority
