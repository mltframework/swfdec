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

