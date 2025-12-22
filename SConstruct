#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

opts = Variables([], ARGUMENTS)

opts.Add("target_name", "Name of the library to be built by SCons", "libresampler")
opts.Update(env)

build_dir = "build/"
env.VariantDir(os.path.join(build_dir, "soxr"), "soxr/src", duplicate=0)

env.Append(CPPPATH=["src/", "soxr/src/"])
env.Append(CPPDEFINES = "SOXR_LIB")

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

sources = Glob("src/*cpp")
soxr_src_files = [
    "cr.c",
    "cr32.c",
    # "cr32s.c",
    "cr64.c",
    # "cr64s.c",
    "data-io.c",
    "dbesi0.c",
    "fft4g32.c",
    "fft4g64.c",
    "filter.c",
    # "pffft32s.c",
    # "pffft64s.c",
    # "util32s.c",
    # "util64s.c",
    "soxr.c",
    "vr32.c",
]
sources += [os.path.join(build_dir, "soxr", s) for s in soxr_src_files]

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

    env["macos_deployment_target"] = "11.0"
    env.Append(LINKFLAGS=["-dynamiclib", "-install_name", "@rpath/{}".format(libname)])

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
