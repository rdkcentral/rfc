---
applyTo: "**/Makefile.am,**/configure.ac,**/*.ac,**/*.mk"
---

# Build System Standards (Autotools)

## Autotools Best Practices

### configure.ac
- Check for required headers and functions
- Provide clear error messages for missing dependencies
- Support cross-compilation
- Allow feature toggles

```autoconf
# GOOD: Check for required features
AC_CHECK_HEADERS([pthread.h], [],
    [AC_MSG_ERROR([pthread.h is required])])

AC_CHECK_LIB([pthread], [pthread_create], [],
    [AC_MSG_ERROR([pthread library is required])])

# GOOD: Optional features with clear naming
AC_ARG_ENABLE([gtest],
    AS_HELP_STRING([--enable-gtest], [Enable Google Test support]),
    [enable_gtest=$enableval],
    [enable_gtest=no])

AM_CONDITIONAL([WITH_GTEST_SUPPORT], [test "x$enable_gtest" = "xyes"])
```

### Makefile.am
- Use non-recursive makefiles when possible
- Minimize intermediate libraries
- Support parallel builds
- Link only what's needed

```makefile
# GOOD: Minimal linking
bin_PROGRAMS = rfcMgr

rfcMgr_SOURCES = rfc_main.cpp rfc_manager.cpp rfc_common.cpp mtlsUtils.cpp rfc_xconf_handler.cpp xconf_handler.cpp
rfcMgr_CPPFLAGS = -std=c++17 -Wall -Werror -DRDK_LOGGER -I${top_srcdir}/rfcMgr -I${top_srcdir}/rfcapi/ $(cjson_CFLAGS) $(curl_CFLAGS)
rfcMgr_LDADD = -lrdkloggers -ldwnlutil -lfwutils -lparsejson -lsecure_wrapper -lcurl

# GOOD: Conditional IARM linking
if IS_IARMBUS_ENABLED
rfcMgr_LDADD += -lIARMBus
endif

# GOOD: Conditional compilation
if WITH_GTEST_SUPPORT
SUBDIRS += rfcMgr/gtest
endif
```

## Cross-Compilation Support

### Platform Detection
```autoconf
# Support different target platforms
case "$host" in
    *-linux*)
        AC_DEFINE([PLATFORM_LINUX], [1], [Linux platform])
        ;;
    *-arm*)
        AC_DEFINE([PLATFORM_ARM], [1], [ARM platform])
        ;;
esac
```

### Compiler Flags
```makefile
# Platform-specific optimizations
if TARGET_ARM
AM_CFLAGS += -march=armv7-a -mfpu=neon
endif

# Debug vs Release
if DEBUG_BUILD
AM_CFLAGS += -g -O0 -DDEBUG
else
AM_CFLAGS += -O2 -DNDEBUG
endif
```

## Dependency Management

### Package Config
```autoconf
# Use pkg-config for external dependencies
PKG_CHECK_MODULES([DBUS], [dbus-1 >= 1.6])
AC_SUBST([DBUS_CFLAGS])
AC_SUBST([DBUS_LIBS])
```

### Header Organization
```makefile
# Include paths
AM_CPPFLAGS = -I$(top_srcdir)/rfcMgr \
              -I$(top_srcdir)/rfcapi \
              -I$(top_srcdir)/tr181api \
              -I$(top_srcdir)/utils \
              $(cjson_CFLAGS) $(curl_CFLAGS)
```

## Build Performance

### Parallel Builds
- Support `make -j`
- Avoid circular dependencies
- Use order-only prerequisites when appropriate

### Incremental Builds
- Proper dependency tracking
- Don't force full rebuilds unless necessary
- Use libtool for shared libraries

## Testing Integration

```makefile
# Test targets
check-local:
	@echo "Running memory leak tests..."
	@for test in $(TESTS); do \
		valgrind --leak-check=full \
		         --error-exitcode=1 \
		         ./$$test || exit 1; \
	done

# Code coverage
if ENABLE_COVERAGE
AM_CFLAGS += --coverage
AM_LDFLAGS += --coverage
endif

coverage: check
	$(LCOV) --capture --directory . --output-file coverage.info
	$(GENHTML) coverage.info --output-directory coverage
```
