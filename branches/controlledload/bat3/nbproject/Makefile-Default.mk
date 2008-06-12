#
# Gererated Makefile - do not edit!
#
# Edit the Makefile in the project folder instead (../Makefile). Each target
# has a -pre and a -post target defined where you can add customized code.
#
# This makefile implements configuration specific macros and targets.


# Environment
MKDIR=mkdir
CP=cp
CCADMIN=CCadmin
RANLIB=ranlib
CC=gcc
CCC=g++
CXX=g++
FC=

# Include project Makefile
include bat3-Makefile.mk

# Object Directory
OBJECTDIR=build/Default/GNU-Linux-x86

# Object Files
OBJECTFILES=

# C Compiler Flags
CFLAGS=

# CC Compiler Flags
CCFLAGS=
CXXFLAGS=

# Fortran Compiler Flags
FFLAGS=

# Link Libraries and Options
LDLIBSOPTIONS=

# Build Targets
.build-conf: ${BUILD_SUBPROJECTS} 
	cd . && ${MAKE} -f Makefile CFLAGS=-Wall CC=/opt/crosstool/gcc-4.0.1-glibc-2.3.5/arm-unknown-linux-gnu/bin/arm-unknown-linux-gnu-gcc

# Subprojects
.build-subprojects:

# Clean Targets
.clean-conf:
	cd . && ${MAKE} -f Makefile clean

# Subprojects
.clean-subprojects:
