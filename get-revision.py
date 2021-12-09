import time

curTime = time.time()
f = open("revision.txt", "w")
f.write(str(int(curTime)));
f.close();

print("-D BUILD_REVISION=%i" % curTime )