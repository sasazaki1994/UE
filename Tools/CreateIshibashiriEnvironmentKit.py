"""Deterministic, free Blender candidates for the Ishibashiri first adoption batch.

Run: blender --background --factory-startup --python Tools/CreateIshibashiriEnvironmentKit.py
No network service, downloaded texture, paid API, or Marketplace asset is used.
"""
import json
import math
import random
from pathlib import Path

import bpy
from mathutils import Vector

ROOT = Path(__file__).resolve().parents[1]
MANIFEST = ROOT / "Art/Environment/Ishibashiri/manifest.json"
OUTPUT = MANIFEST.parent / "Generated"
SEED = 830194
ASSET_IDS = (
    "SM_Ishibashiri_OldCedar_A",
    "SM_Ishibashiri_Rock_A",
    "SM_Ishibashiri_BoundaryStone_A",
)
MATERIAL_NOTE = "PROCEDURAL MATERIAL — UE FINAL MATERIAL / BAKE REQUIRED"
PREVIEW_PREFIX = "PREVIEW_"
COLLISION_PREFIX = "UCX_"


def reset(seed):
    random.seed(seed)
    bpy.ops.object.select_all(action="SELECT")
    bpy.ops.object.delete(use_global=False)
    for datablocks in (bpy.data.materials, bpy.data.curves, bpy.data.meshes):
        for block in list(datablocks):
            datablocks.remove(block)
    scene = bpy.context.scene
    scene.unit_settings.system = "METRIC"
    scene.unit_settings.scale_length = 1.0
    scene.render.engine = "BLENDER_EEVEE_NEXT" if bpy.app.version >= (4, 2, 0) else "BLENDER_EEVEE"
    scene.render.resolution_x = scene.render.resolution_y = 900
    scene.render.resolution_percentage = 100
    scene.render.image_settings.file_format = "PNG"
    scene.world.color = (0.055, 0.06, 0.065)


def procedural_material(name, base, roughness, noise_scale, bump_strength=0.18):
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    bsdf = nodes.get("Principled BSDF")
    bsdf.inputs["Base Color"].default_value = (*base, 1)
    bsdf.inputs["Roughness"].default_value = roughness
    noise = nodes.new("ShaderNodeTexNoise")
    noise.inputs["Scale"].default_value = noise_scale
    noise.inputs["Detail"].default_value = 5
    ramp = nodes.new("ShaderNodeValToRGB")
    ramp.color_ramp.elements[0].color = (*tuple(c * 0.45 for c in base), 1)
    ramp.color_ramp.elements[1].color = (*tuple(min(1, c * 1.35) for c in base), 1)
    bump = nodes.new("ShaderNodeBump")
    bump.inputs["Strength"].default_value = bump_strength
    links.new(noise.outputs["Fac"], ramp.inputs["Fac"])
    links.new(ramp.outputs["Color"], bsdf.inputs["Base Color"])
    links.new(noise.outputs["Fac"], bump.inputs["Height"])
    links.new(bump.outputs["Normal"], bsdf.inputs["Normal"])
    mat["export_note"] = MATERIAL_NOTE
    return mat


def emission_material(name, colour):
    """Unlit material for review annotations that must remain legible headlessly."""
    mat = bpy.data.materials.new(name)
    mat.use_nodes = True
    nodes, links = mat.node_tree.nodes, mat.node_tree.links
    for node in list(nodes):
        nodes.remove(node)
    output = nodes.new("ShaderNodeOutputMaterial")
    emission = nodes.new("ShaderNodeEmission")
    emission.inputs["Color"].default_value = (*colour, 1)
    emission.inputs["Strength"].default_value = 1.0
    links.new(emission.outputs["Emission"], output.inputs["Surface"])
    return mat


def mesh_object(name, verts, faces, material):
    mesh = bpy.data.meshes.new(name)
    mesh.from_pydata(verts, [], faces)
    mesh.update()
    obj = bpy.data.objects.new(name, mesh)
    bpy.context.collection.objects.link(obj)
    obj.data.materials.append(material)
    return obj


