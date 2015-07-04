#
# API for querying the OpenGL API specification / documentation
#
# Get api specification (required):
# svn co --username anonymous --password anonymous https://cvs.khronos.org/svn/repos/ogl/trunk/doc/registry/public/api/ api
#
# Get api documentation:
# ./getdocs.py
#
# Input files are expected to use unix line breaks (LF only).
#
# @author Ben Allen
#

import bs4
import os, re, inspect

# get script directory so we can find resources
thisdir = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

class API(object):
	def __init__(self, name):
		self.name = name
		# name -> APIVersion
		self.versions = dict()
		# name -> Extension
		self.extensions = dict()
		# name -> Enum (all versions)
		self.enums = dict()
		# name -> Command (all versions)
		self.commands = dict()
	# }
# }

class APIVersion(object):
	def __init__(self, api, name, number):
		self.api = api
		self.name = name
		# this is still a string
		self.number = number
		# name -> Enum
		self.enums = dict()
		# name -> Command
		self.commands = dict()
	# }
# }

class Extension(object):
	def __init__(self, name):
		self.name = name
		# name -> API
		self.apis = dict()
		# name -> Enum
		self.enums = dict()
		# name -> Command
		self.commands = dict()
	# }
# }

class Enum(object):
	def __init__(self, name, value):
		self.name = name
		self.value = value
		# API -> APIVersion
		self.apiver = dict()
		# name -> Extension
		self.extensions = dict()
	# }
# }

class Param(object):
	def __init__(self, name, index, proto):
		self.name = name
		self.index = index
		# format string for prototype
		self._proto = proto
		self.doc = []
	# }
	
	def format_proto(self, name=None):
		name = self.name if name == None else name
		return self._proto.format(name=name)
	# }
# }

class Command(object):
	def __init__(self, name, params, proto):
		self.name = name
		self.params = params
		# format string for prototype
		self._proto = proto
		# API -> APIVersion
		self.apiver = dict()
		# name -> Extension
		self.extensions = dict()
		self.doc_desc = []
		self.doc_notes = []
		self.doc_errors = []
	# }
	
	def find_param(self, pname):
		return ([p for p in self.params if p.name == pname] + [None])[0]
	# }
	
	def format_proto(self, name=None):
		name = self.name if name == None else name
		return self._proto.format(name=name)
	# }
# }

# name -> API
apis = dict()

# name -> APIVersion (all apis)
versions = dict()

# name -> Extension (all apis)
extensions = dict()

# name -> Enum (all apis, all versions)
enums = dict()

# name -> Command (all apis, all versions)
commands = dict()

# parse the api specification
_apisoup = bs4.BeautifulSoup(open(thisdir + '/api/gl.xml'), features='xml')

def _stripdocstr(s):
	return re.sub('\n\s+', '\n', s.strip())
# }

# enums
for enum_tags in _apisoup.registry.find_all('enums'):
	for enum_tag in enum_tags.find_all('enum'):
		name = unicode(enum_tag['name'])
		value = unicode(enum_tag['value'])
		enums[name] = Enum(name, value)
	# }
# }

# commands
for command_tag in _apisoup.registry.commands.find_all('command'):
	name = unicode(command_tag.proto.find('name').string).strip()
	# turn the command prototype into a format string
	command_tag.proto.find('name').string = ' {name} '
	# parameters
	params = []
	for (i, ptag) in enumerate(command_tag.find_all('param')):
		pname = unicode(ptag.find('name').string).strip()
		# turn the param prototype into a format string
		ptag.find('name').string = ' {name} '
		params.append(Param(pname, i, unicode(ptag.get_text()).strip()))
	# }
	commands[name] = Command(name, params, unicode(command_tag.proto.get_text()).strip())
# }

# api versions
# TODO assumption: commands and enums only appear in feature tags once
for feature_tag in _apisoup.registry.find_all('feature'):
	# get or create api object
	apiname = unicode(feature_tag['api'])
	api = apis.get(apiname, API(apiname))
	apis[apiname] = api
	# get or create apiversion object, add to api
	vername = unicode(feature_tag['name'])
	ver = versions.get(vername, APIVersion(api, vername, unicode(feature_tag['number'])))
	versions[vername] = ver
	api.versions[vername] = ver
	# add enums and commands to apiversion, set apiversion on enums and commands
	for require_tag in feature_tag.find_all('require'):
		for enum_tag in require_tag.find_all('enum'):
			enum = enums[enum_tag['name'].strip()]
			enum.apiver[api] = ver
			api.enums[enum.name] = enum
			ver.enums[enum.name] = enum
		# }
		for command_tag in require_tag.find_all('command'):
			command = commands[command_tag['name'].strip()]
			command.apiver[api] = ver
			api.commands[command.name] = command
			ver.commands[command.name] = command
		# }
	# }
# }

