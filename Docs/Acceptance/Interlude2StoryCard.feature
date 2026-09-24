Feature: Interlude2 story-card artwork
  The post-Fuchimatoi interlude needs a readable still without changing narrative flow.

  Scenario: Text-free artwork matches the narrative contract
    Given the Interlude2 card depicts Shirotsura after Fuchimatoi
    Then the artwork is a 16:9 PNG with no embedded text
    And the PNG is stored through Git LFS rather than as an inline Git binary
    And Shirotsura uses the existing Advanced mask, clothing, and left-arm corruption design
    And the corruption is visible at the neck and the left edge of the face beneath the mask
    And no new form, horns, excessive glow, or mask ability is introduced
    And Shirotsura is placed on the right with dark negative space across the center

  Scenario: Layout preview uses current CampaignHUD copy
    Given the review-only layout preview overlays the text-free artwork
    Then it shows the heading "INTERLUDE"
    And it shows the body "腕の痕が広がり、顔の痕も深くなっていた。"
    And it shows the prompt "SPACE / X : CONTINUE"
    And it does not alter card copy, chapter order, or game behavior
