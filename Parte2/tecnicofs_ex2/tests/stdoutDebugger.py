
checkpoints = [0]*9

mountCheckpoints = [0]*6

readCheckpoints = [0]*5

writes = 0



last = 0

f = open("out.out",'r')
lines = f.readlines()


for line in lines:
  if line[0]=='S':
    readCheckpoints[(int(line[1]))]+=1


# ======================================================================   

f = open("out2.out",'r')
lines = f.readlines()


for line in lines:
  if line[0]=='M':
    mountCheckpoints[(int(line[1]))]+=1
  elif line[0]=='T':
    checkpoints[(int(line[1]))]+=1 
  elif line[0]=='W':
    writes+=1


print(checkpoints)
print(mountCheckpoints)
print(readCheckpoints)
print(writes)