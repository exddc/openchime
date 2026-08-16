################################################################################
#
# chime - Open Chime doorbell application
#
################################################################################

CHIME_SITE = $(BR2_EXTERNAL_OPENCHIME_PATH)/../chime-src
CHIME_SITE_METHOD = local
CHIME_VERSION_FILE = $(CHIME_SITE)/chime/VERSION
OPENCHIME_VERSION_FILE = $(BR2_EXTERNAL_OPENCHIME_PATH)/version.env
CHIME_BUILD_META_FILE = $(BR2_EXTERNAL_OPENCHIME_PATH)/build_meta.env
CHIME_VERSION = $(strip $(shell head -n 1 $(CHIME_VERSION_FILE) 2>/dev/null))
CHIME_OS_VERSION = $(strip $(shell sed -n 's/^OPENCHIME_OS_VERSION=//p' $(OPENCHIME_VERSION_FILE)))
CHIME_CONFIG_VERSION = $(strip $(shell sed -n 's/^CHIME_CONFIG_VERSION=//p' $(OPENCHIME_VERSION_FILE)))
CHIME_BUILD_ID = $(strip $(shell sed -n 's/^CHIME_BUILD_ID=//p' $(CHIME_BUILD_META_FILE) 2>/dev/null))
CHIME_LICENSE = MIT
CHIME_LICENSE_FILES = chime/README.md
CHIME_DEPENDENCIES = mosquitto openssl
CHIME_SUPPORTS_IN_SOURCE_BUILD = NO

ifeq ($(CHIME_VERSION),)
$(error Missing chime app version in $(CHIME_VERSION_FILE))
endif

ifeq ($(CHIME_OS_VERSION),)
$(error Missing OPENCHIME_OS_VERSION in $(OPENCHIME_VERSION_FILE))
endif

ifeq ($(CHIME_CONFIG_VERSION),)
$(error Missing CHIME_CONFIG_VERSION in $(OPENCHIME_VERSION_FILE))
endif

ifeq ($(CHIME_BUILD_ID),)
$(error Missing CHIME_BUILD_ID in $(CHIME_BUILD_META_FILE))
endif

CHIME_CONF_ENV = \
	PKG_CONFIG="$(HOST_DIR)/bin/pkg-config" \
	PKG_CONFIG_SYSROOT_DIR="$(STAGING_DIR)" \
	PKG_CONFIG_LIBDIR="$(STAGING_DIR)/usr/lib/pkgconfig:$(STAGING_DIR)/usr/share/pkgconfig"

CHIME_CONF_OPTS = \
	-DOC_PRODUCTION_BUILD=ON \
	-DOC_BUILD_TESTS=OFF \
	-DCMAKE_INSTALL_PREFIX=/usr/local \
	-DCHIME_APP_VERSION=$(CHIME_VERSION) \
	-DOPENCHIME_OS_VERSION=$(CHIME_OS_VERSION) \
	-DCHIME_CONFIG_VERSION=$(CHIME_CONFIG_VERSION) \
	-DCHIME_BUILD_ID=$(CHIME_BUILD_ID)

define CHIME_INSTALL_VERSION_FILES
	mkdir -p $(TARGET_DIR)/etc/chime-web/tls
	mkdir -p $(TARGET_DIR)/etc
	printf '%s\n' "$(CHIME_VERSION)" > $(TARGET_DIR)/etc/chime-app-version
	printf '%s\n' "$(CHIME_BUILD_ID)" > $(TARGET_DIR)/etc/chime-build-id
endef
CHIME_POST_INSTALL_TARGET_HOOKS += CHIME_INSTALL_VERSION_FILES

$(eval $(cmake-package))
