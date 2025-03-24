import os
import bpy
import bge_netlogic


class TreeCodeWriterOperator(bpy.types.Operator):
    bl_idname = "bgenetlogic.treecodewriter_operator"
    bl_label = "Timed code writer"
    bl_options = {'REGISTER', 'UNDO'}
    timer = None

    def modal(self, context, event):
        if event.type == "TIMER":
            bge_netlogic._consume_update_tree_code_queue()
        return {'PASS_THROUGH'}

    def execute(self, context):
        if context.window is None:
            bge_netlogic._tree_code_writer_started = False
            return {"FINISHED"}
        if context.window_manager is None:
            bge_netlogic._tree_code_writer_started = False
            return {"FINISHED"}
        if self.timer is not None:
            return {'FINISHED'}
        self.timer = context.window_manager.event_timer_add(
            1.0,
            window=context.window
        )
        context.window_manager.modal_handler_add(self)
        return {"RUNNING_MODAL"}


class WaitForKeyOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.waitforkey"
    bl_label = "Press a Key"
    bl_options = {'REGISTER', 'UNDO'}
    keycode = bpy.props.StringProperty()

    def __init__(self):
        self.socket = None
        self.node = None

    def __del__(self):
        pass

    def execute(self, context):
        return {'FINISHED'}

    def cleanup(self, context):
        if self.socket.value == "Press a key...":
            self.socket.value = ""
        self.socket = None
        self.node = None
        context.region.tag_redraw()

    def modal(self, context, event):
        if event.value == "PRESS":
            if (
                event.type == "LEFTMOUSE" or
                event.type == "MIDDLEMOUSE" or
                event.type == "RIGHTMOUSE"
            ):
                self.socket.value = "Press & Choose"
                return {'FINISHED'}
            else:
                value = event.type
                if(self.socket):
                    self.socket.value = value
                else:
                    self.node.value = value
                self.cleanup(context)
                return {'FINISHED'}
        return {'PASS_THROUGH'}

    def invoke(self, context, event):
        self.socket = context.socket
        self.node = context.node

        if(not self.socket) and (not self.node):
            print("no socket or node")
            return {'FINISHED'}

        if(self.socket):
            self.socket.value = "Press a key..."

        else:
            self.node.value = "Press a key..."
        context.region.tag_redraw()
        context.window_manager.modal_handler_add(self)
        return {'RUNNING_MODAL'}


class NLImportProjectNodes(bpy.types.Operator):
    bl_idname = "bge_netlogic.import_nodes"
    bl_label = "Import Logic Nodes"
    bl_options = {'REGISTER', 'UNDO'}
    filepath = bpy.props.StringProperty(subtype="FILE_PATH")

    @classmethod
    def poll(cls, context):
        tree_type = context.space_data.tree_type
        return tree_type == bge_netlogic.ui.BGELogicTree.bl_idname

    def _create_directories(self):
        local_bge_netlogic_folder = bpy.path.abspath("//bgelogic")
        if not os.path.exists(local_bge_netlogic_folder):
            os.mkdir(local_bge_netlogic_folder)
        local_cells_folder = bpy.path.abspath("//bgelogic/cells")
        if not os.path.exists(local_cells_folder):
            os.mkdir(local_cells_folder)
        local_nodes_folder = bpy.path.abspath("//bgelogic/nodes")
        if not os.path.exists(local_nodes_folder):
            os.mkdir(local_nodes_folder)
        return local_cells_folder, local_nodes_folder

    def _entry_filename(self, p):
        ws = p.rfind("\\")
        us = p.rfind("/")
        if us >= 0 and us > ws:
            return p.split("/")[-1]
        if ws >= 0 and ws > us:
            return p.split("\\")[-1]
        return p

    def _generate_unique_filename(self, output_dir, file_name):
        dot_index = file_name.rfind(".")
        name_part = file_name[:dot_index]
        ext_part = file_name[dot_index + 1:]
        path = os.path.join(output_dir, file_name)
        index = 0
        while os.path.exists(path):
            name = '{}_{}.{}'.format(name_part, index, ext_part)
            path = os.path.join(output_dir, name)
            index += 1
            if index > 100:
                raise RuntimeError(
                    "Can't find a unique name for {}".format(file_name)
                )
        return path

    def _zipextract(self, zip, entry_name, output_dir):
        import shutil
        with zip.open(entry_name) as entry:
            out_file = self._generate_unique_filename(
                output_dir, self._entry_filename(entry_name)
            )
            with open(out_file, "wb") as f:
                shutil.copyfileobj(entry, f)

    def execute(self, context):
        import zipfile
        if not self.filepath:
            return {"FINISHED"}

        if not self.filepath.endswith(".zip"):
            return {"FINISHED"}

        if not zipfile.is_zipfile(self.filepath):
            return {"FINISHED"}

        with zipfile.ZipFile(self.filepath, "r") as f:
            entries = f.namelist()
            cells = [
                x for x in entries if x.startswith("bgelogic/cells/") and
                x.endswith(".py")
            ]
            nodes = [
                x for x in entries if x.startswith("bgelogic/nodes/") and
                x.endswith(".py")
            ]
            if cells or nodes:
                local_cells_folder, local_nodes_folder = (
                    self._create_directories()
                )
                for cell in cells:
                    self._zipextract(f, cell, local_cells_folder)
                for node in nodes:
                    self._zipextract(f, node, local_nodes_folder)
        _do_load_project_nodes(context)
        return {"FINISHED"}

    def invoke(self, context, event):
        self.filepath = ""
        context.window_manager.fileselect_add(self)
        return {"RUNNING_MODAL"}


