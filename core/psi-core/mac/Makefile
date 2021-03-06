#
# Build file for packaging Psi/Mac.
#
# The DMG packager looks for a template.dmg.bz2 for using as its 
# DMG template. If it doesn't find one, it generates a clean one.
#
# If you create a DMG template, you should make one containing all
# the files listed in $(FILES) below, and arrange everything to suit
# your style. The contents of the files themselves does not matter, so
# they can be empty. 
#
# Parameters:
#	* BUILD: Set this to 'no' if you do not want to build the source tree or
#		check dependencies of the 'disk' dir when building the DMG.
#
# Remko Troncon <remko@psi-im.org>
#

# Change these according to your own setup
QTDIR=/usr/local/Trolltech/Qt-4.2.2
QCADIR=/usr/local
GROWLDIR=/Library/Frameworks
PSI_DIR=..

NAME=Psi
VERSION=0.11-devel_$(shell date +"%Y%m%d")
#VERSION=0.11-beta5
FILES=Psi.app 
DISK_DIR=disk
ifneq ($(BUILD),no)
DISK_FILES=$(foreach f,$(FILES), $(addprefix $(DISK_DIR)/,$(f)))
endif
MASTER_DMG=$(NAME)-$(VERSION).dmg


# Other variables
QT_FRAMEWORK_VERSION=4
QT_FRAMEWORKS=QtCore QtXml QtNetwork QtGui QtSql Qt3Support

# The rules
.PHONY: buildall
all: build-psi $(DISK_FILES)

.PHONY: dmg
dmg: build-psi $(DISK_FILES) $(MASTER_DMG)

# Cleanup
.PHONY: clean
clean: clean-dmg
	-rm -rf $(DISK_DIR)

# Help
.PHONY: help
help:
	@echo "Usage:"
	@echo "    make : Build a distributable binary in $(DISK_DIR)/"
	@echo "    make dmg : Build a DMG disk image"

# Building the main source tree
.PHONY: build-psi
ifneq ($(BUILD),no)
build-psi:
	@echo
	@echo --------------------- Building Source Tree --------------------
	$(MAKE) -C $(PSI_DIR)
	@echo
else
build-psi:
endif


################################################################################
# Generation of the final files
################################################################################

# The binary
$(DISK_DIR)/Psi.app: $(PSI_DIR)/src/psi.app
	@echo
	@echo --------------------- Packaging Binary --------------------
	rm -rf "$@"
	ditto -rsrc "$<" "$@"
	mkdir -p "$@/Contents/Frameworks"
	-ditto -rsrc "$(GROWLDIR)/Growl.framework" "$@/Contents/Frameworks/Growl.framework"
	strip "$@/Contents/MacOS/psi"
	#
	#@echo
	#@echo Installing QCA ...
	#cp -f "$(QCADIR)/lib/libqca.2.dylib" "$@/Contents/Frameworks/"
	#install_name_tool -id "@executable_path/../Frameworks/libqca.2.dylib" "$@/Contents/Frameworks/libqca.2.dylib"
	#install_name_tool -change "libqca.2.dylib" "@executable_path/../Frameworks/libqca.2.dylib" "$@/Contents/MacOS/psi"
	#install_name_tool -change "$(QCADIR)/lib/libqca.2.dylib" "@executable_path/../Frameworks/libqca.2.dylib" "$@/Contents/MacOS/psi"
	#mkdir -p "$@/Contents/Resources/crypto"
	#cp -f "$(QTDIR)/plugins/crypto/libqca-openssl.dylib" "$@/Contents/Resources/crypto/"
	#install_name_tool -change "$(QCADIR)/lib/libqca.2.dylib" "@executable_path/../Frameworks/libqca.2.dylib" "$@/Contents/Resources/crypto/libqca-openssl.dylib"
	#install_name_tool -change "libqca.2.dylib" "@executable_path/../Frameworks/libqca.2.dylib" "$@/Contents/Resources/crypto/libqca-openssl.dylib"
	mkdir -p "$@/Contents/Resources/imageformats"
	cp -f "$(QTDIR)/plugins/imageformats/libqjpeg.dylib" "$@/Contents/Resources/imageformats/"
	#
	@echo
	@echo Installing Qt Frameworks ...
	for f in $(QT_FRAMEWORKS); do \
		ditto -rsrc "$(QTDIR)/lib/$$f.framework" "$@/Contents/Frameworks/$$f.framework"; \
		rm -rf "$@/Contents/Frameworks/$$f.framework/Headers"; \
		rm -rf "$@/Contents/Frameworks/$$f.framework/$$f""_debug"; \
		rm -rf "$@/Contents/Frameworks/$$f.framework/$$f""_debug.prl"; \
		rm -rf "$@/Contents/Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/Headers"; \
		rm -rf "$@/Contents/Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f""_debug"; \
		rm -rf "$@/Contents/Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f""_debug.prl"; \
		install_name_tool -id "@executable_path/../Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "$@/Contents/Frameworks/$$f.framework/$$f"; \
		for g in $(QT_FRAMEWORKS); do \
			install_name_tool -change "$(QTDIR)/lib/$$g.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$g" "@executable_path/../Frameworks/$$g.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$g" "$@/Contents/Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f"; \
		done; \
		\# install_name_tool -change "$(QTDIR)/lib/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "@executable_path/../Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "$@/Contents/Frameworks/libqca.2.dylib"; \
		\# install_name_tool -change "$(QTDIR)/lib/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "@executable_path/../Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "$@/Contents/Resources/crypto/libqca-openssl.dylib"; \
		install_name_tool -change "$(QTDIR)/lib/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "@executable_path/../Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "$@/Contents/Resources/imageformats/libqjpeg.dylib"; \
		install_name_tool -change "$(QTDIR)/lib/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "@executable_path/../Frameworks/$$f.framework/Versions/$(QT_FRAMEWORK_VERSION)/$$f" "$@/Contents/MacOS/psi"; \
	done

