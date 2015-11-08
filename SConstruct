#!python
import os
import fnmatch
import multiprocessing
env = Environment(tools=['default','gch'], toolpath='.')

# use ncpu jobs
SetOption('num_jobs', multiprocessing.cpu_count())
print("Using %d parallel jobs" % GetOption('num_jobs'))

# compile all .cc files
src = []
for R,D,F in os.walk('.'):
	for f in fnmatch.filter(F, '*.cc'): src.append(os.path.join(R, f))

# less verbose output
env['GCHCOMSTR']  = "HH $SOURCE"
env['CXXCOMSTR']  = "CC $SOURCE"
env['LINKCOMSTR'] = "LL $TARGET"

# g++ flags
env.Append(CXXFLAGS='-std=c++11'.split())
env.Append(CXXFLAGS='-Wall -Wextra -Wno-parentheses -Wno-reorder -fstrict-enums -Wno-variadic-macros -Wno-unused-parameter'.split())

# precompiled header
env.Append(CXXFLAGS='-Winvalid-pch -include pch.h'.split())
env['precompiled_header'] = File('pch.h')
env['Gch'] = env.Gch(target='pch.h.gch', source=env['precompiled_header'])
pch = env.Alias('pch', 'pch.h.gch')
for s in src: env.Depends(s, pch)

# release/debug build
AddOption('--release', dest='release', action='store_true', default=False)
release = GetOption('release')
print(("Release" if release else "Debug")+" Build");
frel = '-O3 -s -DNDEBUG'
fdbg = '-Og -DDEBUG -D_DEBUG -g'
env.Append(CCFLAGS=Split(frel if release else fdbg))

# libs
libs = 'z X11 Xi GL GLU GLEW pthread readline'
env.Append(LIBS=libs.split());
env.ParseConfig('pkg-config --cflags --libs pangocairo')

# target
cplot = env.Program(target='cplot', source=src)
Default(cplot)

