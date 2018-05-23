project(
  'dazzdb',
  'c',
  version : '0.0.1',
  default_options : [
    'buildtype=release',
    'warning_level=3',
    'c_std=gnu11',
    'b_ndebug=false'],
  license : 'BSD-3',
  meson_version : '>= 0.46.0')

############
# CXXFLAGS #
############

dazzdb_flags = []
cc = meson.get_compiler('c')
foreach cflag: [
  '-Wno-unused-result',
  '-Wno-gnu-statement-expression',
  '-fno-strict-aliasing']
    if cc.has_argument(cflag)
      dazzdb_flags += cflag
    endif
endforeach

################
# dependencies #
################

# libm
dazzdb_m_dep = cc.find_library('m', required : false)

dazzdb_deps = [dazzdb_m_dep]

##########
# dazzdb #
##########

# install library if
# - either running as a proper project
# - or using shared libraries
dazzdb_lib_install = (not meson.is_subproject()) or (get_option('default_library') == 'shared')

dazzdb_libdazzdb = library(
  'dazzdb',
  files([
    'DB.c',
    'QV.c']),
  # use boost SONAME practice:
  #   cause ld startup issues before
  #   you even have the chance of running
  #   into ABI issues.
  soversion : meson.project_version(),
  version : meson.project_version(),
  install : dazzdb_lib_install,
  dependencies : dazzdb_deps,
  c_args : dazzdb_flags)

############
# binaries #
############

dazzdb_exes = ['fasta2DB', 'DB2fasta', 'DBsplit', 'DBdust', 'Catrack', 'DBshow',
  'DBstats', 'DBrm', 'simulator', 'fasta2DAM', 'DAM2fasta', 'DBdump', 'rangen']

foreach i : dazzdb_exes
  executable(
    i,
    files([i + '.c']),
    dependencies : dazzdb_deps,
    install : (not meson.is_subproject()),
    c_args : dazzdb_flags,
    link_with : dazzdb_libdazzdb)
endforeach

###################
# dependency info #
###################

if not meson.is_subproject()
  import('pkgconfig').generate(
    libraries : dazzdb_libdazzdb,
    version : meson.project_version(),
    name : 'dazzdb',
    filebase : 'dazzdb',
    subdirs : 'dazzdb',
    description : 'DAZZDB library')

  install_headers(
    files([
      'DB.h',
      'QV.h']),
    subdir : 'dazzdb')
endif

dazzdb_dep = declare_dependency(
  include_directories : include_directories('.'),
  link_with : dazzdb_libdazzdb,
  version : meson.project_version())
