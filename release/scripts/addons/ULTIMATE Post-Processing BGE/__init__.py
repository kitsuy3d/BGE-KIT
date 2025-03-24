#_this addon originaly created by ThaTimster in 2016  https://www.youtube.com/watch?v=IdR3QEsCm5c
#_but the project was forgotten so i made a simple update to legacy BGE and UPBGE users to enjoy it :)
#_orignaly the addon has 20 filter but i add more and update some and osama msa gived me some of his filters total 46
#Updated by Discord["adelswsm"] Filters by Discord["osama_m.s.a"]


bl_info = {"name": "ULTIMATE Post-Processing Filters",
"category": "Game Engine",
"author": "Tim Crellin (Thatimst3r) updated by Discord[osama_m.s.a] and Discord[adelswsm]",
"blender": (2,79,2),
"location": "Game Render Panel",
"description":" 20 + 26 fragment shaders (2dFilters) to use in game engine.",
"version":(1,0)}

import bpy
import os
from bpy.props import *

#Main panel
class FilterPanel(bpy.types.Panel):
    bl_label = "Post-Processing Filters"
    bl_space_type = "PROPERTIES"
    bl_region_type = "WINDOW"
    bl_context = 'render'
    
    def draw(self, context):
        scn = context.scene
        if bpy.context.mode == 'OBJECT':
            layout = self.layout
            if bpy.context.scene.render.engine != 'BLENDER_GAME':
                row = layout.row()
                layout.label(" Select Blender Game renderer to use addon", icon = 'ERROR')
            elif len(context.selected_objects) != 1 or context.selected_objects[0].type != 'CAMERA':
                row = layout.row()
                layout.label(' To add or remove filters select a camera', icon = 'ERROR')
                
            else:
                layout.label(' Filter order: Shading > Glow > Color', icon = 'INFO')
                layout.row()
                layout.prop(scn, 'Chosen_filter')
                if scn.Chosen_filter == 'MOTION BLUR':
                    result = check_mblur_remove()
                else:
                    result = check_filters(scn.Chosen_filter)
                
                descriptions = {'DOF':'Adds depth of field',
                'SSAO': 'Darkens edges of objects',
                'SSGI': 'Improved global lighting quality',
                'BLOOM':'Adds glow to bright objects',
                'NOISE':'Adds noise to the screen',
                'VIGNETTE':'Darkens the edges of the screen',
                'SHARPEN':'Improve detail clarity and make edges appear sharper',
                'DITHERED':'It gives an old style to the game',
                'RETINEX':'Enhances contrast, maintains color.',
                'TECHNICOLOR':'Shading enhances the colors of the world',
                'SSR REFLECTION':'This shader mirrors objects realistically',
                'WATER':'Underwater distortion effect',
                'CARTOON':'An effect that makes the game look cartoonish with soble outline',
                'COLOR CORRECTION':'Sweetening the colors of the world',
                'CHROMATIC ABERRATION':'Coloured Fringing',
                'BLOOM X':'Adds X shaped glow to objects',
                'BLEACH':'Horror game filter. Low saturation, high contrast.',
                'BARREL':'Adds buldge distortion to the centre of the screen',
                'FXAA':'Efficient Anti-Aliasing', 'GAMEBOY COLOR':'Gameboy style pixelization.',
                'LEVELS':'Color channel remapping with curves',
                'NIGHT VISION':'Infared style night vision',
                'HORIZONTAL':'This effect stretches the screen along the X and Y axis',
                'RADIAL BLUR':'Adds blur around the edges of the screen',
                'RADIOSITY':'An effect that creates a reflection of lighting to make the scene realistic',
                'TOON':'Adds toon outline',
                'HDR':'Adjusts light levels to suit environment',
                'VHS I':'An effect that makes the screen vibrate like the old one',
                'VHS II':'The effect makes the screen scene distorted and old',
                'MOTION BLUR':'Adds blur when moving fast',
                'BLUR':'I dont Need to Explane what this do :)',
                'ETHEREAL GLOW':'Weird SIFI Glow Around The Screen',
                'FADEABLE REVERSE':'Reverse Color Filter With intencity controller max = 1 0ff = 0',
                'FRACTAL WAVEFORM':'Wierd Noise Effect I Found Maby its filmNoise From 70s Cameras',
                'GLITCH':'GLITCH Effect Ported From Godot to bge',
                'PIXELS':'DownScale Image Pixles Rate To make image appear more pixel',
                'POSTERIZE':'Clamp Color and merge close Color To make the render appear more 2D',
                'VGA GLITCH':'You Know whene you have a bad VGA Cable',
                'WATER RIPPLE':'WATER RIPPLE Fragment Shader for BGE with hard-coded center',
                'WAVES':'Makes Screen Render Wavey',
                'FADEABLE GLITCH':'Glitch Effect with Float Controller',
                'LIGHT SCATTER':'Shader makes luminous objects radiate',
                'LENS FLARE':'Simple lightwight sun reflection on lens effect',
                'CRT':'Screen Filter thst make image like old school screens',
                'CRT_DISTORTION':'It creates refraction with blur shader',
                'GAMMA':'Brightens dark spaces'}
                if result == True and scn.Chosen_filter =='DOF' or scn.Chosen_filter =='SSAO' and result == True:
                    row = layout.row()
                    layout.label('Make sure Camera clipping is the same in filter script.',icon='ERROR')
                elif result == True and scn.Chosen_filter =='SSR REFLECTION' or scn.Chosen_filter =='RADIOSITY' and result == True:
                    row = layout.row()
                    layout.label('this filter is heavy on graphics card and maby it will crash your game.',icon='ERROR')
                elif result == True and scn.Chosen_filter == 'TOON':
                    row = layout.row()
                    layout.label('Keep "line_size" property below 1',icon='ERROR')
                elif result == True and scn.Chosen_filter == 'MOTION BLUR':
                    row = layout.row()
                    layout.label('To change blur see "distance" prop on Mblur_empty', icon='ERROR')
                elif result == True and scn.Chosen_filter == 'LENS FLARE':
                    row = layout.row()
                    layout.label('Dont Forget to put sun object Name', icon='ERROR')
                

                row = layout.row()
                col = row.column()
                col.label('Description:   ' + descriptions[scn.Chosen_filter])
                
                row = layout.row()
                col = row.column()
                col.operator('remove.filter', icon='CANCEL')
                
                col = row.column()
                col.operator('add.filter',icon='PLUS')
        
