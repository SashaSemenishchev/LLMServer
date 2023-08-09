from buildlib import *
import hashlib
import argparse
import shutil
import platform
sep = os.sep
PROJECT = {
    "name": "C-Server",
}

CC = "gcc"
CXX = "g++"
SRC_FOLDER = "src"
CCFLAGS = ["-Wall", "-Wextra"]
LDFLAGS = []
parser = argparse.ArgumentParser()
parser.add_argument("--release", action="store_true", help="builds release project")
parser.add_argument("--impl", type=str, default="falcon")
parser.add_argument("-r", "--run", type=list, default=[], help="runs specified targets")
args = parser.parse_args()

if args.release:
    PROJECT["BUILD_DIR"] = "build/release"
    CCFLAGS.append("-O3")
else:
    PROJECT["BUILD_DIR"] = "build/debug"
    CCFLAGS.append("-O0")
    CCFLAGS.append("-g")
    LDFLAGS.append("-g")
TARGET_FILE = "main"
if platform.system() == "Darwin":
    CCFLAGS.append("-DGGML_USE_ACCELERATE")
    LDFLAGS.append("-framework Accelerate")

@target()
def build_impl():
    impl_path = f"{SRC_FOLDER}{sep}ai{sep}impl{sep}{args.impl}"
    if not os.path.exists(impl_path):
        print(f"Impl {args.impl} does not exist")
        sys.exit(1)
    run_cmd(f"make -C {impl_path} all")
    for file in os.listdir(impl_path):
        if os.path.isdir(file): continue
        if not file.endswith(".o"): continue
        src = impl_path + sep + file
        if not run_on_source(src): continue
        shutil.copyfile(src, PROJECT["BUILD_DIR"] + sep + file, follow_symlinks=False)

@target(build_impl)
def compile():
    impl_path = f"{SRC_FOLDER}{sep}ai{sep}impl{sep}{args.impl}"
    for (dirpath, dirnames, filenames) in os.walk(SRC_FOLDER):
        if dirpath.startswith(impl_path): continue
        
        for file in filenames:
            path = dirpath + sep + file
            if not path.endswith(".c"): continue
            if not run_on_source(path): continue
            
            run_cmd(f"{CC} {' '.join(CCFLAGS)} -c {path} -o {PROJECT['BUILD_DIR']}{sep}{file.replace('.c', '.o')}")

@target()
def link():
    changed = False
    srcs = []
    for file in os.listdir(PROJECT["BUILD_DIR"]):
        if os.path.isdir(file): continue
        if not file.endswith(".o"): continue
        src = PROJECT["BUILD_DIR"] + sep + file
        srcs.append(src)
        if run_on_source(src):
            changed = True

    if not changed: return
    run_cmd(f"{CXX} {' '.join(LDFLAGS)} {' '.join(srcs)} -o {TARGET_FILE}")
@target(compile, link)
def main():
    pass
    
PROJECT["TARGETS"] = args.run 
run(PROJECT)
