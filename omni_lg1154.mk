# Inherit from the common Open Source product configuration
$(call inherit-product, $(SRC_TARGET_DIR)/product/full_base.mk)

# Inherit from our custom product configuration
$(call inherit-product, vendor/omni/config/common.mk)

PRODUCT_DEVICE := lg1154
PRODUCT_NAME := omni_lg1154
PRODUCT_BRAND := lg
PRODUCT_MODEL := LG1154
PRODUCT_MANUFACTURER := lg