class Remove_Filter(bpy.types.Operator):
    bl_idname = "remove.filter"
    bl_label = "Remove Filter"
    
    @classmethod
    def poll(cls, context):
        scn = context.scene
        if scn.Chosen_filter == 'MOTION BLUR':
            return check_mblur_remove()
        else:
            return check_remove(scn.Chosen_filter)
    
    def execute(self,context):
        scn = context.scene
                
        set_filter = scn.Chosen_filter
        removeFilter(set_filter)
        if set_filter == 'MOTION BLUR':
            report_string = 'Removed MOTION BLUR from '+scn.name
        else:
            report_string = 'Removed '+set_filter+' from '+context.selected_objects[0].name
        self.report({'INFO'}, report_string)        
        return{'FINISHED'}
    
class Add_Filter(bpy.types.Operator):
    bl_idname = "add.filter"
    bl_label = "Add Filter"
    
    
    @classmethod
    def poll(cls, context):
        scn = context.scene
        if scn.Chosen_filter == 'MOTION BLUR':
            return not check_mblur_remove()
        else:
            return not check_filters(scn.Chosen_filter)
        
    def execute(self, context):
        scn = context.scene
        
        set_filter = scn.Chosen_filter

        script_file = os.path.realpath(__file__)
        directory = os.path.dirname(script_file)
        if set_filter != 'MOTION BLUR':
            addFilter(set_filter,directory, scn.index_num)
        else:
            addMblur(directory, scn.index_num)
        scn.index_num += 1
        if scn.Chosen_filter != 'MOTION BLUR':
            report_string = 'Added '+set_filter+' to '+context.selected_objects[0].name
        else:
            report_string = 'Added MOTION BLUR to ' + scn.name
        self.report({'INFO'}, report_string)
        return{'FINISHED'}

