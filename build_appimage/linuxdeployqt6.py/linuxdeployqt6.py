#! /usr/bin/env python3
from collections import namedtuple
import argparse
import functools
import glob
import inspect
import json
import os
import re
import shutil
import signal
import subprocess
import sys

signal.signal(signal.SIGINT,  signal.SIG_DFL)

OperationMode = namedtuple(
    'OperationMode', 'force dry_run verbosity')

Dirs = namedtuple(
    'Dirs', 'exe_dir lib_dir plugins_dir qml_dir data_dir translations_dir')

Executable = namedtuple('SharedLib', 'name path')
SharedLib = namedtuple('SharedLib', 'name path')
QtModule = namedtuple('QtModule', 'name lib')
QtPlugin = namedtuple('QtPlugin', 'name path')
QtTranslation = namedtuple('QtTranslation', 'name path')
QmlModule = namedtuple('QmlModule', 'name path relative_path lib')

def memoize(function):
    cache = {}

    @functools.wraps(function)
    def wrapper(*args):
        if args not in cache:
            result = function(*args)
            if inspect.isgenerator(result):
                result = list(result)
            cache[args] = result
        return cache[args]

    return wrapper

def log_fatal(msg):
    print('ERROR: '+msg, file=sys.stderr)
    exit(1)

def log_normal(msg):
    global op_mode
    if op_mode.verbosity >= 1:
        print(msg, file=sys.stderr)

def log_verbose(msg):
    global op_mode
    if op_mode.verbosity >= 2:
        print(msg, file=sys.stderr)

@memoize
def resolve_libs(executable):
    if not shutil.which('ldd'):
        log_fatal("cannot find 'ldd' tool in PATH")

    output = subprocess.check_output(['ldd', '-r', executable])
    output = output.decode()
    output = output.split('\n')

    for line in output:
        m = re.match(r'^\s*(\S+\.so[.0-9]*)\s*=>\s*(/\S+)', line)
        if m:
            lib_name = m.group(1)
            lib_path = m.group(2)

            if os.path.isfile(lib_path):
                yield SharedLib(lib_name, os.path.realpath(lib_path))

def find_libs(*path_list):
    unique_path_list = set()

    for path in path_list:
        if not os.path.realpath(path) in [os.path.realpath(p) for p in unique_path_list]:
            unique_path_list.add(path)

    for path in unique_path_list:
        for root, dirs, files in os.walk(path):
            for file_name in files:
                if parse_lib_name(file_name):
                    yield SharedLib(file_name, os.path.join(root, file_name))

def parse_lib_name(file_name):
    m = re.match(r'^lib(\S+)\.so[.0-9]*$', file_name)
    if m:
        return m.group(1)

def format_lib_name(lib_name):
    return 'lib' + lib_name + '.so'

def set_runpath(executable, rpath):
    if not os.path.isabs(rpath):
        if rpath == '.':
            rpath = '$ORIGIN'
        else:
            rpath = os.path.join('$ORIGIN', rpath)

    if not shutil.which('patchelf'):
        log_fatal("cannot find 'patchelf' tool in PATH")

    subprocess.run([
        'patchelf',
        '--set-rpath', rpath,
        executable,
        ],
        check=True, stdout=subprocess.DEVNULL)

@memoize
def is_qt_lib(qtdir, lib):
    lib_path = os.path.realpath(lib.path)
    base_path = os.path.realpath(os.path.join(qtdir, 'lib'))

    if not parse_lib_name(os.path.basename(lib_path)):
        return False

    if not lib_path.startswith(base_path+os.sep):
        return False

    return True

def is_webengine_module(module):
    return 'WebEngine' in module.name

@memoize
def avail_qt_modules(qtdir):
    modules_dir = os.path.join(qtdir, 'mkspecs', 'modules')
    module_map = {}

    for pri_file in glob.glob(os.path.join(modules_dir, '*.pri')):
        with open(pri_file) as fp:
            module = {}

            for line in fp.readlines():
                m = re.match(r'^\s*QT\.[a-zA-Z]+\.(\S+)\s*=\s*(.*)', line)
                if m:
                    module[m.group(1)] = m.group(2)

            if 'module' in module:
                module_map[module['module']] = module

    return module_map

@memoize
def avail_qt_langs(qtdir):
    trans_dir = os.path.join(qtdir, 'translations')

    for tr_file in glob.glob(os.path.join(trans_dir, 'qtbase_*.qm')):
        m = re.match(r'^qtbase_(\S+).qm$', os.path.basename(tr_file))
        if m:
            yield m.group(1)

