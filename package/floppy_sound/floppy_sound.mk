################################################################################
#
# floppy_sound
# "make floppy_sound-dirclean" before build
#
################################################################################

FLOPPY_SOUND_VERSION = 1.0
FLOPPY_SOUND_SITE = $(BR2_EXTERNAL_FLOPPY_SOUND_PATH)/package/floppy_sound/src
FLOPPY_SOUND_SITE_METHOD = local
FLOPPY_DEPENDENCIES = libgpiod alsa-lib

define FLOPPY_SOUND_BUILD_CMDS
    $(MAKE) CC="$(TARGET_CC)" LD="$(TARGET_LD)" -C $(@D)
endef

define FLOPPY_SOUND_INSTALL_TARGET_CMDS
    $(INSTALL) -D -m 0755 $(@D)/floppy_sound $(TARGET_DIR)/usr/bin
    $(INSTALL) -D -m 0755 $(@D)/floppy_sound_debug $(TARGET_DIR)/usr/bin
endef

$(eval $(generic-package))
