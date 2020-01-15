#
# Copyright (C) 2020 The LineageOS Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

DEVICE_PATH := device/lg/lg1154

# Architecture
TARGET_ARCH := arm
TARGET_ARCH_VARIANT := armv7-a-neon
TARGET_CPU_VARIANT := cortex-a9
TARGET_CPU_ABI := armeabi-v7a
TARGET_CPU_ABI2 := armeabi
TARGET_CPU_SMP := true
ARCH_ARM_HAVE_TLS_REGISTER := true

# Board
TARGET_BOARD_PLATFORM := lg1154
TARGET_BOOTLOADER_BOARD_NAME := LG1154

# Bootloader
TARGET_NO_BOOTLOADER := true

# Kernel
BOARD_KERNEL_IMAGE_NAME := zImage
BOARD_KERNEL_CMDLINE := androidboot.hardware=lg1154 androidboot.selinux=permissive
BOARD_KERNEL_BASE := 0x00000000
BOARD_KERNEL_PAGESIZE := 4096
BOARD_KERNEL_TAGS_OFFSET := 0x00000100
BOARD_RAMDISK_OFFSET := 0x01000000
TARGET_KERNEL_ARCH := arm
TARGET_KERNEL_CONFIG := lg1154_kdrv_android_defconfig
TARGET_KERNEL_SOURCE := kernel/lg/lg1154

# Audio
BOARD_USES_ALSA_AUDIO := true

# Graphics
USE_OPENGL_RENDERER := true
TARGET_USES_ION := true
BOARD_EGL_CFG := $(DEVICE_PATH)/egl/egl.cfg

# Media
TARGET_USES_MEDIA_EXTENSIONS := true

# Recovery
TARGET_RECOVERY_FSTAB := $(DEVICE_PATH)/rootdir/etc/fstab.lg1154

# Filesystems
BOARD_HAS_LARGE_FILESYSTEM := true
TARGET_USERIMAGES_USE_EXT4 := true
BOARD_SUPPRESS_SECURE_ERASE := true

# Properties
TARGET_SYSTEM_PROP += $(DEVICE_PATH)/system.prop

# Partitions
BOARD_RECOVERYIMAGE_PARTITION_SIZE := 43554432
BOARD_BOOTIMAGE_PARTITION_SIZE := 43554432
BOARD_SYSTEMIMAGE_PARTITION_SIZE := 704857600
BOARD_FLASH_BLOCK_SIZE := 131072 # (BOARD_KERNEL_PAGESIZE * 64)