# The rest
$(DISK_DIR)/%: $(PSI_DIR)/%
	ditto -rsrc "$<" $@


################################################################################
# Disk image creation
################################################################################

WC_DMG=wc.dmg
WC_DIR=wc
TEMPLATE_DMG=template.dmg

$(TEMPLATE_DMG): $(TEMPLATE_DMG).bz2
	bunzip2 -k $<

$(TEMPLATE_DMG).bz2: 
	@echo
	@echo --------------------- Generating empty template --------------------
	mkdir template
	hdiutil create -size 80m "$(TEMPLATE_DMG)" -srcfolder template -format UDRW -volname "$(NAME)" -quiet
	rmdir template
	bzip2 "$(TEMPLATE_DMG)"
	@echo

$(WC_DMG): $(TEMPLATE_DMG)
	cp $< $@

$(MASTER_DMG): $(WC_DMG) $(DISK_FILES)
	@echo
	@echo --------------------- Creating Disk Image --------------------
	mkdir -p $(WC_DIR)
	hdiutil attach "$(WC_DMG)" -noautoopen -quiet -mountpoint "$(WC_DIR)"
	for i in $(DISK_DIR)/*; do  \
		file=`basename $$i`; \
		rm -rf "$(WC_DIR)/$$file"; \
		ditto -rsrc "$(DISK_DIR)/$$file" "$(WC_DIR)/$$file"; \
	done
	#rm -f "$@"
	#hdiutil create -srcfolder "$(WC_DIR)" -format UDZO -imagekey zlib-level=9 "$@" -volname "$(NAME) $(VERSION)" -scrub -quiet
	WC_DEV=`hdiutil info | grep "$(WC_DIR)" | grep "Apple_HFS" | awk '{print $$1}'` && \
	hdiutil detach $$WC_DEV -quiet -force
	rm -f "$(MASTER_DMG)"
	hdiutil convert "$(WC_DMG)" -quiet -format UDZO -imagekey zlib-level=9 -o "$@"
	rm -rf $(WC_DIR)
	-hdiutil internet-enable -yes -quiet "$@"
	@echo

.PHONY: clean-dmg
clean-dmg:
	-rm -rf $(TEMPLATE_DMG) $(MASTER_DMG) $(WC_DMG) $(NAME)-*.dmg

