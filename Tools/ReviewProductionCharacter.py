"""Evaluate evidence; no score, capture or successful play is inferred from file presence."""
import argparse
import sys
import math
from ProductionCharacter import (NAMES, BLOCKED, folder, read, write, sha, confined,
                                 source_fingerprint, require_report)

SCORES = ('silhouette', 'material_quality', 'texture_quality', 'animation_deformation',
          'game_readability', 'gameplay_compatibility', 'performance', 'overall_production_readiness')
PLAYER = ('Idle', 'Walk', 'Run', 'Jump', 'Slash', 'Dodge', 'Grab', 'Climb', 'HangCling',
          'BoundarySense', 'CorruptionSense')
BOSS = ('Spawn', 'Charge', 'Recovery', 'Grab', 'Climb', '11Route', 'branch', 'rest', 'stamina',
        'shake_buck', 'cling', 'fall', '3Kakon', 'Covered', 'Exposed', 'Purified',
        '3of3', 'Calm', 'Victory', 'Retry')
CAMPAIGN = ('Campaign_Ishibashiri', 'Campaign_Fuchimatoi', 'Campaign_Minedaki', 'Campaign_Magatsune')
CAPTURES = {'Shirotsura': ('Idle', 'Back', 'Run', 'Slash', 'Sense', 'Climb'),
            'Ishibashiri': ('front34', 'side', 'back', 'Charge', 'Grab', 'Climb', 'Kakon', 'Calm')}


def evaluate(name, review):
    base = folder(name)
    manifest = read(base / 'manifest.json')
    fingerprint = source_fingerprint(name, manifest)
    result = {'asset_name': name, 'decision': 'BLOCKED', 'adopted': False,
              'source_fingerprint': fingerprint, 'scores': {k: None for k in SCORES}, 'reasons': []}
    if not fingerprint:
        result['reasons'] = [BLOCKED]
        return result
    try:
        export = require_report(name, 'export', base / 'Export' / ('SK_' + name + '.fbx'))
        imported = require_report(name, 'ue-import')
        if imported.get('export_sha256') != export['artifact_sha256']:
            raise ValueError('UE import is stale')
        if review.get('export_sha256') != export['artifact_sha256'] or not review.get('reviewer'):
            raise ValueError('Review must identify reviewer and current export hash')
        if not all(review.get('capture_conditions', {}).get(k) for k in
                   ('camera_transform', 'fov', 'lighting', 'resolution', 'hardware', 'driver', 'fixed_scenario')):
            raise ValueError('Record matching comparison conditions')
        if review['capture_conditions'].get('render_profile') != 'DX12_SM6_Lumen_VSM':
            raise ValueError('High Quality comparison is required')
        for view in CAPTURES[name]:
            for variant in ('fallback', 'candidate'):
                capture = review['captures'][view][variant]
                file = confined(base / 'Validation', capture['file'])
                if sha(file) != capture['sha256']:
                    raise ValueError('Missing/stale comparison capture: ' + view)
        checks = (*PLAYER, *CAMPAIGN) if name == 'Shirotsura' else (*BOSS, *CAMPAIGN)
        regression = review['regression']
        if any(regression.get(key) == 'fail' for key in checks):
            result.update(decision='REJECT', reasons=['Gameplay regression failed'])
            return result
        if not all(regression.get(key) == 'pass' for key in checks):
            raise ValueError('Gameplay, campaign and Covered inspection checks are incomplete')
        if not review.get('no_missing_or_pink_materials') or not review.get('no_deformation_breaks'):
            raise ValueError('Rendered material and deformation review is incomplete')
        scores = review['scores']
        if not all(type(scores.get(k)) is int and 1 <= scores[k] <= 5 for k in SCORES):
            raise ValueError('All eight scores must be reviewed on a 1..5 scale')
        result['scores'] = scores
        before, after = review['performance']['fallback'], review['performance']['candidate']
        for metrics in (before, after):
            for key in ('vertices', 'triangles', 'materials', 'texture_bytes', 'draw_calls', 'fps', 'frame_ms_p50', 'frame_ms_p95', 'trials'):
                if not isinstance(metrics.get(key), (int, float)) or not math.isfinite(metrics[key]) or metrics[key] <= 0:
                    raise ValueError('Missing positive measured performance metric: ' + key)
            if type(metrics['trials']) is not int or metrics['trials'] < 3:
                raise ValueError('Record at least three complete trials for each variant')
            if type(metrics['successes']) is not int or not 0 <= metrics['successes'] <= metrics['trials']:
                raise ValueError('Invalid success/trial measurement')
        if after['successes'] / after['trials'] < before['successes'] / before['trials']:
            result.update(decision='REJECT', reasons=['Gameplay success rate decreased'])
        elif any(after[key] > before[key] * ratio for key, ratio in (
                ('frame_ms_p50', 1.1), ('frame_ms_p95', 1.1), ('texture_bytes', 1.25), ('draw_calls', 1.2))):
            result.update(decision='REJECT', reasons=['Performance regression exceeds intake budget'])
        elif min(scores.values()) < 3 or scores['gameplay_compatibility'] < 5:
            result.update(decision='REJECT', reasons=['Visual/gameplay rating below minimum'])
        elif min(scores.values()) < 4:
            result.update(decision='CONDITIONAL', reasons=['Document and resolve all score-3 visual issues before adoption'])
        else:
            result.update(decision='ADOPT', reasons=['Reviewed evidence and performance gates passed'])
    except (OSError, ValueError, KeyError, TypeError) as exc:
        result['reasons'].append(str(exc))
    return result


def main():
    if hasattr(sys.stdout, 'reconfigure'):
        sys.stdout.reconfigure(encoding='utf-8')
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--character', choices=NAMES, required=True)
    parser.add_argument('--adopt', action='store_true', help='Record adopted=true only after ADOPT gate; default remains fallback')
    args = parser.parse_args()
    path = folder(args.character) / 'Validation/review.json'
    result = evaluate(args.character, read(path) if path.is_file() else {})
    if args.adopt and result['decision'] == 'ADOPT':
        result['adopted'] = True
    write(folder(args.character) / 'Validation/adoption.json', result)
    manifest = read(folder(args.character) / 'manifest.json')
    manifest['adopted'] = result['adopted']
    if result['decision'] == 'ADOPT':
        manifest.update(gameplay_validation_status='pass', visual_validation_status='pass')
    write(folder(args.character) / 'manifest.json', manifest)
    print(result['decision'] + ': ' + '; '.join(result['reasons']))
    return 0 if result['decision'] in ('ADOPT', 'CONDITIONAL') else 3


if __name__ == '__main__':
    sys.exit(main())