@memoize
def avail_qt_translations():
    return {
        'Qt6Concurrent': 'qtbase',
        'Qt6Core': 'qtbase',
        'Qt6Declarative': 'qtquick1',
        'Qt6Gui': 'qtbase',
        'Qt6Help': 'qt_help',
        'Qt6Multimedia': 'qtmultimedia',
        'Qt6MultimediaWidgets': 'qtmultimedia',
        'Qt6MultimediaQuick': 'qtmultimedia',
        'Qt6Network': 'qtbase',
        'Qt6Qml': 'qtdeclarative',
        'Qt6Quick': 'qtdeclarative',
        'Qt6Script': 'qtscript',
        'Qt6ScriptTools': 'qtscript',
        'Qt6SerialPort': 'qtserialport',
        'Qt6Sql': 'qtbase',
        'Qt6Test': 'qtbase',
        'Qt6Widgets': 'qtbase',
        'Qt6Xml': 'qtbase',
        'Qt6WebEngine': 'qtwebengine',
    }

@memoize
def find_qt_modules(qtdir, executable):
    for lib in resolve_libs(executable):
        if is_qt_lib(qtdir, lib):
            module_name = parse_lib_name(lib.name)
            if module_name:
                yield QtModule(module_name, lib)

@memoize
def find_qt_module_executables(qtdir, module):
    if is_webengine_module(module):
        yield Executable(
            'QtWebEngineProcess',
            os.path.join(qtdir, 'libexec', 'QtWebEngineProcess'))

@memoize
def find_qt_module_libs(qtdir, module):
    yield module.lib

    for lib in resolve_libs(module.lib.path):
        if is_qt_lib(qtdir, lib):
            yield lib

@memoize
def find_qt_module_translations(qtdir, module):
    lang_list = avail_qt_langs(qtdir)
    tr_map = avail_qt_translations()

    if module.name in tr_map:
        for lang_name in lang_list:
            tr_file = os.path.join(qtdir, 'translations', '%s_%s.qm' % (
                tr_map[module.name], lang_name))

            if os.path.isfile(tr_file):
                yield QtTranslation(os.path.basename(tr_file), tr_file)

    if is_webengine_module(module):
        yield QtTranslation(
            'qtwebengine_locales',
            os.path.join(qtdir, 'translations', 'qtwebengine_locales'))

@memoize
def find_qt_module_plugins(qtdir, module):
    module_props = avail_qt_modules(qtdir)

    if module.name in module_props:
        if 'plugin_types' in module_props[module.name]:
            for plugin_name in module_props[module.name]['plugin_types'].split():
                plugin_path = os.path.join(qtdir, 'plugins', plugin_name)

                if os.path.isdir(plugin_path):
                    yield QtPlugin(plugin_name, plugin_path)

@memoize
def find_qt_plugin_libs(qtdir, plugin):
    libs = set()

    for plugin_lib in find_libs(plugin.path):
        for lib in resolve_libs(plugin_lib.path):
            if is_qt_lib(qtdir, lib):
                libs.add(lib)

    return libs

@memoize
def find_qml_modules(qtdir, qmldir):
    proc = subprocess.run(
        [os.path.join(qtdir, 'libexec', 'qmlimportscanner'),
         '-importPath', os.path.join(qtdir, 'qml'),
         '-rootPath', qmldir],
        check=True, text=True, stdout=subprocess.PIPE, stderr=subprocess.DEVNULL)

    js = json.loads(proc.stdout)

    for qml_import in js:
        if qml_import['type'] != 'module':
            continue

        if not 'path' in qml_import:
            continue

        lib = None
        if 'plugin' in qml_import:
            lib_name = format_lib_name(qml_import['plugin'])
            lib_path = os.path.join(qml_import['path'], lib_name)

            lib = SharedLib(lib_name, lib_path)

        relative_path = os.path.relpath(
            qml_import['path'],
            os.path.join(qtdir, 'qml'))

        yield QmlModule(
            qml_import['name'], qml_import['path'], relative_path, lib)

@memoize
def find_qml_module_libs(qtdir, qml_module):
    if qml_module.lib:
        for lib in resolve_libs(qml_module.lib.path):
            if is_qt_lib(qtdir, lib):
                yield lib

def copy_directory(src, dst, ignore=None):
    global op_mode
    if op_mode.dry_run:
        return
    if not op_mode.force:
        if os.path.exists(dst):
            log_fatal("cannot overwrite without -force: %s" % dst)

    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.isfile(dst):
        os.unlink(dst)
    elif os.path.isdir(dst):
        shutil.rmtree(dst, ignore_errors=True)

    shutil.copytree(src, dst, ignore=ignore)

def copy_file(src, dst):
    global op_mode
    if op_mode.dry_run:
        return
    if not op_mode.force:
        if os.path.exists(dst):
            log_fatal("cannot overwrite without -force: %s" % dst)

    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.isfile(dst):
        os.unlink(dst)
    elif os.path.isdir(dst):
        shutil.rmtree(dst, ignore_errors=True)

    shutil.copy2(src, dst)

