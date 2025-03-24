import bge
import mathutils
import math
import random

def emitter():
    c = bge.logic.getCurrentController()
    obj = c.owner
    scene = bge.logic.getCurrentScene()
    cam = scene.active_camera
    logic = c.sensors['Always']
    logic.frequency = 0
    
    obj['particles'] = eval(obj['particleList'])
    obj['scale'] = eval(obj['particleScale'])
    obj['addmode'] = eval(obj['particleAddMode'])
    
    if 'init' not in obj:
        obj['init'] = True
        obj['killTicker'] = 0
        obj['emitcounter'] = 0
        obj['particlelist'] = []   
        obj['multiplier'] = 2
        obj['ticker'] = 0
        obj['counter'] = float(0)
        obj['oldcounter'] = obj['counter']
        obj['listlen'] = (len(obj['particles']))-1
        obj['cullingTime'] = 0
        obj['emitterVisible'] = False
        
        #CACHING
        obj['colorCache'] = []
        obj['colorCacheProgress'] = -1
        obj['scaleCache'] = []
        obj['scaleCacheProgress'] = -1
        obj['speedCache'] = []
        obj['speedCacheProgress'] = -1

    logic.frequency = 0    
    obj['ticker'] +=1
    
    ###Kill Emitter
    if obj['kill'] == True:
        obj['killTicker'] += 1
        if obj['killTicker'] >= obj['lifetime']+10:
            obj.endObject()
    
    if obj['culling'] == True:
        if (cam.sphereInsideFrustum(obj.position, obj['cullingRadius']) != cam.OUTSIDE):
            obj['emitterVisible'] = True
        else:
            obj['emitterVisible'] = False
    else:
        obj['emitterVisible'] = True
    #bge.render.makeScreenshot('//0#.png') 
    for particle in obj['particlelist']:
        if particle['ticker'] % obj['multiplier'] == 0:
            particle_update(particle, obj['particlelist'], obj['multiplier'], obj)
        particle['ticker'] += 1

    if obj['emitteron'] == True and obj['kill'] == False and obj['emitterVisible'] == True:
        obj['cullingTime'] = 0
        randParticle = random.randint(0,obj['listlen'])
    
        obj['counter'] += (obj['amount']/60)
        emitcount =  int(obj['counter']) - int(obj['oldcounter']) 

        
        if (obj['emitcounter'] < obj['emitTime']) or (obj['emitTime'] == 0):

            for x in range(0,emitcount):  
                
                obj['random'] = random.uniform(1,obj['randomscale'])
                                
                particle = scene.addObject("ParticleParent", obj, 0)
                particle["child"] = scene.addObject(obj['particles'][randParticle],obj, 0)
                particle["child"].setParent(particle, False, False)
                
                vect = mathutils.Vector((random.uniform(-obj['rangeEmitX'],obj['rangeEmitX']),random.uniform(-obj['rangeEmitY'],obj['rangeEmitY']),random.uniform(-obj['rangeEmitZ'],obj['rangeEmitZ'])))
                vect = particle.localOrientation * vect

                particle.localPosition += vect
                                                
                obj['particlelist'].append(particle)
                particle['scale'] = obj['scale'][randParticle]
                particle['localEmit'] = obj['localEmit']
                particle['coneX'] = (obj['coneX'])/2
                particle['coneY'] = (obj['coneY'])/2
                particle['coneZ'] = (obj['coneZ'])/2
                euler = particle.localOrientation.to_euler()
                particle["randRotX"] = random.uniform(euler[0]-particle['coneX'],euler[0]+particle['coneX'])
                particle["randRotY"] = random.uniform(euler[1]-particle['coneY'],euler[1]+particle['coneY'])
                particle["randRotZ"] = random.uniform(euler[2]-particle['coneZ'],euler[2]+particle['coneZ'])
                
                particle['lifeticker'] = 0
                particle['scaleticker'] = 0
                particle['colorticker'] = 0
                particle['fadeinticker'] = 0
                particle['fadeoutticker'] = 0
                particle['speedticker'] = 0
                particle['speed'] = [0,0,0]
                particle['ticker'] = random.randint(0,int(obj['multiplier'] - 1))
                particle['speedfade_start'] = obj['speedfade_start']
                particle['speedfade_end'] = obj['speedfade_end']
                particle['lifetime'] = obj['lifetime']
                
                particle['startcolor'] = [obj['start_r'], obj['start_g'], obj['start_b'],obj['alpha']]
                particle['endcolor'] = [obj['end_r'], obj['end_g'], obj['end_b'],obj['alpha']]
                particle['alpha'] = obj['alpha']
                particle['tmpColor'] = [0,0,0,0]
                particle['finColor'] = [0,0,0,0]
                
                particle['startspeed'] = [0,0,obj['startspeed']]
                particle['endspeed'] = [0,0,obj['endspeed']]
                particle['randomMovement'] = obj['randomMovement']
                
                particle['colorfade_start'] = obj['colorfade_start']
                particle['colorfade_end'] = obj['colorfade_end']
                particle['scalefade_start'] = obj['scalefade_start']
                particle['scalefade_end'] = obj['scalefade_end']
                particle['fadein'] = obj['fadein']
                particle['fadeout'] = obj['fadeout']
                
                particle['startscale'] = [obj['startscale_x']*obj['random'],obj['startscale_y']*obj['random'],obj['startscale_z']*obj['random']]
                particle['endscale'] = [obj['endscale_x'],obj['endscale_y'],obj['endscale_z']]
                
                particle["child"].color = particle['finColor']
                particle["child"].localScale[0] = particle['startscale'][0]* particle['scale']
                particle["child"].localScale[1] = particle['startscale'][1]* particle['scale']
                particle["child"].localScale[2] = particle['startscale'][2]* particle['scale']
                particle["child"].applyRotation((random.uniform(0,2*math.pi),0,0), True)
                particle.localOrientation = (particle["randRotX"],particle["randRotY"],particle["randRotZ"])
                euler = particle.localOrientation.to_euler()
                particle['RotX'] = euler[0]
                particle['RotY'] = euler[1]
                particle['RotZ'] = euler[2]
                particle['halo'] = obj['halo']
                particle['rotation'] = obj['rotation']
                particle['addmode'] = obj['addmode'][randParticle]
                particle.localScale = (1,1,1)
                
                particle['randomDirection'] = random.choice([-1,1])
                obj['oldcounter'] = obj['counter']
        else:
            obj['kill'] = True
    else:
        obj['cullingTime'] +=1
        if obj['cullingTime'] >= obj['lifetime']+5:
            logic.frequency = 5
     
    obj['emitcounter'] += obj['multiplier']
    