def _do_load_project_nodes(context):
    print("loading project nodes and cells...")
    current_file = context.blend_data.filepath
    file_dir = os.path.dirname(current_file)
    netlogic_dir = os.path.join(file_dir, "bgelogic")
    # cells_dir = os.path.join(netlogic_dir, "cells")
    nodes_dir = os.path.join(netlogic_dir, "nodes")
    if os.path.exists(nodes_dir):
        bge_netlogic.remove_project_user_nodes()
        bge_netlogic.load_nodes_from(nodes_dir)


class NLLoadProjectNodes(bpy.types.Operator):
    bl_idname = "bge_netlogic.load_nodes"
    bl_label = "Reload Project Nodes"
    bl_options = {'REGISTER', 'UNDO'}
    bl_description = "Reload the custom nodes' definitions."

    @classmethod
    def poll(cls, context):
        current_file = context.blend_data.filepath
        if not current_file:
            return False
        if not os.path.exists(current_file):
            return False
        tree = context.space_data.edit_tree
        if not tree:
            return False
        if not (tree.bl_idname == bge_netlogic.ui.BGELogicTree.bl_idname):
            return False
        return context.space_data.edit_tree is not None

    def execute(self, context):
        _do_load_project_nodes(context)
        return {"FINISHED"}


class NLSelectTreeByNameOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.select_tree_by_name"
    bl_label = "Edit"
    bl_description = "Edit"
    tree_name = bpy.props.StringProperty()

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        assert self.tree_name is not None
        assert len(self.tree_name) > 0
        blt_groups = [
            g for g in bpy.data.node_groups if (
                g.name == self.tree_name
            ) and (
                g.bl_idname == bge_netlogic.ui.BGELogicTree.bl_idname
            )
        ]
        if len(blt_groups) != 1:
            print("Something went wrong here...")
        for t in blt_groups:
            context.space_data.node_tree = t
        return {'FINISHED'}


class NLRemoveTreeByNameOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.remove_tree_by_name"
    bl_label = "Remove"
    bl_options = {'REGISTER', 'UNDO'}
    bl_description = "Remove the tree from the selected objects"
    tree_name = bpy.props.StringProperty()

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        import bge_netlogic.utilities as tools
        stripped_tree_name = tools.strip_tree_name(self.tree_name)
        py_module_name = tools.py_module_name_for_stripped_tree_name(
            stripped_tree_name
        )
        py_module_name = py_module_name.split('NL')[-1]
        objs = [
            ob for ob in context.scene.objects if ob.select_get() and
            tools.object_has_treeitem_for_treename(
                ob, self.tree_name
            )
        ] if not bpy.app.version < (2, 80, 0) else [
            ob for ob in context.scene.objects if ob.select and
            tools.object_has_treeitem_for_treename(
                ob, self.tree_name
            )
        ]
        for ob in objs:
            gs = ob.game
            controllers = [
                c for c in gs.controllers if py_module_name in c.name
            ]
            actuators = [
                a for a in gs.actuators if py_module_name in a.name
            ]
            sensors = [
                s for s in gs.sensors if py_module_name in s.name
            ]
            for s in sensors:
                print("Removed Sensor", s.name, "from", ob.name)
                bpy.ops.logic.sensor_remove(sensor=s.name, object=ob.name)
            for c in controllers:
                print("Removed Controller", c.name, "from", ob.name)
                bpy.ops.logic.controller_remove(
                    controller=c.name, object=ob.name
                )
            for a in actuators:
                print("Removed Actuator", a.name, "from", ob.name)
                bpy.ops.logic.actuator_remove(actuator=a.name, object=ob.name)

            bge_netlogic.utilities.remove_tree_item_from_object(
                ob, self.tree_name
            )
            bge_netlogic.utilities.remove_network_initial_status_key(
                ob, self.tree_name
            )
            print("Succsessfully removed tree {} from object {}.".format(
                self.tree_name,
                ob.name
            ))
        return {'FINISHED'}

    def remove_tree_from_object_pcoll(self, ob, treename):
        index = None
        i = 0
        for item in ob.bgelogic_treelist:
            if item.tree_name == treename:
                index = i
                break
            i += 1
        if index is not None:
            ob.bgelogic_treelist.remove(index)


class NLMakeGroupOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.make_group"
    bl_label = "Pack Into New Tree"
    bl_description = "Convert selected Nodes to a new tree. Will be applied to selected object. WARNING: All Nodes connected to selection must be selected too"
    bl_options = {'REGISTER', 'UNDO'}
    owner = bpy.props.StringProperty()

    @classmethod
    def poll(cls, context):
        return True

    def _index_of(self, item, a_iterable):
        i = 0
        for e in a_iterable:
            if e == item:
                return i
            i += 1

    def group_make(self, group_name, add_nodes):
        node_tree = bpy.data.node_groups.new(group_name, 'BGELogicTree')
        group_name = node_tree.name
        attrs = [
            'value',
            'default_value',
            'use_toggle',
            'true_label',
            'false_label',
            'value_type',
            'bool_editor',
            'int_editor',
            'float_editor',
            'string_editor',
            'radians',
            'float_field',
            'expression_field',
            'input_type',
            'value_x',
            'value_y',
            'value_z',
            'title',
            'local',
            'operator',
            'pulse',
            'hide',
            'label'
        ]

        nodes = node_tree.nodes
        new_nodes = {}
        parent_tree = bpy.context.space_data.edit_tree
        locs = []

        for node in add_nodes:
            added_node = nodes.new(node.bl_idname)
            added_node.location = node.location
            new_nodes[node] = added_node

        for old_node in new_nodes:
            new_node = new_nodes[old_node]
            for attr in dir(old_node):
                if attr in attrs:
                    setattr(new_node, attr, getattr(old_node, attr))
            for socket in old_node.inputs:
                index = self._index_of(socket, old_node.inputs)
                for attr in dir(socket):
                    if attr in attrs:
                        try:
                            setattr(new_node.inputs[index], attr, getattr(socket, attr))
                        except Exception:
                            print('Attribute {} not writable.'.format(attr))
                for link in socket.links:
                    try:
                        output_socket = link.from_socket
                        output_node = new_nodes[output_socket.node]
                        outdex = self._index_of(output_socket, output_socket.node.outputs)
                        node_tree.links.new(new_node.inputs[index], output_node.outputs[outdex])
                    except Exception as e:
                        bpy.data.node_groups.remove(node_tree)
                        self.report({"ERROR"}, "Some linked Nodes are not added to the group!")
                        return None
            locs.append(old_node.location)

        for old_node in new_nodes:
            parent_tree.nodes.remove(old_node)
        redir = parent_tree.nodes.new('NLActionExecuteNetwork')
        redir.inputs[0].value = True

        try:
            redir.inputs[1].value = bpy.context.object
        except Exception:
            self.report({"WARNING"}, 'No Object was selected; Set Object in tree {} manually!'.format(parent_tree.name))
        redir.inputs[2].value = group_name
        redir.location = self.avg_location(locs)
        node_tree.use_fake_user = True
        return node_tree

    def avg_location(self, locs):
        avg_x = 0
        avg_y = 0
        for v in locs:
            avg_x += v[0]
            avg_y += v[1]
        avg_x /= len(locs)
        avg_y /= len(locs)
        return (avg_x, avg_y)

    def execute(self, context):
        nodes_to_group = []
        tree = context.space_data.edit_tree

        if tree is None:
            return {'FINISHED'}
        for node in tree.nodes:
            if node.select:
                nodes_to_group.append(node)
        if len(nodes_to_group) > 0:
            self.group_make(tree.name + '_part', nodes_to_group)
            bge_netlogic._update_all_logic_tree_code()
        return {'FINISHED'}


