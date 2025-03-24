# ##### BEGIN GPL LICENSE BLOCK #####
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software Foundation,
#  Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
#
# ##### END GPL LICENSE BLOCK #####

# <pep8 compliant>
import bpy
from bpy.types import Header, Menu, Panel

class LOGIC_PT_components(bpy.types.Panel):
    bl_space_type = 'LOGIC_EDITOR'
    bl_region_type = 'UI'
    bl_label = 'Game Components'

    @classmethod
    def poll(cls, context):
        ob = context.active_object
        return ob and ob.name

    def draw(self, context):
        layout = self.layout

        ob = context.active_object
        game = ob.game
        
        st = context.space_data
        scene = context.scene

        row = layout.row()
        row.operator("logic.python_component_register", text="Register Component", icon="FILE_SCRIPT")
        row.operator("logic.python_component_create", text="Create Component", icon="ZOOMIN")
        for i, c in enumerate(game.components):
            box = layout.box()
            row = box.row(align=1)
            row.prop(c, "show_expanded", text="", emboss=0)
            if "C_Icons" in c.properties:
                try:
                    icondict = c.properties["C_Icons"].value.split("+")
                    row.label(c.name, icon=icondict[0])
                except:
                    row.label(c.name)
            else:
                row.label(c.name)
            
            row.operator("logic.python_component_reload", icon="RECOVER_LAST", text="").index = i
            # row.separator()
            row.operator("logic.python_component_move_up", icon="TRIA_UP", text="").index = i
            row.operator("logic.python_component_move_down", icon="TRIA_DOWN", text="").index = i
            row = row.row(align=0)
            row.operator("logic.python_component_remove", text="", icon='X').index = i
            lastHeader = None

            if len(c.properties) == 1 and c.properties[0].name == "C_Icons": continue
            if c.show_expanded and len(c.properties) > 0:
                box = box.box().column()
                iconval = 1
                for prop in c.properties:
                    row = box.row()
                    if prop.name[:8] == "C_Header":
                        row = row.box()
                        icon="FULLSCREEN"
                        split=prop.name.split("/")
                        if len(split) > 1: icon=split[1]
                        
                        row.label(text=prop.value, icon=icon)
                        lastHeader = row.column()
                        continue
                    
                    if prop.name == "C_Icons":
                        if type(prop.value) != type(str()):
                            print("warning: {} in 'C_Icons' Must be STRING! like that ('C_Icons','BLENDER+QUESTION')".format(c.name))
                        continue
                    text=prop.name
                    if lastHeader: row = lastHeader.row(); text = "- {}".format(text)
                    
                    try:
                        if "C_Icons" in c.properties:
                            row.label(text=text, icon=icondict[iconval])
                            iconval += 1
                            col = row.column()
                            col.prop(prop, "value", text="")
                        else:
                            row.label(text=text)
                            col = row.column()
                            col.prop(prop, "value", text="")
                    except:
                        row.label(text=text)
                        col = row.column()
                        col.prop(prop, "value", text="")

class LOGIC_PT_properties(Panel):
    bl_space_type = 'LOGIC_EDITOR'
    bl_region_type = 'UI'
    bl_label = "Properties"

    @classmethod
    def poll(cls, context):
        ob = context.active_object
        return ob and ob.game

    def draw(self, context):
        layout = self.layout

        ob = context.active_object
        game = ob.game
        is_font = (ob.type == 'FONT')

        if is_font:
            prop_index, prop_indexR = game.properties.find("Text"), game.properties.find("Text-Res")

            if prop_index != -1:
                layout.operator("object.game_property_remove", text="Remove Text Game Property", icon='X').index = prop_index
                row = layout.row()
                sub = row.row()
                sub.enabled = 0
                prop = game.properties[prop_index]
                sub.prop(prop, "name", text="", icon="FONT_DATA")
                row.prop(prop, "type", text="")
                row.label("See Text Object")
                
                if prop_indexR != -1:
                    sub = row.row(align=True)
                    sub.operator("object.game_property_remove", text="", icon='X').index = prop_indexR
                    row = layout.row()
                    sub = row.row()
                    sub.enabled = 0
                    prop = game.properties[prop_indexR]
                    sub.prop(prop, "name", text="", icon="FONT_DATA")
                    row.prop(prop, "value", text="Resolution")
                    row.label("Text Resolution(0-50)")
                else:
                    sub = row.row(align=True)
                    propsR = sub.operator("object.game_property_new", text="", icon='ZOOMIN',)
                    propsR.name = "Text-Res"
                    propsR.type = 'FLOAT'
            else:
                props = layout.operator("object.game_property_new", text="Add Text Game Property", icon='ZOOMIN')
                props.name = "Text"
                props.type = 'STRING'
                
        props = layout.operator("object.game_property_new", text="Add Game Property", icon='ZOOMIN')
        props.name = ""

        for i, prop in enumerate(game.properties):
            if is_font and i == prop_index or is_font and i == prop_indexR:
                continue

            box = layout.box()
            row = box.row()
            row.prop(prop, "name", text="")
            row.prop(prop, "type", text="")
            row.prop(prop, "value", text="")
            row.prop(prop, "show_debug", text="", toggle=True, icon='INFO')
            sub = row.row(align=True)
            props = sub.operator("object.game_property_move", text="", icon='TRIA_UP')
            props.index = i
            props.direction = 'UP'
            props = sub.operator("object.game_property_move", text="", icon='TRIA_DOWN')
            props.index = i
            props.direction = 'DOWN'
            row.operator("object.game_property_remove", text="", icon='X', emboss=False).index = i


class LOGIC_MT_logicbricks_add(Menu):
    bl_label = "Add"

    def draw(self, context):
        layout = self.layout

        layout.operator_menu_enum("logic.sensor_add", "type", text="Sensor")
        layout.operator_menu_enum("logic.controller_add", "type", text="Controller")
        layout.operator_menu_enum("logic.actuator_add", "type", text="Actuator")


class LOGIC_HT_header(Header):
    bl_space_type = 'LOGIC_EDITOR'

    def draw(self, context):
        layout = self.layout.row(align=True)

        layout.template_header()

        LOGIC_MT_editor_menus.draw_collapsible(context, layout)


class LOGIC_MT_editor_menus(Menu):
    bl_idname = "LOGIC_MT_editor_menus"
    bl_label = ""

    def draw(self, context):
        self.draw_menus(self.layout, context)

    @staticmethod
    def draw_menus(layout, context):
        layout.menu("LOGIC_MT_view")
        layout.menu("LOGIC_MT_logicbricks_add")


class LOGIC_MT_view(Menu):
    bl_label = "View"

    def draw(self, context):
        layout = self.layout

        layout.operator("logic.properties", icon='MENU_PANEL')

        layout.separator()

        layout.operator("screen.area_dupli")
        layout.operator("screen.screen_full_area")
        layout.operator("screen.screen_full_area", text="Toggle Fullscreen Area").use_hide_panels = True


classes = (
    LOGIC_PT_components,
    LOGIC_PT_properties,
    LOGIC_MT_logicbricks_add,
    LOGIC_HT_header,
    LOGIC_MT_editor_menus,
    LOGIC_MT_view,
)

if __name__ == "__main__":  # only for live edit.
    from bpy.utils import register_class
    for cls in classes:
        register_class(cls)
