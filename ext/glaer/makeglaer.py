#!/bin/env python
#
# GLAER
#
# GL API Entrypoint Retriever
#
# TODO support doc comment formats other than VS/xml
#
# @author Ben Allen
#

import sys

try:
	import bs4
except ImportError:
	sys.stderr.write('ERROR: Python module bs4 is required to make GLAER.\n')
	sys.stderr.write('Run "pip install [--user] beautifulsoup4" to satisfy.\n')
	sys.exit('makeglaer.py: Requirements not satisfied. Aborting.')
# }

import os, re, inspect
import argparse

# get script directory so we can find resources
thisdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

# main version number
version = '0.0.0'

# output files (default paths)
_out_h = './glaer.h'
_out_c = './glaer.c'

# generator name
_genname = 'Default'

# http://stackoverflow.com/questions/3853722/python-argparse-how-to-insert-newline-in-the-help-text
class SmartFormatter(argparse.HelpFormatter):
    def _split_lines(self, text, width):
        # this is the RawTextHelpFormatter._split_lines
        if text.startswith('R|'):
            return text[2:].splitlines()
		# }
        return argparse.HelpFormatter._split_lines(self, text, width)
	# }
# }

# setup arguments
_parser = argparse.ArgumentParser(formatter_class=SmartFormatter, description='''
Build script to generate the GLAER source files.
''')
_parser.add_argument('-oh', '--output-header', help='Output header file name. Default is "./glaer.h".', dest='outh')
_parser.add_argument('-oc', '--output-source', help='Output source file name. Default is "./glaer.c".', dest='outc')
_parser.add_argument('-g', '--generator', help='''R|Generator name. The following generators are available:

  Default        = Generate source code with no inline documentation.
  Visual Studio  = Generate source code with inline documentation in
                   Visual Studio XML format.

The default is "Default".
''', dest='gen')

# parse arguments
_args = _parser.parse_args()
_out_h = _args.outh if _args.outh else _out_h
_out_c = _args.outc if _args.outc else _out_c
_genname = _args.gen if _args.gen else _genname

print 'GLAER: Output header:', _out_h
print 'GLAER: Output source:', _out_c

class DefaultGenerator(object):
	name = 'Default'
	
	@classmethod
	def comment_command_summary(self, cmd):
		return ''
	# }
	
	@classmethod
	def comment_command(self, cmd):
		return ''
	# }
# }

class VisualStudioGenerator(object):
	name = 'Visual Studio'
	
	@classmethod
	def comment_paras(cls, paras):
		lines = []
		for para in paras:
			lines.append('/// <para>\n')
			for line in para.split('\n'):
				lines.append('/// ' + line + '\n')
			# }
			lines.append('/// </para>\n')
		# }
		return u''.join(lines)
	# }
	
	@classmethod
	def comment_command_summary(cls, cmd):
		paras = cmd.doc_desc
		# limit to first 2 paragraphs of description
		if len(paras) > 2: paras = [paras[0], paras[1]]
		
		# TODO doc source
		
		# notes
		if len(cmd.doc_notes) > 0:
			paras.append('<para>--- NOTES ---</para>')
			paras += cmd.doc_notes
		# }
		
		# errors
		# there seems be a limit on doc length that VS will show as a tooltip
		# adding errors causes that to be exceeded for some things
		#if len(cmd.doc_errors) > 0:
		#	paras.append('<para>--- ERRORS ---</para>')
		#	paras += cmd.doc_errors
		# }
		
		return u'\n/// <summary>\n{text}/// </summary>\n'.format(text=cls.comment_paras(paras)).encode('ascii', errors='ignore')
	# }
	
	@classmethod
	def comment_command_params(cls, cmd):
		parts = []
		for param in cmd.params:
			parts.append('/// <param name="{name}">\n{text}/// </param>\n'.format(name=param.name, text=cls.comment_paras(param.doc)))
		# }
		return u''.join(parts).encode('ascii', errors='ignore')
	# }
	
	@classmethod
	def comment_command(cls, cmd):
		return cls.comment_command_summary(cmd) + cls.comment_command_params(cmd)
	# }
# }

# find generator by name
_generators = { 'Default' : DefaultGenerator, 'Visual Studio' : VisualStudioGenerator }
_gen = _generators.get(_genname)
if not _gen:
	sys.stderr.write('WARNING: Unknown GLAER generator "{0}", using "Default".\n'.format(_genname))
	_gen = DefaultGenerator
# }
print 'GLAER: Using generator "{0}".'.format(_gen.name)

# delay this import because it takes a while
print 'GLAER: Loading OpenGL API specification...'
import glapi
print 'GLAER: OpenGL API specification loaded.'