class NLAdd4KeyTemplateOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.add_4_key_temp"
    bl_label = "4 Key Movement"
    bl_description = "Add 4 Key Movement (WASD with normalized vector)"
    bl_options = {'REGISTER', 'UNDO'}
    owner = bpy.props.StringProperty()

    @classmethod
    def poll(cls, context):
        return True

    def add_key_cond(self, tree, key, ypos, nodes):
        key_cond = tree.nodes.new('NLKeyPressedCondition')
        key_cond.inputs[0].value = key
        key_cond.label = key + " Key"
        key_cond.location = (0, ypos)
        key_cond.pulse = True
        nodes.append(key_cond)
        return key_cond

    def add_movement_setter(self, tree, name, key, axis, value, node_list, xpos=220, ypos=0):
        set_movement = tree.nodes.new('NLActionSetGlobalValue')
        tree.links.new(key.outputs[0], set_movement.inputs[0])
        set_movement.inputs[1].value = 'movement'
        set_movement.inputs[3].value = axis
        set_movement.inputs[4].value_type = 'INTEGER'
        set_movement.inputs[4].int_editor = value
        set_movement.label = name
        set_movement.location = (xpos, ypos)
        node_list.append(set_movement)
        return set_movement

    def add_movement_getter(self, tree, name, axis, node_list, xpos=0, ypos=-160):
        set_movement = tree.nodes.new('NLParameterGetGlobalValue')
        set_movement.inputs[0].value = 'movement'
        set_movement.inputs[1].value = axis
        set_movement.label = name
        set_movement.location = (xpos, ypos)
        node_list.append(set_movement)
        return set_movement

    def add_or_cond(self, tree, name, node_1, node_2, node_list, xpos=440, ypos=-20):
        or_cond = tree.nodes.new('NLConditionOrNode')
        tree.links.new(node_1.outputs[0], or_cond.inputs[0])
        tree.links.new(node_2.outputs[0], or_cond.inputs[1])
        or_cond.label = name
        or_cond.location = (xpos, ypos)
        node_list.append(or_cond)
        return or_cond

    def add_invert_bool(self, tree, name, from_node, node_list, xpos=660, ypos=-20):
        invert_bool = tree.nodes.new('NLInvertBoolNode')
        tree.links.new(from_node.outputs[0], invert_bool.inputs[0])
        invert_bool.label = name
        invert_bool.location = (xpos, ypos)
        node_list.append(invert_bool)
        return invert_bool

    def execute(self, context):
        tree = context.space_data.edit_tree
        nodes = []

        if tree is None:
            return {'FINISHED'}
        for node in tree.nodes:
            node.select = False

        wkey = self.add_key_cond(tree, 'W', 0, nodes)
        skey = self.add_key_cond(tree, 'S', -40, nodes)
        akey = self.add_key_cond(tree, 'A', -80, nodes)
        dkey = self.add_key_cond(tree, 'D', -120, nodes)

        forward = self.add_movement_setter(tree, 'Set Forward', wkey, 'x', 1, nodes)
        backward = self.add_movement_setter(tree, 'Set Backward', skey, 'x', -1, nodes, ypos=-40)
        leftward = self.add_movement_setter(tree, 'Set Left', akey, 'y', 1, nodes, ypos=-80)
        rightward = self.add_movement_setter(tree, 'Set Right', dkey, 'y', -1, nodes, ypos=-120)

        reset_x = self.add_or_cond(tree, 'W or S Pressed', forward, backward, nodes)
        reset_y = self.add_or_cond(tree, 'A or D Pressed', leftward, rightward, nodes, ypos = -100)

        inverted_x = self.add_invert_bool(tree, 'If Not', reset_x, nodes)
        inverted_y = self.add_invert_bool(tree, 'If Not', reset_y, nodes, ypos=-100)

        self.add_movement_setter(tree, 'Cancel X Movement', inverted_x, 'x', 0, nodes, xpos=880, ypos=-20)
        self.add_movement_setter(tree, 'Cancel Y Movement', inverted_y, 'y', 0, nodes, xpos=880, ypos=-100)

        get_x = self.add_movement_getter(tree, 'Get X', 'x', nodes)
        get_y = self.add_movement_getter(tree, 'Get Y', 'y', nodes, ypos=-200)

        assemble_vec = tree.nodes.new('NLParameterVector3SimpleNode')
        tree.links.new(get_x.outputs[0], assemble_vec.inputs[0])
        tree.links.new(get_y.outputs[0], assemble_vec.inputs[1])
        assemble_vec.label = 'Build Vector'
        assemble_vec.location = (220, -160)
        nodes.append(assemble_vec)

        calc_vec = tree.nodes.new('NLVectorMath')
        tree.links.new(assemble_vec.outputs[0], calc_vec.inputs[1])
        calc_vec.label = 'Normalize'
        calc_vec.location = (440, -160)
        nodes.append(calc_vec)

        speed = tree.nodes.new('NLParameterFloatValue')
        speed.inputs[0].value = .1
        speed.label = 'Speed'
        speed.location = (440, -200)
        nodes.append(speed)

        calc_speed = tree.nodes.new('NLArithmeticOpParameterNode')
        tree.links.new(calc_vec.outputs[0], calc_speed.inputs[0])
        tree.links.new(speed.outputs[0], calc_speed.inputs[1])
        calc_speed.operator = 'MUL'
        calc_speed.label = 'Calc Speed'
        calc_speed.location = (660, -240)
        nodes.append(calc_speed)

        update = tree.nodes.new('NLOnUpdateConditionNode')
        update.location = (660, -160)
        nodes.append(update)

        owner = tree.nodes.new('NLOwnerGameObjectParameterNode')
        owner.location = (660, -200)
        nodes.append(owner)

        apply_loc = tree.nodes.new('NLActionApplyLocation')
        tree.links.new(update.outputs[0], apply_loc.inputs[0])
        tree.links.new(owner.outputs[0], apply_loc.inputs[1])
        tree.links.new(calc_speed.outputs[0], apply_loc.inputs[2])
        apply_loc.label = 'Move Object'
        apply_loc.location = (880, -160)
        nodes.append(apply_loc)

        for node in nodes:
            node.select = True
            if node.label == 'Speed':
                continue
            node.hide = True
            for socket in node.inputs:
                if not socket.is_linked:
                    socket.hide = True
            for socket in node.outputs:
                if not socket.is_linked:
                    socket.hide = True

        bpy.ops.transform.translate()
        return {'FINISHED'}


class NLApplyLogicOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.apply_logic"
    bl_label = "Apply Logic"
    bl_description = "Apply the current tree to the selected objects."
    bl_options = {'REGISTER', 'UNDO'}
    owner = bpy.props.StringProperty()

    @classmethod
    def poll(cls, context):
        tree = context.space_data.edit_tree
        if not tree:
            return False
        if not (tree.bl_idname == bge_netlogic.ui.BGELogicTree.bl_idname):
            return False
        scene = context.scene
        for ob in scene.objects:
            if not bpy.app.version < (2, 80, 0):
                if ob.select_get():
                    return True
            else:
                if ob.select:
                    return True
        return False

    def execute(self, context):
        current_scene = context.scene
        tree = context.space_data.edit_tree
        tree.use_fake_user = True
        py_module_name = bge_netlogic.utilities.py_module_name_for_tree(tree)
        selected_objects = [
            ob for ob in current_scene.objects if ob.select_get()
        ] if not bpy.app.version < (2, 80, 0) else [
            ob for ob in current_scene.objects if ob.select
        ]
        initial_status = bge_netlogic.utilities.compute_initial_status_of_tree(
            tree.name, selected_objects
        )
        initial_status = True if initial_status is None else False
        for obj in selected_objects:
            print(
                "Applied tree {} to object {}".format(
                    tree.name,
                    obj.name
                )
            )
            self._setup_logic_bricks_for_object(
                tree, py_module_name, obj, context
            )
            tree_collection = obj.bgelogic_treelist
            contains = False
            for t in tree_collection:
                if t.tree_name == tree.name:
                    contains = True
                    break
            if not contains:
                new_entry = tree_collection.add()
                new_entry.tree_name = tree.name
                # this will set both new_entry.tree_initial_status and add a
                # game property that makes the status usable at runtime
                bge_netlogic.utilities.set_network_initial_status_key(
                    obj, tree.name, initial_status
                )
        return {'FINISHED'}

    def _setup_logic_bricks_for_object(
        self,
        tree,
        py_module_name,
        obj,
        context
    ):
        game_settings = obj.game
        disp_name = py_module_name
        disp_name = disp_name.split('NL')[-1] + '_NL'
        sensor_name = disp_name
        sensor = None
        for s in game_settings.sensors:
            if s.name == sensor_name:
                sensor = s
                break
        if sensor is None:
            bpy.ops.logic.sensor_add(
                type="DELAY",
                object=obj.name
            )
            sensor = game_settings.sensors[-1]
            sensor.show_expanded = False
        sensor.pin = True
        sensor.name = sensor_name
        sensor.type = "DELAY"
        sensor.use_repeat = True
        sensor.delay = 0
        sensor.duration = 0
        # create the controller
        controller_name = disp_name + '_PY'
        controller = None
        for c in game_settings.controllers:
            if c.name == controller_name:
                controller = c
                break
        if controller is None:
            bpy.ops.logic.controller_add(
                type="PYTHON",
                object=obj.name
            )
            controller = game_settings.controllers[-1]
            controller.show_expanded = False
            bpy.ops.logic.controller_add(
                type="LOGIC_OR",
                object=obj.name,
                name=disp_name + '_OR'
            )
            game_settings.controllers[-1].show_expanded = False
        controller.name = controller_name
        controller.type = "PYTHON"
        controller.mode = "MODULE"
        controller.module = bge_netlogic.utilities.py_controller_module_string(
            py_module_name
        )
        # link the brick
        sensor.link(controller)


