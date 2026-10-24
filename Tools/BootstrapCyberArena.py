"""
Cyber Siege -- first test arena.
Run inside the Unreal Editor:  File > Execute Python Script...  (or the Output Log's Python bar)

Builds /Game/Maps/CyberArena from engine primitives only: a floor, perimeter walls,
cover blocks, neon trim, lights, sky, fog, and a post-process volume with Lumen on.
No imported assets. Re-runnable: it overwrites the map each time.
"""
import unreal
import math
import random

MAP_PATH = "/Game/Maps/CyberArena"
MAT_PATH = "/Game/Materials"
CUBE = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")
ARENA_HALF = 2400.0        # 48 m across
WALL_H = 500.0
random.seed(7)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

# ---------- materials (emissive neon, made from nodes so no .uasset is shipped) ----------
def make_emissive(name, rgb, strength=12.0):
    path = f"{MAT_PATH}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(name, MAT_PATH, unreal.Material, unreal.MaterialFactoryNew())
    col = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -500, 0)
    col.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    mul = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionMultiply, -250, 0)
    k = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -500, 200)
    k.set_editor_property("r", strength)
    unreal.MaterialEditingLibrary.connect_material_expressions(col, "", mul, "A")
    unreal.MaterialEditingLibrary.connect_material_expressions(k, "", mul, "B")
    unreal.MaterialEditingLibrary.connect_material_property(mul, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    return mat

def make_matte(name, rgb, roughness=0.65, metallic=0.15):
    path = f"{MAT_PATH}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(name, MAT_PATH, unreal.Material, unreal.MaterialFactoryNew())
    col = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    r = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200)
    r.set_editor_property("r", roughness)
    m = unreal.MaterialEditingLibrary.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300)
    m.set_editor_property("r", metallic)
    unreal.MaterialEditingLibrary.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    unreal.MaterialEditingLibrary.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    unreal.MaterialEditingLibrary.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)
    unreal.MaterialEditingLibrary.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    return mat

unreal.EditorAssetLibrary.make_directory(MAT_PATH)
M_FLOOR  = make_matte("M_ArenaFloor",  (0.05, 0.06, 0.08), 0.55, 0.30)
M_WALL   = make_matte("M_ArenaWall",   (0.10, 0.11, 0.14), 0.70, 0.10)
M_COVER  = make_matte("M_ArenaCover",  (0.16, 0.17, 0.20), 0.60, 0.25)
M_CYAN   = make_emissive("M_NeonCyan",    (0.13, 0.83, 0.93), 14.0)
M_MAGENTA= make_emissive("M_NeonMagenta", (0.93, 0.18, 0.62), 10.0)

# ---------- level ----------
unreal.EditorAssetLibrary.make_directory("/Game/Maps")
les.new_level(MAP_PATH)

def box(name, loc, scale, mat=None, rot=(0, 0, 0)):
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(*rot))
    a.set_actor_label(name)
    smc = a.static_mesh_component
    smc.set_static_mesh(CUBE)
    smc.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(*scale))
    if mat:
        smc.set_material(0, mat)
    return a

# Floor: 100cm cube scaled to the arena. Top surface at z=0.
box("Floor", (0, 0, -50), (ARENA_HALF * 2 / 100.0, ARENA_HALF * 2 / 100.0, 1.0), M_FLOOR)

# Perimeter walls
t = 60.0
for name, loc, scale in [
    ("Wall_N", ( 0,  ARENA_HALF + t/2, WALL_H/2), (ARENA_HALF*2/100 + 1.2, t/100, WALL_H/100)),
    ("Wall_S", ( 0, -ARENA_HALF - t/2, WALL_H/2), (ARENA_HALF*2/100 + 1.2, t/100, WALL_H/100)),
    ("Wall_E", ( ARENA_HALF + t/2, 0, WALL_H/2), (t/100, ARENA_HALF*2/100 + 1.2, WALL_H/100)),
    ("Wall_W", (-ARENA_HALF - t/2, 0, WALL_H/2), (t/100, ARENA_HALF*2/100 + 1.2, WALL_H/100)),
]:
    box(name, loc, scale, M_WALL)

