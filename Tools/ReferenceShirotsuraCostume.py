"""Layered, weathered ritual costume authored from the supplied Shirotsura image.

Blender 3.6. Call build(m, mats) with the existing geometry helpers and palette.
The panels are folded, curved, perforated meshes with real thickness rather than
flat decal cards. No external assets, text fonts, or image dependencies are used.
"""
import math
import random

import bpy
from mathutils import Vector

TAU = math.tau


def _solid(ob, edge, thickness=.0015):
    if edge not in list(ob.data.materials):
        ob.data.materials.append(edge)
    modifier = ob.modifiers.new('Turned fabric edge', 'SOLIDIFY')
    modifier.thickness = thickness
    modifier.offset = 0.0
    modifier.material_offset = 0
    modifier.material_offset_rim = 1
    for poly in ob.data.polygons:
        poly.use_smooth = True
    return ob


def _strap(m, name, points, width, mat, thickness=.002):
    pts = [Vector(p) for p in points]
    vertices = []
    for i, point in enumerate(pts):
        tangent = pts[min(i + 1, len(pts) - 1)] - pts[max(0, i - 1)]
        cross = Vector((tangent.z, 0, -tangent.x)).normalized()
        vertices.extend([point - cross * width * .5, point + cross * width * .5])
    ob = m.mesh(name, vertices, [(i * 2, i * 2 + 1, i * 2 + 3, i * 2 + 2)
                                for i in range(len(pts) - 1)], mat)
    modifier = ob.modifiers.new('Supple leather thickness', 'SOLIDIFY')
    modifier.thickness = thickness
    bevel = ob.modifiers.new('Soft strap edges', 'BEVEL')
    bevel.width = .0007
    bevel.segments = 2
    return ob


def _buckle(m, center, angle, mats, name='Harness_buckle', size=.014):
    center = Vector(center)
    u = Vector((math.cos(angle), 0, math.sin(angle)))
    v = Vector((-math.sin(angle), 0, math.cos(angle)))
    points = [center + u * a * size + v * b * size * .8
              for a, b in [(-1, -1), (1, -1), (1, 1), (-1, 1), (-1, -1)]]
    m.curve(name, points, .0017, mats['metal'])
    m.tube(name + '_tongue', [center - u * size, center + u * size * .7],
           [.0011, .0008], mats['metal'], 6)


