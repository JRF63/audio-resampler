#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

opts = Variables([], ARGUMENTS)

opts.Add("target_name", "Name of the library to be built by SCons", "libresampler")
opts.Update(env)

env.Append(CPPPATH=["src/", "soxr/src/"])
env.Append(CPPDEFINES = "SOXR_LIB")

if env["platform"] == "windows":
    suffix = "Debug"
    if env["target"] == "template_release":
        suffix = "Release"
else:
    suffix = ""

# Boost C++ headers
boost_path = env.get('boost_inc')

if not boost_path:
    boost_root = os.environ.get('BOOST_ROOT')
    
    if boost_root:
        boost_path = os.path.join(boost_root, 'include')
    else:
        if env["platform"] == "windows":
            vcpkg_root = os.environ.get('VCPKG_INSTALLATION_ROOT')
            triplet = "x64-windows"
            boost_path = os.path.join(vcpkg_root, 'installed', triplet, 'include')
        elif env["platform"] == "macos":
            try:
                with os.popen('brew --prefix') as f:
                    boost_path = os.path.join(f.read().strip(), 'include')
            except:
                pass
        else:
            # linux
            boost_path = "/usr/include"

# Add the Boost headers if they exist
if boost_path and os.path.exists(boost_path):
    env.Append(CPPPATH=[boost_path])

if env["target"] == "template_release":
    if env["platform"] == "windows":
        env.Append(CCFLAGS=["/GL"])
        env.Append(LINKFLAGS=["/LTCG"])
    else:
        env.Append(CCFLAGS=["-flto"])
        env.Append(LINKFLAGS=["-flto"])

sources = Glob("src/*.cpp")
sources += [
    "soxr/src/cr.c",
    "soxr/src/cr32.c",
    # "soxr/src/cr32s.c",
    "soxr/src/cr64.c",
    # "soxr/src/cr64s.c",
    "soxr/src/data-io.c",
    "soxr/src/dbesi0.c",
    "soxr/src/fft4g32.c",
    "soxr/src/fft4g64.c",
    "soxr/src/filter.c",
    # "soxr/src/pffft32s.c",
    # "soxr/src/pffft64s.c",
    # "soxr/src/util32s.c",
    # "soxr/src/util64s.c",
    "soxr/src/soxr.c",
    "soxr/src/vr32.c",
]

if env["platform"] == "windows":
    library = env.SharedLibrary(
        "project/bin/{}{}{}".format(env["target_name"], env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )
elif env["platform"] == "macos":
    libname = "{}.{}.{}{}".format(
            env["target_name"],
            env["platform"],
            env["target"],
            env["SHLIBSUFFIX"],
        )

    env.Append(CCFLAGS=["-mmacosx-version-min=11.0"])
    env.Append(CXXFLAGS=["-mmacosx-version-min=11.0"])
    env.Append(LINKFLAGS=["-dynamiclib", "-install_name", "@rpath/{}".format(libname)])
    env.Append(LINKFLAGS=["-mmacosx-version-min=11.0"])

    library = env.SharedLibrary(
        "project/bin/{}".format(libname),
        source=sources,
    )
else:
    library = env.SharedLibrary(
        "project/bin/{}{}{}".format(env["target_name"], env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library)
