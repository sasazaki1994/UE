"""Validate the offline character PBR cache (never called by game startup)."""
import hashlib, json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
MANIFEST=ROOT/'Art/Materials/CC0PBR/manifest.json'
ROLES={'weathered_stone','aged_whitewood','coarse_indigo_cloth'}
MAPS={'base_color','normal_gl','roughness'}
def sha256(path):
    h=hashlib.sha256()
    with path.open('rb') as stream:
        for chunk in iter(lambda:stream.read(1024*1024),b''):h.update(chunk)
    return h.hexdigest()
def validate(data,require_files=True):
    errors=[];assets=data.get('assets',[])
    if {a.get('role') for a in assets}!=ROLES:errors.append('manifest must contain exactly the three roles')
    if not data.get('license',{}).get('verified'):errors.append('official CC0 license is unverified')
    for asset in assets:
        role=asset.get('role','<missing>')
        for field in ('asset_id','source_url','resolution'):
            if not asset.get(field):errors.append(f'{role}: missing {field}')
        maps=asset.get('maps',{})
        if set(maps)!=MAPS:errors.append(f'{role}: expected three PBR maps')
        for kind,record in maps.items():
            if not isinstance(record,dict):errors.append(f'{role}/{kind}: unresolved map');continue
            for field in ('url','path','sha256'):
                if not record.get(field):errors.append(f'{role}/{kind}: missing {field}')
            path=ROOT/record.get('path','')
            if require_files and (not path.is_file() or sha256(path)!=record.get('sha256')):errors.append(f'{role}/{kind}: absent or hash mismatch')
    return errors
if __name__=='__main__':
    errors=validate(json.loads(MANIFEST.read_text(encoding='utf-8')))
    if errors:raise SystemExit('PBR_CACHE_INVALID\n'+'\n'.join(errors))
    print('PBR_CACHE_VALID')
