Feature: Ishibashiri first-batch Blender evidence
  The procedural fallback must produce inspectable candidates without claiming UE approval.

  Scenario: Generate the bounded first adoption batch
    Given the manifest names OldCedar_A, Rock_A, and BoundaryStone_A as the first adoption batch
    When Blender 3.6.23 runs the generator in a clean output directory
    Then exactly those three asset directories are generated
    And each asset has a non-empty FBX, GLB, report, and preview
    And the candidates remain marked BLENDER PRODUCTION CANDIDATE

  Scenario: Validate exported geometry rather than exporter exit status
    Given the three generated FBX and GLB files
    When each file is imported into a fresh Blender scene
    Then visual meshes have finite non-empty geometry and dimensions within tolerance
    And each export contains its named non-empty UCX collision proxy
    And each preview is a non-trivial image with a 172 cm scale reference

  Scenario: Compare round trips against canonical geometry measurements
    Given source metrics measured from evaluated mesh vertices rather than cached object bounds
    When FBX and GLB are imported into separate clean Blender scenes
    Then the validator records object transforms and local-space and world-space bounds
    And visual object, vertex, and triangle counts match the generated candidate
    And physical dimensions and non-empty collision geometry remain within 3 percent
    And no PREVIEW object is present in either production export

  Scenario: Preserve the downstream review boundary
    Given Blender generation and round-trip checks pass
    When the evidence artifact is packaged with checksums
    Then UE import, UE runtime, and visual approval remain NOT_RUN
    And no generated binary is committed automatically
