Feature: Minedaki full encounter
  The giant's changing posture creates the route while common Nushi systems own completion.

  Scenario: The climb reads as a living mountain without changing route authority
    Given the player is attached to Minedaki's existing BodyGrabFrame
    When the body breathes and slowly changes posture
    Then the player and all enabled anchors should follow the same body-relative transform
    And breathing amplitude should settle after Calm
    And route movement should remain bounded enough for stable input

  Scenario: Shake is a readable cling decision
    Given the player is climbing the first wall route
    When the body enters ShakeWarning before Shaking
    Then local light, body tension, and Corruption Sense should warn of danger
    And the gameplay Shake check should occur only after the warning interval
    And reaching a rest node should restore stamina

  Scenario: The arm becomes a safe bridge
    Given Kakon 1 has been purified at the upper rest point
    When the ArmBridgeTransition begins
    Then its opening quarter should anticipate with local light and camera framing
    And the arm should form the route only during the remaining transition
    And the arm transition should not invoke the Shake failure check
    And arm route nodes should open only after bridge formation completes

  Scenario: Purification settles the mountain rather than damaging it
    Given the current Kakon is reached through the enabled route
    When the player purifies it
    Then the shared purification presentation envelope should pulse inward through local light
    And progression should remain owned by common Kakon and Nushi components
    And three purifications should reduce breathing and stabilize the Calm camera

  Scenario: Calm Minedaki by purifying all three Kakon
    Given Minedaki is Active with three registered Kakon
    When the player climbs the wall and purifies Kakon 1, 2, and 3 in route order
    Then progress should be 3 of 3
    And Minedaki should be Calm
    And the encounter should be Completed
    And the Minedaki action should reach Calm before the Victory capture
    And the HUD should show Victory

  Scenario: Body movement opens each later route
    Given the original route to Kakon 1 is open
    When Kakon 1 is purified and the arm transition finishes
    Then the arm bridge anchors should become available
    When Kakon 2 is purified and the final transition finishes
    Then the shoulder and head anchors should become available
    And a grabbed player should retain the same transform relative to BodyGrabFrame

  Scenario Outline: Recover without resetting progress
    Given <count> Kakon have been purified
    When the grabbed player falls and reaches a recovery ledge
    Then progress should remain <count> of 3
    And the player should walk to a recovery anchor and use Grab to reconnect
    Examples:
      | count |
      | 1     |
      | 2     |

  Scenario: Retry repeatedly without state leakage
    Given the encounter is completed or recovery is in progress
    When Retry is requested twice
    Then all three Kakon and common progress should reset
    And the boss, body, routes, player, Grab, stamina, camera, encounter and HUD should reset
    And recovery grace and Minedaki telemetry should reset
    And no encounter actor should be duplicated