def tapered_tube(name, points, radii, material, sides=16):
    points = [Vector(p) for p in points]
    verts = []
    for i, point in enumerate(points):
        tangent = (points[min(i + 1, len(points)-1)] - points[max(0, i-1)]).normalized()
        axis = Vector((0, 1, 0)) if abs(tangent.y) < .85 else Vector((1, 0, 0))
        u, v = tangent.cross(axis).normalized(), tangent.cross(tangent.cross(axis).normalized()).normalized()
        for side in range(sides):
            angle = 2 * math.pi * side / sides + i * .09
            irregular = 1 + .045 * math.sin(side * 5.0 + i * .71)
            verts.append(point + radii[i] * irregular * (u*math.cos(angle) + v*math.sin(angle)))
    faces = [tuple(reversed(range(sides)))]
    for ring in range(len(points)-1):
        for side in range(sides):
            a = ring*sides + side
            faces.append((a, ring*sides+(side+1)%sides,
                          (ring+1)*sides+(side+1)%sides, a+sides))
    faces.append(tuple((len(points)-1)*sides+i for i in range(sides)))
    return mesh_object(name, verts, faces, material)


def old_cedar(mats):
    height = 21.0
    rings, sides = 105, 48
    points, radii = [], []
    for i in range(rings):
        t = i / (rings-1)
        flare = .22 * math.exp(-t*22)
        points.append((.15*math.sin(t*2.5)+.10*t*t, .09*math.sin(t*4.1), height*t))
        radii.append((.47*(1-t)**.72 + .055) + flare)
    visual = [tapered_tube("Cedar_Trunk", points, radii, mats["Bark"], sides)]
    for branch in range(11):
        z = 8.1 + branch*1.02
        angle = branch*2.399 + .25
        length = 1.78 - branch*.045 + random.uniform(-.12, .12)
        origin = Vector((.15*math.sin(z/height*2.5), .09*math.sin(z/height*4.1), z))
        direction = Vector((math.cos(angle), math.sin(angle), .20 + branch*.018)).normalized()
        pts = [origin + direction*length*(j/14) + Vector((0, 0, -.18*(j/14)**2)) for j in range(15)]
        visual.append(tapered_tube("Cedar_Branch_%02d" % branch, pts,
                                   [.15*(1-j/15)+.022 for j in range(15)], mats["Bark"], 16))
        for cluster in range(1 if branch < 3 else 2):
            location = pts[-1] + Vector((0, 0, .25*cluster))
            bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=3, radius=1, location=location)
            leaf = bpy.context.object
            leaf.name = "Cedar_Foliage_%02d_%d" % (branch, cluster)
            leaf.scale = (.68, .48, .62)
            leaf.rotation_euler[2] = angle
            bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
            for vertex in leaf.data.vertices:
                vertex.co *= 1 + .10*math.sin(vertex.index*1.73)
            leaf.data.materials.append(mats["Foliage"])
            visual.append(leaf)
    # Root flare lobes are geometry and remain bark-only.
    for root in range(6):
        a = root*math.pi/3 + .2
        pts = [(0, 0, .14), (.55*math.cos(a), .55*math.sin(a), .08),
               (1.05*math.cos(a), 1.05*math.sin(a), .015)]
        visual.append(tapered_tube("Cedar_Root_%02d" % root, pts, [.25, .16, .035], mats["Bark"], 12))
    bpy.ops.mesh.primitive_cylinder_add(vertices=12, radius=.48, depth=20.0, location=(0, 0, 10.0))
    collision = bpy.context.object
    collision.name = "UCX_SM_Ishibashiri_OldCedar_A_00"
    return visual, collision


def rock(mats):
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=5, radius=1)
    obj = bpy.context.object
    obj.name = "Rock_Faceted"
    for vertex in obj.data.vertices:
        p = vertex.co.normalized()
        # Quantized low-frequency displacement creates planes; deterministic chips affect upper edges.
        n = math.sin(p.x*7.1) + math.sin(p.y*9.3+1.2) + math.sin(p.z*6.7-.8)
        displacement = round(n*2.2)/22.0
        chip = .82 if p.z > .45 and p.x + p.y > .72 else 1.0
        vertex.co *= (1 + displacement) * chip
        vertex.co.x *= 1.78
        vertex.co.y *= 1.31
        vertex.co.z *= 1.12
        if vertex.co.z < -.72:
            vertex.co.z = -.72 + (vertex.co.z+.72)*.12
    obj.location.z = .72
    obj.data.materials.append(mats["WetRock"])
    # Bake the ground placement into vertices so the exported asset origin remains at Z=0.
    bpy.context.view_layer.objects.active = obj
    obj.select_set(True)
    bpy.ops.object.transform_apply(location=True, rotation=False, scale=False)
    bpy.ops.mesh.primitive_ico_sphere_add(subdivisions=2, radius=1, location=(0, 0, .70))
    collision = bpy.context.object
    collision.name = "UCX_SM_Ishibashiri_Rock_A_00"
    collision.scale = (1.64, 1.18, .98)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return [obj], collision


