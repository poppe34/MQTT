Import('RTT_ROOT')
Import('rtconfig')
from building import *

src = Glob('*.c')
CPPPATH = [RTT_ROOT + '/components/MQTT']
group = DefineGroup('mqtt', src, depend = ['RT_USING_MQTT'], CPPPATH = CPPPATH)

Return('group')
