import bge
def main():
    cont = bge.logic.getCurrentController()
    own = cont.owner # main object
    sens = cont.sensors[0]
    if sens.positive:
        try:
            print(str(own))
        except:
            pass
main()