def boundary_stone(mats):
    # Six independently gridded faces make erosion/chips possible without a beveled-cube shortcut.
    verts, faces, n = [], [], 35
    lean = math.radians(6.2)
    def point(face, u, v):
        x, y, z = 0.0, 0.0, 0.0
        if face < 2:
            x = (-.28 if face == 0 else .28); y=(u-.5)*.52; z=v*1.94
        elif face < 4:
            y = (-.26 if face == 2 else .26); x=(u-.5)*.56; z=v*1.94
        else:
            z = 0 if face == 4 else 1.94; x=(u-.5)*.56; y=(v-.5)*.52
        crown_chip = .10*max(0, (z-1.72)/.22) * (1 if x > .05 else .35)
        z -= crown_chip
        erosion = .012*math.sin((u*17 + v*13 + face)*2.1) + .007*math.sin(v*43+face)
        x += erosion + math.tan(lean)*z
        y += .007*math.sin(u*31 + v*11)
        # Slightly buried-looking broad base.
        if z < .22: x *= 1.0 + .11*(1-z/.22)
        return (x, y, z)
    for face in range(6):
        base = len(verts)
        for j in range(n+1):
            for i in range(n+1): verts.append(point(face, i/n, j/n))
        for j in range(n):
            for i in range(n):
                a = base+j*(n+1)+i
                faces.extend(((a,a+1,a+n+2), (a,a+n+2,a+n+1)))
    obj = mesh_object("BoundaryStone_Weathered", verts, faces, mats["BoundaryStone"])
    bpy.ops.mesh.primitive_cube_add(size=1, location=(math.tan(lean)*.92, 0, .92))
    collision = bpy.context.object
    collision.name = "UCX_SM_Ishibashiri_BoundaryStone_A_00"
    collision.dimensions = (.58, .54, 1.84)
    collision.rotation_euler[1] = lean
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    return [obj], collision


def geometry_metrics(objects):
    """Measure authored vertices; object bound_box can be stale after direct mesh edits."""
    points = [obj.matrix_world @ vertex.co for obj in objects for vertex in obj.data.vertices]
    mins = [min(p[i] for p in points) for i in range(3)]
    maxs = [max(p[i] for p in points) for i in range(3)]
    vertices = triangles = 0
    for obj in objects:
        obj.data.update()
        vertices += len(obj.data.vertices)
        obj.data.calc_loop_triangles()
        triangles += len(obj.data.loop_triangles)
    return [round((maxs[i]-mins[i])*100, 3) for i in range(3)], vertices, triangles


def world_bounds(objects):
    points = [obj.matrix_world @ Vector(corner) for obj in objects for corner in obj.bound_box]
    return ([min(point[axis] for point in points) for axis in range(3)],
            [max(point[axis] for point in points) for axis in range(3)])


def set_preview_visibility(visual):
    """Keep collision in the scene/exports, but never in human-review renders."""
    visual_ids = {id(obj) for obj in visual}
    for obj in bpy.context.scene.objects:
        if obj.name.startswith(COLLISION_PREFIX):
            obj.hide_render = True
        elif id(obj) in visual_ids:
            obj.hide_render = False


