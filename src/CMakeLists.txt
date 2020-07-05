find_package (PkgConfig REQUIRED)
pkg_check_modules (SDL_IMAGE REQUIRED SDL_image>=1.2)
pkg_check_modules (SDL REQUIRED sdl)
pkg_check_modules (ZLIB REQUIRED zlib)
pkg_check_modules (CURL REQUIRED libcurl)
pkg_check_modules (ZIP REQUIRED libzip)

set (GUICHAN_LIBRARIES "-lguichan -lguichan_sdl")

add_executable (
    gala
    6821.cpp
    AY8910.cpp
    chooser.cpp
    cli.cpp
    CPU.cpp
    Debug.cpp
    DiskChoose.cpp
    Disk.cpp
    DiskFTP.cpp
    DiskImage.cpp
    draw.cpp
    font.cpp
    Frame.cpp
    ftpparse.cpp
    Harddisk.cpp
    Joystick.cpp
    Keyboard.cpp
    Log.cpp
    main.cpp
    Memory.cpp
    Mockingboard.cpp
    MouseInterface.cpp
    ParallelPrinter.cpp
    platform.cpp
    Registry.cpp
    Riff.cpp
    SaveState.cpp
    SerialComms.cpp
    SoundCore.cpp
    Speaker.cpp
    stretch.cpp
    Timer.cpp
    Video.cpp
    wwrapper.cpp
)

target_link_libraries (${PROJECT_NAME} ${GUICHAN_LIBRARIES} ${SDL_LIBRARIES} ${SDL_IMAGE_LIBRARIES} ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} ${ZIP_LIBRARIES})
target_include_directories (gala PUBLIC ${SDL_INCLUDE_DIRS})
target_compile_options (gala PUBLIC -Wwrite-strings ${SDL_CFLAGS_OTHER})