#add mblur
def addMblur(directory, pass_num):
    used_camera = bpy.context.active_object
    bpy.ops.object.empty_add(type='SPHERE',view_align=False, location=(0,0,0))
    bpy.context.active_object.name = 'Mblur_empty'
    bpy.ops.object.game_property_new(name='distance')
    bpy.context.active_object.game.properties['distance'].value = 0.15
    for i in range(16):
        bpy.ops.object.game_property_new(name='x'+str(i))
    for j in range(16):
        bpy.ops.object.game_property_new(name='w'+str(j))
    
    #adds mblur 2d filter
    if 'mblur.glsl' not in bpy.data.texts:
        set_path = os.path.join(directory,'filters','mblur.txt')
        text = bpy.data.texts.load(set_path)
    
        for area in bpy.context.screen.areas:
            if area.type == 'TEXT_EDITOR':
                ctx = bpy.context.copy()
                ctx['edit_text'] = text
                
                area.spaces[0].text = bpy.data.texts['mblur.txt']  
                break
        bpy.data.texts['mblur.txt'].name = 'mblur.glsl'
    if 'mblur_camerainfo.py' not in bpy.data.texts:
        new_path = os.path.join(directory, 'filters', 'mblur_camerainfo.txt')
        text = bpy.data.texts.load(new_path)
        bpy.data.texts['mblur_camerainfo.txt'].name = 'mblur_camerainfo.py'
    
    #adding logic
    current = bpy.context.active_object
    bpy.ops.logic.sensor_add(type='ALWAYS', name='get_info',object='Mblur_empty')
    bpy.ops.logic.controller_add(type='PYTHON',object='Mblur_empty')
    link_sensor = current.game.sensors[-1]
    link_controller = current.game.controllers[-1]
    link_sensor.use_pulse_true_level = True
    link_sensor.show_expanded = False
    link_controller.text = bpy.data.texts['mblur_camerainfo.py']
    link_controller.show_expanded = False
    link_sensor.link(link_controller)  
    
    #adding 2d filter
    bpy.ops.logic.sensor_add(type='ALWAYS', name='filter_always',object='Mblur_empty')
    bpy.ops.logic.controller_add(type='LOGIC_AND',object='Mblur_empty')
    filter_sensor = current.game.sensors[-1]
    filter_controller = current.game.controllers[-1]
    filter_sensor.show_expanded = False
    filter_controller.show_expanded = False
    filter_sensor.link(filter_controller)
    bpy.ops.logic.actuator_add(type='FILTER_2D',name='mblur', object='Mblur_empty')
    added = current.game.actuators[-1]
        
    added.mode = 'CUSTOMFILTER'
    added.filter_pass = pass_num
    added.glsl_shader = bpy.data.texts['mblur.glsl']
    added.show_expanded = False
    added.link(filter_controller)
    for i in bpy.context.scene.objects:
        i.select = False
    bpy.context.scene.objects.active = bpy.data.objects[used_camera.name]
    bpy.context.scene.objects[used_camera.name].select = True

