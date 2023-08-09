import sys
import os
import pickle
import hashlib
def run_cmd(cmd):
    print(cmd)

    if err := os.system(cmd) != 0:
        print(f'{cmd} exited with non-zero ({err}) code. Terminating')
        sys.exit(err)

targets = {}
cache = {}
def target(*args_or_func):
    def decorator(func):
        global targets
        # def wrapper(*args, **kwargs):
        #     return func(*args, **kwargs)
        setattr(func, "deps", args_or_func)
        targets[func.__name__] = func
        return func
    
    return decorator

def digest(file):
    # BUF_SIZE is totally arbitrary, change for your app!
    BUF_SIZE = 1024

    sha1 = hashlib.sha1()

    with open(file, 'rb') as f:
        while True:
            data = f.read(BUF_SIZE)
            if not data:
                break
            sha1.update(data)
    return sha1.hexdigest()
cache_updated = False
def run_on_source(source_file):
    global cache
    global cache_updated
    cache_entry = cache.get(source_file)
    source_hash = digest(source_file)
    if cache_entry != None:
       if cache_entry == source_hash:
           print(source_file + " is up to date")
           return False
    cache_updated = True
    cache[source_file] = source_hash
    return True

called_so_far = set()
def call_target(called_so_far, _target):
    # global called_so_far
    if _target in called_so_far: return
    called_so_far.add(_target)
    
    deps = getattr(_target, "deps", None)
    if deps != None:
        for dep in deps: call_target(called_so_far, dep)
    if _target == None:
        raise TypeError("Target is none!")
    _target()
    print(f":{_target.__name__}")


def handle_packages():
    pass

def build(dir, _targets=[]):
    global called_so_far
    global cache
    global cache_updated
    cache_file = f"{dir}{os.sep}cache{os.sep}it"
    if os.path.exists(cache_file):
        with open(cache_file, "rb") as f:
            cache = pickle.load(f)
    else:
        cache = dict()
    if len(_targets) == 0:
        found_main = False
        for k, v in targets.items():
            if k == "main":
                call_target(called_so_far, v)
                found_main = True
                break
        if not found_main:
            for target in targets.values():
                call_target(called_so_far, target)
    else:
        for target_name in _targets:
            target = targets.get(target_name)
            if target == None:
                print(f"No such target: {target_name}")
                sys.exit(1)
            call_target(called_so_far, target)
    
    if not cache_updated: return
    with open(cache_file, "wb") as f:
        pickle.dump(cache, f)

def check_directories(dir):
    os.makedirs(f"{dir}{os.sep}cache", exist_ok=True)
    os.makedirs(f"{dir}{os.sep}bin", exist_ok=True)
def run(project):
    build_dir = project["BUILD_DIR"]
    check_directories(build_dir)
    handle_packages()
    build(build_dir, project["TARGETS"])
    