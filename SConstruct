env = Environment(CFLAGS='-Wall')
throt = env.Program(target = 'throt', source = ['throt.c'])
env.Install('/usr/local/bin', throt)
env.Alias('install', ['/usr/local/bin'])