class UpdateCodeMessageBox(bpy.types.Operator):
    bl_idname = "message.messagebox"
    bl_label = ""


class NLGenerateLogicNetworkOperatorAll(bpy.types.Operator):
    bl_idname = "bge_netlogic.generate_logicnetwork_all"
    bl_label = "Generate LogicNetwork"
    bl_options = {'REGISTER', 'UNDO'}
    bl_description = "Create the code needed to execute the all logic trees"

    @classmethod
    def poll(cls, context):
        return True

    def __init__(self):
        pass

    def _create_external_text_buffer(self, context, buffer_name):
        file_path = bpy.path.abspath("//{}".format(buffer_name))
        return FileTextBuffer(file_path)

    def _create_text_buffer(self, context, buffer_name, external=False):
        if external is True:
            return self._create_external_text_buffer(context, buffer_name)
        blender_text_data_index = bpy.data.texts.find(buffer_name)
        blender_text_data = None
        if blender_text_data_index < 0:
            blender_text_data = bpy.data.texts.new(name=buffer_name)
        else:
            blender_text_data = bpy.data.texts[blender_text_data_index]
        return BLTextBuffer(blender_text_data)

    def execute(self, context):
        # ensure that the local "bgelogic" folder exists
        local_bgelogic_folder = bpy.path.abspath("//bgelogic")
        if not os.path.exists(local_bgelogic_folder):
            try:
                os.mkdir(local_bgelogic_folder)
            except PermissionError:
                self.report({"ERROR"}, "Cannot generate the code because the blender file has \
                    not been saved or the user has no write permission for \
                        the containing folder.")
                return {"FINISHED"}
        for tree in bpy.data.node_groups:
            if tree.bl_idname == bge_netlogic.ui.BGELogicTree.bl_idname:
                tree_code_generator.TreeCodeGenerator().write_code_for_tree(tree)
        return {"FINISHED"}


class NLGenerateLogicNetworkOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.generate_logicnetwork"
    bl_label = "Generate LogicNetwork"
    bl_options = {'REGISTER', 'UNDO'}
    bl_description = "Create the code needed to execute the current logic tree"

    @classmethod
    def poll(cls, context):
        if not context.space_data:
            raise Exception(
                "TREE TO EDIT NOT FOUND - Update Manually"
            )
            self.report({"ERROR"}, "TREE TO EDIT NOT FOUND - Update Manually")

            def oops(self, context):
                self.layout.label("Tree to edit not found - update manually!")

            bpy.context.window_manager.popup_menu(
                oops,
                title="Error",
                icon='ERROR'
            )

        tree = context.space_data.edit_tree
        if not tree:
            return False
        if not (tree.bl_idname == bge_netlogic.ui.BGELogicTree.bl_idname):
            return False
        return context.space_data.edit_tree is not None

    def __init__(self):
        pass

    def _create_external_text_buffer(self, context, buffer_name):
        file_path = bpy.path.abspath("//{}".format(buffer_name))
        return FileTextBuffer(file_path)

    def _create_text_buffer(self, context, buffer_name, external=False):
        if external is True:
            return self._create_external_text_buffer(context, buffer_name)
        blender_text_data_index = bpy.data.texts.find(buffer_name)
        blender_text_data = None
        if blender_text_data_index < 0:
            blender_text_data = bpy.data.texts.new(name=buffer_name)
        else:
            blender_text_data = bpy.data.texts[blender_text_data_index]
        return BLTextBuffer(blender_text_data)

    def execute(self, context):
        # ensure that the local "bgelogic" folder exists
        local_bgelogic_folder = bpy.path.abspath("//bgelogic")
        if not os.path.exists(local_bgelogic_folder):
            try:
                os.mkdir(local_bgelogic_folder)
            except PermissionError:
                self.report({"ERROR"} ,"Cannot generate the code because the blender file has \
                    not been saved or the user has no write permission for \
                        the containing folder.")
                return {"FINISHED"}
        # write the current tree in a python module,
        # in the directory of the current blender file
        if (context is None) or (context.space_data is None) or (
            context.space_data.edit_tree is None
        ):
            print("NLGenerateLogicNetworkOperator.execute: \
                no context, space_data or edit_tree. Abort writing tree.")
            self.report(
                {'ERROR'},
                'Tree to edit not found! Press "Update Code" manually.'
            )
            return {"FINISHED"}

        tree = context.space_data.edit_tree
        tree_code_generator.TreeCodeGenerator().write_code_for_tree(tree)
        return {"FINISHED"}


class NLAddPropertyOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.add_game_prop"
    bl_label = "Add Game Property"
    bl_options = {'REGISTER', 'UNDO'}
    bl_description = "Adds a property available to the UPBGE"

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        bpy.ops.object.game_property_new()
        bge_netlogic.update_current_tree_code()
        return {'FINISHED'}


class NLAddComponentOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.add_component"
    bl_label = "Add Component"
    bl_options = {'REGISTER', 'UNDO'}
    bl_description = "Add a python Component to the selected object."

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        bpy.ops.logic.add_python_component()
        bge_netlogic.update_current_tree_code()
        return {'FINISHED'}


class NLRemovePropertyOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.remove_game_prop"
    bl_label = "Add Game Property"
    bl_description = "Remove this property"
    bl_options = {'REGISTER', 'UNDO'}
    index = bpy.props.IntProperty()

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        bpy.ops.object.game_property_remove(index=self.index)
        bge_netlogic.update_current_tree_code()
        return {'FINISHED'}


class NLMovePropertyOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.move_game_prop"
    bl_label = "Move Game Property"
    bl_description = "Move Game Property"
    bl_options = {'REGISTER', 'UNDO'}
    index = bpy.props.IntProperty()
    direction = bpy.props.StringProperty()

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        bpy.ops.object.game_property_move(
            index=self.index,
            direction=self.direction
        )
        bge_netlogic.update_current_tree_code()
        return {'FINISHED'}


class NLLoadSoundOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.load_sound"
    bl_label = "Load Sound"
    bl_description = "Load any sound file"

    def execute(self, context):
        bpy.ops.sound.open()
        return {'FINISHED'}


class NLSwitchInitialNetworkStatusOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.switch_network_status"
    bl_label = "Enable/Disable at start"
    bl_description = "Enables of disables the logic tree at start for the \
        selected objects"
    bl_options = {'REGISTER', 'UNDO'}
    tree_name = bpy.props.StringProperty()
    current_status = bpy.props.BoolProperty()

    @classmethod
    def poll(cls, context):
        return True

    def execute(self, context):
        current_status = self.current_status
        new_status = not current_status
        tree_name = self.tree_name
        scene = context.scene
        updated_objects = [
            ob for ob in scene.objects if ob.select_get() and
            bge_netlogic.utilities.object_has_treeitem_for_treename(
                ob, tree_name
            )
        ] if not bpy.app.version < (2, 80, 0) else [
            ob for ob in context.scene.objects if ob.select and
            bge_netlogic.utilities.object_has_treeitem_for_treename(
                ob, tree_name
            )
        ]
        for ob in updated_objects:
            bge_netlogic.utilities.set_network_initial_status_key(
                ob, tree_name, new_status
            )
        bge_netlogic.update_current_tree_code()
        return {'FINISHED'}


# Popup the code templates for custom nodes and cells
class NLPopupTemplatesOperator(bpy.types.Operator):
    bl_idname = "bge_netlogic.popup_templates"
    bl_label = "Show Custom Node Templates"
    bl_description = "Load the template code for custom nodes \
        and cells in the text editor"
    bl_options = {'REGISTER', 'UNDO'}

    @classmethod
    def poll(cls, context): return True

    def execute(self, context):
        node_code = self.get_or_create_text_object("my_custom_nodes.py")
        cell_code = self.get_or_create_text_object("my_custom_cells.py")
        self.load_template(node_code, "my_custom_nodes.txt")
        self.load_template(cell_code, "my_custom_cells.txt")
        self.report({"INFO"}, "Templates available in the text editor")
        return {'FINISHED'}

    def load_template(self, text_object, file_name):
        import os
        this_dir = os.path.dirname(os.path.realpath(__file__))
        parent_dir = os.path.dirname(this_dir)
        templates_dir = os.path.join(parent_dir, "templates")
        template_file = os.path.join(templates_dir, file_name)
        text_data = "Error Reading Template File"
        with open(template_file, "r") as f:
            text_data = f.read()
        text_object.from_string(text_data)

    def get_or_create_text_object(self, name):
        index = bpy.data.texts.find(name)
        if index < 0:
            bpy.ops.text.new()
            result = bpy.data.texts[-1]
            result.name = name
            return result
        else:
            return bpy.data.texts[index]