def write_file(dst, content):
    global op_mode
    if op_mode.dry_run:
        return
    if not op_mode.force:
        if os.path.exists(dst):
            log_fatal("cannot overwrite without -force: %s" % dst)

    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if os.path.isfile(dst):
        os.unlink(dst)
    elif os.path.isdir(dst):
        shutil.rmtree(dst, ignore_errors=True)

    with open(dst, 'w') as fp:
        fp.write(content)

def deploy_exe(dirs, exe):
    inst_path = os.path.join(dirs.exe_dir, exe.name)
    if os.path.realpath(exe.path) == os.path.realpath(inst_path):
        return

    log_verbose('Deploying executable %s' % inst_path)
    copy_file(exe.path, inst_path)

def deploy_lib(dirs, lib):
    inst_path = os.path.join(dirs.lib_dir, lib.name)

    log_verbose('Deploying library %s' % inst_path)
    copy_file(lib.path, inst_path)

def deploy_qt_translation(dirs, tr):
    inst_path = os.path.join(dirs.translations_dir, tr.name)

    log_verbose('Deploying translation data %s' % inst_path)

    if os.path.isdir(tr.path):
        copy_directory(tr.path, inst_path)
    else:
        copy_file(tr.path, inst_path)

def deploy_qt_plugin(dirs, plugin):
    inst_path = os.path.join(dirs.plugins_dir, plugin.name)

    log_verbose('Deploying plugin %s' % inst_path)
    copy_directory(
        plugin.path, inst_path, shutil.ignore_patterns('*.debug'))

def deploy_qml_module(dirs, qml_module):
    inst_path = os.path.join(dirs.qml_dir, qml_module.relative_path)

    log_verbose('Deploying qml module %s' % inst_path)
    copy_directory(
        qml_module.path, inst_path, shutil.ignore_patterns('*.debug'))

def deploy_resources(dirs, qtdir):
    src_path = os.path.join(qtdir, 'resources')
    inst_path = os.path.join(dirs.data_dir, 'resources')

    if os.path.exists(src_path):
        log_verbose('Deploying data directory %s' % inst_path)
        copy_directory(src_path, inst_path)

def deploy_qt_conf(dirs):
    inst_path = os.path.join(dirs.exe_dir, 'qt.conf')

    log_verbose('Deploying configuration file %s' % inst_path)
    write_file(inst_path, '''
[Paths]
Plugins = {plugins_dir}
Imports = {qml_dir}
Qml2Imports = {qml_dir}
Data = {data_dir}
Translations = {translations_dir}
'''.lstrip().format(
    plugins_dir=os.path.relpath(dirs.plugins_dir, dirs.exe_dir),
    qml_dir=os.path.relpath(dirs.qml_dir, dirs.exe_dir),
    data_dir=os.path.relpath(dirs.data_dir, dirs.exe_dir),
    translations_dir=os.path.relpath(dirs.translations_dir, dirs.exe_dir)))

def update_deployed_runpath(dirs, file_path):
    log_verbose('Updating run path of %s' % file_path)

    global op_mode
    if op_mode.dry_run:
        return

    set_runpath(
        file_path,
        os.path.relpath(dirs.lib_dir, os.path.dirname(file_path)))

parser = argparse.ArgumentParser(
    description="Unofficial tool to make Linux Qt6 applications self-contained.",
    add_help=False)

op_mode_group = parser.add_argument_group("operation mode")

op_mode_group.add_argument("-h", "-help", action="store_true",
                    help="Print help message and exit")
op_mode_group.add_argument("-f", "-force", action="store_true",
                    help="Force overwriting existing files")
op_mode_group.add_argument("-n", "-dry-run", action="store_true",
                    help="Print what is going to be deployed, but don't deploy anything")
op_mode_group.add_argument("-v", "-verbose", metavar="level", type=int, default=1,
                    help="Verbosity level")

qt_opts_group = parser.add_argument_group("qt options")

qt_opts_group.add_argument("-qtdir", metavar="path", required=True,
                    help="Qt installation directory (e.g. /opt/Qt/6.2.4/gcc_64)")

input_opts_group = parser.add_argument_group("input options")

input_opts_group.add_argument("executable", nargs='+',
                    help="Input executable")

input_opts_group.add_argument("-qmlscandir", metavar="path", action="append",
                    help="Input directory to scan for qml imports")

output_opts_group = parser.add_argument_group("output options")

output_opts_group.add_argument("-out-dir", metavar="path",
                    help="Output directory (by default directory of first executable)")
output_opts_group.add_argument("-out-exe-dir", metavar="path",
                    help="Output directory for executable (by default same as -out-dir)")
output_opts_group.add_argument("-out-lib-dir", metavar="path",
                    help="Output directory for libraries (by default same as -out-dir)")
