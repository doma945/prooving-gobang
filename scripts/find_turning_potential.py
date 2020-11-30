import os
import subprocess
from subprocess import Popen, PIPE


def get_result(potencial_n):
    p = Popen(["./AMOBA", "-potencial_n", str(potencial_n)],
                stdout=PIPE, stdin=PIPE, stderr=PIPE, bufsize=1, universal_newlines=True)

    pn = True
    for line in p.stdout:
        if(line[:2] == "PN"):
            if(line[4]=='0'): pn = True
            else: pn = False
            #print(line, end='')
        #print(line, end='')
    return pn

if(__name__ == "__main__"):
    # === Build ===
    os.chdir("build")
    if 1:
        os.system("sed -i 's/#define COL.*/#define COL {}/g' ../src/common.h".format(10))
        build = Popen(["make", "-j4"],
                    stdout=PIPE, stdin=PIPE, stderr=PIPE, bufsize=1, universal_newlines=True)

        for line in build.stdout:
            #print(line, end='')
            pass
        print("Build done")

    # === Search for turning point ===
    bottom = 0
    up = 1024
    act = up//2
    while(bottom+1 < up):
        print("\rAct:",act, "interval:", bottom, up, flush=True, end='   ')
        pn = get_result(act)
        if(not pn): up = act
        else: bottom = act

        act= (bottom+up)//2

    print("\n{}({}) is proof, and {}({}) is disproof".format(bottom/128,bottom, (up)/128, up))