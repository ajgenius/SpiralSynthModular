import os, sys; sys.path.insert(0,os.path.dirname(os.path.abspath(__file__)))
from panels import *
def make(name, build, w, h, help, commands=None):
    pid=pid_of(name); props,ver=props_of(name); pl=P(pid,commands)
    props.update(EXTRA.get(name,{}))
    children=build(pl,props); finish(name,pid,props,ver,pl,children,w,h,help)

def sprop(name,label): return {name.upper():{'class':'StringProperty','type':'string','default':'','property':name,'label':label,'hint':label,'read_only':False,'internal':True}}
EXTRA={'DiskWriter':sprop('Filename','File name'),'Stream':sprop('FileName','File name'),'SpiralLoop':sprop('Name','File name'),
 'PoshSampler':dict(sprop('Name','File name'),NUM={'class':'IntProperty','type':'integer','default':0,'min':0,'max':7,'step':1,'page':1,'logarithmic':False,'property':'Num','label':'Sample','hint':'Sample','read_only':False,'internal':True})}


def posh(pl,props):
    n=len([k for k in props if re.match(r'SAMPLE\d+VOLUME$',k)])
    pages=[]
    for i in range(n):
        s=f'SAMPLE{i}'
        ch=pl.knob(s+'VOLUME','Volume',10,8)+pl.knob(s+'PITCHMOD','Pitch',60,8)+pl.spin(s+'NOTE','Note',110,8,50)+pl.spin(s+'OCTAVE','Octave',110,30,50)
        ch+=pl.led(s+'LOOP','Loop',8,66,70)+pl.led(s+'PINGPONG','Ping pong',80,66,80)+pl.led(s+'RETRIG','Retrigger',164,66,80)
        pl.prefix=f'sample{i+1}-'
        ch+=pl.file('Load','NAME','LOAD',190,8,title=f'Load sample {i+1}',then=[('NUM',i)])+pl.file('Save','NAME','SAVE',190,32,save=True,title=f'Save sample {i+1}',then=[('NUM',i)])
        pl.prefix=''
        pages.append({'children':ch,'title':{'default':str(i+1),'key':f'plugin.{pl.pid}.panel.samples.page.{i}'}})
    return [{'bounds':[2,2,256,104],'name':'samples','page_padding':0,'pages':pages,'tab_height':14,'type':'Notebook'}]
make('PoshSampler',posh,260,108,'One page per sample: volume, pitch, the note and octave it plays on, looping and retrigger; Load and Save choose its WAV file. Recording and editing samples stay in the editor.',commands={'LOAD':1,'SAVE':2})

make('Distributor',lambda pl,p: pl.spin('CHANNELCOUNT','Channels',13,6,50,command='SETCHANNELCOUNT',h=15,below=True,size=8),76,24,'The number of channels the input is distributed across.',commands={'SETCHANNELCOUNT':{'command':1,'args':['CHANNELCOUNT']}})
make('Splitter',lambda pl,p: pl.spin('CHANNELCOUNT','Channels',13,6,50,command='SETCHANNELCOUNT',h=15,below=True,size=8),76,24,'The number of outputs the input is split to.',commands={'SETCHANNELCOUNT':{'command':1,'args':['CHANNELCOUNT']}})
make('EnvFollower',lambda pl,p: pl.knob('ATTACK','Attack',6,4)+pl.knob('DECAY','Decay',62,4),116,64,'How fast the envelope follows a rising and a falling input.')
def snap(pl,p):
    pl.used.add('NOTE')
    return [{'bounds':[0,2,296,64],'first_note':0,'note_count':132,'name':'keys','note_off':2,'note_on':1,'property':'NOTE','scroll_x':100,'type':'PianoKeys','white_key_width':10}]
if False: make('NoteSnap',snap,296,68,'Click a key to set the note the input snaps to.')
make('Transpose',lambda pl,p: pl.spin('AMOUNT','Amount',13,6,50,mn=-48,mx=48,h=20,below=True),76,44,'The transposition in semitones.')

pl=P(0,{'OPENREAD':1,'OPENWRITE':2,'OPENDUPLEX':3,'CLOSE':4,'SET_VOLUME':{'command':5,'args':['VOLUME']}}); props,ver=props_of('Output')
ch=pl.knob('VOLUME','Volume',10,8)
ch[0]['write']=[pl.command('SET_VOLUME','value')]
for i,(lab,cmd) in enumerate([('Output','OPENWRITE'),('Input','OPENREAD'),('Duplex','OPENDUPLEX'),('Close','CLOSE')]):
    ch+=pl.button(lab,cmd,60+(i%2)*58,8+(i//2)*24,56)
finish('Output',0,props,ver,pl,ch,180,64,'Volume of the sound card output, and opening it for output, input or both, or closing it.')