def _robe_panel(m, mats, index, theta, span, top, bottom, outer, tier):
    rng = random.Random(9137 + index * 107 + tier * 819)
    cols, rows = 14, 19
    phase = rng.uniform(-math.pi, math.pi)
    hem = []
    for j in range(cols + 1):
        # Alternating slender tears, not a row of equal triangular teeth.
        value = rng.uniform(-.014, .014)
        if j in (2, 7, 11):
            value += rng.uniform(.017, .052)
        if j in (4, 10):
            value -= rng.uniform(.017, .037)
        hem.append(value)
    holes = [(rng.uniform(.15, .85), rng.uniform(.77, .94),
              rng.uniform(.021, .052), rng.uniform(.019, .038))
             for _ in range(3 if tier == 0 else 2)]
    vertices = []
    for row in range(rows + 1):
        t = row / rows
        for col in range(cols + 1):
            u = col / cols
            angle = theta + (u - .5) * span * (.73 + .27 * t)
            z = top + (bottom + hem[col] - top) * t
            # Fold valleys begin at the gathered waist; crests relax toward hem.
            fold = (math.sin(u * math.pi * 5.0 + phase) * .009
                    + math.sin(u * math.pi * 11.0 + phase * .4) * .003)
            fold *= .45 + .55 * t
            flutter = math.sin(t * math.pi * 2.2 + u * 2.1 + phase) * .005 * t
            rx = .148 + outer + .128 * math.sin(t * math.pi * .58)
            ry = .105 + outer * .65 + .057 * t
            radius = fold + flutter
            x = (rx + radius) * math.sin(angle)
            y = -(ry + radius) * math.cos(angle)
            x += math.sin(t * 3.5 + phase) * .009 * t
            vertices.append((x, y, z))
    faces = []
    for row in range(rows):
        for col in range(cols):
            u = (col + .5) / cols
            t = (row + .5) / rows
            perforated = any(((u - hu) / hw) ** 2 + ((t - ht) / hh) ** 2 < 1
                             for hu, ht, hw, hh in holes)
            if perforated:
                continue
            a = row * (cols + 1) + col
            faces.append((a, a + 1, a + cols + 2, a + cols + 1))
    side = 'L' if math.sin(theta) >= 0 else 'R'
    name = 'Robe_panel_' + side + '_tier' + str(tier) + '_' + str(index)
    ob = m.mesh(name, vertices, faces, mats['cloth'])
    _solid(ob, mats['cloth_edge'], .0016)
    ob['garment_part'] = 'articulated robe panel'
    # A few visible running stitches and broken seams, laid on the fold mesh.
    for col in (2, cols - 2):
        for row in range(3 + index % 3, rows - 3, 3):
            point = Vector(vertices[row * (cols + 1) + col])
            point += Vector((math.sin(theta), -math.cos(theta), 0)) * .0017
            delta = Vector((math.cos(theta), math.sin(theta), .35)) * .0034
            m.tube(name + '_repair_stitch', [point - delta, point + delta],
                   [.00042, .00042], mats['cloth_edge'], 5)
    # Small forked abrasion lines on selected panels echo the cracked, worn
    # weave in the reference without covering the robe in bright decoration.
    if index % 2 == 0:
        for branch in range(2):
            row = 6 + branch * 5
            col = 4 + branch * 3
            trace = []
            for dr, dc in [(0, 0), (1, 1), (2, 0), (3, 1)]:
                p = Vector(vertices[(row + dr) * (cols + 1) + col + dc])
                p += Vector((math.sin(theta), -math.cos(theta), 0)) * .001
                trace.append(p)
            m.curve(name + '_worn_fiber', trace, .00045, mats['cloth_edge'])
    return ob


def _sleeve(m, mats, side, short=False):
    rng = random.Random(431 if side > 0 else 433)
    start = Vector((side * .165, .005, 1.337))
    end = Vector((side * (.274 if short else .282), -.005, 1.175 if short else 1.095))
    direction = (end - start).normalized()
    u = Vector((0, 1, 0))
    v = direction.cross(u).normalized()
    rings, segments = 14, 40
    vertices = []
    hem = [rng.uniform(-.012, .019) + (.02 if i % 7 == 0 else 0) for i in range(segments)]
    for row in range(rings + 1):
        t = row / rings
        radius = .072 + .028 * math.sin(t * math.pi * .73)
        for col in range(segments):
            angle = TAU * col / segments
            ridges = .009 * math.sin(angle * 8 + .6) + .003 * math.sin(angle * 17)
            center = start.lerp(end, t) + direction * hem[col] * t ** 5
            vertices.append(center + (u * math.cos(angle) + v * math.sin(angle))
                            * (radius + ridges * (.5 + .5 * t)))
    faces = []
    for row in range(rings):
        for col in range(segments):
            if row >= rings - 3 and (col + row * 3) % 23 in (0, 1):
                continue
            a = row * segments + col
            b = row * segments + (col + 1) % segments
            faces.append((a, b, b + segments, a + segments))
    label = 'L' if side > 0 else 'R'
    ob = m.mesh('Sleeve_' + label + '_torn_robe', vertices, faces, mats['cloth'])
    _solid(ob, mats['cloth_edge'], .0018)
    # Three staggered shoulder flaps are real curved shells above the sleeve.
    for layer in range(3):
        verts, faces = [], []
        for row in range(5):
            t = .08 + layer * .14 + row * .047
            for col in range(13):
                angle = -.85 + col * .22
                radius = .078 + layer * .009 + .006 * math.sin(col * 1.8)
                center = start.lerp(end, t)
                center += direction * ((.009 if col % 3 == 0 else -.005) if row == 4 else 0)
                verts.append(center + (u * math.cos(angle) + v * math.sin(angle)) * radius)
        for row in range(4):
            for col in range(12):
                a = row * 13 + col
                faces.append((a, a + 1, a + 14, a + 13))
        flap = m.mesh('Sleeve_' + label + '_shoulder_layer_' + str(layer), verts, faces, mats['cloth'])
        _solid(flap, mats['cloth_edge'], .0013)
    return ob


