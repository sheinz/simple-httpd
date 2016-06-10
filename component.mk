
INC_DIRS += $(simple-httpd_ROOT)

simple-httpd_SRC_DIR = $(simple-httpd_ROOT)

$(eval $(call component_compile_rules,simple-httpd))
