import Options
from os import unlink, symlink, popen
from os.path import exists 

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
  opt.tool_options("compiler_cxx")

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  conf.check(lib='png', libpath=['/lib', '/usr/lib', '/usr/local/lib', '/usr/local/libpng/lib', '/usr/local/pkg/libpng-1.4.1/lib'])

def build(bld):
  obj = bld.new_task_gen("cxx", "shlib", "node_addon")
  obj.target = "png"
  obj.source = "src/common.cpp src/png_encoder.cpp src/png.cpp src/fixed_png_stack.cpp src/dynamic_png_stack.cpp src/module.cpp src/buffer_compat.cpp"
  obj.uselib = "PNG"
  obj.cxxflags = ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"]

def shutdown():
  if Options.commands['clean']:
    if exists('png.node'): unlink('png.node')
  else:
    if exists('build/default/png.node') and not exists('png.node'):
      symlink('build/default/png.node', 'png.node')

