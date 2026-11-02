"""
Cyber Siege -- Mission 1, "Bring Them Home": gray-box level.
Run inside the Unreal Editor:  File > Execute Python Script...

Builds /Game/Maps/M01_BringThemHome from engine primitives and places every gameplay actor the
mission needs: director (beats), script (rules), triggers, spawn points, cover, breach walls,
lens markers, the squad, the three rescuees, talk points, tagged spots, player starts per beat,
nav bounds, lighting. Imports Content/Data/m01_Lines.csv as the dialogue table.

Re-runnable: overwrites the map and the table. Real art replaces the boxes; the actors stay.

Layout (cm, X east, Y north, Z up): courtyard at the origin, the six-story block east of it,
the street and the wreck beyond, the parking deck and rail yard to the south, the base far off.
"""
import unreal

MAP_PATH = "/Game/Maps/M01_BringThemHome"
MAT_PATH = "/Game/Materials"
DATA_PATH = "/Game/Data"
CUBE = unreal.EditorAssetLibrary.load_asset("/Engine/BasicShapes/Cube")

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def cls(name):
    return unreal.load_class(None, f"/Script/CyberSiege.{name}")

# ---------------------------------------------------------------------------- materials
def make_matte(name, rgb, roughness=0.7, metallic=0.05):
    path = f"{MAT_PATH}/{name}"
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        return unreal.EditorAssetLibrary.load_asset(path)
    tools = unreal.AssetToolsHelpers.get_asset_tools()
    mat = tools.create_asset(name, MAT_PATH, unreal.Material, unreal.MaterialFactoryNew())
    mel = unreal.MaterialEditingLibrary
    col = mel.create_material_expression(mat, unreal.MaterialExpressionConstant3Vector, -400, 0)
    col.set_editor_property("constant", unreal.LinearColor(rgb[0], rgb[1], rgb[2], 1.0))
    r = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 200); r.set_editor_property("r", roughness)
    m = mel.create_material_expression(mat, unreal.MaterialExpressionConstant, -400, 300); m.set_editor_property("r", metallic)
    mel.connect_material_property(col, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(m, "", unreal.MaterialProperty.MP_METALLIC)
    mel.recompile_material(mat)
    unreal.EditorAssetLibrary.save_asset(path)
    return mat

unreal.EditorAssetLibrary.make_directory(MAT_PATH)
M_GROUND = make_matte("M_GB_Ground", (0.22, 0.22, 0.23))
M_WALL   = make_matte("M_GB_Wall",   (0.48, 0.46, 0.42))
M_FLOOR  = make_matte("M_GB_Floor",  (0.35, 0.34, 0.32))
M_COVER  = make_matte("M_GB_Cover",  (0.30, 0.36, 0.42))
M_WRECK  = make_matte("M_GB_Wreck",  (0.25, 0.30, 0.28), 0.5, 0.6)
M_MARK   = make_matte("M_GB_Mark",   (0.75, 0.30, 0.25))
M_PLASTER= make_matte("M_GB_Plaster",(0.80, 0.78, 0.70))

# ---------------------------------------------------------------------------- map
unreal.EditorAssetLibrary.make_directory("/Game/Maps")
les.new_level(MAP_PATH)

def box(name, loc, size, mat=M_WALL, rot=(0, 0, 0), tags=None, hidden=False):
    """size in cm (full extents)."""
    a = eas.spawn_actor_from_class(unreal.StaticMeshActor, unreal.Vector(*loc), unreal.Rotator(rot[0], rot[1], rot[2]))
    a.set_actor_label(name)
    a.static_mesh_component.set_static_mesh(CUBE)
    a.static_mesh_component.set_editor_property("mobility", unreal.ComponentMobility.STATIC)
    a.set_actor_scale3d(unreal.Vector(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    if mat: a.static_mesh_component.set_material(0, mat)
    if tags: a.set_editor_property("tags", tags)
    if hidden:
        a.set_actor_hidden_in_game(True)
        a.set_actor_enable_collision(False)
    return a

def spot(name, loc, yaw=0.0, tags=None):
    a = eas.spawn_actor_from_class(unreal.TargetPoint, unreal.Vector(*loc), unreal.Rotator(0, yaw, 0))
    a.set_actor_label(name)
    if tags: a.set_editor_property("tags", tags)
    return a

def actor(cname, name, loc, yaw=0.0, props=None, tags=None):
    a = eas.spawn_actor_from_class(cls(cname), unreal.Vector(*loc), unreal.Rotator(0, yaw, 0))
    a.set_actor_label(name)
    for k, v in (props or {}).items():
        a.set_editor_property(k, v)
    if tags: a.set_editor_property("tags", tags)
    return a

def trigger(name, loc, extent, kind, beat="", event="", wait_for="", require_beat="", dwell=10.0, repeatable=False):
    return actor("CSMissionTrigger", name, loc, 0, {
        "kind": kind, "beat": beat, "event": event, "wait_for": wait_for, "require_beat": require_beat,
        "dwell_seconds": dwell, "repeatable": repeatable, "extent": unreal.Vector(*extent)})

K = unreal.CSTriggerKind
def t_beat(name, loc, extent, beat):                 return trigger(name, loc, extent, K.ADVANCE_BEAT, beat=beat)
def t_event(name, loc, extent, ev, require=""):      return trigger(name, loc, extent, K.FIRE_EVENT, event=ev, require_beat=require)
def t_dwell(name, loc, extent, ev, secs, require=""):return trigger(name, loc, extent, K.DWELL, event=ev, dwell=secs, require_beat=require, repeatable=True)
def t_wait(name, loc, extent, ev, wait, require=""): return trigger(name, loc, extent, K.EVENT_AND_INSIDE, event=ev, wait_for=wait, require_beat=require)

def spawn_pt(name, loc, approach):
    return actor("CSSpawnPoint", name, loc, 0, {"approach": approach})

def cover(name, loc, yaw):
    return actor("CSCoverPoint", name, loc, yaw)

def cover_block(name, loc, size, face_yaw):
    """A cover block plus a cover point on its sheltered side, facing the threat."""
    box(name, loc, size, M_COVER)
    import math
    dx = -math.cos(math.radians(face_yaw)) * (size[0] / 2 + 60)
    dy = -math.sin(math.radians(face_yaw)) * (size[1] / 2 + 60)
    cover(name + "_cp", (loc[0] + dx, loc[1] + dy, loc[2] - size[2] / 2), face_yaw)

def marker(name, loc, kind, label="", show="", hide="", route=None, hold_name="", strike_event="strike_confirmed", auto=15.0):
    props = {"kind": kind, "label": label, "show_on_event": show, "hide_on_event": hide, "hold_name": hold_name,
             "strike_event": strike_event, "strike_auto_confirm": auto}
    if route: props["route_points"] = [unreal.Vector(*p) for p in route]
    return actor("CSLensMarker", name, loc, 0, props)

def player_start(name, loc, yaw, tag):
    a = eas.spawn_actor_from_class(unreal.PlayerStart, unreal.Vector(*loc), unreal.Rotator(0, yaw, 0))
    a.set_actor_label(name); a.set_editor_property("player_start_tag", tag)
    return a

GZ = 0.0  # ground

# ============================================================================ COURTYARD (x 0..3000)
box("Ground_Courtyard", (1500, 0, GZ - 25), (3600, 3600, 50), M_GROUND)
box("Vehicle_Floor", (-200, 0, GZ + 5), (500, 260, 10), M_WRECK)
box("Vehicle_Roof", (-200, 0, GZ + 250), (500, 260, 20), M_WRECK)
box("Vehicle_Side_N", (-200, 130, GZ + 125), (500, 10, 250), M_WRECK)
box("Vehicle_Side_S", (-200, -130, GZ + 125), (500, 10, 250), M_WRECK)
box("Vehicle_Back", (-450, 0, GZ + 125), (10, 260, 250), M_WRECK)
box("Vehicle_Door", (50, 0, GZ + 125), (10, 260, 250), M_WRECK, tags=["vehicle_door"])
box("Arcade_Roof", (1500, -1300, GZ + 330), (3000, 400, 30), M_WALL)
for i in range(6):
    box(f"Arcade_Col_{i}", (300 + i * 480, -1120, GZ + 165), (40, 40, 330), M_WALL)
box("Court_Wall_N", (1500, 1800, GZ + 250), (3000, 40, 500), M_WALL)
box("Court_Wall_S", (1500, -1800, GZ + 250), (3000, 40, 500), M_WALL)
box("Court_Wall_W", (-600, 0, GZ + 250), (40, 3600, 500), M_WALL)
cover_block("Court_Planter_1", (900, 300, GZ + 50), (300, 120, 100), 0)
cover_block("Court_Planter_2", (1900, -300, GZ + 50), (300, 120, 100), 0)
# roof shooter across the courtyard: the block's roofline
spawn_pt("Spawn_RoofShooter", (3100, 600, 2450), "court_roof")

player_start("Start_Insertion", (-200, 0, GZ + 100), 0, "insertion")
t_event("Trig_Courtyard", (1500, -800, GZ + 150), (900, 500, 150), "courtyard")
t_dwell("Trig_Courtyard_Idle", (1500, 0, GZ + 150), (1600, 1600, 150), "idle_courtyard", 10.0)
t_event("Trig_StairwellDoor", (2950, -1000, GZ + 150), (250, 300, 150), "stairwell_door")
marker("Lens_Objective", (8600, 0, 200), unreal.CSMarkerKind.OBJECTIVE, "Kestrel 2-1")
marker("Lens_Route_Court", (1500, -900, GZ), unreal.CSMarkerKind.ROUTE, "", show="lens_show", hide="beat_block",
       route=[(2900, -1000, GZ), (4200, -1000, 2450), (5200, 0, 2450)])

# ============================================================================ THE BLOCK (x 3200..5200, 6 floors)
BX0, BX1, BY0, BY1 = 3200, 5200, -1200, 1200
FH = 400
CX, CY = (BX0 + BX1) / 2, (BY0 + BY1) / 2
for f in range(7):  # 6 slabs + roof
    z = GZ + f * FH
    if f == 0:
        box("Block_Slab_0", (CX, CY, z - 25), (2000, 2400, 50), M_FLOOR)
        continue
    # slab with a stair hole at the south-west corner (x 3200..3600, y -1200..-800)
    box(f"Block_Slab_{f}_a", (CX + 200, CY, z - 15), (1600, 2400, 30), M_FLOOR)
    box(f"Block_Slab_{f}_b", (3400, 200, z - 15), (400, 2000, 30), M_FLOOR)
# perimeter walls, with a doorway at the stairwell (south wall, x 3300) and the roof door
for f in range(6):
    z = GZ + f * FH + FH / 2
    if f == 3:   # door out to the balcony at x CX-600
        box(f"Block_W_N_{f}_a", (CX - 950, BY1, z), (700, 30, FH), M_WALL)
        box(f"Block_W_N_{f}_b", (CX + 400, BY1, z), (1200, 30, FH), M_WALL)
    elif f == 4: # the ramp lands at x CX+1400: opening at the east end
        box(f"Block_W_N_{f}", (CX - 200, BY1, z), (1600, 30, FH), M_WALL)
    else:
        box(f"Block_W_N_{f}", (CX, BY1, z), (2000, 30, FH), M_WALL)
    box(f"Block_W_E_{f}", (BX1, CY, z), (30, 2400, FH), M_WALL)
    if f == 0:
        box(f"Block_W_W_{f}_a", (BX0, 300, z), (30, 1800, FH), M_WALL)   # doorway at y -1000 on the west face: the stairwell door
    else:
        box(f"Block_W_W_{f}", (BX0, CY, z), (30, 2400, FH), M_WALL)
    box(f"Block_W_S_{f}", (CX, BY0, z), (2000, 30, FH), M_WALL)
# stairwell door: a breach wall Mercer opens in Beat 1
actor("CSBreachWall", "Breach_StairDoor", (BX0, -1000, GZ + 110), 0,
      {"event_name": "door1", "fuse_seconds": 3.0, "panel_extent": unreal.Vector(20, 180, 220), "blast_damage": 0.0}, tags=["door1"])
# stairs: boxes as steps (navmesh and the character step up them; no rotated geometry to get wrong)
import math
def steps(name, x, y0, z0, dir_y, rise, run, n, width, mat=M_FLOOR, along="y"):
    """n steps starting at (x, y0, z0), each `run` further along dir_y and `rise` higher. along='x' swaps axes."""
    for i in range(n):
        if along == "y":
            box(f"{name}_{i}", (x, y0 + dir_y * (i + 0.5) * run, z0 + (i + 1) * rise - rise / 2), (width, run, rise), mat)
        else:
            box(f"{name}_{i}", (x + dir_y * (i + 0.5) * run, y0, z0 + (i + 1) * rise - rise / 2), (run, width, rise), mat)
for f in range(6):
    z0 = GZ + f * FH
    if f in (2, 3):  # floors 3-4: the stairwell is collapsed
        box(f"Rubble_Stair_{f}", (3400, -1000, z0 + 200), (380, 380, 380), M_WALL)
        continue
    steps(f"Stair_{f}_A", 3300, -1190, z0, +1, 20, 38, 10, 180)           # up 200 going north
    box(f"Stair_{f}_Land", (3400, -790, z0 + 190), (400, 60, 20), M_FLOOR)
    steps(f"Stair_{f}_B", 3500, -810, z0 + 200, -1, 20, 38, 10, 180)      # up another 200 going south
# interior: a corridor down the middle (y -200..200) with apartments either side, doors every 500 cm
for f in range(1, 6):
    z = GZ + f * FH + FH / 2
    for side, y in (("N", 200), ("S", -200)):
        for i in range(4):
            x0 = BX0 + 200 + i * 500
            box(f"Block_Int_{f}_{side}_{i}", (x0 + 150, y, z), (300, 20, FH), M_WALL)   # wall segment, 200 cm gap = a door
    # a cover block in the corridor
    cover_block(f"Block_Cover_{f}", (BX0 + 1200, 0, GZ + f * FH + 50), (120, 200, 100), 0)
# floor 3 -> 4 : balcony on the north face, ramp up outside
box("Balcony_3", (CX, BY1 + 200, GZ + 3 * FH - 15), (1400, 400, 30), M_FLOOR)
box("Balcony_Rail", (CX, BY1 + 400, GZ + 3 * FH + 50), (1400, 20, 100), M_WALL)
steps("Balcony_Steps", CX + 700, BY1 + 200, GZ + 3 * FH, +1, 20, 38, 20, 300, along="x")   # up to floor 4 along the north face
box("Balcony_4", (CX + 1500, BY1 + 150, GZ + 4 * FH - 15), (300, 300, 30), M_FLOOR)
spot("Spot_Balcony", (CX + 300, BY1 + 200, GZ + 3 * FH), 0, ["balcony"])
spot("Spot_HoldStairwell", (3400, -600, GZ + FH), 90, ["hold_stairwell"])
spot("Spot_FamilyDoor", (BX0 + 900, -210, GZ + 2 * FH), 0, ["family_door"])
t_event("Trig_FamilyDoor", (BX0 + 900, -450, GZ + 2 * FH + 100), (120, 200, 120), "family_door_opened")
# floor 5: the gun covers the last stairwell; a plaster wall behind it to breach
actor("CSBreachWall", "Breach_Wall5", (BX0 + 700, -400, GZ + 5 * FH + 110), 90,
      {"event_name": "wall5", "fuse_seconds": 6.0, "panel_extent": unreal.Vector(20, 300, 220)}, tags=["wall5"])
spawn_pt("Spawn_Floor5_Gun", (3450, -700, GZ + 5 * FH + 10), "floor5_gun")
for f, n in ((1, 2), (2, 3), (3, 2), (5, 3)):
    for i in range(n):
        spawn_pt(f"Spawn_Floor{f}_{i}", (BX0 + 600 + i * 400, (-1 if i % 2 else 1) * 700, GZ + f * FH + 10), f"floor{f}")
# triggers per floor (the stairwell top of each flight)
t_beat("Trig_Block", (3400, -1000, GZ + 150), (250, 250, 150), "block")
t_event("Trig_Floor2", (3800, -900, GZ + FH + 150), (250, 250, 150), "floor2", "block")
t_event("Trig_Floor3", (3800, -900, GZ + 2 * FH + 150), (250, 250, 150), "floor3", "block")
t_dwell("Trig_Balcony_Idle", (CX, BY1 + 200, GZ + 3 * FH + 150), (800, 250, 150), "balcony_idle", 5.0, "block")
t_event("Trig_Floor4", (CX + 1400, BY1 - 300, GZ + 4 * FH + 150), (250, 250, 150), "floor4", "block")
t_event("Trig_Floor5", (3800, -900, GZ + 5 * FH + 150), (250, 250, 150), "floor5", "block")
t_event("Trig_Roof", (3600, -900, GZ + 6 * FH + 150), (300, 300, 150), "roof", "block")
player_start("Start_Block", (3000, -1000, GZ + 100), 0, "block")

# fire escape: zigzag ramps down the east face to the street
box("Roof_Door_Gap", (BX1 + 150, -1050, GZ + 6 * FH - 10), (300, 300, 20), M_FLOOR)   # landing off the roof's east edge
for f in range(6, 0, -1):
    z0 = GZ + (f - 1) * FH; dir_y = 1 if f % 2 == 0 else -1
    y_start = -1050 if dir_y == 1 else -140
    steps(f"FireEsc_{f}", BX1 + 150, y_start, z0, dir_y, 20, 38, 20, 200)                 # 20 steps: 400 up over 760 along
    box(f"FireEsc_Land_{f}", (BX1 + 150, y_start + dir_y * 910, z0 + FH - 10), (300, 300, 20), M_FLOOR)

# ============================================================================ THE STREET (x 5400..7400)
box("Ground_Street", (6500, 0, GZ - 25), (2400, 4400, 50), M_GROUND)
box("Awning", (5600, 0, GZ + 300), (400, 1200, 20), M_WALL)
for i in range(5):
    cover_block(f"Car_{i}", (5900, -1200 + i * 600, GZ + 70), (420, 200, 140), 0)
box("Gun_Nest", (7000, 1200, GZ + 60), (300, 300, 120), M_COVER)
spawn_pt("Spawn_StreetGun", (7000, 1200, GZ + 10), "street_gun")
spawn_pt("Spawn_StreetFlank_0", (7100, 700, GZ + 10), "street_flank")
spawn_pt("Spawn_StreetFlank_1", (7200, 200, GZ + 10), "street_flank")
# pharmacy + alley + dock on the south side
box("Pharmacy", (6400, -1900, GZ + 200), (1200, 600, 400), M_WALL)
box("Dock", (7100, -1900, GZ + 60), (600, 600, 120), M_FLOOR)
box("Dock_Wall", (7100, -2250, GZ + 260), (600, 30, 400), M_WALL)
cover_block("Dock_Crate_0", (6900, -1700, GZ + 60), (160, 160, 120), 270)
cover_block("Dock_Crate_1", (7300, -1750, GZ + 60), (160, 160, 120), 270)
spawn_pt("Spawn_Dock_0", (7000, -2000, GZ + 130), "dock")
spawn_pt("Spawn_Dock_1", (7250, -2050, GZ + 130), "dock")
t_beat("Trig_Street", (BX1 + 150, -1050, GZ + 150), (300, 300, 150), "street")
t_event("Trig_StreetCars", (5700, 0, GZ + 150), (300, 1400, 150), "street_cars", "street")
t_event("Trig_Dock", (6600, -1700, GZ + 150), (300, 300, 150), "dock_enter", "street")
marker("Lens_Route_Street", (5900, -1300, GZ), unreal.CSMarkerKind.ROUTE, "", show="street_lens", hide="street_clear",
       route=[(6300, -1500, GZ), (6900, -1600, GZ), (7300, -1200, GZ), (7200, 600, GZ)])
player_start("Start_Street", (5500, -800, GZ + 100), 0, "street")

# ============================================================================ THE WRECK (x 7600..9600)
box("Ground_Wreck", (8600, 0, GZ - 25), (2200, 6400, 50), M_GROUND)
box("Ground_Wreck_E", (10600, 0, GZ - 25), (1800, 2200, 50), M_GROUND)
# helicopter on its side: fuselage, tail, a cabin opening on the west face
box("Heli_Floor", (8600, 0, GZ + 10), (900, 320, 20), M_WRECK)
box("Heli_Roof", (8600, 0, GZ + 320), (900, 320, 20), M_WRECK)
box("Heli_Side_N", (8600, 160, GZ + 165), (900, 20, 310), M_WRECK)
box("Heli_Side_S", (8600, -160, GZ + 165), (900, 20, 310), M_WRECK)
box("Heli_Back", (9050, 0, GZ + 165), (20, 320, 310), M_WRECK)
box("Heli_Panel", (8520, -60, GZ + 80), (60, 120, 120), M_WRECK)   # the instrument panel on Reyes's legs
box("Heli_Tail", (9300, 60, GZ + 220), (700, 120, 120), M_WRECK)
box("Heli_Rotor", (8600, -200, GZ + 330), (1400, 60, 20), M_WRECK, rot=(0, 20, 0))
spot("Spot_CabinDoor", (8100, 0, GZ), 180, ["cabin_door"])
spot("Spot_BellOkafor", (8350, 120, GZ), 0, ["bell_okafor"])
spot("Spot_MercerPanel", (8300, -120, GZ), 0, ["mercer_panel"])
spot("Spot_BellReyes", (8420, -60, GZ), 0, ["bell_reyes"])
# approaches
box("Bus", (8600, 900, GZ + 150), (1100, 260, 300), M_COVER, tags=["bus"])
cover("Bus_cp_a", (8300, 700, GZ), 90); cover("Bus_cp_b", (8900, 700, GZ), 90)
box("Overpass_Stairs", (9500, -700, GZ + 100), (300, 500, 200), M_COVER)
cover("Overpass_cp", (9300, -700, GZ), 270)
cover_block("Wreck_Barrier_N", (8000, 1500, GZ + 50), (400, 100, 100), 90)
cover_block("Wreck_Barrier_S", (9000, -1500, GZ + 50), (400, 100, 100), 270)
cover_block("Wreck_Barrier_E", (10000, 300, GZ + 50), (100, 400, 100), 0)
for i in range(3):
    spawn_pt(f"Spawn_North_{i}", (8100 + i * 450, 3000, GZ + 10), "north")
    spawn_pt(f"Spawn_East_{i}", (11300, -400 + i * 400, GZ + 10), "east")
    spawn_pt(f"Spawn_South_{i}", (8100 + i * 450, -3000, GZ + 10), "south")
# hold markers (Beat 4)
for nm, loc in (("Pharmacy corner", (7700, -800, GZ)), ("Bus", (8600, 650, GZ)), ("Overpass stairs", (9350, -700, GZ))):
    marker(f"Lens_Hold_{nm.split()[0]}", loc, unreal.CSMarkerKind.HOLD, nm, show="hold_ready", hide="beat_save", hold_name=nm.split()[0].lower())
# mortar deck to the north, three hundred out, raised
box("ParkingDeck_N", (8600, 4200, GZ + 400), (1600, 800, 40), M_FLOOR)
for i in range(4):
    box(f"ParkingDeck_N_Col_{i}", (8000 + i * 400, 4200, GZ + 200), (60, 60, 400), M_WALL)
spawn_pt("Spawn_Mortar_0", (8500, 4200, GZ + 430), "mortar")
spawn_pt("Spawn_Mortar_1", (8700, 4200, GZ + 430), "mortar")
marker("Lens_Strike", (8600, 4200, GZ + 420), unreal.CSMarkerKind.STRIKE, "Mortar team", show="beat_save", strike_event="strike_confirmed", auto=15.0)
marker("Lens_Threat_Mortar", (8600, 4200, GZ + 420), unreal.CSMarkerKind.THREAT, "", show="mortar_seen", hide="beat_save")
t_event("Trig_WreckApproach", (7600, 0, GZ + 150), (200, 800, 150), "wreck_approach", "street")
t_beat("Trig_Wreck", (8000, 0, GZ + 150), (300, 500, 150), "wreck")
t_event("Trig_Cabin", (8350, 0, GZ + 150), (250, 160, 150), "cabin", "wreck")
player_start("Start_Wreck", (7800, 0, GZ + 100), 0, "wreck")
player_start("Start_Hold", (8100, -300, GZ + 100), 0, "hold")
player_start("Start_Save", (8100, -300, GZ + 100), 0, "save")

# ============================================================================ PARKING + RAIL YARD (south)
box("Ground_Parking", (8600, -3800, GZ - 25), (1200, 1400, 50), M_GROUND)
box("ParkingDeck_S", (8600, -3800, GZ + 500), (1200, 1400, 40), M_FLOOR)
for i in range(4):
    box(f"ParkingDeck_S_Col_{i}", (8100 + i * 330, -3300, GZ + 250), (60, 60, 500), M_WALL)
    box(f"ParkingDeck_S_Col2_{i}", (8100 + i * 330, -4300, GZ + 250), (60, 60, 500), M_WALL)
box("Rubble_Collapse", (8600, -3500, GZ + 200), (1200, 300, 400), M_WALL, tags=["rubble"], hidden=True)
t_event("Trig_Parking", (8600, -3200, GZ + 150), (600, 200, 150), "parking", "extraction")
t_event("Trig_Collapse", (8600, -4300, GZ + 150), (600, 200, 150), "collapse", "extraction")
box("Ground_Yard", (8600, -5600, GZ - 25), (3000, 2200, 50), M_GROUND)
for i in range(3):
    for j in range(2):
        x = 7700 + i * 900; y = -5000 - j * 700
        if i == 0 and j == 0:
            # the shelter container: open on the north side
            box("Container_Shelter_W", (x - 300, y, GZ + 130), (20, 300, 260), M_COVER)
            box("Container_Shelter_E", (x + 300, y, GZ + 130), (20, 300, 260), M_COVER)
            box("Container_Shelter_S", (x, y - 150, GZ + 130), (600, 20, 260), M_COVER)
            box("Container_Shelter_Top", (x, y, GZ + 270), (600, 300, 20), M_COVER)
            spot("Spot_Container", (x, y, GZ), 90, ["container"])
            cover("Container_Door_cp", (x, y + 200, GZ), 90)
        else:
            cover_block(f"Container_{i}_{j}", (x, y, GZ + 130), (600, 300, 260), 90)
for i in range(4):
    spawn_pt(f"Spawn_Yard_{i}", (7600 + i * 600, -6400, GZ + 10), "yard")
box("Flatcar", (8600, -6600, GZ + 60), (1400, 300, 120), M_WRECK)
box("Flatcar_Cab", (9400, -6600, GZ + 200), (400, 300, 400), M_WRECK)
spot("Spot_Flatcar", (8600, -6600, GZ + 120), 0, ["flatcar"])
t_event("Trig_RailYard", (8600, -4700, GZ + 150), (800, 200, 150), "rail_yard", "extraction")
t_wait("Trig_YardExtract", (8600, -6400, GZ + 150), (800, 200, 150), "yard_extract", "yard_clear", "extraction")
player_start("Start_Extraction", (8100, -300, GZ + 100), 270, "extraction")

# ============================================================================ THE BASE (far east)
BXB = 20000
box("Ground_Base", (BXB, 0, GZ - 25), (3000, 3000, 50), M_GROUND)
box("Base_Tent_Aid", (BXB - 800, 600, GZ + 150), (800, 600, 300), M_WALL)
box("Base_Cot_Reyes", (BXB - 900, 400, GZ + 30), (200, 80, 60), M_COVER)
box("Base_Cot_Okafor", (BXB - 700, 400, GZ + 30), (200, 80, 60), M_COVER)
box("Base_Terminal", (BXB + 600, 600, GZ + 60), (120, 80, 120), M_COVER)
box("Base_Vehicle", (BXB + 700, -800, GZ + 120), (500, 260, 240), M_WRECK)
box("Base_Bunk", (BXB - 900, -900, GZ + 30), (200, 80, 60), M_COVER)
spot("Spot_BaseStart", (BXB, -200, GZ), 90, ["base_start"])
actor("CSTalkPoint", "Talk_Bell", (BXB - 800, 800, GZ + 85), 270, {"display_name": "Bell", "event": "talk_bell", "required": True})
actor("CSTalkPoint", "Talk_Reyes", (BXB - 900, 250, GZ + 85), 90, {"display_name": "Reyes", "event": "talk_reyes", "required": True})
actor("CSTalkPoint", "Talk_Chen", (BXB + 600, 450, GZ + 85), 90, {"display_name": "Chen", "event": "talk_chen", "required": True})
actor("CSTalkPoint", "Talk_Mercer", (BXB + 500, -600, GZ + 85), 270, {"display_name": "Mercer", "event": "talk_mercer"})
actor("CSTalkPoint", "Talk_Halvorsen", (BXB + 900, -600, GZ + 85), 270, {"display_name": "Halvorsen", "event": "talk_halvorsen"})
t_wait("Trig_Bunk", (BXB - 900, -900, GZ + 150), (250, 250, 150), "bunk", "talks_done", "base")
player_start("Start_Base", (BXB, -200, GZ + 100), 90, "base")

# ============================================================================ PEOPLE
actor("CSSquadMate", "Squad_Bell", (-200, 200, GZ + 100), 0, {"callsign": "bell", "display_name": "Bell", "follow_offset": unreal.Vector(-220, 180, 0), "team_color": unreal.LinearColor(0.55, 0.90, 0.60, 1), "invulnerable": True})
actor("CSSquadMate", "Squad_Chen", (-200, -200, GZ + 100), 0, {"callsign": "chen", "display_name": "Chen", "follow_offset": unreal.Vector(-220, -180, 0), "team_color": unreal.LinearColor(0.75, 0.70, 1.00, 1), "invulnerable": True})
actor("CSSquadMate", "Squad_Mercer", (-400, 0, GZ + 100), 0, {"callsign": "mercer", "display_name": "Mercer", "follow_offset": unreal.Vector(-420, 0, 0), "team_color": unreal.LinearColor(1.00, 0.72, 0.45, 1), "invulnerable": True})
actor("CSRescuee", "Rescuee_Reyes", (8450, -60, GZ + 100), 180, {"id": "reyes", "display_name": "Reyes", "state": unreal.CSRescueeState.TRAPPED, "walk_speed": 260.0})
actor("CSRescuee", "Rescuee_Okafor", (8400, 120, GZ + 100), 180, {"id": "okafor", "display_name": "Okafor", "state": unreal.CSRescueeState.WAITING, "walk_speed": 0.0})
actor("CSRescuee", "Rescuee_Halvorsen", (8150, 0, GZ + 100), 180, {"id": "halvorsen", "display_name": "Halvorsen", "state": unreal.CSRescueeState.WAITING, "walk_speed": 380.0, "can_shoot": True})

# ============================================================================ DIRECTOR: beats
def beat(name, objective, enter_event="", checkpoint=True):
    b = unreal.CSBeat()
    b.set_editor_property("name", name); b.set_editor_property("objective", objective)
    b.set_editor_property("on_enter_event", enter_event); b.set_editor_property("checkpoint", checkpoint)
    return b
director = actor("CSMissionDirector", "MissionDirector", (0, 0, 0), 0, {"mission_id": "m01", "beats": [
    beat("insertion",  "Reach Kestrel 2-1"),
    beat("block",      "Cross the block from above"),
    beat("street",     "Get past the gun"),
    beat("wreck",      "Reach the crew"),
    beat("hold",       "Hold the wreck"),
    beat("save",       "Hold the wreck"),
    beat("extraction", "Get everyone to the rail yard"),
    beat("base",       "Talk to the squad"),
]})

# ============================================================================ WAVES
def wave(start, approaches, riflemen=4, heavies=0, gunners=0, gap=1.2, push="", cleared="", reached="", weakest=False, radius=250.0):
    w = unreal.CSWave()
    w.set_editor_property("start_event", start); w.set_editor_property("approaches", approaches)
    w.set_editor_property("riflemen", riflemen); w.set_editor_property("heavies", heavies); w.set_editor_property("gunners", gunners)
    w.set_editor_property("seconds_between_spawns", gap); w.set_editor_property("push_toward_tag", push)
    w.set_editor_property("cleared_event", cleared); w.set_editor_property("reached_event", reached)
    w.set_editor_property("add_weakest_approach", weakest); w.set_editor_property("reached_radius", radius)
    return w
actor("CSWaveDirector", "WaveDirector", (0, 0, 0), 0, {"waves": [
    wave("beat_insertion", ["court_roof"], 1, gap=0.1),
    wave("beat_block",     ["floor1"], 2, gap=0.3, cleared="stair1_clear"),
    wave("floor2",         ["floor2"], 3, gap=0.8),
    wave("floor3",         ["floor3"], 2, gap=0.8, cleared="floor3_clear"),
    wave("floor5",         ["floor5"], 3, gap=0.6),
    wave("floor5",         ["floor5_gun"], 0, gunners=1, gap=0.1, cleared="floor5_clear"),
    wave("beat_street",    ["street_gun"], 0, gunners=1, gap=0.1, cleared="street_clear"),
    wave("beat_street",    ["street_flank"], 2, gap=0.5),
    wave("dock_enter",     ["dock"], 2, gap=0.3),
    wave("beat_hold",      ["north"], 4, gap=1.5, push="cabin_door", cleared="wave1_clear"),
    wave("wave2",          ["east"], 6, gap=1.2, push="cabin_door", cleared="wave2_clear", weakest=True),
    wave("wave3",          ["north", "east", "south"], 7, heavies=1, gunners=1, gap=1.0, push="cabin_door", cleared="wave3_clear", reached="door_reached"),
    wave("beat_save",      ["mortar"], 2, gap=0.2),
    wave("rail_yard",      ["yard"], 7, gap=1.0, push="container", cleared="yard_clear", reached="container_reached", radius=400.0),
]})

# ============================================================================ SCRIPT: rules
A = unreal.CSRuleAction
def rule(on, action, name="", who="", seconds=0.0, text="", once=True, only_if="", cut_if=""):
    r = unreal.CSRule()
    r.set_editor_property("on_event", on); r.set_editor_property("action", action)
    r.set_editor_property("name", name); r.set_editor_property("who", who); r.set_editor_property("seconds", seconds)
    r.set_editor_property("text", text); r.set_editor_property("once", once)
    r.set_editor_property("only_if", only_if); r.set_editor_property("cut_if", cut_if)
    return r
actor("CSMissionScript", "MissionScript", (0, 0, 0), 0, {"rules": [
    # Beat 1
    rule("beat_insertion", A.PRESENCE, text="Mission 1 -- Bring Them Home"),
    rule("drop_open",      A.HIDE_TAGGED, "vehicle_door"),
    rule("door_breach",    A.SQUAD_BREACH, "door1", "mercer"),
    rule("door1_breached", A.FIRE_EVENT, "block_open"),
    # Beat 2
    rule("floor2",         A.SQUAD_HOLD, "hold_stairwell", "bell"),
    rule("floor3",         A.SQUAD_MOVE_TO, "balcony", "mercer"),
    rule("floor3",         A.SQUAD_FOLLOW, who="bell"),
    rule("floor5",         A.SQUAD_BREACH, "wall5", "mercer", seconds=4.0),   # Mercer takes it on his own if the player waits
    # Beat 3
    rule("street_split",   A.SQUAD_HOLD, "bus", "bell"),       # Bell and Mercer draw the gun from behind the cars
    rule("street_split",   A.SQUAD_HOLD, "bus", "mercer"),
    rule("street_clear",   A.SQUAD_FOLLOW),
    rule("street_clear",   A.PRESENCE, text="Alex reached the wreck -- Mission 1"),
    # Beat 4
    rule("beat_wreck",     A.RESCUEE_STATE, "halvorsen", "waiting"),
    rule("wreck_crew_met", A.SQUAD_SCRIPTED, "bell_okafor", "bell"),
    rule("wreck_crew_met", A.SQUAD_SCRIPTED, "mercer_panel", "mercer"),
    rule("wreck_decision", A.SQUAD_SCRIPTED, "bell_reyes", "bell"),
    rule("hold_ready",     A.SQUAD_FOLLOW, who="mercer"),
    rule("hold_ready",     A.SQUAD_FOLLOW, who="chen"),
    rule("hold_ready",     A.FIRE_EVENT, "hold_timeout", seconds=25.0),
    rule("cmd_hold",       A.ADVANCE_BEAT, "hold", only_if="hold_ready"),
    rule("hold_timeout",   A.ADVANCE_BEAT, "hold"),
    # Beat 5
    rule("beat_hold",      A.START_CLOCK, "panel_free", seconds=240.0),
    rule("beat_hold",      A.PRESENCE, text="Alex is holding the wreck -- Mission 1"),
    rule("beat_hold",      A.FIRE_EVENT, "clock_2min", seconds=120.0),
    rule("beat_hold",      A.FIRE_EVENT, "clock_1min", seconds=180.0),
    rule("wave1_clear",    A.FIRE_EVENT, "wave2", seconds=6.0),
    rule("wave2_clear",    A.FIRE_EVENT, "wave3", seconds=6.0),
    rule("wave3",          A.FIRE_EVENT, "heavy_spawned", seconds=9.0),
    rule("door_reached",   A.FIRE_EVENT, "door_fail", seconds=5.0),
    rule("door_fail",      A.FAIL, text="They got into the cabin.", cut_if="panel_free"),
    rule("panel_free",     A.SQUAD_SCRIPTED, "mercer_panel", "mercer"),
    rule("panel_free",     A.FIRE_EVENT, "mortar_setup", seconds=20.0),
    rule("mortar_seen",    A.ADVANCE_BEAT, "save"),
    # Beat 6
    rule("strike_confirmed", A.FIRE_EVENT, "strike_landed", seconds=12.0),
    rule("strike_landed",  A.KILL_ENEMIES_FROM, "mortar"),
    rule("strike_landed",  A.ACHIEVEMENT, "m01_strike"),
    rule("strike_landed",  A.RESCUEE_STATE, "reyes", "waiting"),
    rule("strike_landed",  A.FIRE_EVENT, "waves_broken", seconds=4.0),
    rule("waves_broken",   A.KILL_ENEMIES_FROM),          # the regulars pull back
    rule("extract_go",     A.ADVANCE_BEAT, "extraction"),
    # Beat 7
    rule("beat_extraction", A.SQUAD_FOLLOW),
    rule("beat_extraction", A.RESCUEE_FOLLOW, "reyes", "bell"),
    rule("beat_extraction", A.SQUAD_CARRY, "okafor", "mercer"),
    rule("beat_extraction", A.RESCUEE_FOLLOW, "halvorsen", "player"),
    rule("beat_extraction", A.PRESENCE, text="Alex is getting them out -- Mission 1"),
    rule("collapse",       A.SHOW_TAGGED, "rubble"),
    rule("yard_shelter",   A.RESCUEE_SHELTER, "reyes", "container"),
    rule("yard_shelter",   A.RESCUEE_SHELTER, "okafor", "container"),
    rule("yard_shelter",   A.RESCUEE_SHELTER, "halvorsen", "container"),
    rule("yard_shelter",   A.SQUAD_HOLD, "container", "bell"),
    rule("yard_shelter",   A.SQUAD_FOLLOW, who="mercer"),
    rule("yard_shelter",   A.FIRE_EVENT, "yard_rejoin", seconds=8.0),
    rule("container_reached", A.FIRE_EVENT, "container_fail", seconds=10.0),
    rule("container_fail", A.FAIL, text="They got to the container.", cut_if="yard_clear"),
    rule("yard_clear",     A.SQUAD_FOLLOW),
    rule("yard_clear",     A.RESCUEE_FOLLOW, "reyes", "bell"),
    rule("yard_clear",     A.SQUAD_CARRY, "okafor", "mercer"),
    rule("yard_clear",     A.RESCUEE_FOLLOW, "halvorsen", "player"),
    rule("aboard",         A.RESCUEE_STATE, "reyes", "aboard"),
    rule("aboard",         A.RESCUEE_STATE, "okafor", "aboard"),
    rule("aboard",         A.RESCUEE_STATE, "halvorsen", "aboard"),
    rule("aboard",         A.FIRE_EVENT, "depart", seconds=3.0),
    rule("depart_done",    A.ADVANCE_BEAT, "base"),
    # Beat 8
    rule("beat_base",      A.TELEPORT_PLAYER_TO, "base_start"),
    rule("beat_base",      A.SQUAD_SCRIPTED, "talk_bell_spot", "bell"),
    rule("beat_base",      A.PRESENCE, text="Back at base -- Mission 1"),
    rule("mission_complete", A.ACHIEVEMENT, "m01_complete"),
    rule("mission_complete", A.OBJECTIVE, text="Mission complete"),
    rule("mission_complete", A.PRESENCE, text="Finished Mission 1"),
]})
spot("Spot_TalkBell", (BXB - 1100, 800, GZ), 0, ["talk_bell_spot"])

# ============================================================================ NAV + LIGHT
nav = eas.spawn_actor_from_class(unreal.NavMeshBoundsVolume, unreal.Vector(9000, -1500, 1500), unreal.Rotator(0, 0, 0))
nav.set_actor_label("NavBounds"); nav.set_actor_scale3d(unreal.Vector(260, 180, 40))

sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 3000), unreal.Rotator(-38, 140, 0))
sun.set_actor_label("Sun"); sun.light_component.set_intensity(4.0)
sun.light_component.set_light_color(unreal.LinearColor(1.0, 0.93, 0.85, 1.0))
sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 2000), unreal.Rotator(0, 0, 0))
sky.set_actor_label("SkyLight"); sky.light_component.set_editor_property("real_time_capture", True)
sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
atmo = eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)); atmo.set_actor_label("SkyAtmosphere")
fog = eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0)); fog.set_actor_label("HeightFog")
fog.component.set_editor_property("fog_density", 0.02)
try:
    fog.component.set_editor_property("volumetric_fog", True)   # property name moved in 5.8; the fog is fine without it
