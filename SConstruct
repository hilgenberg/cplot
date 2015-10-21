#!python
import os
import fnmatch
import multiprocessing

SetOption('num_jobs', multiprocessing.cpu_count())
print 'Using', GetOption('num_jobs'), 'parallel jobs';

src = []
for root, dirs, files in os.walk('.'):
	for f in fnmatch.filter(files, '*.cc'):
		src.append(os.path.join(root, f))

env = Environment()
env['CXXCOMSTR']  = "CC $SOURCE"
env['LINKCOMSTR'] = "LL $TARGET"
env.Append(CXXFLAGS='-std=c++11'.split())
env.Append(CXXFLAGS='-Wall -Wextra -Wno-parentheses -Wno-reorder -fstrict-enums -Wno-variadic-macros -Wno-unused-parameter'.split())


frel = '-O2 -DNDEBUG'.split()
fdbg = '-DDEBUG -DDEBUG -g'.split()
env.Append(CCFLAGS=fdbg)

libs = 'z X11 Xi GL GLU GLEW pthread readline'.split()
env.Append(LIBS=libs);
env.ParseConfig('pkg-config --cflags --libs pangocairo')

Default(env.Program(target='cplot', source=src))
