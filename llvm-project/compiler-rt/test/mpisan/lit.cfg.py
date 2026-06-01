import os

# Setup config name.
config.name = "MPISAN" + config.name_suffix

# Setup source root.
config.test_source_root = os.path.dirname(__file__)

# Default compiler flags for MPI usage sanitizer tests.
clang_mpisan_cflags = ["-fsanitize=mpi-usage", config.target_cflags]
clang_mpisan_cxxflags = config.cxx_mode_flags + clang_mpisan_cflags


def build_invocation(compile_flags):
    return " " + " ".join([config.clang] + compile_flags) + " "


config.substitutions.append(("%clang ", build_invocation([config.target_cflags])))
config.substitutions.append(
    ("%clangxx ", build_invocation(config.cxx_mode_flags + [config.target_cflags]))
)
config.substitutions.append(
    ("%clang_mpisan ", build_invocation(clang_mpisan_cflags))
)
config.substitutions.append(
    ("%clangxx_mpisan ", build_invocation(clang_mpisan_cxxflags))
)

# Default test suffixes.
config.suffixes = [".c", ".cpp"]

# MPISan is only supported on Linux and macOS, x86_64 and AArch64.
if config.target_os not in ["Darwin", "Linux"]:
    config.unsupported = True
elif "64" not in config.host_arch and "aarch64" not in config.host_arch:
    config.unsupported = True