# extensions
for extension_tag in _apisoup.registry.extensions.find_all('extension'):
	# create extension object
	ext = Extension(unicode(extension_tag['name']))
	extensions[ext.name] = ext
	# supported apis
	for apiname in [name.strip() for name in extension_tag['supported'].split('|')]:
		api = apis.get(apiname)
		# many extensions mention 'glcore' in the supported string
		if not api: continue
		ext.apis[api.name] = api
		api.extensions[ext.name] = ext
	# }
	# add enums and commands to extension
	for require_tag in extension_tag.find_all('require'):
		for enum_tag in require_tag.find_all('enum'):
			enum = enums[enum_tag['name'].strip()]
			enum.extensions[ext.name] = ext
			ext.enums[enum.name] = enum
		# }
		for command_tag in require_tag.find_all('command'):
			command = commands[command_tag['name'].strip()]
			command.extensions[ext.name] = ext
			ext.commands[command.name] = command
		# }
	# }
# }

# documentation
for man in ['man2', 'man3', 'man4']:
	if os.path.isdir(thisdir + '/docs/' + man):
		for filename in os.listdir(thisdir + '/docs/' + man):
			if not filename.endswith('.xml'): continue
			with open(thisdir + '/docs/{man}/{name}'.format(man=man, name=filename)) as file:
				soup = bs4.BeautifulSoup(file, features='xml')
				
				try:
					
					# commands this doc page applies to
					# refnamediv is unfortunately not always usable for this
					doccmds = []
					# There are doc files for GLU and GLX commands (which are not part of GL itself),
					# GLSL functions, and some other things; we have to make sure they don't break anything.
					if soup.refentry and soup.refentry.refsynopsisdiv:
						for synoptag in soup.refentry.refsynopsisdiv.find_all('funcsynopsis'):
							if synoptag:
								# func prototypes according to doc page
								for prototag in synoptag.find_all('funcprototype'):
									cmd = commands.get(prototag.funcdef.function.string.strip())
									if cmd:
										doccmds.append(cmd)
										# re-write param names according to doc page;
										# these sometimes differ from the param names in gl.xml.
										# we _CANNOT_ re-write the entire prototype because the doc pages
										# contain mistakes like misspelt typenames.
										for (i, ptag) in enumerate(prototag.find_all('paramdef')):
											# functions of no args show up with one arg with def 'void'
											# 'void' may or may not be inside a parameter tag, which may not exist
											if ptag.get_text().strip() != 'void':
												# glTextureParameterfv has a stray '.' on a param name and a stray newline too
												cmd.params[i].name = unicode(ptag.parameter.string).replace('\n', ' ').strip(' .')
											# }
										# }
									# }
								# }
							# }
						# }
					# }
					
					# no relevant GL commands -> do nothing
					if len(doccmds) == 0: continue
					
					# doc section tags
					# both 'id' (man2, man3) and 'xml:id' (man4) are used
					params_tag = soup.refentry.find(lambda tag: tag.name == 'refsect1' and 'parameters' in [tag.get('xml:id'), tag.get('id')])
					desc_tag = soup.refentry.find(lambda tag: tag.name == 'refsect1' and 'description' in [tag.get('xml:id'), tag.get('id')])
					notes_tag = soup.refentry.find(lambda tag: tag.name == 'refsect1' and 'notes' in [tag.get('xml:id'), tag.get('id')])
					errors_tag = soup.refentry.find(lambda tag: tag.name == 'refsect1' and 'errors' in [tag.get('xml:id'), tag.get('id')])
					
					# command doc
					doc_desc = [_stripdocstr(tag.get_text()) for tag in desc_tag.find_all('para')]
					doc_notes = [_stripdocstr(tag.get_text()) for tag in notes_tag.find_all('para')] if notes_tag else []
					doc_errors = [_stripdocstr(tag.get_text()) for tag in errors_tag.find_all('para')] if errors_tag else []
					
					# parameter doc
					param_doc = dict()
					# some commands have no parameters
					if params_tag:
						for ptag in params_tag.variablelist.find_all('varlistentry'):
							# these param tags can be for several parameters
							pnames = [tag.string.strip() for tag in ptag.term.find_all('parameter')]
							doc = []
							for ltag in ptag.find_all('listitem'):
								doc += [_stripdocstr(tag.get_text()) for tag in ltag.find_all('para')]
							# }
							for pname in pnames: param_doc[pname] = doc
						# }
					# }
					
					# apply to commands and params
					for cmd in doccmds:
						cmd.doc_desc = doc_desc
						cmd.doc_notes = doc_notes
						cmd.doc_errors = doc_errors
						for (pname, pdoc) in param_doc.iteritems():
							param = cmd.find_param(pname)
							if param:
								param.doc = pdoc
							# }
						# }
					# }
				
				except:
					print 'glapi: error processing file', filename
					raise
				# }
			# }
		# }
	# }
# }

# alias specific APIs
gl = apis['gl']




























