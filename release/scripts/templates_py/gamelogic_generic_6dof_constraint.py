import bge
def main():
    cont = bge.logic.getCurrentController()
    own = cont.owner # main object

    root = own.scene.objects['MyObject'] # mover object
    generic6dof = 12

    oid = own.getPhysicsId()
    rid = root.getPhysicsId()
    #position off set
    pivotInAx=0
    pivotInAy=0
    pivotInAz=0
    #axis off set
    axisInAx=0
    axisInAy=0
    axisInAz=0

    #for the generic 6dof constraint (12), the 3 arguments are euler angles. See rigid body constraint pivot for angles.
    angleX=0
    angleY=0
    angleZ=0


    disableConnectedBodies = 128
    #flag = disableConnectedBodies

    cons = bge.constraints.createConstraint(oid,rid,generic6dof,pivotInAx,pivotInAy,pivotInAz,angleX,angleY,angleZ,disableConnectedBodies)

    #params 0,1,2 are linear limits, low,high value. if low > high then disable limit
    cons.setParam(0,0,0)
    cons.setParam(1,0,0)
    cons.setParam(2,-10,10)
    cons.setParam(3,0,0)
    cons.setParam(4,0,0)
    cons.setParam(5,0,0)

    #params 6,7,8 are translational motors with with target velocity and maximum force
    #cons.setParam(6,0,1000)
    #cons.setParam(7,0.4,1000)
    #cons.setParam(8,1,1)

    #param 9,10 and 11 are rotational motors, with target velocity and maximum force
    #cons.setParam(9,2,1000)
    #cons.setParam(10,2,1000)
    #cons.setParam(11,2,1)

    #param 12-14 are for linear springs and 15-17 for angular springs
    #cons.setParam(12,8,0)
    #cons.setParam(13,8,0)
    #cons.setParam(14,8,0)

    #cons.setParam(15,8,0)
    #cons.setParam(16,8,0)
    #cons.setParam(17,8,0)

    #bge.consid = cons.getConstraintId() # can save in bge
    #bge.constraints.removeConstraint(bge.consid) # and get from bge

main()
