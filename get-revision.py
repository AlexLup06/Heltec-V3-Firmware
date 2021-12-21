import time
# Simple Script to create a revision based on the current unix time

curTime = time.time()
f = open("revision.txt", "w")
f.write(str(int(curTime)));
f.close();

# Print Revision as Build Flag to STDOUT, BUILD_REVISION will be available as #DEFINE in the build environment
print("-D BUILD_REVISION=%i" % curTime )