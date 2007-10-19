SUBDIRS = icons

pkgconfigdir = $(libdir)/pkgconfig
if WITH_GTK
pkgconfig_DATA = swfdec-@SWFDEC_MAJORMINOR@.pc swfdec-gtk-@SWFDEC_MAJORMINOR@.pc
else
pkgconfig_DATA = swfdec-@SWFDEC_MAJORMINOR@.pc
endif

CLEANFILES = swfdec-$(SWFDEC_MAJORMINOR).pc swfdec-gtk-$(SWFDEC_MAJORMINOR).pc

swfdec-@SWFDEC_MAJORMINOR@.pc: swfdec.pc
	cp swfdec.pc swfdec-@SWFDEC_MAJORMINOR@.pc

swfdec-gtk-@SWFDEC_MAJORMINOR@.pc: swfdec-gtk.pc
	cp swfdec-gtk.pc swfdec-gtk-@SWFDEC_MAJORMINOR@.pc

gtk_update_icon_cache = gtk-update-icon-cache -f -t $(datadir)/icons/hicolor

install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache

update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
	else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
	fi
