"""Procedural Blender props and decal maps for the Ishibashiri ritual line and passage traces.

Run: blender --background --factory-startup --python Tools/CreateIshibashiriPropKit.py
Writes FBX, reports and decal PNGs to Art/Environment/Ishibashiri/Generated/Props (ignored by git).
Surfaces come from the CC0 Poly Haven textures assigned by ImportPolyHavenEnvironment.py.
"""
import json
import math
import random
from pathlib import Path

import bmesh
import bpy
import numpy as np
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
OUTPUT = ROOT / "Art/Environment/Ishibashiri/Generated/Props"
SEED = 830194
DECAL_SIZE = 1024


def reset():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    random.seed(SEED)


def slot(obj, name):
    material = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    obj.data.materials.append(material)
    return len(obj.data.materials) - 1


def ring_mesh(bm, uv, rings, closed_top):
    """Bridge equal-length vertex rings; rings are (points, v) with u following the perimeter."""
    layers = []
    for points, v in rings:
        layers.append(([bm.verts.new(p) for p in points], v))
    for (lower, v0), (upper, v1) in zip(layers, layers[1:]):
        count = len(lower)
        perimeter = [0.0]
        for i in range(count):
            perimeter.append(perimeter[-1] + (lower[(i + 1) % count].co - lower[i].co).length)
        for i in range(count):
            j = (i + 1) % count
            face = bm.faces.new((lower[i], lower[j], upper[j], upper[i]))
            for loop, (u, v) in zip(face.loops, ((perimeter[i], v0), (perimeter[i + 1], v0),
                                                 (perimeter[i + 1], v1), (perimeter[i], v1))):
                loop[uv].uv = (u * 2.0, v * 2.0)
    if closed_top:
        top, v = layers[-1]
        apex = bm.verts.new(closed_top)
        for i in range(len(top)):
            face = bm.faces.new((top[i], top[(i + 1) % len(top)], apex))
            for loop in face.loops:
                loop[uv].uv = (loop.vert.co.x * 2.0, loop.vert.co.y * 2.0)
    return layers


def ritual_post():
    """Aged square stake with chamfered edges, a pointed cap, two cord grooves and a rotted foot."""
    mesh = bpy.data.meshes.new("SM_Ishibashiri_RitualPost_A")
    obj = bpy.data.objects.new(mesh.name, mesh)
    bpy.context.collection.objects.link(obj)
    slot(obj, "Wood")
    half, chamfer, height, cap = .12, .028, 3.08, .14
    section = []
    for corner in range(4):
        angle = corner * math.pi / 2
        centre = Vector((math.cos(angle + math.pi / 4), math.sin(angle + math.pi / 4), 0)) * (half - chamfer) * math.sqrt(2)
        for step in range(4):
            a = angle + step * (math.pi / 2) / 3
            section.append(centre + Vector((math.cos(a), math.sin(a), 0)) * chamfer)
        mid = (centre + Vector((math.cos(angle + math.pi / 2), math.sin(angle + math.pi / 2), 0)) * chamfer)
        nxt = Vector((math.cos(angle + 3 * math.pi / 4), math.sin(angle + 3 * math.pi / 4), 0)) * (half - chamfer) * math.sqrt(2)
        nxt += Vector((math.cos(angle + math.pi / 2), math.sin(angle + math.pi / 2), 0)) * chamfer
        for k in range(1, 4):
            section.append(mid.lerp(nxt, k / 4))
    rng = random.Random(SEED + 1)
    phases = [rng.random() * math.tau for _ in range(6)]
    rings = []
    count = 150
    for r in range(count + 1):
        z = height * r / count
        scale = 1.0
        for groove in (2.62, 2.78):
            scale -= .075 * math.exp(-((z - groove) / .018) ** 2)
        if z < .35:
            scale -= .05 * (1 - z / .35) * (.6 + .4 * math.sin(z * 31 + phases[0]))
        points = []
        for i, p in enumerate(section):
            angle = math.atan2(p.y, p.x)
            # Long grain checks and irregular weathering keep the silhouette from reading as a box.
            grain = .006 * math.sin(angle * 9 + phases[1]) * math.sin(z * 2.3 + phases[2])
            check = -.008 * max(0.0, math.sin(angle * 5 + phases[3])) ** 12
            wobble = Vector((.004 * math.sin(z * 1.7 + phases[4]), .004 * math.sin(z * 1.3 + phases[5]), 0))
            points.append(Vector((p.x * (scale + grain + check), p.y * (scale + grain + check), z)) + wobble)
        rings.append((points, z))
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    ring_mesh(bm, uv, rings, (0, 0, height + cap))
    bottom = [bm.verts.new(Vector((p.x, p.y, 0))) for p in rings[0][0]]
    bm.faces.new(list(reversed(bottom)))
    bmesh.ops.remove_doubles(bm, verts=bm.verts, dist=.0005)
    bm.to_mesh(mesh)
    bm.free()
    for polygon in mesh.polygons:
        polygon.use_smooth = True
    return obj


