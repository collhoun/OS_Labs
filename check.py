import sys
nums = [int(l, 16) for l in open(sys.argv[1]) if l.strip()]
n = len(nums)
s = sum(nums)
print("N =", n)
print("mean = %x" % ((2 * s + n) // (2 * n)))