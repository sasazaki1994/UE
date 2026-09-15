"""Preflight launcher: Python-only missing-source check, Blender for actual geometry.

python Tools/ValidateProductionCharacter.py --character Shirotsura --allow-missing
python Tools/ValidateProductionCharacter.py --character Shirotsura --blender <blender.exe>
Exit 0 pass (or explicitly allowed missing), 2 invalid, 3 missing, 4 tool unavailable.
"""
import argparse
import shutil
import subprocess
import sys
from ProductionCharacter import (ROOT, NAMES, BLOCKED, folder, read, write,
                                 source_path, source_fingerprint, invalidate)


def main(argv=None):
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
        sys.stderr.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--character', required=True, choices=NAMES)
    parser.add_argument('--blender', default=shutil.which('blender'))
    parser.add_argument('--allow-missing', action='store_true')
    args = parser.parse_args(argv)
    report_path = folder(args.character) / 'Validation/preflight.json'
    try:
        manifest = read(folder(args.character) / 'manifest.json')
        source = source_path(args.character, manifest)
        unregistered = [p for p in (folder(args.character) / 'TripoSource').rglob('*')
                        if p.is_file() and p.name != '.gitkeep']
        if source is None and unregistered:
            raise ValueError('Unregistered intake files: set source_file and generation provenance in manifest')
        if source is None or not source.is_file():
            # A named but absent file is a broken intake, not an optional missing asset.
            status = 'invalid_source' if source is not None else 'missing_source'
            report = {'asset_name': args.character, 'status': status,
                      'message': 'Referenced source file does not exist' if source else BLOCKED,
                      'metrics': None, 'adopted': False}
            write(report_path, report)
            invalidate(args.character, status)
            print(report['message'])
            return 2 if source else (0 if args.allow_missing else 3)
        if source.stat().st_size == 0:
            raise ValueError('Source file is empty')
        if not manifest.get('generated_by') or not manifest.get('generation_date'):
            raise ValueError('Record Tripo task/export provenance in generated_by and generation_date')
        if not args.blender:
            invalidate(args.character, 'blocked_tool')
            write(report_path, {'asset_name': args.character, 'status': 'blocked_tool',
                  'message': 'Blender required to inspect actual mesh data',
                  'source_fingerprint': source_fingerprint(args.character, manifest)})
            print('BLOCKED — BLENDER REQUIRED')
            return 4
        return subprocess.call([args.blender, '--background', '--factory-startup',
            '--disable-autoexec', '--python-exit-code', '2', '--python',
            str(ROOT / 'Tools/ProductionCharacterBlender.py'), '--', 'preflight',
            '--character', args.character])
    except (ValueError, OSError, KeyError) as exc:
        if (folder(args.character) / 'manifest.json').is_file():
            try:
                invalidate(args.character, 'invalid_source')
            except (ValueError, OSError):
                pass
        write(report_path, {'asset_name': args.character, 'status': 'invalid_source',
                           'message': str(exc), 'adopted': False})
        print(str(exc), file=sys.stderr)
        return 2


if __name__ == '__main__':
    sys.exit(main())
