TEMPLATE = subdirs

SUBDIRS += mvvmcore \
	mvvmwidgets \
	mvvmquick \
	imports

mvvmwidgets.depends += mvvmcore
mvvmquick.depends += mvvmcore
imports.depends += mvvmcore mvvmquick

qtHaveModule(datasync) {
	SUBDIRS += mvvmdatasynccore \
		mvvmdatasyncwidgets \
		mvvmdatasyncquick

	mvvmdatasynccore.depends += mvvmcore
	mvvmdatasyncwidgets.depends += mvvmdatasynccore mvvmwidgets
	mvvmdatasyncquick.depends += mvvmdatasynccore mvvmquick
	imports.depends += mvvmdatasynccore mvvmdatasyncquick
}

imports.CONFIG += no_lrelease_target

prepareRecursiveTarget(lrelease)
QMAKE_EXTRA_TARGETS += lrelease
