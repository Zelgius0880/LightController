Import("env")

for item in env.get("CPPPATH"):
    if ".pio/libdeps" in str(item):
        env.Replace(CPPFLAGS=[f"-isystem{item}" for item in env.get("CPPPATH") if ".pio/libdeps" in str(item)])
