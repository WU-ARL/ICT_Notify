# -*- Mode: python; py-indent-offset: 4; indent-tabs-mode: nil; coding: utf-8; -*-

VERSION = '0.1.0'
APPNAME = 'NotificationLib'
GIT_TAG_PREFIX = 'NotificationLib-'

from waflib import Logs, Utils, Context
import os

def options(opt):
    opt.load(['compiler_c', 'compiler_cxx', 'gnu_dirs'])
    opt.load(['default-compiler-flags', 'boost',
              'coverage', 'sanitizers', 'pch'],
             tooldir=['.waf-tools'])

    opt.add_option('--with-tests', action='store_true', default=False,
                   dest='with_tests', help='''Build unit tests''')

def configure(conf):
    conf.load(['compiler_c', 'compiler_cxx', 'gnu_dirs',
               'default-compiler-flags', 'boost', 'pch', 'coverage'])

    if 'PKG_CONFIG_PATH' not in os.environ:
        os.environ['PKG_CONFIG_PATH'] = Utils.subst_vars('${LIBDIR}/pkgconfig', conf.env)
    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    boost_libs = 'system iostreams thread log log_setup'
    conf.check_boost(lib=boost_libs, mt=True)

    conf.check_compiler_flags()

    # Loading "late" to prevent tests from being compiled with profiling flags
    conf.load('coverage')

    conf.load('sanitizers')

    # If there happens to be a static library, waf will put the corresponding -L flags
    # before dynamic library flags.  This can result in compilation failure when the
    # system has a different version of the NotificationLib library installed.
    conf.env['STLIBPATH'] = ['.'] + conf.env['STLIBPATH']

    conf.write_config_header('config.hpp')

def build(bld):
    libnotification = bld(
        target="NotificationLib",
        vnum = VERSION,
        cnum = VERSION,
        features=['cxx', 'cxxshlib'],
        source =  bld.path.ant_glob(['src/**/*.cpp', 'src/**/*.proto']),
        use = 'BOOST NDN_CXX',
        includes = ['src', '.'],
        export_includes=['src', '.'],
        )

    bld.install_files(
        dest = "%s/NotificationLib" % bld.env['INCLUDEDIR'],
        files = bld.path.ant_glob(['src/**/*.hpp', 'src/**/*.h', 'common.hpp']),
        cwd = bld.path.find_dir("src"),
        relative_trick = False,
        )

    bld.install_files(
        dest = "%s/NotificationLib" % bld.env['INCLUDEDIR'],
        files = bld.path.get_bld().ant_glob(['src/**/*.hpp', 'src/**/*.h', 'common.hpp', 'config.hpp']),
        cwd = bld.path.get_bld().find_dir("src"),
        relative_trick = False,
        )

    pc = bld(
        features = "subst",
        source='NotificationLib.pc.in',
        target='NotificationLib.pc',
        install_path = '${LIBDIR}/pkgconfig',
        PREFIX       = bld.env['PREFIX'],
        INCLUDEDIR   = "%s/NotificationLib" % bld.env['INCLUDEDIR'],
        VERSION      = VERSION,
        )

def version(ctx):
    if getattr(Context.g_module, 'VERSION_BASE', None):
        return

    Context.g_module.VERSION_BASE = Context.g_module.VERSION
    Context.g_module.VERSION_SPLIT = [v for v in VERSION_BASE.split('.')]

    try:
        cmd = ['git', 'describe', '--match', '%s*' % GIT_TAG_PREFIX]
        p = Utils.subprocess.Popen(cmd, stdout=Utils.subprocess.PIPE,
                                   stderr=None, stdin=None)
        out = p.communicate()[0].strip()
        if p.returncode == 0 and out != "":
            Context.g_module.VERSION = out[11:]
    except:
        pass

def dist(ctx):
    version(ctx)

def distcheck(ctx):
    version(ctx)