def tube(bm, uv, path, radius, sides, twist=0.0, phase=0.0):
    rings = []
    normal_ref = Vector((0, 0, 1))
    length = 0.0
    for i, point in enumerate(path):
        if i:
            length += (point - path[i - 1]).length
        tangent = (path[min(i + 1, len(path) - 1)] - path[max(i - 1, 0)]).normalized()
        side = tangent.cross(normal_ref)
        if side.length < 1e-4:
            side = tangent.cross(Vector((1, 0, 0)))
        side.normalize()
        up = side.cross(tangent).normalized()
        r = radius(i / (len(path) - 1))
        ring = []
        for s in range(sides):
            a = math.tau * s / sides + twist * length + phase
            ring.append(point + (side * math.cos(a) + up * math.sin(a)) * r)
        rings.append((ring, length))
    ring_mesh(bm, uv, rings, None)


def old_rope():
    """Weathered shimenawa: three twisted straw strands, hanging tassels and four zigzag paper strips."""
    mesh = bpy.data.meshes.new("SM_Ishibashiri_OldRope_A")
    obj = bpy.data.objects.new(mesh.name, mesh)
    bpy.context.collection.objects.link(obj)
    rope_slot, paper_slot = slot(obj, "Rope"), slot(obj, "Paper")
    length, sag, samples = 16.0, .9, 420
    centre = [Vector((length * t, 0, -sag * 4 * t * (1 - t))) for t in (i / samples for i in range(samples + 1))]
    thickness = lambda t: .05 + .045 * math.sin(math.pi * t)
    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    for strand in range(3):
        phase = strand * math.tau / 3
        path = []
        travelled = 0.0
        for i, point in enumerate(centre):
            if i:
                travelled += (point - centre[i - 1]).length
            t = i / samples
            a = travelled / .42 * math.tau + phase
            offset = thickness(t) * .55
            path.append(point + Vector((0, math.cos(a) * offset, math.sin(a) * offset)))
        tube(bm, uv, path, lambda t: thickness(t) * .62, 8, twist=9.0, phase=phase)
    for t in (.32, .5, .68):
        anchor = Vector((length * t, 0, -sag * 4 * t * (1 - t) - thickness(t) * .8))
        drop = [anchor + Vector((0, 0, -.42 * k / 10)) for k in range(11)]
        tube(bm, uv, drop, lambda k: .035 + .03 * k ** 2, 10)
    rng = random.Random(SEED + 2)
    paper = []
    for t in (.2, .41, .59, .8):
        anchor = Vector((length * t, 0, -sag * 4 * t * (1 - t) - thickness(t) * .5))
        width, drop = .07, .12
        # Folded shide: alternating offsets fall in four steps, each slightly twisted with age.
        for step in range(4):
            x0 = anchor.x + (.035 if step % 2 else -.035)
            top = anchor.z - step * drop
            twist = rng.uniform(-.15, .15)
            quad = [Vector((x0 - width / 2, twist * width, top)), Vector((x0 + width / 2, -twist * width, top)),
                    Vector((x0 + width / 2, -twist * width, top - drop * 1.05)),
                    Vector((x0 - width / 2, twist * width, top - drop * 1.05))]
            verts = [bm.verts.new(v) for v in quad]
            face = bm.faces.new(verts)
            for loop, coords in zip(face.loops, ((0, 0), (1, 0), (1, 1), (0, 1))):
                loop[uv].uv = coords
            paper.append(face)
    paper = set(paper)
    for face in bm.faces:
        face.material_index = paper_slot if face in paper else rope_slot
        face.smooth = face not in paper
    bm.to_mesh(mesh)
    bm.free()
    return obj