#add the correct filter with logic
def addFilter(new_filter,directory,pass_num):
    
    current = bpy.context.selected_objects[0]
    
    prop_dict = {'DOF': [],
    'SSAO': [],
    'SSGI':[],
    'BLOOM':[('bloom_amount', 'FLOAT', 0.5)],
    'NOISE':[('noise_amount','FLOAT',-0.1),('timer','TIMER',0.0)],
    'VIGNETTE':[('vignette_size','FLOAT',0.7)],
    'SHARPEN':[('Sharpness','FLOAT',1.8)],
    'RETINEX':[('retinex','FLOAT',-1.2)],
    'TECHNICOLOR':[],
    'DITHERED':[],
    'SSR REFLECTION':[('reflectance','FLOAT',0.2),('samples','INT',15),('roughness','FLOAT',0.13)],
    'CARTOON':[],
    'CHROMATIC ABERRATION':[('abberation', 'FLOAT', 0.82)],
    'WATER':[('timer','TIMER',0.0)],
    'BLOOM X':[('bloom_x', 'FLOAT', 0.4)],
    'BLEACH':[('bleach_amount', 'FLOAT', 1.0)],
    'COLOR CORRECTION':[],
    'BARREL':[],
    'FXAA':[],
    'GAMEBOY COLOR':[],
    'LEVELS':[],
    'NIGHT VISION':[('vision_strength','FLOAT', 7.0)],
    'HORIZONTAL':[('scale_y','FLOAT', 1.0),('scale_x','FLOAT', 1.1)],
    'RADIAL BLUR':[('radial_density', 'FLOAT', 0.05)],
    'RADIOSITY':[('xs', 'INT', 10)],
    'TOON':[],
    'HDR':[('avgL','FLOAT',0.0),('HDRamount','FLOAT',0.1)],
    'VHS I':[('Timer','TIMER',0.0)],
    'VHS II':[('timer','TIMER',0.0)],
    'BLUR':[('blurAmount','FLOAT', 2.0)],
    'ETHEREAL GLOW':[('intensity','FLOAT',9.0),('timer','TIMER',0.0)],
    'FADEABLE REVERSE':[('reverseAmount','FLOAT', 1.0)],
    'FRACTAL WAVEFORM':[('timer','TIMER',0.0)],
    'GLITCH':[('timer','TIMER',0.0)],
    'PIXELS':[('Pixeliz','FLOAT', 3.0)],
    'POSTERIZE':[('posterizeLevel','FLOAT',8.0)],
    'VGA GLITCH':[('timer','TIMER',0.0)],
    'WATER RIPPLE':[('amount','FLOAT',0.02),('timer','TIMER',0.0)],
    'WAVES':[('timer','TIMER',0.0)],
    'FADEABLE GLITCH':[('fade_glitch','FLOAT',0.01),('timer','TIMER',0.0)],
    'LIGHT SCATTER':[('weight','FLOAT',0.3),('dec','FLOAT',0.8),('dens','FLOAT',0.45),('sun_x','FLOAT',0.5),('sun_y','FLOAT',0.8)],
    'LENS FLARE':[('lf_sunX','FLOAT',0.0),('lf_sunY','FLOAT',0.0),('sundirect','FLOAT',0.0),('timer','TIMER',0.0),('Sun_Name','STRING',"Lamp")],
    'CRT':[('timer','TIMER',0.0)],
    'CRT_DISTORTION':[],
    'GAMMA':[('gamma',"FLOAT",1.5)]}
    
    property_list = prop_dict[new_filter]
    if '.' in new_filter:
        new_filter = derive_name(new_filter)
        
    filter_name = new_filter +'.txt'
    
    if new_filter+'.glsl' not in bpy.data.texts:
        set_path = os.path.join(directory,'filters',filter_name)
        text = bpy.data.texts.load(set_path)
    
        for area in bpy.context.screen.areas:
            if area.type == 'TEXT_EDITOR':
                ctx = bpy.context.copy()
                ctx['edit_text'] = text
                
                area.spaces[0].text = bpy.data.texts[new_filter + '.txt']                
                break
        bpy.data.texts[new_filter +'.txt'].name = new_filter +'.glsl'
        bpy.data.texts[new_filter +'.glsl'].use_fake_user = True

        
    #additional scripts for filter
    if new_filter == 'HDR':
        if 'ReadLum.py' not in bpy.data.texts:
            new_path = os.path.join(directory, 'filters', 'ReadLum.txt')
            text = bpy.data.texts.load(new_path)
            bpy.data.texts['ReadLum.txt'].name = 'ReadLum.py'
        bpy.ops.logic.sensor_add(type='ALWAYS', name='check_lum',object=current.name)
        bpy.ops.logic.controller_add(type='PYTHON',object=current.name)
        link_sensor = current.game.sensors[-1]
        link_controller = current.game.controllers[-1]
        link_sensor.use_pulse_true_level = True
        link_sensor.show_expanded = False
        link_controller.text = bpy.data.texts['ReadLum.py']
        link_controller.show_expanded = False
        link_sensor.link(link_controller)        
    
    if new_filter == 'LENS FLARE':
        if 'LENS FLARE_sun_Pos.py' not in bpy.data.texts:
            new_path = os.path.join(directory, 'filters', 'LENS FLARE_sun_Pos.txt')
            text = bpy.data.texts.load(new_path)
            bpy.data.texts['LENS FLARE_sun_Pos.txt'].name = 'LENS FLARE_sun_Pos.py'
        bpy.ops.logic.sensor_add(type='ALWAYS', name='LF_update',object=current.name)
        bpy.ops.logic.controller_add(type='PYTHON',object=current.name)
        link_sensor = current.game.sensors[-1]
        link_controller = current.game.controllers[-1]
        link_sensor.use_pulse_true_level = True
        link_sensor.show_expanded = False
        link_controller.text = bpy.data.texts['LENS FLARE_sun_Pos.py']
        link_controller.show_expanded = False
        link_sensor.link(link_controller)
    
    #check for current always sensor if non existant add a new one
    if 'filter_always' not in current.game.sensors:
        bpy.ops.logic.sensor_add(type='ALWAYS', name='filter_always',object=current.name)
        bpy.ops.logic.controller_add(type='LOGIC_AND',object=current.name)
        link_sensor = current.game.sensors[-1]
        link_controller = current.game.controllers[-1]
        link_sensor.show_expanded = False
        link_controller.show_expanded = False
        link_sensor.link(link_controller)

    sensor = current.game.sensors['filter_always']
    controller = sensor.controllers[0]
    
    bpy.ops.logic.actuator_add(type='FILTER_2D',name=new_filter, object=current.name)
    added = current.game.actuators[-1]
    
    added.mode = 'CUSTOMFILTER'
    added.filter_pass = pass_num
    added.glsl_shader = bpy.data.texts[new_filter +'.glsl']
    
    added.link(controller)
    
    if len(property_list) > 0:
        for prop in property_list:
            if prop[0] not in current.game.properties:
                bpy.ops.object.game_property_new(type=prop[1],name=prop[0])
                current.game.properties[prop[0]].value = prop[2]

