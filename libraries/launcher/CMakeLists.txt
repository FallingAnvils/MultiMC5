cmake_minimum_required(VERSION 3.1)
project(launcher Java)
find_package(Java 1.6 REQUIRED COMPONENTS Development)

include(UseJava)
set(CMAKE_JAVA_JAR_ENTRY_POINT org.multiservermc.EntryPoint)
set(CMAKE_JAVA_COMPILE_FLAGS -target 1.6 -source 1.6 -Xlint:deprecation -Xlint:unchecked)

set(SRC
    org/multiservermc/EntryPoint.java
    org/multiservermc/Launcher.java
    org/multiservermc/LegacyFrame.java
    org/multiservermc/NotFoundException.java
    org/multiservermc/ParamBucket.java
    org/multiservermc/ParseException.java
    org/multiservermc/Utils.java
    org/multiservermc/onesix/OneSixLauncher.java
    net/minecraft/Launcher.java
)
add_jar(NewLaunch ${SRC})
install_jar(NewLaunch "${JARS_DEST_DIR}")
