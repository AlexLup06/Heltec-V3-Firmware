Import("env", "projenv")
import os

def after_build(source, target, env):
    print("Upload firmware.bin to pi@192.168.0.200...")
    os.system('bash -c "rsync .pio/build/heltec_wifi_lora_32_V2/firmware.bin pi@192.168.0.200:/var/www/html/esp32/firmware.bin"')
    print("Upload revision.txt to pi@192.168.0.200...")
    os.system('bash -c "rsync revision.txt pi@192.168.0.200:/var/www/html/esp32/revision.txt"')

env.AddPostAction("buildprog", after_build)