def removeFilter(selected_filter):
    if selected_filter != 'MOTION BLUR':
        
        if '.' in selected_filter:
            selected_filter = derive_name(selected_filter)     
        current = bpy.context.selected_objects[0]
        
        filters_used = []
        
        for i in bpy.data.cameras:
            if i.name in bpy.context.scene.objects.keys():
                current = bpy.context.scene.objects[i.name]
                for j in current.game.actuators:
                    if j.type == 'FILTER_2D' and j.mode == "CUSTOMFILTER" and j.glsl_shader != None:
                        filters_used.append(j.glsl_shader.name)
        current = bpy.context.selected_objects[0]
        #remove logic
        sensor1 = current.game.sensors['filter_always']
        controller1 = sensor1.controllers[0]
        if len(controller1.actuators) < 2:
            bpy.ops.logic.sensor_remove(sensor=sensor1.name, object=current.name)
            bpy.ops.logic.controller_remove(controller=controller1.name, object=current.name)
        bpy.ops.logic.actuator_remove(actuator=selected_filter, object=current.name)
        
        #remove properties
        prop_dict = {'DOF': [],
        'SSAO': [],
        'SSGI':[],
        'BLOOM':['bloom_amount'],
        'NOISE':['noise_amount','timer'],
        'VIGNETTE':['vignette_size'],
        'SHARPEN':['Sharpness'],
        'RETINEX':['retinex'],
        'TECHNICOLOR':[],
        'DITHERED':[],
        'SSR REFLECTION':['roughness','reflectance','samples'],
        'WATER':['timer'],
        'CARTOON':[],
        'CHROMATIC ABERRATION':['abberation'],
        'BLOOM X':['bloom_x'],
        'BLEACH':['bleach_amount'],
        'COLOR CORRECTION':[],
        'BARREL':[],
        'FXAA':[],
        'GAMEBOY COLOR':[],
        'LEVELS':[],
        'NIGHT VISION':['vision_strength'],
        'HORIZONTAL':['scale_y','scale_x'],
        'RADIAL BLUR':['radial_density'],
        'RADIOSITY':['xs'],
        'TOON':[],
        'HDR':['avgL', 'HDRamount'],
        'VHS I':['Timer'],
        'VHS II':['timer'],
        'BLUR':['blurAmount'],
        'ETHEREAL GLOW':['intensity','timer'],
        'FADEABLE REVERSE':['reverseAmount'],
        'FRACTAL WAVEFORM':['timer'],
        'GLITCH':['timer'],
        'PIXELS':['Pixeliz'],
        'POSTERIZE':['posterizeLevel'],
        'VGA GLITCH':['timer'],
        'WATER RIPPLE':['amount','timer'],
        'WAVES':['timer'],
        'FADEABLE GLITCH':['fade_glitch','timer'],
        'LIGHT SCATTER':['weight','dec','dens','sun_x','sun_y'],
        'LENS FLARE':['lf_sunX','lf_sunY','sundirect','timer','Sun_Name'],
        'CRT':['timer'],
        'CRT_DISTORTION':[],
        'GAMMA':['gamma']}
        
        timer_props = ['NOISE.glsl','WATER.glsl','ETHEREAL GLOW.glsl','FRACTAL WAVEFORM.glsl','GLITCH.glsl','VGA GLITCH.glsl','WATER RIPPLE.glsl','WAVES.glsl','FADEABLE GLITCH.glsl','CRT.glsl','VHS I.glsl','VHS II.glsl','LENS FLARE.glsl']
        remove_list = prop_dict[selected_filter]
        
        
        for prop in remove_list:
            for each in range(len(current.game.properties)):
                #checks if property is the correct one.
                if current.game.properties[each].name == prop:
                    
                    if prop == 'timer':
                        #removes the current filter
                        timer_props.remove(selected_filter+'.glsl')
                        remove_timer = True
                        for i in timer_props:
                            if i in filters_used:
                                remove_timer = False
                                break
                        if remove_timer == True:                
                            bpy.ops.object.game_property_remove(index=each)
                            break
                        else:
                            break
                    else:
                        bpy.ops.object.game_property_remove(index=each)
                        break
        bpy.data.texts.remove(bpy.data.texts[selected_filter +'.glsl'], do_unlink=True)
    else:
        used_camera = bpy.context.active_object
        for ob in bpy.context.scene.objects:
            ob.select = ob.name == 'Mblur_empty'
        bpy.ops.object.delete()
        bpy.data.texts.remove(bpy.data.texts['mblur.glsl'], do_unlink=True)
        bpy.data.texts.remove(bpy.data.texts['mblur_camerainfo.py'], do_unlink=True)
        for i in bpy.context.scene.objects:
                i.select = False
        bpy.context.scene.objects.active = bpy.data.objects[used_camera.name]
        bpy.context.scene.objects[used_camera.name].select = True        
        
    #reassign passes
    
    filters_used = []
    
    scn = bpy.context.scene
    
    for i in scn.objects:
        if i.type == 'CAMERA':
            current = bpy.context.scene.objects[i.name]
            for j in current.game.actuators:
                if j.type == 'FILTER_2D' and j.mode == 'CUSTOMFILTER' and j.glsl_shader != None:
                    filters_used.append(j.glsl_shader.name)
    if 'Mblur_empty' in bpy.context.scene.objects:
        filters_used.append('Mblur_empty')
    if len(filters_used) == 0:
        scn.index_num = 0
    
    #removing HDr logic
    if selected_filter == 'HDR':
        #remove logic
        sensor1 = current.game.sensors['check_lum']
        controller1 = sensor1.controllers[0]
        bpy.ops.logic.sensor_remove(sensor=sensor1.name, object=current.name)
        bpy.ops.logic.controller_remove(controller=controller1.name, object=current.name)
        bpy.data.texts.remove(bpy.data.texts['ReadLum.py'])
    
    #removing Lens Flare logic
    if selected_filter == 'LENS FLARE':
        #remove logic
        sensor1 = current.game.sensors['LF_update']
        controller1 = sensor1.controllers[0]
        bpy.ops.logic.sensor_remove(sensor=sensor1.name, object=current.name)
        bpy.ops.logic.controller_remove(controller=controller1.name, object=current.name)
        bpy.data.texts.remove(bpy.data.texts['LENS FLARE_sun_Pos.py'])

