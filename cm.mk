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

# Inherit from the common Open Source product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)

# Inherit from our custom product configuration
$(call inherit-product, vendor/cm/config/common_full_tablet_wifionly.mk)

# Inherit from onyx device
$(call inherit-product, device/lg/lg1154/device.mk)

PRODUCT_RELEASE_NAME := LG Smart TV
PRODUCT_DEVICE := lg1154
PRODUCT_NAME := cm_lg1154
PRODUCT_BRAND := LG
PRODUCT_MODEL := LG1154
PRODUCT_MANUFACTURER := lg
