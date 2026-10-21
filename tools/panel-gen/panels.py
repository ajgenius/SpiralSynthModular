import json, os, re, subprocess, sys
ROOT=os.path.abspath(os.path.join(os.path.dirname(__file__),'..','..'))
BUILD=os.path.abspath(os.environ.get('PANELGEN_BUILD',os.path.join(ROOT,'.build')))
DUMP=os.environ.get('PANELGEN_DUMP',os.path.join(BUILD,'tools','panel-gen','dump'))
def props_of(name):
    so=f'{BUILD}/Plugins/Devices/{name}Plugin/.libs/{name}Plugin_DSP.so'
    arg=so if os.path.exists(so) else name+'Plugin'
    out=subprocess.run([DUMP,arg],capture_output=True,text=True).stdout
    ver=subprocess.run([DUMP,arg,'v'],capture_output=True,text=True).stdout.strip().splitlines()[-1]
    return {p['property'].upper():p for p in json.loads(out)}, int(ver)
def prop_entry(pid,p):
    e={'class':p['class'],'default':p['default'],
       'display_name':{'default':p['label'] or p['property'],'key':f"plugin.{pid}.property.{p['property'].upper()}"},
       'hint':{'default':p['hint'] or p['property'],'key':f"plugin.{pid}.property.{p['property'].upper()}.hint"},
       'internal':p['internal'],'property':p['property'].upper(),'read_only':p['read_only'],'type':p['type']}
    if p['type']=='string': e.pop('default'); e['default']=''
    if p['type'] in('number','integer'):
        e.update({'logarithmic':p['logarithmic'],'max':p['max'],'min':p['min'],'page':p['page'],'step':p['step']})
    return e
def identity(pid,name,ver):
    return [{'class':'StringProperty','default':name,'display_name':{'default':'DeviceType','key':f'plugin.{pid}.property.DEVICETYPE'},'hint':{'default':'DeviceType','key':f'plugin.{pid}.property.DEVICETYPE.hint'},'internal':False,'omitted':'identity','property':'DEVICETYPE','read_only':True,'type':'string'},
            {'class':'UnsignedProperty','default':ver,'display_name':{'default':'DeviceVersion','key':f'plugin.{pid}.property.DEVICEVERSION'},'hint':{'default':'DeviceVersion','key':f'plugin.{pid}.property.DEVICEVERSION.hint'},'internal':False,'logarithmic':False,'max':0,'min':0,'omitted':'identity','page':0,'property':'DEVICEVERSION','read_only':True,'step':0,'type':'integer'}]
def command_entry(pid,commands):
    return {'class':'StringProperty','commands':commands,'default':'','display_name':{'default':'COMMAND','key':f'plugin.{pid}.property.COMMAND'},'hint':{'default':'Command','key':f'plugin.{pid}.property.COMMAND.hint'},'internal':True,'omitted':'internal','property':'COMMAND','read_only':False,'type':'string'}
class P:
    def __init__(s,pid,commands=None): s.pid=pid; s.used=set(); s.n=0; s.names=set(); s.prefix=''; s.commands=commands or {}
    def command(s,name,*with_):
        if name not in s.commands: raise SystemExit(f'unknown command {name}')
        w={'property':'COMMAND','value':name}
        if with_: w['with']=list(with_)
        return w
    def t(s,name,text,kind='text'): return {'default':text,'key':f'plugin.{s.pid}.panel.{name}.{kind}'}
    def uid(s,b):
        b=s.prefix+b
        if b in s.names: raise SystemExit(f'duplicate control name {b}; set pl.prefix')
        s.names.add(b); return b
    def knob(s,prop,label,x,y,size=40):
        s.used.add(prop); n=s.uid(prop.lower())
        return [{'bounds':[x,y,size,size],'name':n+'-knob','property':prop,'type':'Knob'},
                {'bounds':[x-6,y+size,size+12,12],'name':n+'-label','text':s.t(n+'-label',label),'type':'Label'}]
    def spin(s,prop,label,x,y,w=60,command=None,mn=None,mx=None,h=18,below=False,size=None):
        s.used.add(prop); n=s.uid(prop.lower())
        c={'bounds':[x,y,w,h],'name':n+'-spinner','property':prop,'simple':True,'step':1,'digits':0,'type':'Spinner'}
        if command is not None: c['write']=[s.command(command,'value')]
        if mn is not None: c['min']=mn; c['max']=mx
        l={'bounds':[x,y+h,w,12] if below else [x+w+4,y,90,18],'name':n+'-label','text':s.t(n+'-label',label),'type':'Label'}
        if size: l['size']=size
        return [c,l]
    def led(s,prop,label,x,y,w=80):
        s.used.add(prop); n=s.uid(prop.lower())
        return [{'bordered':False,'bounds':[x,y,w,20],'name':n+'-ledbutton','property':prop,'text':s.t(n+'-ledbutton',label),'type':'LEDButton'}]
    def file(s,label,prop,command,x,y,save=False,ext='.wav',title=None,then=None,w=56):
        s.used.add(prop); n=s.uid(re.sub('[^a-z0-9]+','-',label.lower()))
        f={'property':prop,'save':save,'extension':ext,'title':s.t(n,title or label,'title')}
        c={'bounds':[x,y,w,20],'name':n,'text':s.t(n,label),'type':'FileChoosingButton','file':f,'write':[]}
        if then:
            c['write']=[{'property':k,'value':v} for k,v in then]; s.used.update(k for k,_ in then)
        c['write'].append(s.command(command))
        return [c]
    def button(s,label,command,x,y,w=56,write=None):
        n=s.uid(re.sub('[^a-z0-9]+','-',label.lower()))
        c={'bounds':[x,y,w,20],'name':n,'text':s.t(n,label),'type':'Button'}
        if write:
            c['write']=[{'property':k,'value':v} for k,v in write]; s.used.update(k for k,_ in write)
        if command: c.setdefault('write',[]).append(s.command(command))
        return [c]
def doc(name,pid,props,ver,children,w,h,help,extra_props=None):
    pl=P(pid)
    return pl
def finish(name,pid,props,ver,pl,children,w,h,help):
    used=sorted(pl.used)
    entries=identity(pid,name,ver)+[prop_entry(pid,props[u]) for u in used if u in props]
    if pl.commands: entries.append(command_entry(pid,pl.commands))
    entries.sort(key=lambda e:e['property'])
    d={'host_version':'0.3.43','panel':{'children':children,'height':h,'help':{'default':help,'key':f'plugin.{pid}.panel.help'},'schema_version':1,'width':w},'plugin_id':pid,'properties':entries,'schema_version':1}
    missing=[u for u in used if u not in props]
    if missing: print(name,'MISSING',missing)
    path=f'{ROOT}/Plugins/Devices/{name}Plugin/prebuilt-ui.json'
    open(path,'w').write(json.dumps(d,indent=2,sort_keys=True)+'\n'); print('wrote',path)
def pid_of(name): return json.load(open(f'{ROOT}/Plugins/Devices/{name}Plugin/info.json'))['id']
