cmake_minimum_required(VERSION 3.16)

project(untitled1 VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(ONNXRUNTIME_ROOT "/home/oussema/Documents/onnxruntime" CACHE PATH "Root path for ONNX Runtime build.")
set(ONNXRUNTIME_INCLUDE_DIR "${ONNXRUNTIME_ROOT}/include")
set(ONNXRUNTIME_LIB_PATH "${ONNXRUNTIME_ROOT}/build/Android/RelWithDebInfo/libonnxruntime.so")

find_package(Qt6 REQUIRED COMPONENTS Widgets Quick Bluetooth CorePrivate)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(appuntitled1
    main.cpp
)

qt_add_qml_module(appuntitled1
    URI untitled1
    VERSION 1.0
    QML_FILES
        Main.qml
        SOURCES
        SOURCES
        SOURCES bleclient.h bleclient.cpp
        SOURCES guiwindow.h guiwindow.cpp
        RESOURCES android/AndroidManifest.xml android/build.gradle android/res/values/libs.xml android/res/xml/qtprovider_paths.xml
        SOURCES initialformwindow.h initialformwindow.cpp
        RESOURCES android/src/org/qtproject/example/androidnotifier/NotificationClient.java
        )

    qt_add_resources(appuntitled1 "myresources"
        PREFIX "/"
        FILES
            health_classifier.onnx
    )

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
set_target_properties(appuntitled1 PROPERTIES
#    MACOSX_BUNDLE_GUI_IDENTIFIER com.example.appuntitled1
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)
target_include_directories(appuntitled1 PRIVATE
    # Headers needed to use the ONNX Runtime C++ API
    ${ONNXRUNTIME_INCLUDE_DIR}
)
target_link_libraries(appuntitled1
    PRIVATE ${ONNXRUNTIME_LIB_PATH}
    # Android System Libraries often required by ONNX Runtime
        -llog       # For Android logging
        -latomic    # For atomic operations
    PRIVATE Qt6::Widgets
    Qt6::CorePrivate
    PRIVATE Qt6::Quick
    PRIVATE Qt6::Bluetooth
)

#if(ANDROID)
    # Define the deployment path within the Android build directory
 #   set(ANDROID_AARCH64_LIB_DIR "${CMAKE_BINARY_DIR}/android-build/lib/arm64-v8a")
    # Create the directory if it doesn't exist
  #  file(MAKE_DIRECTORY ${ANDROID_AARCH64_LIB_DIR})

    # Add a custom command to copy the .so file after the target is built
   # add_custom_command(TARGET appuntitled1 POST_BUILD
    #    COMMAND ${CMAKE_COMMAND} -E copy
     #   ${ONNXRUNTIME_LIB_PATH}
      #  ${ANDROID_AARCH64_LIB_DIR}
       # COMMENT "Copying ONNX Runtime shared library for Android deployment."
    #)
#endif()

if(ANDROID)
    # CRITICAL FIX: This property tells the Qt Android build system
    # to correctly package and load the external shared library.
    set_target_properties(appuntitled1 PROPERTIES
        QT_ANDROID_EXTRA_LIBS
        "${ONNXRUNTIME_LIB_PATH}"
    )
endif()

include(GNUInstallDirs)
install(TARGETS appuntitled1
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