output_opts_group.add_argument("-out-plugins-dir", metavar="path",
                    help="Output directory for plugins (by default same as -out-dir)")
output_opts_group.add_argument("-out-qml-dir", metavar="path",
                    help="Output directory for qml modules (by default same as -out-dir)")
output_opts_group.add_argument("-out-data-dir", metavar="path",
                    help="Output directory for data files (by default same as -out-dir)")
output_opts_group.add_argument("-out-translations-dir", metavar="path",
                    help="Output directory for translations"+
                    " (by default 'translations' inside -out-dir)")

deploy_opts_group = parser.add_argument_group("deployment options")

deploy_opts_group.add_argument("-no-conf", action="store_true",
                    help="Skip qt.conf deployment")
deploy_opts_group.add_argument("-no-exe", action="store_true",
                    help="Skip executable deployment")
deploy_opts_group.add_argument("-no-lib", action="store_true",
                    help="Skip libraries deployment")
deploy_opts_group.add_argument("-no-plugins", action="store_true",
                    help="Skip plugins deployment")
deploy_opts_group.add_argument("-no-qml", action="store_true",
                    help="Skip qml modules deployment")
deploy_opts_group.add_argument("-no-data", action="store_true",
                    help="Skip data files deployment")
deploy_opts_group.add_argument("-no-translations", action="store_true",
                    help="Skip translations deployment")

need_help = '-h' in sys.argv or '-help' in sys.argv

if not need_help:
    args = parser.parse_args()
    need_help = args.h

if need_help:
    parser.print_help()
    exit(0)

op_mode = OperationMode(
    force = args.f,
    dry_run = args.n,
    verbosity = args.v)

default_dir = args.out_dir or os.path.abspath(os.path.dirname(args.executable[0]))

dirs = Dirs(
    exe_dir = args.out_exe_dir or default_dir,
    lib_dir = args.out_lib_dir or default_dir,
    plugins_dir = args.out_plugins_dir or default_dir,
    qml_dir = args.out_qml_dir or default_dir,
    data_dir = args.out_data_dir or default_dir,
    translations_dir = args.out_translations_dir or os.path.join(default_dir, 'translations'))

all_qt_modules = set()
all_qt_translations = set()
all_qt_plugins = set()
all_qml_modules = set()

all_libs = set()
all_executables = set()

for executable in args.executable:
    log_normal('Scanning dependencies of %s ...' % executable)

    all_executables.add(
        Executable(os.path.basename(executable), executable))

    for qt_module in find_qt_modules(args.qtdir, executable):
        all_qt_modules.add(qt_module)

        for exe in find_qt_module_executables(args.qtdir, qt_module):
            all_executables.add(exe)

        for lib in find_qt_module_libs(args.qtdir, qt_module):
            all_libs.add(lib)

        for tr in find_qt_module_translations(args.qtdir, qt_module):
            all_qt_translations.add(tr)

        for qt_plugin in find_qt_module_plugins(args.qtdir, qt_module):
            if not qt_plugin in all_qt_plugins:
                all_qt_plugins.add(qt_plugin)

                for lib in find_qt_plugin_libs(args.qtdir, qt_plugin):
                    all_libs.add(lib)

for qmlscandir in (args.qmlscandir or []):
    log_normal('Scanning qml imports of %s ...' % qmlscandir)

    for qml_module in find_qml_modules(args.qtdir, qmlscandir):
        all_qml_modules.add(qml_module)

        for lib in find_qml_module_libs(args.qtdir, qml_module):
            all_libs.add(lib)

log_normal('Deploying files ...')

if not args.no_conf:
    deploy_qt_conf(dirs)

if not args.no_exe:
    for exe in sorted(all_executables):
        deploy_exe(dirs, exe)

if not args.no_lib:
    for lib in sorted(all_libs):
        deploy_lib(dirs, lib)

if not args.no_plugins:
    for plugin in sorted(all_qt_plugins):
        deploy_qt_plugin(dirs, plugin)

if not args.no_qml:
    for qml_module in sorted(all_qml_modules):
        deploy_qml_module(dirs, qml_module)

if not args.no_data:
    deploy_resources(dirs, args.qtdir)

if not args.no_translations:
    for tr in sorted(all_qt_translations):
        deploy_qt_translation(dirs, tr)

if not args.no_exe:
    log_normal('Updating executables run paths ...')

    for exe in sorted(all_executables):
        update_deployed_runpath(
            dirs,
            os.path.join(dirs.exe_dir, exe.name))

if not args.no_lib or not args.no_plugins or not ars.no_qml:
    log_normal('Updating libraries run paths ...')

    for lib in sorted(find_libs(dirs.lib_dir, dirs.plugins_dir, dirs.qml_dir)):
        update_deployed_runpath(
            dirs,
            lib.path)

log_normal('Deployment succeeded.')