def _ofuda(m, mats, name, points, width, seed):
    rng = random.Random(seed)
    pts = [Vector(p) for p in points]
    vertices = []
    for i, point in enumerate(pts):
        half = width * (.49 + rng.uniform(-.045, .045))
        vertices.extend([point + Vector((-half, 0, 0)),
                         point + Vector((0, -.0017, .0008)),
                         point + Vector((half, .001, rng.uniform(-.003, .003)))])
    faces = []
    for i in range(len(pts) - 1):
        for j in range(2):
            a = i * 3 + j
            faces.append((a, a + 1, a + 4, a + 3))
    ob = m.mesh(name, vertices, faces, mats['paper'])
    _solid(ob, mats['linen'], .00055)
    # Authored talisman-like brush strokes, intentionally decorative rather
    # than garbled font text. Each cluster follows the bent paper's surface.
    for i in range(1, len(pts) - 1):
        point = pts[i] + Vector((0, -.0029, 0))
        unit = width * .23
        patterns = [
            [(-.8, .5), (.8, .35)], [(0, .95), (-.08, -.9)],
            [(-.75, -.2), (.6, -.4)], [(-.2, .2), (-.8, -.75)],
            [(.1, .1), (.68, -.8)],
        ]
        for j, pattern in enumerate(patterns):
            if j == 2 and (i + seed) % 2:
                continue
            coords = [point + Vector((x * unit, 0, z * unit)) for x, z in pattern]
            m.tube(name + '_red_ink', coords, [.00052, .00035], mats['red'], 5)
    return ob


def _tassel(m, mats, name, center, length, radius, material, seed):
    rng = random.Random(seed)
    center = Vector(center)
    for ring in range(3):
        r = radius * (.48 + ring * .18)
        points = [center + Vector((r * math.cos(a * TAU / 12),
                                   r * math.sin(a * TAU / 12), -.003 * ring))
                  for a in range(13)]
        m.curve(name + '_bound_neck', points, .0016, material)
    for i in range(28):
        angle = i * TAU / 28
        radial = Vector((math.cos(angle), math.sin(angle), 0))
        tip = center + radial * radius * rng.uniform(.7, 1.5)
        tip.z -= length * rng.uniform(.88, 1.09)
        points = [center + radial * radius * .6,
                  center + radial * radius * .82 + Vector((.004, 0, -length * .5)), tip]
        m.curve(name + '_thread', points, .00105, material)


def _bell(m, mats, center, scale, name):
    c = Vector(center)
    # Lathed bronze shell with a genuinely open underside and rolled lip.
    profile = [(.12, .008), (.38, .004), (.65, -.005), (.82, -.020),
               (.78, -.034), (.57, -.043), (.47, -.044)]
    vertices = []
    for radius, z in profile:
        for j in range(32):
            a = TAU * j / 32
            vertices.append(c + Vector((math.cos(a) * radius * scale,
                                        math.sin(a) * radius * scale, z * scale / .032)))
    faces = [tuple(reversed(range(32)))]
    for row in range(len(profile) - 1):
        for j in range(32):
            a = row * 32 + j
            b = row * 32 + (j + 1) % 32
            faces.append((a, b, b + 32, a + 32))
    ob = m.mesh(name, vertices, faces, mats['gold'])
    for p in ob.data.polygons:
        p.use_smooth = True
    modifier = ob.modifiers.new('Bell shell thickness', 'SOLIDIFY')
    modifier.thickness = .0015
    m.ico(name + '_clapper', c + Vector((0, 0, -.039 * scale / .032)),
          (.005, .005, .010), mats['metal'], 2)
    loop = [c + Vector((math.cos(a * TAU / 16) * .005, 0,
                        .012 + math.sin(a * TAU / 16) * .007)) for a in range(17)]
    m.curve(name + '_hanging_loop', loop, .0017, mats['gold'])
    # Dark inset slots read as aged ritual bells at gameplay distance.
    for sign in (-1, 1):
        m.ico(name + '_sound_hole', c + Vector((sign * scale * .32, -scale * .70, -.012)),
              (.0023, .0007, .0032), mats['leather'], 2)