def stepping_stone():
    """Low irregular slab for the basin procession line; its top stays nearly flat for walking."""
    mesh = bpy.data.meshes.new("SM_Ishibashiri_SteppingStone_A")
    obj = bpy.data.objects.new(mesh.name, mesh)
    bpy.context.collection.objects.link(obj)
    slot(obj, "Stone")
    rng = random.Random(SEED + 5)
    phases = [rng.random() * math.tau for _ in range(5)]
    segments, rings, half_x, half_y, height = 64, 9, 1.45, .575, .12

    def outline(a):
        # Superellipse with low-frequency chipping, so each edge reads as split stone rather than a rounded card.
        c, s = math.cos(a), math.sin(a)
        r = 1 / (abs(c) ** 3.2 + abs(s) ** 3.2) ** (1 / 3.2)
        r *= 1 + .05 * math.sin(3 * a + phases[0]) + .03 * math.sin(7 * a + phases[1]) + .015 * math.sin(13 * a + phases[2])
        return Vector((c * r * half_x, s * r * half_y, 0))

    bm = bmesh.new()
    uv = bm.loops.layers.uv.new("UVMap")
    centre = bm.verts.new((0, 0, height))
    grid = []
    for ring in range(1, rings + 1):
        t = ring / rings
        row = []
        for seg in range(segments):
            p = outline(math.tau * seg / segments) * t
            dome = .012 * (1 - t * t) + .004 * math.sin(p.x * 9 + phases[3]) * math.sin(p.y * 11 + phases[4])
            # The outer ring rolls over into the side so the edge is not razor sharp.
            z = height + dome - (.03 * ((t - .85) / .15) ** 2 if t > .85 else 0)
            row.append(bm.verts.new((p.x, p.y, z)))
        grid.append(row)
    skirt = [bm.verts.new((v.co.x * 1.03, v.co.y * 1.03, 0)) for v in grid[-1]]
    faces = []
    for seg in range(segments):
        nxt = (seg + 1) % segments
        faces.append(bm.faces.new((centre, grid[0][seg], grid[0][nxt])))
        for ring in range(rings - 1):
            faces.append(bm.faces.new((grid[ring][seg], grid[ring + 1][seg], grid[ring + 1][nxt], grid[ring][nxt])))
        faces.append(bm.faces.new((grid[-1][seg], skirt[seg], skirt[nxt], grid[-1][nxt])))
    bm.faces.new(list(reversed(skirt)))
    for face in bm.faces:
        for loop in face.loops:
            # Planar projection on top; the skirt borrows X and height so side texels are not smeared.
            co = loop.vert.co
            loop[uv].uv = (co.x * .8, (co.y if co.z > height * .7 else co.z - .6) * .8)
        face.smooth = True
    bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
    bm.to_mesh(mesh)
    bm.free()
    return obj


def export(obj):
    OUTPUT.mkdir(parents=True, exist_ok=True)
    bpy.ops.object.select_all(action="DESELECT")
    obj.select_set(True)
    bpy.context.view_layer.objects.active = obj
    path = OUTPUT / (obj.name + ".fbx")
    bpy.ops.export_scene.fbx(filepath=str(path), use_selection=True, object_types={"MESH"}, axis_forward="-Y",
                             axis_up="Z", apply_unit_scale=True, mesh_smooth_type="FACE", add_leaf_bones=False,
                             bake_anim=False, path_mode="STRIP")
    points = [v.co for v in obj.data.vertices]
    lower = [min(p[i] for p in points) for i in range(3)]
    upper = [max(p[i] for p in points) for i in range(3)]
    report = {"asset": obj.name, "source": "Blender procedural (CreateIshibashiriPropKit.py)", "seed": SEED,
              "triangles": sum(len(p.vertices) - 2 for p in obj.data.polygons),
              "dimensions_cm": [round((upper[i] - lower[i]) * 100, 2) for i in range(3)],
              "bounds_min_cm": [round(lower[i] * 100, 2) for i in range(3)],
              "material_slots": [m.name for m in obj.data.materials], "blender_version": bpy.app.version_string}
    (OUTPUT / (obj.name + "_report.json")).write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print("PROPKIT_EXPORTED", json.dumps(report))
    return report


def save_png(name, rgba):
    image = bpy.data.images.new(name, width=DECAL_SIZE, height=DECAL_SIZE, alpha=True)
    image.colorspace_settings.name = "sRGB" if name.endswith("BaseColor") else "Non-Color"
    image.pixels.foreach_set(np.clip(rgba, 0, 1).astype(np.float32).ravel())
    image.filepath_raw = str(OUTPUT / (name + ".png"))
    image.file_format = "PNG"
    image.save()


def blur(field, passes):
    for _ in range(passes):
        field = (field + np.roll(field, 1, 0) + np.roll(field, -1, 0) + np.roll(field, 1, 1) + np.roll(field, -1, 1)) / 5
    return field


def normal_from(height, strength):
    dx = np.roll(height, -1, axis=1) - np.roll(height, 1, axis=1)
    dy = np.roll(height, -1, axis=0) - np.roll(height, 1, axis=0)
    normal = np.stack((-dx * strength, -dy * strength, np.ones_like(height)), axis=-1)
    normal /= np.linalg.norm(normal, axis=-1, keepdims=True)
    return normal * .5 + .5


def noise(rng, y, x, octaves):
    field = np.zeros_like(x)
    for frequency, weight in octaves:
        for _ in range(3):
            a, b = rng.integers(-frequency, frequency + 1, 2)
            field += np.sin(math.tau * (a * x + b * y) + rng.random() * math.tau) * weight / 3
    return field


