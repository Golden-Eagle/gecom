#!/bin/env python
#
# Retrieve GL API documentation minimally
#

import urllib2, bs4, os, errno

def get_page(url):
	url = 'https://cvs.khronos.org/svn/repos/ogl/trunk/ecosystem/public/sdk/docs/' + url
	headers = {'User-Agent': 'Mozilla/5.0'}
	req = urllib2.Request(url,headers=headers)
	return urllib2.urlopen(req).read()
# }

# save all xml docs
for version in [2, 3, 4]:
	man = 'man{0}'.format(version)
	manpage = get_page(man)
	try:
		os.makedirs('./docs/' + man)
	except OSError, e:
		if e.errno != errno.EEXIST: raise
	# }
	soup = bs4.BeautifulSoup(manpage, features='xml')
	file_tags = [file_tag for file_tag in soup.find('svn').find('index').find_all('file')]
	for i, file_tag in enumerate(file_tags):
		href = str(file_tag['href']).strip()
		# skip non-xml
		if not href.endswith('.xml'): continue
		print '[{man} {i:5} /{c:5}] {href}'.format(man=man, i=i+1, c=len(file_tags), href=href)
		# skip files we already have
		filename = './docs/{man}/{href}'.format(man=man, href=href)
		if os.path.exists(filename + '.flag'):
			print ' - skipped'
			continue
		# }
		# download and save
		page = get_page('{man}/{href}'.format(man=man, href=href))
		with open(filename, 'w') as file:
			file.write(page)
		# }
		# write a separate file to mark completion
		# prevent corruption if downloader is aborted
		with open(filename + '.flag', 'w'): pass
	# }
# }
