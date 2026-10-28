import random, sys
n = int(sys.argv[1])
path = sys.argv[2]
with open(path, "w") as f:
    for _ in range(n):
        f.write("%0128x\n" % random.getrandbits(512))