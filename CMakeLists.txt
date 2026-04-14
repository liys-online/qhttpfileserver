cmake_minimum_required(VERSION 3.16)

project(httpserver LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(ENABLE_GUI "Enable GUI" OFF)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Core)
list(APPEND QT_MODULES Core HttpServer)
if(ENABLE_GUI)
  message(STATUS "GUI enabled")
  list(APPEND QT_MODULES Gui Widgets)
  add_compile_definitions(ENABLE_GUI)
  set(APP_ICON ":/httpfileserver.ico")
  add_compile_definitions(APP_ICON=\"${APP_ICON}\")
endif()
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS ${QT_MODULES})

set(PROJECT_SOURCES
  main.cpp
  httpfileserver.h httpfileserver.cpp
  util.h util.cpp
  router/router.h router/router.cpp
  router/filerouter.h router/filerouter.cpp
  router/staticresourcerouter.h router/staticresourcerouter.cpp
  resource.qrc
  app_icon.rc
)
include_directories(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/router
)
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
  if(APPLE)
    qt_add_executable(httpserver MACOSX_BUNDLE ${PROJECT_SOURCES})
    add_custom_command(TARGET httpserver POST_BUILD
      COMMAND ${MACDEPLOYQT_EXECUTABLE}${QT_VERSION_MAJOR}
              $<TARGET_BUNDLE_DIR:httpserver>
              -always-overwrite -dmg
    )
  elseif(WIN32)
    qt_add_executable(httpserver ${PROJECT_SOURCES})
  else()
    qt_add_executable(httpserver MANUAL_FINALIZATION ${PROJECT_SOURCES})
  endif()
else()
    add_executable(httpserver ${PROJECT_SOURCES})
endif()

target_link_libraries(httpserver PRIVATE Qt${QT_VERSION_MAJOR}::Core Qt${QT_VERSION_MAJOR}::HttpServer)
if(ENABLE_GUI)
  target_link_libraries(httpserver PRIVATE Qt${QT_VERSION_MAJOR}::Gui Qt${QT_VERSION_MAJOR}::Widgets)
endif()
if(APPLE)
  install(FILES ${CMAKE_BINARY_DIR}/httpserver.dmg
    DESTINATION ${CMAKE_INSTALL_PREFIX}
  )
else()
  install(TARGETS httpserver
      LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
      RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
endif()
