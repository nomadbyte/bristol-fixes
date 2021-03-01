SUBDIRS= libbristolaudio libbristolmidi libbvg @BRIGHTON_X11_DIR@ libbrightonC11 libbristolic libbrighton libbristol brighton bristol bin
#SUBDIRS= libbristolaudio libbristolmidi libbrightonX11 libbrightonC11 libbristolic libbrighton libbristol brighton bristol bin

ACLOCAL_AMFLAGS=-I m4

bristoldir=${BRISTOL_DIR}

install-man:
	$(INSTALL) -d $(DESTDIR)$(mandir)/man1
	$(INSTALL) -m 0644 $(srcdir)/bristol.1 $(DESTDIR)$(mandir)/man1/
	$(INSTALL) -m 0644 $(srcdir)/bristoljackstats.1 $(DESTDIR)$(mandir)/man1/
	gzip -9fn $(DESTDIR)$(mandir)/man1/bristol.1
	gzip -9fn $(DESTDIR)$(mandir)/man1/bristoljackstats.1
	cd $(DESTDIR)$(mandir)/man1 && ln -sf bristol.1.gz brighton.1.gz && ln -sf bristol.1.gz startBristol.1.gz

installdb:
	mkdir -p -m 0755 $(DESTDIR)$(bristoldir)
	tar cfp - $(srcdir)/bitmaps $(srcdir)/memory | (cd $(DESTDIR)$(bristoldir) ; tar xfp -)
	find $(DESTDIR)$(bristoldir)/. -type d -exec chmod 0755 {} \; -o -type f -exec chmod 0644 {} \;

uninstalldb:
	rm -rf $(DESTDIR)$(bristoldir)/bitmaps $(DESTDIR)$(bristoldir)/memory
	rm -f $(DESTDIR)${bindir}/startBristol

installbristolbin:
	mkdir -p -m 0755 $(DESTDIR)$(bindir)
	$(install_sh) -c -m 0755 bin/startBristol $(DESTDIR)$(bindir)

install: install-recursive installdb installbristolbin install-man

uninstall: uninstall-recursive uninstalldb
	rm -rf $(DESTDIR)$(bristoldir)/memory
	rm -rf $(DESTDIR)$(bristoldir)/bitmaps
	rm -rf  $(DESTDIR)$(bristoldir)
	rm -rf $(DESTDIR)$(mandir)/man1/bristol.1 $(DESTDIR)$(mandir)/man1/brighton.1 $(DESTDIR)$(mandir)/man1/startBristol.1 $(DESTDIR)$(mandir)/man1/bristoljackstats.1
	rm -rf $(DESTDIR)$(mandir)/man1/bristol.1.gz $(DESTDIR)$(mandir)/man1/brighton.1.gz $(DESTDIR)$(mandir)/man1/startBristol.1.gz $(DESTDIR)$(mandir)/man1/bristoljackstats.1.gz

EXTRA_DIST=include bitmaps memory HOWTO COPYING COPYING.GPL bristol.1 bristoljackstats.1

