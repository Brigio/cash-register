cmake_minimum_required(VERSION 3.16)

project(Cash_Register
    VERSION 0.3
    LANGUAGES CXX
)

# Informazioni applicazione
add_compile_definitions(
    APP_NAME="${PROJECT_NAME}"
    APP_VERSION="${PROJECT_VERSION}"
)

# Impostazioni Qt automatiche
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

# Standard C++
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt6 (Arch Linux)
find_package(Qt6 REQUIRED COMPONENTS
    Widgets
    Sql
    PrintSupport
    LinguistTools
)

# Sorgenti
set(PROJECT_SOURCES
    main.cpp
    registerwindow.cpp
    registerwindow.h
    registerwindow.ui
    groupswindow.cpp
    groupswindow.h
    groupswindow.ui
    aboutdialog.cpp
    aboutdialog.h
    aboutdialog.ui
    coloredelegate.h
    resource.qrc
)

# Eseguibile
qt_add_executable(Cash_Register
    MANUAL_FINALIZATION
    ${PROJECT_SOURCES}
)

# Traduzioni
qt_add_translations(Cash_Register
    TS_FILES
        translations/Cash_Register_en_150.ts
        translations/Cash_Register_it_IT.ts
)

# Link librerie Qt
target_link_libraries(Cash_Register
    PRIVATE
        Qt6::Widgets
        Qt6::Sql
        Qt6::PrintSupport
)

# Installazione (standard Arch Linux)
include(GNUInstallDirs)

install(TARGETS Cash_Register
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# Installa file di traduzione (.qm)
install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/translations"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/Cash_Register"
        FILES_MATCHING PATTERN "*.qm")

# Finalizzazione Qt6
qt_finalize_executable(Cash_Register)