def preview(path, visual, dimensions):
    set_preview_visibility(visual)
    lower, upper = world_bounds(visual)
    width = upper[0] - lower[0]
    gray = procedural_material("PreviewGround", (.22, .23, .24), .95, 3)
    ground_size = max(max(dimensions) / 100 * 3.0, width + 4.0,
                      (upper[1] - lower[1]) + 4.0, 5.0)
    bpy.ops.mesh.primitive_plane_add(size=ground_size, location=(0, 0, -.012))
    ground = bpy.context.object; ground.name = "PREVIEW_Ground"; ground.data.materials.append(gray)
    # A neutral 172 cm capsule makes scale legible in the artifact without being
    # selected for (or included in) either production export.
    human_x = lower[0] - .62
    bpy.ops.mesh.primitive_uv_sphere_add(segments=24, ring_count=12, location=(human_x, 0, .86))
    human = bpy.context.object; human.name = "PREVIEW_HumanScale_172cm"
    human.scale = (.24, .24, .86)
    bpy.ops.object.transform_apply(location=False, rotation=False, scale=True)
    human.data.materials.append(procedural_material("HumanScale", (.32, .36, .40), .72, 2, 0))
    review_objects = visual + [human]
    review_lower, review_upper = world_bounds(review_objects)
    target = Vector(tuple((review_lower[i] + review_upper[i]) * .5 for i in range(3)))
    # The AABB diagonal is a conservative fit for the projected 3/4 silhouette.
    span = (Vector(review_upper) - Vector(review_lower)).length
    distance = max(span * 1.8, 6.0)
    view_direction = Vector((.78, -1.0, .34)).normalized()
    bpy.ops.object.camera_add(location=target + view_direction * distance)
    camera = bpy.context.object; camera.name = "PREVIEW_Camera"
    camera.rotation_euler = (target-camera.location).to_track_quat("-Z", "Y").to_euler()
    camera.data.type = "ORTHO"
    # A conservative 15% review margin also protects the scale reference from cropping.
    camera.data.ortho_scale = span * 1.30
    bpy.context.scene.camera = camera
    scene = bpy.context.scene
    scene.world.use_nodes = True
    scene.world.node_tree.nodes["Background"].inputs["Color"].default_value = (.12, .13, .15, 1)
    scene.world.node_tree.nodes["Background"].inputs["Strength"].default_value = .65
    if hasattr(scene, "eevee"):
        scene.eevee.use_gtao = True
        scene.eevee.gtao_distance = 3
        scene.eevee.gtao_factor = 1.15
    for index, (location, energy, size) in enumerate((
            ((-distance*.65, -distance*.55, target.z + distance*.85), 1250, distance*.7),
            ((distance*.55, -distance*.25, target.z + distance*.35), 700, distance*.55))):
        bpy.ops.object.light_add(type="AREA", location=location)
        light=bpy.context.object; light.name="PREVIEW_Light_%02d" % index; light.data.energy=energy; light.data.size=size
        light.rotation_euler=(target-light.location).to_track_quat("-Z","Y").to_euler()
    bpy.context.scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)


def contact_sheet(entries):
    """Render the three already-rendered previews into one dependency-free sheet."""
    reset(SEED)
    scene = bpy.context.scene
    scene.render.engine = "BLENDER_EEVEE"
    scene.render.resolution_x, scene.render.resolution_y = 1800, 720
    scene.render.resolution_percentage = 100
    scene.world.color = (.018, .022, .028)
    for index, (asset, preview_path, dimensions, triangles) in enumerate(entries):
        x = (index - 1) * 6.0
        bpy.ops.mesh.primitive_plane_add(size=2, location=(x, 0, 0.65), rotation=(math.pi/2, 0, 0))
        panel = bpy.context.object
        panel.name = "CONTACT_Panel_%02d" % index
        panel.scale = (2.65, 2.65, 1)
        mat = bpy.data.materials.new("Contact_" + asset); mat.use_nodes = True
        nodes = mat.node_tree.nodes; links = mat.node_tree.links
        for node in list(nodes): nodes.remove(node)
        output = nodes.new("ShaderNodeOutputMaterial"); emission = nodes.new("ShaderNodeEmission")
        emission.inputs["Strength"].default_value = 1.0
        texture = nodes.new("ShaderNodeTexImage"); texture.image = bpy.data.images.load(str(preview_path), check_existing=False)
        links.new(texture.outputs["Color"], emission.inputs["Color"]); links.new(emission.outputs["Emission"], output.inputs["Surface"])
        panel.data.materials.append(mat)
        bpy.ops.object.text_add(location=(x-2.7, -.02, -2.35), rotation=(math.pi/2, 0, 0))
        label = bpy.context.object
        label.data.body = "%s\nDimensions: %.1f x %.1f x %.1f cm\nTriangles: %s" % (asset, *dimensions, format(triangles, ","))
        label.data.align_x = "LEFT"; label.data.size = .25; label.data.space_line = 1.05
        label.data.materials.append(emission_material("Label_" + str(index), (.75, .88, .92)))
    bpy.ops.object.camera_add(location=(0, -18, 0))
    camera = bpy.context.object; camera.rotation_euler = (math.pi/2, 0, 0)
    camera.data.type = "ORTHO"; camera.data.ortho_scale = 7.2
    scene.camera = camera
    scene.render.filepath = str(OUTPUT / "IshibashiriEnvironment_FirstBatch_ContactSheet.png")
    bpy.ops.render.render(write_still=True)