def build(m, mats):
    """Build the reference robe, harness, layered skirt and ritual waist trim."""
    before = set(bpy.context.scene.objects)
    torso = m.loft('Work_jacket', [
        (0, .004, .956, .140, .093), (0, .002, 1.008, .144, .098),
        (0, 0, 1.075, .154, .106), (0, .002, 1.145, .166, .112),
        (0, .003, 1.215, .181, .113), (0, .009, 1.275, .191, .109),
        (0, .013, 1.323, .172, .094), (0, .011, 1.355, .119, .078),
        (0, .008, 1.382, .078, .066),
    ], mats['cloth'], n=64)
    for vertex in torso.data.vertices:
        p = vertex.co
        angle = math.atan2(p.y, p.x)
        fold = math.sin(angle * 11 + p.z * 16) * .004 + math.sin(angle * 19 - p.z * 24) * .002
        p.x += math.cos(angle) * fold
        p.y += math.sin(angle) * fold
    for poly in torso.data.polygons:
        poly.use_smooth = True
    bevel = torso.modifiers.new('Soft robe shell', 'BEVEL')
    bevel.width = .0015
    bevel.segments = 2
    _sleeve(m, mats, -1)
    _sleeve(m, mats, 1, short=True)

    # Crossing inner lapels remain visible behind the leather chest harness.
    for side in (-1, 1):
        points = [(side * .078, -.077, 1.355), (side * .040, -.113, 1.27),
                  (-side * .019, -.122, 1.17), (-side * .095, -.114, 1.05),
                  (-side * .127, -.102, .985)]
        _strap(m, 'Chest_robe_crossed_lapel', points, .025, mats['cloth_edge'], .0018)

    # Each full-length sector has its own hem, folds and tears; shorter tiers
    # overlap the waist gathers and cover the panel attachment line.
    for index in range(12):
        theta = (index + .5) * TAU / 12
        front = math.cos(theta)
        bottom = .30 + .038 * math.sin(index * 2.1) + max(0, front) * .025
        _robe_panel(m, mats, index, theta, .505 if index in (0, 11) else .56,
                    .956, bottom, .008, 0)
    for index in range(10):
        theta = (index + .5) * TAU / 10
        bottom = .47 + .085 * math.sin(index * 1.8 + .4)
        _robe_panel(m, mats, index, theta, .58 if index in (0, 9) else .67,
                    .970, bottom, .022, 1)
    for index in range(8):
        theta = (index + .5) * TAU / 8
        bottom = .70 + .045 * math.sin(index * 2.3)
        _robe_panel(m, mats, index, theta, .69 if index in (0, 7) else .82,
                    .992, bottom, .033, 2)

    # Fitted shoulder-to-waist leather straps with a clear central overlap.
    for side in (-1, 1):
        points = [(side * .142, -.080, 1.346), (side * .108, -.119, 1.300),
                  (side * .054, -.134, 1.225), (0, -.143 - (side + 1) * .003, 1.160),
                  (-side * .063, -.128, 1.080), (-side * .126, -.111, 1.011)]
        _strap(m, 'Harness_cross_leather_' + str(side), points, .025, mats['leather'], .0035)
        for idx in (1, 2, 4):
            p = Vector(points[idx]) + Vector((0, -.0048, 0))
            if idx == 2:
                _buckle(m, p, side * .86, mats, size=.014)
            else:
                m.ico('Harness_bronze_rivet', p, (.0026, .0011, .0026), mats['metal'], 2)
        back = [(side * .142, -.08, 1.346), (side * .145, .048, 1.341),
                (side * .100, .111, 1.267), (-side * .100, .115, 1.038)]
        _strap(m, 'Harness_rear_leather_' + str(side), back, .023, mats['leather'], .003)

    # Two shoulder strips of folded shrine paper show below the scarf.
    for side in (-1, 1):
        for tier in range(4):
            x = side * (.114 + tier * .008)
            z = 1.327 - tier * .036
            _ofuda(m, mats, 'Chest_ofuda_' + str(side) + '_' + str(tier),
                   [(x, -.110 - tier * .005, z),
                    (x + side * .006, -.125 - tier * .005, z - .030),
                    (x + side * .012, -.128 - tier * .005, z - .060)],
                   .032, 31 + tier)

    # Broad folded obi under three asymmetric twisted hemp rope passes.
    obi = m.loft('Waist_wrapped_obi', [(0, 0, .961, .165, .121),
                                       (0, 0, .985, .164, .121),
                                       (0, 0, 1.011, .159, .115),
                                       (0, 0, 1.025, .155, .109)], mats['linen'], n=64, folds=.012)
    for poly in obi.data.polygons:
        poly.use_smooth = True
    for level in range(3):
        points = []
        for j in range(65):
            a = TAU * j / 64
            points.append((.169 * math.sin(a), -.124 * math.cos(a),
                           .965 + level * .019 + .0045 * math.sin(a * 2 + level)))
        m.rope('Waist_hemp_rope_' + str(level), points, .0085, mats['linen'], turns=51)
    # Interwoven central knot: three crossing rope loops rather than a sphere.
    for i, z in enumerate((.975, .988)):
        points = [(-.055, -.137, z), (-.034, -.160, z + .013),
                  (.002, -.163, z - .009), (.035, -.149, z + .010),
                  (.024, -.172, z + .022), (-.012, -.166, z + .018),
                  (-.039, -.154, z - .004), (.006, -.170, z - .010)]
        m.rope('Waist_central_knot_' + str(i), points, .009, mats['linen'], turns=15)

    # Red shrine cords and an asymmetrical pair of hanging tassels.
    for offset in range(2):
        x = .163 + offset * .010
        y = -.119 - offset * .009
        pts = [(x - .02, y, 1.002), (x + .025, y - .006, .989),
               (x + .035, y, .951), (x + .004, y - .005, .938),
               (x - .017, y - .014, .969), (x + .012, y - .017, .984),
               (x + .029, y - .022, .964), (x + .054, y - .009, .86 - offset * .03),
               (x + .075, y + .006, .77 - offset * .06)]
        m.rope('Waist_red_braided_knot', pts, .0048, mats['red'], turns=17)
        _tassel(m, mats, 'Waist_red_tassel_' + str(offset), pts[-1],
                .080 + offset * .018, .009, mats['red'], 715 + offset)
    _tassel(m, mats, 'Waist_undyed_prayer_tassel', (.185, -.132, .856),
            .119, .011, mats['linen'], 671)

    # Hanging folded ofuda descend with the long robe instead of floating flat.
    for i, (x, end, width) in enumerate([(-.085, .735, .032), (-.123, .784, .028),
                                        (.088, .567, .037), (.121, .678, .031)]):
        points = []
        for j in range(8):
            t = j / 7
            z = .962 + (end - .962) * t
            points.append((x + math.sin(t * 3 + i) * .012 * t,
                           -.149 - t * .046 + .0035 * math.sin(t * 12 + i), z))
        _ofuda(m, mats, 'Waist_ofuda_strip_' + str(i), points, width, 771 + i)
    for i, (center, scale) in enumerate([((.160, -.169, .927), .027),
                                        ((.175, -.185, .872), .031)]):
        m.rope('Waist_bell_suspension_' + str(i),
               [(center[0] - .006, center[1] + .025, .978),
                (center[0], center[1], center[2] + .021)], .003, mats['linen'], turns=6)
        _bell(m, mats, center, scale, 'Waist_bronze_bell_' + str(i))

    added = [ob for ob in bpy.context.scene.objects if ob not in before]
    for ob in added:
        ob['reference_remake'] = 'layered ritual robe'
        name = ob.name.lower()
        if name.startswith('robe_panel_'):
            ob['rig_region'] = 'robe_deform'
            ob['rig_side'] = 'L' if name.startswith('robe_panel_l_') else 'R'
        elif name.startswith('waist_'):
            ob['rig_region'] = 'pelvis'
        elif name.startswith('sleeve_'):
            ob['rig_region'] = 'upperarm_L' if name.startswith('sleeve_l_') else 'upperarm_R'
        else:
            ob['rig_region'] = 'torso_deform'
    print('REFERENCE_COSTUME_PASS', len(added))
    return added
