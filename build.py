#!/usr/bin/env python3

# Seadragon build script. Okay, this *should* be good.
# `/` in Kakoune begins a search; if you look for `with Executable`, you
# should find where we tell the build script what sources to compile.

MAJOR_VER=0
MINOR_VER=1
PATCH_VER=0
PROJECT_NAME="seadragon"
try:
    with open("build/bump", "r") as f:
        BUMP_VER = int(f.read())
except:
    BUMP_VER=0
BUMP_VER += 1
try:
    with open("build/bump", "w") as f:
        f.write(str(BUMP_VER))
except:
    print("Failed to save tweak version")
VERSION=str(MAJOR_VER) + '.' + str(MINOR_VER) + '.' + str(PATCH_VER)

import glob
import os
import argparse
from pathlib import Path
from string import Template
import itertools
import subprocess
from tempfile import NamedTemporaryFile
import multiprocessing

# should be fine to just hardcode these, IMO
BUILD_DIR = 'build'
BIN_DIR = BUILD_DIR
OBJ_DIR = os.path.join(BUILD_DIR, 'obj')
EXE_EXT = '.exe' if os.name == 'nt' else ''
BIN_NAME = PROJECT_NAME + EXE_EXT

DEFAULT_FLAGS = {'-Wall', '-pedantic', '-D_POSIX_C_SOURCE=200809L' }
DEBUG_FLAGS = {'-g', '-Og', '-D_DEBUG'}
# pixelherodev's personal flag set :P I'm insane, I know.
PIXELS_DEVEL_FLAGS = { '-Werror', '-Wextra', '-Wno-error=reorder', '-Wno-error=pedantic', '-Wno-error=unused-parameter', '-Wno-error=missing-field-initializers', '-Wno-error=deprecated-declarations', '-pedantic', '-march=native', '-mtune=native', '-falign-functions=32' }
RELEASE_FLAGS = {'-O2'}

# these *do* need to be in order, so no sets!
# We don't need anything in BASE right now, but we might later.
BASE_LDFLAGS = [ ]
DEFAULT_LDFLAGS = ['-static', '-static-libgcc' ] + BASE_LDFLAGS

# parse arguments
parser = argparse.ArgumentParser(description='Builds ' + PROJECT_NAME)
parser.add_argument('--pixel', dest='cflags', action='append_const',
                    const=PIXELS_DEVEL_FLAGS,
                    help='Enable additional development flags because you\'re pixelherodev and you make bad life choices')
parser.add_argument('--release', dest='cflags_mode', action='store_const',
                    const=RELEASE_FLAGS, default=DEBUG_FLAGS,
                    help='Enable release build')
parser.add_argument('-B', dest='force_rebuild', action='store_true',
                    help='Force rebuilding the project')
parser.add_argument('-r', dest='dynamic', action='store_true',
                    help='Build a dynamic executable')
