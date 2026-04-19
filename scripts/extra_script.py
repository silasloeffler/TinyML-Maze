"""
PlatformIO extra build script.
Applies Cortex-M4 FPU and floating-point ABI flags early in the build.
"""
Import("env")

vfp_flags = [
    "-mthumb",
    "-march=armv7e-m",
    "-mfloat-abi=hard",
    "-mfpu=fpv4-sp-d16",
]

env.Append(CCFLAGS=vfp_flags)
env.Append(CXXFLAGS=vfp_flags)
env.Append(LINKFLAGS=vfp_flags)
env.Append(ASFLAGS=vfp_flags)
