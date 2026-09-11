from pathlib import Path


ROOT = Path(__file__).parents[1]


def read(path: str) -> str:
    return (ROOT / path).read_text(encoding="utf-8")


def test_legacy_defaults_and_sm6_cook_target_coexist() -> None:
    config = read("Config/DefaultEngine.ini")
    assert "DefaultGraphicsRHI=DefaultGraphicsRHI_DX11" in config
    assert "r.DynamicGlobalIlluminationMethod=0" in config
    assert "r.ReflectionMethod=0" in config
    assert "r.Shadow.Virtual.Enable=0" in config
    assert "+D3D11TargetedShaderFormats=PCD3D_SM5" in config
    assert "+D3D12TargetedShaderFormats=PCD3D_SM6" in config


def test_high_quality_profile_is_explicit_and_complete() -> None:
    script = read("Tools/Prototype.ps1")
    assert "[switch]$HighQuality" in script
    for setting in ("'-d3d12'", "'-sm6'", "r.DynamicGlobalIlluminationMethod 1",
                    "r.ReflectionMethod 1", "r.Shadow.Virtual.Enable 1",
                    "r.VolumetricFog 1", "r.BloomQuality 4",
                    "r.DefaultFeature.AutoExposure 1"):
        assert setting in script


def test_basin_has_atmosphere_and_restrained_volumetric_fog() -> None:
    source = read("Source/IshibashiriPrototype/Private/BasinPrototypeArena.cpp")
    assert "USkyAtmosphereComponent" in source
    assert 'TEXT("BasinSkyAtmosphere")' in source
    assert "Fog->SetVolumetricFog(true)" in source
    assert "Fog->SetVolumetricFogExtinctionScale(.55f)" in source