except Exception:
    pass
ppv = eas.spawn_actor_from_class(unreal.PostProcessVolume, unreal.Vector(0, 0, 0), unreal.Rotator(0, 0, 0))
ppv.set_actor_label("PostProcess_Global"); ppv.set_editor_property("unbound", True)
# the base is night: a point light over the aid tent
pl = eas.spawn_actor_from_class(unreal.PointLight, unreal.Vector(BXB - 800, 600, 450), unreal.Rotator(0, 0, 0))
pl.set_actor_label("Base_Light"); pl.point_light_component.set_intensity(8000.0)
pl.point_light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)

les.save_current_level()

# ============================================================================ DIALOGUE TABLE
unreal.EditorAssetLibrary.make_directory(DATA_PATH)
table_path = f"{DATA_PATH}/m01_Lines"
if unreal.EditorAssetLibrary.does_asset_exist(table_path):
    unreal.EditorAssetLibrary.delete_asset(table_path)
factory = unreal.DataTableFactory()
factory.set_editor_property("struct", unreal.load_object(None, "/Script/CyberSiege.CSDialogueLine"))
table = unreal.AssetToolsHelpers.get_asset_tools().create_asset("m01_Lines", DATA_PATH, unreal.DataTable, factory)
csv_path = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_content_dir()) + "Data/m01_Lines.csv"
ok = unreal.DataTableFunctionLibrary.fill_data_table_from_csv_file(table, csv_path)
unreal.EditorAssetLibrary.save_asset(table_path)
unreal.log(f"Mission 1 built. Dialogue table from {csv_path}: {'ok' if ok else 'FAILED -- check the CSV columns'}")
unreal.log("Set Project Settings > Maps & Modes > Game Default Map to M01_BringThemHome (DefaultEngine.ini already does), then Play.")
