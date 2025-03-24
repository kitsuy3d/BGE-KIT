# This menu code can run from the text editer with run script
# where it can access the scene objects

import bpy

# change all selected - use_backface_culling false

for object in bpy.context.selected_objects:
    for mat in object.data.materials:
        #mat.game_settings.use_backface_culling=False
        pass

# change scene object by in name and not in name

for ob in bpy.context.scene.objects:
    if "part-of-my-object-name-here" in ob.name and not "_" in ob.name:
        #ob.game.activity_culling.use_physics = True
        #ob.game.activity_culling.physics_radius = 400
        #ob.hide_render = True
        pass