def build_glaer_h():
	out = open(_out_h, 'w')
	
	# header guard and extern "C"
	out.write('''
#ifndef GLAER_H
#define GLAER_H

#ifdef __cplusplus
extern "C" {
#endif
''')
	
	# version numbers
	out.write('''
#define GLAER_VERSION_MAJOR {0}
#define GLAER_VERSION_MINOR {1}
#define GLAER_VERSION_PATCH {2}
'''.format(*version.split('.')))
	
	# manually authored code
	with open(thisdir + '/common/glaer.h') as file:
		for line in file:
			out.write(line)
		# }
	# }
	
	out.write('\n/*** GLAER: begin automatically generated code ***/\n')
	
	# defines for enums in GL namespace
	out.write('\n/* Defiens for enums in GL namespace */\n')
	out.write('#ifndef GLAER_NO_GL_ENUMS\n')
	for enum in glapi.enums.itervalues():
		out.write('#define {name} {value}\n'.format(name=enum.name, value=enum.value))
	# }
	out.write('#endif /* GLAER_NO_GL_ENUMS */\n')
	
	# typedefs for glaer_gl function pointers
	out.write('\n/* typedefs for glaer_gl function pointer types */\n')
	for cmd in glapi.commands.itervalues():
		out.write('typedef ' + cmd.format_proto('(APIENTRY *GlaerPFn_{name})'.format(name=cmd.name)) + '(')
		out.write(', '.join([param.format_proto() for param in cmd.params]))
		out.write(');\n')
	# }
	
	# context struct
	out.write('\n/* GLAER Context */\nstruct GlaerContext_ {\n')
	for cmd in glapi.commands.itervalues():
		out.write('\tGlaerPFn_{name} glaer_{name};\n'.format(name=cmd.name))
	# }
	out.write('}; /* struct GlaerContext_ */\n')
	
	# real functions in GLAER namespace
	out.write('\n/* Real functions in GLAER namespace */\n')
	for cmd in glapi.commands.itervalues():
		out.write(_gen.comment_command(cmd))
		out.write('GLAERAPI ' + cmd.format_proto('APIENTRY glaer_{name}'.format(name=cmd.name)) + '(')
		out.write(', '.join([param.format_proto() for param in cmd.params]))
		out.write(');\n')
	# }
	
	# defines for functions in GL namespace
	out.write('\n/* Defines for functions in GL namespace */\n')
	out.write('#ifndef GLAER_NO_GL_FUNCTIONS\n')
	for cmd in glapi.commands.itervalues():
		out.write(_gen.comment_command_summary(cmd))
		out.write('#define {name} glaer_{name}\n'.format(name=cmd.name))
	# }
	out.write('\n#endif /* GLAER_NO_GL_FUNCTIONS */\n')
	
	out.write('\n/*** GLAER: end automatically generated code ***/\n')
	
	# close extern "C" and header guard
	out.write('''
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* GLAER_H */
''')
	
	out.close()
# }

def build_glaer_c():
	out = open(_out_c, 'w')
	
	# manually authored code
	with open(thisdir + '/common/glaer.c') as file:
		for line in file:
			out.write(line)
		# }
	# }
	
	out.write('\n/*** GLAER: begin automatically generated code ***/\n')
	
	# glaerInitCurrentContext()
	out.write('''
GLboolean APIENTRY glaerInitCurrentContext() {
	GlaerContext *ctx;
	GLAER_GET_PROC_ADDRESS_DECL
	ctx = glaerGetCurrentContext();
	GLAER_GET_PROC_ADDRESS_INIT
''')
	for cmd in glapi.commands.itervalues():
		out.write('\tctx->glaer_{name} = (GlaerPFn_{name}) glaerGetProcAddress("{name}");\n'.format(name=cmd.name))
	# }
	out.write('\treturn 1;\n}\n')
	
	# glaer_gl function definitions
	out.write('\n/* glaer_gl function definitions */\n')
	for cmd in glapi.commands.itervalues():
		out.write(cmd.format_proto('glaer_{name}'.format(name=cmd.name)) + '(')
		out.write(', '.join([param.format_proto() for param in cmd.params]))
		# function body depends on whether function returns void or not
		if cmd.format_proto('').strip() == 'void':
			out.write(') {\n\t')
		else:
			out.write(') {\n\treturn ')
		# }
		out.write('glaerGetCurrentContext()->glaer_{name}('.format(name=cmd.name))
		out.write(', '.join([param.name for param in cmd.params]))
		out.write(');\n}\n')
	# }
	
	out.write('\n/*** GLAER: end automatically generated code ***/\n')
	
	out.close()
# }

def main():
	print 'GLAER: Generating header...'
	build_glaer_h()
	print 'GLAER: Generating source...'
	build_glaer_c()
	print 'GLAER: Generation finished.'
# }

if __name__ == '__main__':
	main()
# }






