def particle_update(obj, list, multiplier, emitter):
    scene = bge.logic.getCurrentScene()
    if obj['lifeticker']%2 == 0:
        obj['child'].alignAxisToVect(scene.active_camera.position,0,1.0)
    obj['child'].applyRotation((obj['rotation']*multiplier*obj['randomDirection'],0,0),True)    



###Fade between two colors###
    if obj['lifeticker'] > emitter['colorCacheProgress']:
        if obj['lifeticker'] < obj['colorfade_start']:
            obj['tmpColor'] = obj['startcolor']
        elif (obj['lifeticker'] >= obj['colorfade_start']) and (obj['lifeticker'] <= obj['colorfade_end']):
            fadetime = obj['colorfade_end']-obj['colorfade_start']
            fadefactor_down = obj['colorticker']/fadetime
            fadefactor_up = 1 - fadefactor_down
            obj['tmpColor'][0] = (obj['startcolor'][0]* fadefactor_up + obj['endcolor'][0] * fadefactor_down)#*obj['alpha']
            obj['tmpColor'][1] = (obj['startcolor'][1]* fadefactor_up + obj['endcolor'][1] * fadefactor_down)#*obj['alpha']
            obj['tmpColor'][2] = (obj['startcolor'][2]* fadefactor_up + obj['endcolor'][2] * fadefactor_down)#*obj['alpha']
            obj['tmpColor'][3] = obj['startcolor'][3]
            obj['colorticker'] += multiplier
        elif obj['lifeticker'] > obj['colorfade_end']:
            obj['tmpColor'] = obj['endcolor']
    
        if obj['lifeticker'] <= obj['fadein'] : 
            fadeinfactor_down = obj['fadeinticker']/obj['fadein']
            fadeinfactor_up = 1 - fadeinfactor_down
            if obj['addmode']:      
                obj['finColor'][0] = obj['tmpColor'][0] * fadeinfactor_down
                obj['finColor'][1] = obj['tmpColor'][1] * fadeinfactor_down
                obj['finColor'][2] = obj['tmpColor'][2] * fadeinfactor_down
                obj['finColor'][3] = obj['startcolor'][3]
            else:
                
                obj['finColor'][0] = obj['tmpColor'][0]
                obj['finColor'][1] = obj['tmpColor'][1]
                obj['finColor'][2] = obj['tmpColor'][2]
                obj['finColor'][3] = obj['tmpColor'][3] * fadeinfactor_down      
            obj['fadeinticker'] += multiplier
            
        elif (obj['lifeticker'] > (obj['lifetime'] - obj['fadeout'])) and (obj['lifeticker'] > obj['fadein']):
            fadeoutfactor_down = obj['fadeoutticker']/(obj['fadeout']-multiplier)
            fadeoutfactor_up = 1 - fadeoutfactor_down
            
            if obj['addmode']:
                obj['finColor'][0] = obj['tmpColor'][0] * fadeoutfactor_up
                obj['finColor'][1] = obj['tmpColor'][1] * fadeoutfactor_up
                obj['finColor'][2] = obj['tmpColor'][2] * fadeoutfactor_up
                obj['finColor'][3] = 1
            else:
                obj['finColor'][0] = obj['tmpColor'][0]
                obj['finColor'][1] = obj['tmpColor'][1]
                obj['finColor'][2] = obj['tmpColor'][2]
                obj['finColor'][3] = obj['tmpColor'][3] * fadeoutfactor_up
            
            obj['fadeoutticker'] += multiplier
        else:
            obj['finColor'][0] = obj['tmpColor'][0]
            obj['finColor'][1] = obj['tmpColor'][1]
            obj['finColor'][2] = obj['tmpColor'][2]
            obj['finColor'][3] = obj['tmpColor'][3]
            
        #caching color
        emitter['colorCache'].append(mathutils.Vector(obj['finColor']))
        emitter['colorCacheProgress'] += multiplier
    else:
        obj['finColor'] = emitter['colorCache'][int(obj['lifeticker'] / multiplier)]
    ## Final Color calculation
    obj["child"].color[0] = obj['finColor'][0]
    obj["child"].color[1] = obj['finColor'][1]
    obj["child"].color[2] = obj['finColor'][2]
    obj["child"].color[3] = obj['finColor'][3]


    ## Calculate speed
    if obj['lifeticker'] > emitter['speedCacheProgress']:
        
        speedfade = obj['speedfade_end']-obj['speedfade_start']
        speedfactor_down = obj['speedticker']/speedfade
        speedfactor_up = 1-speedfactor_down
    
        if obj['lifeticker'] <= obj['speedfade_start']:
            obj['speed'][0] = obj['startspeed'][0]
            obj['speed'][1] = obj['startspeed'][1]
            obj['speed'][2] = obj['startspeed'][2]
        elif obj['lifeticker'] > obj['speedfade_start'] and obj['lifeticker'] < obj['speedfade_end']:
            obj['speed'][0] = obj['startspeed'][0] * speedfactor_up + obj['endspeed'][0] * speedfactor_down
            obj['speed'][1] = obj['startspeed'][1] * speedfactor_up + obj['endspeed'][1] * speedfactor_down
            obj['speed'][2] = obj['startspeed'][2] * speedfactor_up + obj['endspeed'][2] * speedfactor_down
            obj['speedticker'] += multiplier
        elif obj['lifeticker'] >= obj['speedfade_end']:
            obj['speed'][0] = obj['endspeed'][0]
            obj['speed'][1] = obj['endspeed'][1]
            obj['speed'][2] = obj['endspeed'][2]
            
        obj['speed'][0] *= multiplier *0.01
        obj['speed'][1] *= multiplier *0.01
        obj['speed'][2] *= multiplier *0.01
        
        #caching speed
        emitter['speedCache'].append(mathutils.Vector(obj['speed']))
        emitter['speedCacheProgress'] += multiplier
    else:
        obj['speed'] = emitter['speedCache'][int(obj['lifeticker'] / multiplier)]
        
    obj.applyMovement(obj['speed'], obj['localEmit'])

    ##Calculate scale
    if obj['lifeticker'] > emitter['scaleCacheProgress']:
   
        if obj['lifeticker'] <= obj['scalefade_start']:
            obj['child'].localScale[0] = obj['startscale'][0] * obj['scale']
            obj['child'].localScale[1] = obj['startscale'][1] * obj['scale']
            obj['child'].localScale[2] = obj['startscale'][2] * obj['scale']
        elif obj['lifeticker'] > obj['scalefade_start'] and obj['lifeticker'] < obj['scalefade_end']:
            scaletime = obj['scalefade_end'] - obj['scalefade_start']
            scalefactor_down = obj['scaleticker']/scaletime
            scalefactor_up = 1 - scalefactor_down
            
            obj["child"].localScale[0] = (obj['startscale'][0]* scalefactor_up + obj['endscale'][0] * scalefactor_down)* obj['scale']
            obj["child"].localScale[1] = (obj['startscale'][1]* scalefactor_up + obj['endscale'][1] * scalefactor_down)* obj['scale']
            obj["child"].localScale[2] = (obj['startscale'][2]* scalefactor_up + obj['endscale'][2] * scalefactor_down)* obj['scale']
            obj['scaleticker'] += multiplier
        elif obj['lifeticker'] >= obj['scalefade_end']:
            obj['child'].localScale[0] = obj['endscale'][0] * obj['scale']
            obj['child'].localScale[1] = obj['endscale'][1] * obj['scale']
            obj['child'].localScale[2] = obj['endscale'][2] * obj['scale']
        
        if obj['child'].localScale[0] < 0:
            obj['child'].localScale[0] = 0
        if obj['child'].localScale[1] < 0:
            obj['child'].localScale[1] = 0
        if obj['child'].localScale[2] < 0:
            obj['child'].localScale[2] = 0
        #caching scale
        emitter['scaleCache'].append(mathutils.Vector(obj['child'].localScale))
        emitter['scaleCacheProgress'] += multiplier
        
    else:
        obj['child'].localScale = emitter['scaleCache'][int(obj['lifeticker'] / multiplier)]
        
### Random movement
    if obj['lifeticker']%30 == 0:
        obj['randRotX'] = random.uniform((obj['RotX']-obj['randomMovement']),(obj['RotX']+obj['randomMovement']))
        obj['randRotY'] = random.uniform((obj['RotY']-obj['randomMovement']),(obj['RotY']+obj['randomMovement']))
        obj['randRotZ'] = random.uniform((obj['RotZ']-obj['randomMovement']),(obj['RotZ']+obj['randomMovement']))

    obj['RotX'] = obj['RotX'] * 0.9 + obj['randRotX'] * 0.1
    obj['RotY'] = obj['RotY'] * 0.9 + obj['randRotY'] * 0.1
    obj['RotZ'] = obj['RotZ'] * 0.9 + obj['randRotZ'] * 0.1

    obj.localOrientation = (obj['RotX'],obj['RotY'],obj['RotZ'])       

###Lifeticker
    obj['lifeticker'] += multiplier
    
###kill particle after lifetime  
    if (obj['lifeticker'] > obj['lifetime']) and obj['lifetime'] != 0:
        list.remove(obj)
        obj["child"].endObject()
        obj.endObject()
    
    
    
