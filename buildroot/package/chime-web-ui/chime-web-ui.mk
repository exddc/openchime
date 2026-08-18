################################################################################
#
# chime-web-ui - production Svelte bundle for chime-webd
#
################################################################################

CHIME_WEB_UI_SITE = $(BR2_EXTERNAL_OPENCHIME_PATH)/../webui-src
CHIME_WEB_UI_SITE_METHOD = local
CHIME_WEB_UI_VERSION = 1
CHIME_WEB_UI_LICENSE = MIT
CHIME_WEB_UI_LICENSE_FILES = README.md
CHIME_WEB_UI_SUPPORTS_IN_SOURCE_BUILD = YES
CHIME_WEB_UI_INSTALL_TARGET = YES
CHIME_WEB_UI_TARGET_DIST = /usr/local/share/chime-web-ui/dist
CHIME_WEB_UI_ASSERT = $(BR2_EXTERNAL_OPENCHIME_PATH)/board/raspberrypi0w/assert_chime_web_ui_dist.sh

define CHIME_WEB_UI_BUILD_CMDS
	if [ ! -f $(@D)/package.json ]; then \
		echo "ERROR: missing $(@D)/package.json (webui sources were not staged)" >&2; \
		exit 1; \
	fi
	if [ ! -f $(@D)/bun.lock ]; then \
		echo "ERROR: missing $(@D)/bun.lock (locked Bun dependencies are required)" >&2; \
		exit 1; \
	fi
	BUN="bun"; \
	if ! command -v "$$BUN" >/dev/null 2>&1; then \
		if [ -x /usr/local/bin/bun ]; then \
			BUN=/usr/local/bin/bun; \
		else \
			echo "ERROR: bun is required to build chime-web-ui" >&2; \
			exit 1; \
		fi; \
	fi; \
	echo "[chime-web-ui] Using $$BUN ($$("$$BUN" --version))"; \
	cd $(@D) && "$$BUN" install --frozen-lockfile; \
	cd $(@D) && "$$BUN" run build
	rm -rf $(@D)/node_modules
	bash $(CHIME_WEB_UI_ASSERT) $(@D)/dist
endef

define CHIME_WEB_UI_INSTALL_TARGET_CMDS
	rm -rf $(TARGET_DIR)$(CHIME_WEB_UI_TARGET_DIST)
	mkdir -p $(TARGET_DIR)$(CHIME_WEB_UI_TARGET_DIST)
	cp -a $(@D)/dist/. $(TARGET_DIR)$(CHIME_WEB_UI_TARGET_DIST)/
	chmod -R a+rX $(TARGET_DIR)/usr/local/share/chime-web-ui
	bash $(CHIME_WEB_UI_ASSERT) $(TARGET_DIR)$(CHIME_WEB_UI_TARGET_DIST)
endef

$(eval $(generic-package))