def generate(asset, builder, manifest_asset):
    seed = SEED + ASSET_IDS.index(asset)
    reset(seed)
    mats = {
        "Bark": procedural_material("Bark", (.16,.105,.065), .88, 24, .32),
        "Foliage": procedural_material("Foliage", (.055,.12,.065), .82, 8, .12),
        "WetRock": procedural_material("WetRock", (.105,.12,.125), .58, 5, .28),
        "BoundaryStone": procedural_material("BoundaryStone", (.19,.20,.19), .76, 7, .25),
        "Moss": procedural_material("Moss", (.09,.14,.065), .95, 11, .2),
    }
    visual, collision = builder(mats)
    dimensions, vertices, triangles = geometry_metrics(visual)
    collision_dimensions, collision_vertices, collision_triangles = geometry_metrics([collision])
    directory = OUTPUT / asset.removeprefix("SM_Ishibashiri_")
    directory.mkdir(parents=True, exist_ok=True)
    # Production exports intentionally include collision and are completed before
    # any PREVIEW_ helper is created.
    selected = visual + [collision]
    if any(obj.name.startswith(PREVIEW_PREFIX) for obj in selected):
        raise RuntimeError("preview-only object entered production export selection")
    bpy.ops.object.select_all(action="DESELECT")
    for obj in selected: obj.select_set(True)
    bpy.context.view_layer.objects.active = visual[0]
    bpy.ops.export_scene.gltf(filepath=str(directory/(asset+".glb")), use_selection=True, export_format="GLB")
    bpy.ops.export_scene.fbx(filepath=str(directory/(asset+".fbx")), use_selection=True,
        object_types={"MESH"}, axis_forward="-Y", axis_up="Z", apply_unit_scale=True,
        add_leaf_bones=False, bake_anim=False)
    report = {
        "asset_id": asset, "generator": "Blender procedural", "blender_version": bpy.app.version_string,
        "seed": seed, "dimensions_cm": {"x": dimensions[0], "y": dimensions[1], "z": dimensions[2]},
        "vertex_count": vertices, "triangle_count": triangles, "mesh_object_count": len(visual),
        "visual_object_names": [obj.name for obj in visual],
        "collision": {"name": collision.name, "dimensions_cm": collision_dimensions,
                      "vertex_count": collision_vertices, "triangle_count": collision_triangles},
        "material_count": len({slot.material.name for obj in visual for slot in obj.material_slots}),
        "material_note": MATERIAL_NOTE, "lod_status": {"LOD0":"GENERATED", "LOD1":"NOT_GENERATED", "LOD2":"NOT_GENERATED"},
        "collision_status": "GENERATED: " + collision.name, "pivot": manifest_asset["pivot_policy"],
        "unit": "meter (UE import centimeters)", "forward_axis": "-Y", "generation_status": "BLENDER PRODUCTION CANDIDATE",
        "ue_import_status": "NOT_RUN", "ue_visual_review_status": "NOT_RUN",
    }
    (directory/(asset+"_report.json")).write_text(json.dumps(report, indent=2, ensure_ascii=False)+"\n", encoding="utf-8")
    preview(directory/(asset+"_preview.png"), visual, dimensions)


def main():
    manifest = json.loads(MANIFEST.read_text(encoding="utf-8"))
    specs = {a["asset_id"]: a for a in manifest["assets"]}
    builders = (old_cedar, rock, boundary_stone)
    entries = []
    for asset, builder in zip(ASSET_IDS, builders):
        generate(asset, builder, specs[asset])
        directory = OUTPUT / asset.removeprefix("SM_Ishibashiri_")
        report = json.loads((directory/(asset+"_report.json")).read_text(encoding="utf-8"))
        entries.append((asset, directory/(asset+"_preview.png"),
                        list(report["dimensions_cm"].values()), report["triangle_count"]))
    contact_sheet(entries)
    print("Generated exactly three BLENDER PRODUCTION CANDIDATE assets in", OUTPUT)


if __name__ == "__main__":
    main()
