Import("env")
import os

board = env.BoardConfig()

BUILD_DIR = env.subst("$BUILD_DIR")
PROGNAME = env.subst("${PROGNAME}")

MERGED_BIN = os.path.join(BUILD_DIR, f"{PROGNAME}_merged.bin")


def merge_bin(source, target, env):

    chip = board.get("build.mcu", "esp32")
    flash_size = board.get("upload.flash_size", "4MB")

    images = []

    # bootloader / partitions / boot_app0 etc
    for addr, file in env.get("FLASH_EXTRA_IMAGES", []):
        images.append(env.subst(addr))
        images.append(env.subst(file))

    # main firmware
    images.append(env.subst("$ESP32_APP_OFFSET"))
    images.append(env.subst("$BUILD_DIR/${PROGNAME}.bin"))

    cmd = [
        env.subst("$PYTHONEXE"),
        env.subst("$OBJCOPY"),  # esptool
        "--chip",
        chip,
        "merge-bin",
        "--flash-size",
        flash_size,
        "-o",
        MERGED_BIN,
    ] + images

    print("Generating merged binary:", MERGED_BIN)
    env.Execute(" ".join(cmd))


env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", merge_bin)