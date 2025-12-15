#!/usr/bin/env python
import os
import sys

env = SConscript("godot-cpp/SConstruct")

opts = Variables([], ARGUMENTS)

opts.Add("target_name", "Name of the library to be built by SCons", "libresampler")
opts.Update(env)

env.Append(CPPPATH=["src/", "audio-resampler/"])

if env["platform"] == "windows":
    suffix = "Debug"
    if env["target"] == "template_release":
        suffix = "Release"
else:
    suffix = ""

if env["target"] == "template_release":
    if env["platform"] == "windows":
        env.Append(CCFLAGS=["/GL"])
        env.Append(LINKFLAGS=["/LTCG"])
    else:
        env.Append(CCFLAGS=["-flto"])
        env.Append(LINKFLAGS=["flto"])

sources = Glob("src/*.cpp")
sources += Glob("audio-resampler/resampler.c")

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
