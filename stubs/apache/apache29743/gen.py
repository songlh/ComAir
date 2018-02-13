
import sys
import os

if __name__=='__main__':
    iNum = int(sys.argv[2])
    sRoot = os.path.dirname(os.path.realpath(__file__))
    of = open(sys.argv[1], 'w')
    for i in range(iNum):
        of.write(sRoot + "/" + str(i) + ".txt" )
        of.write("\n")


