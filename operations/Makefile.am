if HAVE_CXX14
common_cxx = common-cxx
endif

if ENABLE_WORKSHOP
workshop = workshop
endif

SUBDIRS = \
	core		\
	common		\
	common-gpl3+	\
	$(common_cxx)	\
	generated	\
	seamless-clone 	\
	transform 	\
	json 		\
	external	\
	$(workshop)

EXTRA_DIST = \
	Makefile-common.am