# Neon trim along the wall tops (thin emissive strips)
for name, loc, scale, mat in [
    ("Trim_N", ( 0,  ARENA_HALF + t/2, WALL_H + 6), (ARENA_HALF*2/100, 0.12, 0.12), M_CYAN),
    ("Trim_S", ( 0, -ARENA_HALF - t/2, WALL_H + 6), (ARENA_HALF*2/100, 0.12, 0.12), M_CYAN),
    ("Trim_E", ( ARENA_HALF + t/2, 0, WALL_H + 6), (0.12, ARENA_HALF*2/100, 0.12), M_MAGENTA),
    ("Trim_W", (-ARENA_HALF - t/2, 0, WALL_H + 6), (0.12, ARENA_HALF*2/100, 0.12), M_MAGENTA),
]:
    box(name, loc, scale, mat)

# Cover: a ring of blocks and a few pillars. Nothing within 6 m of spawn.
for i in range(12):
    ang = i / 12.0 * 2 * math.pi
    r = random.uniform(900, 1700)
    x, y = math.cos(ang) * r, math.sin(ang) * r
    w, d, h = random.uniform(1.4, 2.6), random.uniform(1.4, 2.6), random.uniform(1.2, 2.2)
    box(f"Cover_{i:02d}", (x, y, h*50), (w, d, h), M_COVER, rot=(0, random.uniform(0, 90), 0))
    if i % 3 == 0:
        box(f"CoverTrim_{i:02d}", (x, y, h*100 + 4), (w*0.9, 0.08, 0.06), M_CYAN if i % 2 else M_MAGENTA,
            rot=(0, random.uniform(0, 90), 0))

for i in range(4):
    ang = i / 4.0 * 2 * math.pi + math.pi/4
    x, y = math.cos(ang) * 2000, math.sin(ang) * 2000
    box(f"Pillar_{i}", (x, y, 260), (1.0, 1.0, 5.2), M_WALL)
    box(f"PillarLight_{i}", (x, y, 420), (1.06, 1.06, 0.10), M_CYAN)

# Lighting + atmosphere
sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1500), unreal.Rotator(-42, 35, 0))
sun.set_actor_label("Sun")
sun.light_component.set_intensity(3.0)
sun.light_component.set_light_color(unreal.LinearColor(0.85, 0.90, 1.0, 1.0))
sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)

sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 1200), unreal.Rotator(0, 0, 0))
sky.set_actor_label("SkyLight")
sky.light_component.set_editor_property("real_time_capture", True)
sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)

atmo = eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
atmo.set_actor_label("SkyAtmosphere")

fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
fog.set_actor_label("HeightFog")
fog.component.set_editor_property("fog_density", 0.012)
fog.component.set_editor_property("fog_height_falloff", 0.35)
fog.component.set_editor_property("volumetric_fog", True)

ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
ppv.set_actor_label("PostProcess_Global")
ppv.set_editor_property("unbound", True)
pps = ppv.get_editor_property("settings")
pps.set_editor_property("override_bloom_intensity", True)
pps.set_editor_property("bloom_intensity", 0.55)
pps.set_editor_property("override_auto_exposure_min_brightness", True)
pps.set_editor_property("auto_exposure_min_brightness", 0.6)
pps.set_editor_property("override_auto_exposure_max_brightness", True)
pps.set_editor_property("auto_exposure_max_brightness", 1.6)
ppv.set_editor_property("settings", pps)

start = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(0, 0, 120), unreal.Rotator(0, 0, 0))
start.set_actor_label("PlayerStart")

les.save_current_level()
unreal.log("Cyber Siege: arena built and saved at " + MAP_PATH)
