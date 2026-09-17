"""Gameplay-authority contract: SHA256 digests of the gameplay sources.

Tests/production-gameplay-contract.json pins the sources that the production
visual intake must not change. Tests/test_production_intake.py verifies the
digests; this module owns the normalization so the test and the re-baseline
command can never drift apart.

Re-baseline deliberately after an intentional gameplay change:

    python Tools/GameplayContract.py --update

Without --update the script prints the files whose digest differs and exits 1.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
CONTRACT = ROOT / 'Tests' / 'production-gameplay-contract.json'

# Lines added by the production intake that are allowed to differ from the
# pinned gameplay authority. Everything else must match byte for byte.
PLAYER_OPTIONAL_RIG = (
    '    // This optional asset is not supplied by main; preserve the authored base pose without a CDO load error.\n'
    '    if (FPackageName::DoesPackageExist(TEXT("/Game/Characters/Rigged/Shirotsura/CR_Shirotsura_Climbing")))\n'
    '    {\n'
)


def normalize(filename: str, source: str) -> str:
    source = source.replace('#include "ProductionVisuals.h"\n', '')
    if filename.endswith('PrototypePlayer.cpp'):
        source = source.replace('#include "KakonActor.h"\n', '')
        source = source.replace('#include "Misc/PackageName.h"\n', '')
        source = source.replace(PLAYER_OPTIONAL_RIG, '')
        source = source.replace('        static ConstructorHelpers::FClassFinder<UControlRig>',
                                '    static ConstructorHelpers::FClassFinder<UControlRig>')
        source = source.replace('        if (ClimbingRig.Succeeded()) ClimbingControlRig->SetControlRigClass(ClimbingRig.Class);\n    }',
                                '    if (ClimbingRig.Succeeded()) ClimbingControlRig->SetControlRigClass(ClimbingRig.Class);')
        source = source.replace('    ProductionVisuals::ApplyAtBeginPlay(GetMesh(), TEXT("Shirotsura"));\n', '')
    if filename.endswith('IshibashiriBoss.cpp'):
        source = source.replace('    ProductionVisuals::ApplyAtBeginPlay(Creature, TEXT("Ishibashiri"));\n', '')
    if filename.endswith('CampaignGameInstance.cpp'):
        source = source.replace('ChapterWorldContext', 'WorldContext')
    return source


def digest(filename: str) -> str:
    source = (ROOT / filename).read_text(encoding='utf-8')
    return hashlib.sha256(normalize(filename, source).encode()).hexdigest()


def load() -> dict:
    return json.loads(CONTRACT.read_text(encoding='utf-8'))


def changed_files(contract: dict) -> list[str]:
    return [name for name, pinned in contract['sha256'].items() if digest(name) != pinned]


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument('--update', action='store_true', help='rewrite the pinned digests and source_commit')
    args = parser.parse_args(argv)
    contract = load()
    changed = changed_files(contract)
    if not changed:
        print('gameplay contract: all digests match')
        return 0
    for name in changed:
        print(('update ' if args.update else 'MISMATCH ') + name)
    if not args.update:
        return 1
    for name in changed:
        contract['sha256'][name] = digest(name)
    try:
        contract['source_commit'] = subprocess.check_output(['git', '-C', str(ROOT), 'rev-parse', 'HEAD'], text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        pass
    CONTRACT.write_text(json.dumps(contract, indent=2) + '\n', encoding='utf-8')
    return 0


if __name__ == '__main__':
    sys.exit(main())
