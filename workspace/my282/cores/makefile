CORES = fceumm gambatte gpsp pcsx_rearmed picodrive snes9x2005

###############################

# Match MinUI's proven my282 alpha set while using NextUI's current,
# reproducible core build machinery.

fceumm_REPO = https://github.com/libretro/libretro-fceumm
gambatte_REPO = https://github.com/libretro/gambatte-libretro

pcsx_rearmed_MAKEFILE = Makefile.libretro
pcsx_rearmed_SUBMODULES = frontend/libpicofe
# As with PicoDrive, uppercase PLATFORM selects PCSX's internal frontend.
pcsx_rearmed_FLAGS = PLATFORM=libretro

picodrive_REPO = https://github.com/irixxxx/picodrive
picodrive_MAKEFILE = Makefile.libretro
picodrive_HASH = b0be121b7d58d6ee1ee2809974e62893c80a8264
# The outer core build uses uppercase PLATFORM to select workspace/my282.
# PicoDrive also uses uppercase PLATFORM internally to select its frontend
# objects, while lowercase platform selects the target-specific flags.
picodrive_FLAGS = PLATFORM=libretro

snes9x2005_REPO = https://github.com/libretro/snes9x2005

###############################

include ../../all/cores/makefile