def decal(name, height, mask, wet, dry):
    alpha = np.clip(mask, 0, 1)
    colour = dry[None, None, :] * (1 - wet[..., None]) + np.array((.035, .03, .028))[None, None, :] * wet[..., None]
    save_png(name + "_BaseColor", np.concatenate((colour, alpha[..., None]), axis=-1))
    save_png(name + "_Normal", np.concatenate((normal_from(height, 26.0), np.ones_like(alpha)[..., None]), axis=-1))


def footprint_decal():
    """Cloven boar print in V-up texture space: two long toe pits ahead of two dewclaw dents."""
    rng = np.random.default_rng(SEED + 3)
    y, x = np.mgrid[0:DECAL_SIZE, 0:DECAL_SIZE].astype(np.float32) / DECAL_SIZE
    wobble = noise(rng, y, x, ((3, .03), (9, .012)))
    depth = np.zeros_like(x)
    for cx, cy, rx, ry, tilt, d in ((.39, .55, .13, .29, .10, 1.0), (.61, .55, .13, .29, -.10, 1.0),
                                     (.27, .17, .055, .07, .3, .55), (.73, .17, .055, .07, -.3, .55)):
        dx, dy = x - cx, y - cy
        u = dx * math.cos(tilt) - dy * math.sin(tilt)
        v = dx * math.sin(tilt) + dy * math.cos(tilt)
        # Toes taper towards the tip, so the radius narrows with positive v.
        taper = 1 - .45 * np.clip(v / ry, 0, 1)
        r = np.sqrt((u / (rx * taper)) ** 2 + (v / ry) ** 2) + wobble
        depth = np.maximum(depth, d * np.clip(1 - r, 0, 1) ** .6)
    depth = blur(depth, 3)
    rim = blur((depth > .02).astype(np.float32), 18) - (depth > .02)
    height = -depth + np.clip(rim, 0, 1) * .45
    mask = np.clip(blur((depth > .01).astype(np.float32), 45) * 2.6, 0, 1)
    wet = np.clip(depth * 1.4, 0, 1) * (.75 + .25 * noise(rng, y, x, ((17, 1.0),)))
    decal("D_Ishibashiri_Footprint_A", height, mask, wet, np.array((.11, .085, .06)))


def crack_decal():
    """Branching narrow black fissures with a faint dark halo and no emissive."""
    rng = np.random.default_rng(SEED + 4)
    size = DECAL_SIZE
    yy, xx = np.mgrid[0:size, 0:size].astype(np.float32)
    crack = np.zeros((size, size), np.float32)
    branches = [(size * .08, size * .5, 0.0, 9.0, 0)]
    while branches:
        px, py, heading, width, depth = branches.pop()
        course = heading
        for _ in range(int(rng.integers(36, 44)) if depth == 0 else int(rng.integers(8, 18))):
            # Pull back towards the initial course so fissures run across the ground instead of curling.
            heading += rng.normal(0, .3) + (course - heading) * .25
            nx, ny = px + math.cos(heading) * 22, py + math.sin(heading) * 22
            lo_x, hi_x = int(max(0, min(px, nx) - width - 2)), int(min(size, max(px, nx) + width + 2))
            lo_y, hi_y = int(max(0, min(py, ny) - width - 2)), int(min(size, max(py, ny) + width + 2))
            if lo_x >= hi_x or lo_y >= hi_y:
                break
            sx, sy = xx[lo_y:hi_y, lo_x:hi_x], yy[lo_y:hi_y, lo_x:hi_x]
            ex, ey = nx - px, ny - py
            t = np.clip(((sx - px) * ex + (sy - py) * ey) / (ex * ex + ey * ey), 0, 1)
            distance = np.hypot(sx - (px + t * ex), sy - (py + t * ey))
            crack[lo_y:hi_y, lo_x:hi_x] = np.maximum(crack[lo_y:hi_y, lo_x:hi_x], np.clip(1 - distance / width, 0, 1))
            if depth < 2 and rng.random() < .16:
                branches.append((nx, ny, heading + rng.choice((-1, 1)) * rng.uniform(.5, 1.1), width * .55, depth + 1))
            px, py, width = nx, ny, max(1.6, width * .975)
            if not (0 < px < size and 0 < py < size):
                break
    halo = blur(crack, 14)
    height = -blur(crack, 2) + halo * .2
    mask = np.clip(crack * 1.2 + halo * 1.8, 0, 1)
    wet = np.clip(crack * 1.3 + halo * .9, 0, 1)
    decal("D_Ishibashiri_CorruptionCrack_A", height, mask, wet, np.array((.06, .045, .05)))


def main():
    reports = []
    for builder in (ritual_post, old_rope, stepping_stone):
        reset()
        reports.append(export(builder()))
    reset()
    OUTPUT.mkdir(parents=True, exist_ok=True)
    footprint_decal()
    crack_decal()
    print("PROPKIT_PASS", len(reports), "meshes, 2 decals")


if __name__ == "__main__":
    main()