def check_mblur_remove():
    return 'Mblur_empty' in bpy.context.scene.objects
    
def check_filters(filter_tocheck):
    
    if '.' in filter_tocheck:
        selected = derive_name(filter_tocheck)     
    else:
        selected = filter_tocheck
    
    for i in bpy.context.scene.objects:
        current = bpy.context.scene.objects[i.name]
        for j in current.game.actuators:
            if j.type == 'FILTER_2D' and j.mode == 'CUSTOMFILTER' and j.glsl_shader != None:
                if selected +'.glsl' == j.glsl_shader.name:
                    return True
    return False


def check_remove(filter_tocheck):
    
    if '.' in filter_tocheck:
        chosen = derive_name(filter_tocheck)
    else:
        chosen = filter_tocheck
    #check that the desired filter is on selected object
    for i in bpy.context.selected_objects[0].game.actuators:
        if i.type == 'FILTER_2D' and i.mode == 'CUSTOMFILTER' and i.glsl_shader != None:
            if chosen+'.glsl' == i.glsl_shader.name:
                return True
    return False

def derive_name(name):
    new_name = ''
    for letter in name:
        if letter != '.':
            new_name += letter
    return new_name
  

def register():
    bpy.types.Scene.index_num = IntProperty(default = 0)
    bpy.types.Scene.Chosen_filter = EnumProperty(items = [('HDR','HDR','Adjusts light levels to suit environment'),
    ('HORIZONTAL', 'HORIZONTAL', 'This effect stretches the screen along the X and Y axis'),
    ('GAMMA','GAMMA', 'Brightens dark spaces'),
    ('GAMEBOY COLOR','GAMEBOY COLOR', 'Gameboy style pixelization.'),
    ('GLITCH', 'GLITCH', 'Glitch Effect'),
    ('DOF','DOF','Adds depth of field'),
    ('DITHERED','DITHERED','It gives an old style to the game'),
    ('CRT','CRT','Old Big Screens'),
    ('CARTOON','CARTOON','An effect that makes the game look cartoonish'),
    ('COLOR CORRECTION', 'COLOR CORRECTION', 'Sweetening the colors of the world'),
    ('CHROMATIC ABERRATION','CHROMATIC ABERRATION','Coloured Fringing'),
    ('CRT_DISTORTION','CRT_DISTORTION','It creates refraction with blur shader'),
    ('BLOOM X','BLOOM X', 'Adds X shaped glow to objects'),
    ('BLOOM','BLOOM', 'Adds glow to bright objects'),
    ('BLUR', 'BLUR', 'Screen Blur'),
    ('BLEACH', 'BLEACH', 'Horror game filter. Low saturation, high contrast.'),
    ('BARREL', 'BARREL', 'Adds buldge distortion to the centre of the screen'),
    ('SHARPEN','SHARPEN', 'Improve detail clarity and make edges appear sharper'),
    ('SSAO','SSAO', 'Darkens edges of objects'),
    ('SSGI','SSGI', 'Improved global lighting quality'),
    ('SSR REFLECTION','SSR REFLECTION','This shader mirrors objects realistically'),
    ('TOON','TOON','Adds toon outline'),
    ('TECHNICOLOR','TECHNICOLOR','Shading enhances the colors of the world'),
    ('VHS I','VHS I','An effect that makes the screen vibrate like the old one'),
    ('VHS II','VHS II','The effect makes the screen scene distorted and old'),
    ('VGA GLITCH', 'VGA GLITCH', 'Bad Cable Noise'),
    ('VIGNETTE','VIGNETTE', 'Darkens the edges of the screen'),
    ('RADIAL BLUR', 'RADIAL BLUR', 'Adds blur around the edges of the screen'),
    ('RADIOSITY', 'RADIOSITY', 'An effect that creates a reflection of lighting to make the scene realistic'),
    ('RETINEX','RETINEX','Enhances contrast, maintains color.'),
    ('NOISE','NOISE','Adds noise to the screen'),
    ('NIGHT VISION', 'NIGHT VISION', 'Infared style night vision'),
    ('MOTION BLUR', 'MOTION BLUR', 'Adds Motion Blur'),
    ('ETHEREAL GLOW', 'ETHEREAL GLOW', 'Glow Around The Screen'),
    ('FXAA','FXAA','Efficient Anti-Aliasing'),
    ('FADEABLE REVERSE', 'FADEABLE REVERSE', 'Reverse Color'),
    ('FADEABLE GLITCH','FADEABLE GLITCH','Glitch with Controller'),
    ('FRACTAL WAVEFORM', 'FRACTAL WAVEFORM', 'Film Noise'),
    ('PIXELS', 'PIXELS', 'Pixel Shader'),
    ('POSTERIZE', 'POSTERIZE', 'Limit Rendered Color'),
    ('WATER','WATER','Underwater distortion effect'),
    ('WATER RIPPLE', 'WATER RIPPLE', 'I really dont know what is this'),
    ('WAVES','WAVES','sea sickness'),
    ('LIGHT SCATTER','LIGHT SCATTER','Shader makes luminous objects radiate'),
    ('LENS FLARE','LENS FLARE','my eyes are burning'),
    ('LEVELS','LEVELS', 'Color channel remapping with curves')],
    name = "Choose Filter", default='POSTERIZE')
    bpy.utils.register_module(__name__)
    

def unregister():
    bpy.utils.unregister_module(__name__)
    del bpy.types.Scene.index_num
    del bpy.types.Scene.Chosen_filter