parser.add_argument('-j', dest='num_threads', nargs='?',
                    type=int, default=max(multiprocessing.cpu_count() // 2, 1),
                    help='Number of threads to run `make` with')

args = parser.parse_args()
if '-O2' not in args.cflags_mode:
    VERSION = VERSION + '-' + str(BUMP_VER)
CFLAGS = DEFAULT_FLAGS | {'-std=c99'} | args.cflags_mode | (set(itertools.chain(*args.cflags)) if args.cflags else set()) | { '-DPROJECT_VERSION=' + VERSION }
LDFLAGS = DEFAULT_LDFLAGS
if args.dynamic:
    LDFLAGS = BASE_LDFLAGS

class Target:
    all = {}

    def __init__(self, name, artifacts):
        self.name = name
        self.artifacts = set(artifacts)
        self.sources = {}   # source => object mapping
        self.headers = set()
        self.cflags = []
        self.includes = []
        self.dependencies = set()
        # target names must be unique
        assert name not in self.all
        self.all[name] = self

    def __enter__(self):
        return self
    def __exit__(self, exc_type, exc_value, traceback):
        pass

    @property
    def paths(self):
        return set(os.path.join(BIN_DIR, a) for a in self.artifacts)

    @staticmethod
    def obj_from_c(cname, skipn=0):
        return os.path.join(OBJ_DIR, *Path(cname).with_suffix('.o').parts[skipn:])

    def add_sources_glob(self, *paths, obj_skip=0):
        srcs = set(itertools.chain.from_iterable(glob.glob(path, recursive=True) for path in paths))
        objs = {src: self.obj_from_c(src, skipn=obj_skip) for src in srcs}
        self.sources.update(objs)

    def add_headers_glob(self, *paths):
        self.headers |= set(itertools.chain.from_iterable(glob.glob(path, recursive=True) for path in paths))

    def add_includes(self, *paths):
        for p in paths:
            if p not in self.includes:
                self.includes.append(p)

    def add_cflags(self, *flags):
        for flag in flags:
            if flag not in self.cflags:
                self.cflags.append(flag)

    def add_dependencies(self, *deps):
        self.dependencies |= {d.name if isinstance(d, Target) else d for d in deps}

    def create_obj_dirs(self):
        for dir in {os.path.dirname(obj) for obj in self.sources.values()}:
            os.makedirs(dir, exist_ok=True)

    def get_transitive_dependencies(self, tdeps=None):
        if tdeps is None:
            tdeps = {}
        for name in self.dependencies:
            if name in tdeps:
                continue
            dep = Target.all[name]
            tdeps[name] = dep
            dep.get_transitive_dependencies(tdeps)
        return tdeps

class SourceLibrary(Target):
    def __init__(self, name):
        super().__init__(name, set())

class Executable(Target):
    def __init__(self, name):
        super().__init__(name, {name + EXE_EXT})

with SourceLibrary(PROJECT_NAME) as ProjectLibrary:
    # For now, we'll add all headers in src/ recursively
    ProjectLibrary.add_headers_glob('src/**.h')
    # For now, we'll add all .c sources in src/ recursively
    ProjectLibrary.add_sources_glob('src/**.c')

with Executable('test') as Test:
    Test.add_sources_glob('test/main.c')
    Test.add_headers_glob('test/test.h')
    Test.add_dependencies(PROJECT_NAME)
    Test.add_includes('src/')

# create object directories
for tgt in Target.all.values():
    tgt.create_obj_dirs()

t = []
# emit heading
t.append('''\
# AUTOGENERATED FILE; DO NOT MODIFY (use `build.py` instead)
CFLAGS+={CFLAGS}
LDFLAGS+={LDFLAGS}
INCLUDES+={includes_all}

.PHONY: default all test
default: test
\t./{BIN_DIR}/test

all: {targets_all}

HEADERS={headers_all}
{OBJ_DIR}/%.o: %.c $(HEADERS)
\t$(CC) $< $(CFLAGS) $(EXTRA_CFLAGS) $(INCLUDES) -c -o $@
'''.format(
    CFLAGS=' '.join(sorted(CFLAGS)),
    LDFLAGS=' '.join(LDFLAGS),
    BIN_DIR=BIN_DIR,
    OBJ_DIR=OBJ_DIR,
    targets_all=' '.join(Target.all),
    # TODO: These should be per-target (but currently cannot be because of %.o: %.c rules)
    includes_all=' '.join(sorted({'-I' + inc for inc in itertools.chain.from_iterable(tgt.includes for tgt in Target.all.values())})),
    headers_all=' '.join(sorted(set(itertools.chain.from_iterable(tgt.headers for tgt in Target.all.values())))),
    PROJECT_NAME = PROJECT_NAME,
))

def emit_target(tgt, emitted=None):
    if emitted is None:
        emitted = set()
    if tgt.name in emitted:
        return
    emitted.add(tgt.name)

    dep_depends_objects = []
    dep_depends_headers = []
    # emit the target's own dependencies
    for dep in sorted(tgt.get_transitive_dependencies().values(), key=lambda v: v.name):
        emit_target(dep, emitted)
        dep_depends_objects.append('$({dep_name}_OBJECTS)'.format(dep_name=dep.name))
        dep_depends_headers.append('$({dep_name}_HEADERS)'.format(dep_name=dep.name))

    artifacts = sorted(tgt.paths)
    params = dict(
        t_name=tgt.name,
        t_artifacts=' '.join(artifacts),
        t_objects=' '.join(sorted(tgt.sources.values())),
        t_headers=' '.join(sorted(tgt.headers)),
        t_cflags = ' '.join(sorted(tgt.cflags)),
        dep_depends_objects=''.join(' ' + dd for dd in dep_depends_objects),
        dep_depends_headers=''.join(' ' + dd for dd in dep_depends_headers),
    )
    t.append('''\

### TARGET: {t_name}
'''.format(**params))
    if artifacts:
        t.append('''\

{t_name}: {t_artifacts}
'''.format(**params))
    t.append('''\

{t_name}_OBJECTS = {t_objects}
$({t_name}_OBJECTS): EXTRA_CFLAGS := {t_cflags}

{t_name}_HEADERS = {t_headers}

'''.format(**params))
    for a in artifacts:
        t.append('''\
{t_artifact}: $({t_name}_OBJECTS) {dep_depends_objects}
\t$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^
'''.format(**params, t_artifact=a))

emitted = set()
for t_name in sorted(Target.all):
    emit_target(Target.all[t_name], emitted)

t = ''.join(t)
#print('-' * 10)
#if '-Werror' in CFLAGS:
#    print("Makefile generated.")
#else:
#    print(t.rstrip())
#print('-' * 10)

with open('Makefile', 'w') as f:
    f.write(t)

extra = []
if args.force_rebuild:
    extra.append('-B')
if args.num_threads is not ...:
    extra.append('-j%u' % args.num_threads if args.num_threads else '-j')
return_code = subprocess.call(['make'] + extra + ['-f', f.name, '--no-print-directory'])
exit(return_code)

