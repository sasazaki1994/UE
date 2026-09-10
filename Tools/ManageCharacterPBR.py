"""Load and validate the offline character PBR cache without Blender."""
import argparse
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / 'Art/Materials/CC0PBR/manifest.json'
ROLES = {'weathered_stone', 'aged_whitewood', 'coarse_indigo_cloth'}
MAPS = {'base_color', 'normal_gl', 'roughness'}


class ManifestValidationError(ValueError):
    """A manifest cannot safely be used for PBR application."""

    def __init__(self, errors):
        self.errors = list(errors)
        super().__init__('PBR_CACHE_INVALID\n' + '\n'.join(self.errors))


def sha256(path):
    h = hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b''):
            h.update(chunk)
    return h.hexdigest()


def load_manifest(path=MANIFEST):
    path = Path(path)
    try:
        data = json.loads(path.read_text(encoding='utf-8'))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise ManifestValidationError([f'{path}: invalid JSON or unreadable file: {exc}']) from exc
    if not isinstance(data, dict):
        raise ManifestValidationError(['manifest root must be a JSON object'])
    return data


def validate(data, require_files=True, root=ROOT):
    """Return all validation errors; never start file consumers or mutate assets."""
    errors = []
    if not isinstance(data, dict):
        return ['manifest root must be a JSON object']
    if data.get('schema_version') != 1:
        errors.append('manifest: schema_version must be 1')
    license_record = data.get('license')
    if not isinstance(license_record, dict):
        errors.append('manifest: license must be an object')
    else:
        for field in ('name', 'verification_url'):
            if not isinstance(license_record.get(field), str) or not license_record[field].strip():
                errors.append(f'license: missing or invalid {field}')
        if license_record.get('verified') is not True:
            errors.append('official CC0 license is unverified')
    assets = data.get('assets')
    if not isinstance(assets, list):
        return errors + ['manifest: assets must be an array']

    role_counts = {role: 0 for role in ROLES}
    for index, asset in enumerate(assets):
        location = f'assets[{index}]'
        if not isinstance(asset, dict):
            errors.append(f'{location}: asset must be an object')
            continue
        role = asset.get('role')
        if role not in ROLES:
            errors.append(f'{location}: unknown or missing role {role!r}')
        else:
            role_counts[role] += 1
        label = role if isinstance(role, str) else location
        for field in ('asset_id', 'source_url', 'resolution'):
            if not isinstance(asset.get(field), str) or not asset[field].strip():
                errors.append(f'{label}: missing or invalid {field}')
        maps = asset.get('maps')
        if not isinstance(maps, dict):
            errors.append(f'{label}: maps must be an object')
            continue
        missing = MAPS - set(maps)
        extra = set(maps) - MAPS
        if missing or extra:
            errors.append(f'{label}: maps must contain exactly {sorted(MAPS)}')
        for kind in MAPS:
            record = maps.get(kind)
            if not isinstance(record, dict):
                errors.append(f'{label}/{kind}: map must be an object')
                continue
            for field in ('url', 'path', 'sha256'):
                if not isinstance(record.get(field), str) or not record[field].strip():
                    errors.append(f'{label}/{kind}: missing or invalid {field}')
            relative_path = record.get('path')
            expected_hash = record.get('sha256')
            if isinstance(expected_hash, str) and (len(expected_hash) != 64 or
                    any(character not in '0123456789abcdefABCDEF' for character in expected_hash)):
                errors.append(f'{label}/{kind}: sha256 must be 64 hexadecimal characters')
            if require_files and isinstance(relative_path, str) and relative_path:
                path = Path(root) / relative_path
                if not path.is_file():
                    errors.append(f'{label}/{kind}: file is missing: {relative_path}')
                elif isinstance(expected_hash, str) and sha256(path) != expected_hash.lower():
                    errors.append(f'{label}/{kind}: SHA-256 mismatch: {relative_path}')
    for role, count in role_counts.items():
        if count != 1:
            errors.append(f'{role}: expected exactly one asset, found {count}')
    return errors


def load_validated_manifest(path=MANIFEST, require_files=True, root=None):
    path = Path(path)
    data = load_manifest(path)
    errors = validate(data, require_files=require_files,
                      root=Path(root) if root is not None else ROOT)
    if errors:
        raise ManifestValidationError(errors)
    return data


def main(argv=None):
    parser = argparse.ArgumentParser()
    parser.add_argument('command', choices=('verify',))
    parser.add_argument('--manifest', type=Path, default=MANIFEST)
    args = parser.parse_args(argv)
    load_validated_manifest(args.manifest)
    print('PBR_CACHE_VALID')


if __name__ == '__main__':
    try:
        main()
    except ManifestValidationError as exc:
        raise SystemExit(str(exc)) from exc
