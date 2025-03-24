# Configuration Blender
import bpy

bpy.context.user_preferences.system.use_region_overlap = True
bpy.context.user_preferences.view.use_pro_mode = True
#bpy.context.user_preferences.view.use_mouse_depth_cursor = False
#bpy.context.user_preferences.view.use_mouse_depth_navigate = False
#bpy.context.user_preferences.view.use_zoom_to_mouse = False
#bpy.context.user_preferences.view.use_rotate_around_active = False
#bpy.context.user_preferences.edit.use_drag_immediately = False
#bpy.context.user_preferences.edit.use_insertkey_xyz_to_rgb = False
#bpy.context.user_preferences.inputs.select_mouse = 'RIGHT'
#bpy.context.user_preferences.inputs.view_zoom_method = 'DOLLY'
#bpy.context.user_preferences.inputs.view_zoom_axis = 'VERTICAL'
#bpy.context.user_preferences.inputs.view_rotate_method = 'TURNTABLE'
#bpy.context.user_preferences.inputs.invert_mouse_zoom = False

def scriptrunner():
    global gui
    global Bucket
    global Spray
    global getPresets

    # swatch: Standard (unchanged) colors of blenders ui
    # alias: A batch of solid colors
    # snuff: Some tryout colors
    # 
    swatch, solid, snuff = getPresets()

    # Blenders standard widgets
    # 
    #print(dir(gui))

    # Changes for the glowing "Checkboxes"
    # ("unglow" can be removed to keep the default)
    # 
    unglow = Bucket()
    unglow.text = solid.black
    unglow.text_sel = solid.black
    unglow.apply(gui, 'wcol_option')

    # Changes for the "Slider widget"
    # ("brighten" can be deleted to keep the default)
    # 
    brighten = Bucket()
    brighten.inner = (0.080, 0.080, 0.080, 1.0)
    brighten.inner_sel = (0.080, 0.080, 0.080, 1.0)
    brighten.item = (0.3, 0.3, 0.3, 0.2)
    brighten.shadedown = 4
    brighten.shadetop = -24
    brighten.show_shaded = True
    brighten.apply(gui, 'wcol_numslider')

    # Changes for the commands aka "Tool widget"
    # 
    defocus = Bucket()
    defocus.outline = (0.500, 0.300, 0.000)
    defocus.inner = (0.4, 0.4, 0.4, 0.3) # play icon
    defocus.inner_sel = (0.092, 0.092, 0.092, 1.0)
    defocus.item = (0.098, 0.098, 0.098, 1.0)
    defocus.shadedown = -5
    defocus.shadetop = 10
    defocus.show_shaded = True
    defocus.apply(gui, 'wcol_tool')

    # Over paints the "Frames of sadness"
    # 
    allUnframed = Spray([
        'wcol_num', 
        'wcol_numslider', 
        'wcol_text', ])

    for i, q in allUnframed:
        allUnframed[i].outline = (0.500, 0.300, 0.000)
        allUnframed[i].apply(gui, q)

    # It'll "bind" the colors of the "Toggle buttons"
    # 
    allBindings = Spray([
        'wcol_menu', 
        'wcol_option', 
        'wcol_radio', 
        'wcol_regular', 
        'wcol_toggle', ])

    for i, q in allBindings:
        allBindings[i].inner = swatch.charcoal
        allBindings[i].inner_sel = swatch.skyblue
        allBindings[i].apply(gui, q)

    # Additional tweaks to the previous "Toggle buttons"
    # 
    allUnframed = Spray([
        'wcol_menu', 
        'wcol_option', 
        'wcol_radio', 
        'wcol_regular', 
        'wcol_toggle', ])

    for i, q in allUnframed:
        allUnframed[i].outline = (0.620, 0.400, 0.247)
        allUnframed[i].apply(gui, q)

    # Brightens some text fonts
    # ("allBindings" created dark "text" on dark "buttons")
    # 
    allAdjusted = Spray([
        'wcol_regular', 
        'wcol_toggle', ])

    for i, q in allAdjusted:
        allAdjusted[i].text = (0.800, 0.800, 0.800)
        allAdjusted[i].text_sel = solid.black
        allAdjusted[i].show_shaded = True  #its OFF by default...
        allAdjusted[i].shadedown = -15
        allAdjusted[i].shadetop = 15
        allAdjusted[i].apply(gui, q)







    return True

def getPresets():
    global Presetfactory
    swatch = Presetfactory()
    solid = Presetfactory()
    snuff = Presetfactory()

    # skyblue: The color of "Inner Selected" of "Radio Widgets"
    # charcoal: The ubiquitos charcoal for toggles and switches
    # 
    @swatch
    def skyblue(): 
        return (0.62, 0.4, 0.247, 1.0)
    @swatch
    def charcoal(): 
        return (0.3, 0.3, 0.3, 0.5)

    # A batch of opaque (or solid) colors.
    # 
    @solid
    def black(): 
        return (0.000, 0.000, 0.000)
    @solid
    def white():
        return (0.000, 0.500, 0.000)

    # A batch of tryout colors
    # 
    @snuff
    def white():
        return (0.000, 0.500, 0.000, 1.0)
    @snuff
    def red():
        return (0.000, 0.500, 0.000, 1.0)
    @snuff
    def yellow():
        return (0.000, 1.000, 0.000, 1.0)

    return swatch, solid, snuff

# (All "buckets" and "sprays" are interfaces...
#  ...only "fill()" gets to see "gui")
#
class Bucket():
    def __init__(self):
        self.scriptwork = {
            'text': None, 
            'text_sel': None, 
            'inner': None, 
            'inner_sel': None, 
            'item': None, 
            'outline': None, 
            'shadedown': None, 
            'shadetop': None, 
            'show_shaded': None, }

    def apply(self, gui, firstname):
        # 
        # Pythons "setattr()" accesses blenders:
        # >> gui.<button>.<setting> = <color>

        knockout = self.scriptwork
        for q in knockout.keys():
            if knockout[q] != None:
                setattr(
                    getattr(gui, firstname), q, knockout[q] )

        # Prevents multiple reruns
        # (the pending changes were already applied)
        # 
        for q in knockout.keys():
            knockout[q] = None

    def __getattr__(self, label):
        return self.scriptwork[label]

    def __setattr__(self, label, contents):
        if label != 'scriptwork':
            if label not in self.__dict__['scriptwork'].keys():
                print("Attention, unkown gui property:", label)
                print("Available are:", self.__dict__['scriptwork'].keys())
        if label == 'scriptwork':
            self.__dict__['scriptwork'] = contents
            return None
        self.__dict__['scriptwork'][label] = contents
        return None
    pass

class Spray():
    def __init__(self, dispatch):
        global Bucket
        self.scriptwork = {
            'bucketnames': dispatch, 
            'buckets': [Bucket() for q in dispatch ], }

    def __iter__(self):
        for f in range(len(self.scriptwork['bucketnames'])):
            i = 0 + f
            q = '' + self.scriptwork['bucketnames'][f]
            yield i, q

    def __getitem__(self, frame):
        return self.scriptwork['buckets'][frame]
    pass

# (Steals and stores the decorator user)
# 
class Presetfactory():
    def __init__(self):
        self.scriptwork = {}

    def __call__(self, target):
        firstname = '' + target.__name__
        self.scriptwork[firstname] = target
        return "Succeffully collected the preset."

    def __getattr__(self, label):
        return self.scriptwork[label]()
    pass

# (This script accesses blenders's "user_interface")
# 
import bpy
gui = bpy.context.user_preferences.themes[0].user_interface
scriptrunner()