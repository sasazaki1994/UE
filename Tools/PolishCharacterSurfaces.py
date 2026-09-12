"""Deterministic, bakeable material detail for the folklore characters (Blender 3.6).

Call polish_materials(mats, hero) after the original refinement's material pass.
Only existing materials change. All detail feeds Base Color, Roughness or Normal,
so the existing Cycles texture-atlas bake carries it into the Unreal assets.
"""


class _Surface:
    """Small node authoring helper; positions stay stable when meshes are joined."""

    def __init__(self, material):
        self.material = material
        self.tree = material.node_tree
        self.shader = self.tree.nodes.get('Principled BSDF')
        self.base = tuple(self.shader.inputs['Base Color'].default_value)[:3]
        # This pass owns the generated surface inputs. Retain the actual shader
        # (metalness/emission) and output but remove superseded texture networks.
        for node in list(self.tree.nodes):
            if node != self.shader and node.type != 'OUTPUT_MATERIAL':
                self.tree.nodes.remove(node)
        self.position = self.node('ShaderNodeNewGeometry', 'Stable world position').outputs['Position']
        self.shader.location = (720, 60)

    def node(self, kind, label):
        result = self.tree.nodes.new(kind)
        result.name = 'SurfacePolish_' + label
        result.label = label
        return result

    def link(self, output, input_socket):
        self.tree.links.new(output, input_socket)

    def vector(self, scale):
        node = self.node('ShaderNodeVectorMath', 'Directional grain ' + str(scale))
        node.operation = 'MULTIPLY'
        self.link(self.position, node.inputs[0])
        node.inputs[1].default_value = scale
        return node.outputs['Vector']

    def noise(self, scale, detail=2.0, vector=None, roughness=.55):
        node = self.node('ShaderNodeTexNoise', 'Grain ' + str(scale))
        node.inputs['Scale'].default_value = scale
        node.inputs['Detail'].default_value = detail
        node.inputs['Roughness'].default_value = roughness
        self.link(vector if vector is not None else self.position, node.inputs['Vector'])
        return node.outputs['Fac']

    def wave(self, scale, direction='Z', distortion=0.0, vector=None):
        node = self.node('ShaderNodeTexWave', 'Natural bands ' + direction)
        node.wave_type = 'BANDS'
        node.bands_direction = direction
        node.wave_profile = 'SIN'
        node.inputs['Scale'].default_value = scale
        node.inputs['Distortion'].default_value = distortion
        node.inputs['Detail'].default_value = 2.0
        node.inputs['Detail Scale'].default_value = .7
        node.inputs['Detail Roughness'].default_value = .6
        self.link(vector if vector is not None else self.position, node.inputs['Vector'])
        return node.outputs['Fac']

    def math(self, operation, first, second):
        node = self.node('ShaderNodeMath', operation.title())
        node.operation = operation
        for socket, value in zip(node.inputs, (first, second)):
            if isinstance(value, (int, float)):
                socket.default_value = value
            else:
                self.link(value, socket)
        return node.outputs[0]

    def mix(self, first, second, amount):
        return self.math('ADD', self.math('MULTIPLY', first, 1.0 - amount),
                         self.math('MULTIPLY', second, amount))

    def ramp(self, source, stops, label):
        node = self.node('ShaderNodeValToRGB', label)
        node.color_ramp.interpolation = 'EASE'
        for i, (position, color) in enumerate(stops):
            element = node.color_ramp.elements[i] if i < 2 else node.color_ramp.elements.new(position)
            element.position = position
            element.color = (*color, 1.0) if len(color) == 3 else color
        self.link(source, node.inputs['Fac'])
        return node.outputs['Color']

    def tint(self, amount, warmth=0.0):
        return tuple(max(.001, min(1.0, color * amount * shift))
                     for color, shift in zip(self.base, (1.0 + warmth, 1.0, 1.0 - warmth)))

    def color(self, source, low=.76, high=1.12, warmth=0.0):
        value = self.ramp(source, [(.15, self.tint(low, warmth)),
                                   (.85, self.tint(high, -warmth * .4))], 'Restrained color variation')
        self.link(value, self.shader.inputs['Base Color'])

    def rough(self, source, low, high):
        value = self.ramp(source, [(.15, (low,) * 3), (.85, (high,) * 3)], 'Material roughness')
        self.link(value, self.shader.inputs['Roughness'])

    def bump(self, source, distance, strength=.24, normal=None):
        node = self.node('ShaderNodeBump', 'Fine relief')
        node.inputs['Distance'].default_value = distance
        node.inputs['Strength'].default_value = strength
        self.link(source, node.inputs['Height'])
        if normal is not None:
            self.link(normal, node.inputs['Normal'])
        self.link(node.outputs['Normal'], self.shader.inputs['Normal'])
        return node.outputs['Normal']

    def finish(self, role):
        # A tidy left-to-right graph remains inspectable in the source .blend.
        generated = [node for node in self.tree.nodes if node.name.startswith('SurfacePolish_')]
        for index, node in enumerate(generated):
            node.location = (-1200 + (index // 5) * 260, 420 - (index % 5) * 230)
            node.width = 205
        self.shader.location = (-900 + ((len(generated) + 4) // 5) * 260, 200)
        for node in self.tree.nodes:
            if node.type == 'OUTPUT_MATERIAL':
                node.location = (self.shader.location.x + 310, 200)
        self.material['surface_polish_version'] = 1
        self.material['surface_role'] = role


def polish_materials(mats, hero):
    """Replace generic grain with quiet, physically distinct authored surfaces."""
    materials = mats.values() if hasattr(mats, 'values') else mats
    polished = 0
    for mat in materials:
        if not mat or not mat.use_nodes or not mat.node_tree.nodes.get('Principled BSDF'):
            continue
        name = mat.name.lower()
        if any(word in name for word in ('crimson', 'emissive', 'studio')):
            continue
        # Always read the original scalar color, never the last pass's texture
        # output. Repeated invocation produces the same graph and appearance.
        s = _Surface(mat)
        role = 'charcoal'
        if 'granite' in name:
            role = 'weathered stone'
            broad = s.noise(1.7, 3.0)
            strata = s.wave(1.8, 'Z', 7.0, s.vector((.7, .7, 1.0)))
            layered = s.mix(broad, strata, .21)
            s.color(layered, .35, .67, .08)
            # Sparse mineral seams interrupt the horizontal bedding without
            # turning every face into an evenly cracked Voronoi tile.
            cells = s.node('ShaderNodeTexVoronoi', 'Eroded mineral seams')
            cells.feature = 'DISTANCE_TO_EDGE'
            cells.inputs['Scale'].default_value = 1.6
            s.link(s.position, cells.inputs['Vector'])
            seams = s.ramp(cells.outputs['Distance'],
                           [(.015, (.12,) * 3), (.06, (.8,) * 3)], 'Subtle seam recess')
            relief = s.mix(layered, seams, .13)
            normal = s.bump(relief, .029, .2)
            fine = s.noise(46, 2)
            s.bump(fine, .008, .20, normal)
            s.rough(s.mix(broad, fine, .35), .69, .91)
        elif 'hide' in name or 'bristles' in name:
            role = 'coarse boar hide'
            broad = s.noise(1.65, 3)
            strands = s.noise(18, 2, s.vector((4.0, .25, 2.8)))
            s.color(s.mix(broad, strands, .23), .72, 1.13, .075)
            normal = s.bump(s.noise(9, 2), .024, .17)
            s.bump(strands, .009, .24, normal)
            s.rough(strands, .68 if 'hide' in name else .73, .88)
        elif any(word in name for word in ('cloth', 'fold', 'indigo', 'linen')):
            role = 'woven cloth'
            broad = s.noise(9.0 if hero else 1.5, 3)
            # Two perpendicular thread directions and slight unevenness make a
            # weave, while keeping color broad enough for the 2K game atlas.
            warp = s.wave(145 if hero else 24, 'X', .7)
            weft = s.wave(145 if hero else 24, 'Z', .7)
            weave = s.mix(s.math('MULTIPLY', warp, weft), s.noise(320 if hero else 45), .2)
            s.color(broad, .76, 1.1, .025)
            s.bump(weave, .00038 if hero else .0018, .21)
            s.rough(s.mix(broad, weave, .3), .78, .94)
        elif 'whitewood' in name:
            role = 'aged carved whitewood'
            grain = s.noise(42, 2.5, s.vector((2.3, 2.3, .065)))
            pores = s.noise(500, 2)
            s.color(s.mix(s.noise(5.5, 2), grain, .55), .70, 1.015, .035)
            normal = s.bump(grain, .0005, .16)
            s.bump(pores, .00008, .12, normal)
            s.rough(grain, .48, .72)
        elif any(word in name for word in ('straw', 'cord')):
            role = 'twisted natural fiber'
            grain = s.noise(90 if hero else 12, 2, s.vector((1.8, 1.8, .15)))
            s.color(s.mix(s.noise(17 if hero else 2.5), grain, .38), .7, 1.10, .065)
            s.bump(grain, .0006 if hero else .004, .25)
            s.rough(grain, .72, .93)
        elif 'corruption' in name or 'blighted root' in name:
            role = 'blackened petrified bark'
            grain = s.noise(27 if hero else 4.5, 3, s.vector((3.4, 3.4, .15)))
            broad = s.noise(7 if hero else 1.0, 2)
            s.color(s.mix(broad, grain, .65), .60, 1.50, .10)
            normal = s.bump(grain, .002 if hero else .017, .24)
            s.bump(s.noise(190 if hero else 22), .0005 if hero else .006, .16, normal)
            s.rough(grain, .47, .77)
        elif 'moss' in name or 'lichen' in name:
            role = 'dry lichen' if 'lichen' in name else 'moss'
            broad = s.noise(4, 3)
            fine = s.noise(54, 2.0, roughness=.65)
            s.color(s.mix(broad, fine, .18), .37, .76, .04)
            normal = s.bump(broad, .013, .20)
            s.bump(fine, .011, .24, normal)
            s.rough(broad, .86, .98)
        elif 'offerings' in name:
            role = 'aged washi paper'
            grain = s.noise(24, 2, s.vector((1.0, .25, 2.5)))
            s.color(s.mix(s.noise(2.3), grain, .20), .79, 1.06, .035)
            s.bump(grain, .0012, .18)
            s.rough(grain, .86, .97)
        elif 'hair' in name:
            role = 'black hair'
            grain = s.noise(65, 2, s.vector((2.4, 2.4, .06)))
            s.color(grain, .66, 1.12)
            s.bump(grain, .00027, .16)
            s.rough(grain, .35, .57)
        elif 'exposed' in name:
            role = 'weathered skin'
            broad = s.noise(15, 2)
            fine = s.noise(350, 2)
            s.color(broad, .85, 1.07, .025)
            s.bump(fine, .00013, .15)
            s.rough(broad, .53, .70)
        elif 'boundary blade' in name or 'sharpened' in name:
            role = 'forged steel'
            brushed = s.noise(110, 2, s.vector((1.0, 1.0, .025)))
            s.color(brushed, .91, 1.04)
            s.bump(brushed, .000018, .10)
            s.rough(brushed, .19 if 'sharpened' in name else .25,
                    .27 if 'sharpened' in name else .36)
        elif 'redblack mineral' in name:
            role = 'dark mineral'
            grain = s.noise(4.5, 3)
            s.color(grain, .5, 1.20)
            s.bump(s.noise(32), .004, .14)
            s.rough(grain, .27, .48)
        else:
            grain = s.noise(180 if hero else 26, 2)
            s.color(s.noise(12 if hero else 2), .80, 1.08)
            s.bump(grain, .00025 if hero else .003, .16)
            s.rough(grain, .64, .84)
        s.finish(role)
        polished += 1
    print('SURFACE_POLISH_PASS', 'Shirotsura' if hero else 'Ishibashiri', polished)
    return polished
