project(vbb)

cmake_minimum_required(VERSION 2.8)

SET(CMAKE_BUILD_TYPE "Release")
SET(CMAKE_INSTALL_PREFIX "/usr/local" CACHE STRING "Local installation directory")

OPTION(BUILD_DOC "Build documentation" ON)

find_package(Doxygen)
IF (DOXYGEN_FOUND)
  ADD_CUSTOM_TARGET( doc
        COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile
        COMMENT "Generating API documentation with Doxygen"
        VERBATIM )
else (DOXYGEN_FOUND)
  message("Doxygen need to be installed to generate the documentation of this project")
endif (DOXYGEN_FOUND)

FIND_PACKAGE(SDL REQUIRED)
FIND_PACKAGE(SDL_ttf REQUIRED)

# Needs package extra-cmake-modules. Otherwise, you will have to set manually the include dirs and library for Poppler
FIND_PACKAGE(ECM REQUIRED NO_MODULE)
SET(CMAKE_MODULE_PATH ${ECM_FIND_MODULE_DIR})

FIND_PACKAGE(Poppler REQUIRED)

SET(X11_LIBRARIES -lX11)

INCLUDE_DIRECTORIES(${SDL_INCLUDE_DIR} ${SDL_TTF_INCLUDE_DIR} ${Poppler_INCLUDE_DIRS})
ADD_DEFINITIONS(-Wall -Winline -O2)

ADD_EXECUTABLE(vbb main.cpp config.cpp pdfslides.cpp canvas.cpp)
TARGET_LINK_LIBRARIES(vbb ${SDL_LIBRARY} ${SDL_TTF_LIBRARY} ${Poppler_LIBRARIES} ${X11_LIBRARIES})

FILE(MAKE_DIRECTORY vbb)
INSTALL(DIRECTORY "vbb" DESTINATION "/etc" DIRECTORY_PERMISSIONS 
    OWNER_WRITE OWNER_READ OWNER_EXECUTE
    GROUP_WRITE GROUP_READ GROUP_EXECUTE
    WORLD_READ WORLD_EXECUTE)
INSTALL(TARGETS vbb DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
INSTALL(FILES vbb.cfg DESTINATION /etc/vbb)
INSTALL(FILES vbb_splash.pdf DESTINATION /etc/vbb)

# This is a hack to install vbb_menu and man page vbb.1 in your language.
IF (DEFINED ENV{LANG})
 SET(FULL_LANG $ENV{LANG})
 STRING(REGEX MATCH "^[^_]+_.*" L ${FULL_LANG})
 IF (L)
  STRING(REGEX REPLACE "^([^_]+)_.*" "\\1" L ${FULL_LANG})
 ELSE()
  STRING(REGEX MATCH "^[^.]+[.].*" L ${FULL_LANG})
  IF (L)
   STRING(REGEX REPLACE "^([^.]+)[.].*" "\\1" L ${FULL_LANG})
  ELSE()
   SET(L ${FULL_LANG})
  ENDIF()
 ENDIF()
ELSE()
 MESSAGE(WARNING "Environemnt variable LANG undefined. Assuming English.")
 SET(L "en")
ENDIF()

IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vbb_${L}_menu)
 SET(LANG_MENUCONF_FILE vbb_${L}_menu)
ELSE()
 MESSAGE(WARNING "Sorry, we don't provide menu configuration file for your local language (${L}). English will be installed. Would you consider to contribute with a translation of file vbb_en_menu as file vbb_${L}_menu?.")
 SET(LANG_MENUCONF_FILE vbb_en_menu)
ENDIF()

IF (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/vbb_${L}_man.1)
 SET(LANG_MENUCONF_FILE vbb_${L}_man.1)
ELSE()
 MESSAGE(WARNING "Sorry, we don't provide a manual page for your local language (${L}). English will be installed. Would you consider to contribute with a translation of file vbb_en_man.1 as file vbb_${L}_menu?.")
 SET(LANG_MENUCONF_FILE vbb_en)
ENDIF()

INSTALL(FILES vbb_${L}_menu DESTINATION /etc/vbb RENAME vbb_menu)
INSTALL(FILES vbb_${L}_man.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1 RENAME vbb.1)

