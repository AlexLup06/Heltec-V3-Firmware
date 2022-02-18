Import("env", "projenv")
import os

# This will be executed after every build (upload wont trigger this)
# 
def after_build(source, target, env):
    print("Upload firmware.bin to pi@192.168.1.1...")
    os.system('bash -c "rsync .pio/build/heltec_wifi_lora_32_V2/firmware.bin root@192.168.1.1:/www/esp32/firmware.bin"')
    print("Upload revision.txt to pi@192.168.1.1...")
    os.system('bash -c "rsync revision.txt root@192.168.1.1:/www/esp32/revision.txt"')

# Hook buildprog stage
env.AddPostAction("buildprog", after_build)