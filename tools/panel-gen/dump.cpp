// SPDX-License-Identifier: GPL-2.0-or-later
/*  SpiralSynthModular
 *  Copyright (C) 2026 ajgenius
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#include "Device.h"
#include "CoreDevices.h"
#include "LinearProperty.h"
#include "ToggleProperty.h"
#include "IntProperty.h"
#include "FloatProperty.h"
#include "LongProperty.h"
#include <dlfcn.h>
#include <cstdio>
#include <typeinfo>
#include <string>
using namespace Spiral::Core;
template<class T> static bool Lin(Property*p,const char*type){ LinearProperty<T>*l=dynamic_cast<LinearProperty<T>*>(p); if(!l)return false;
 printf("{\"class\":\"%s\",\"type\":\"%s\",\"default\":%g,\"min\":%g,\"max\":%g,\"step\":%g,\"page\":%g,\"logarithmic\":%s",p->ClassName().c_str(),type,(double)l->Default(),(double)l->Min(),(double)l->Max(),(double)l->SmallStep(),(double)l->Step(),l->IsLogarithmic()?"true":"false"); return true;}
int main(int argc,char**argv){
 HostInfo host=HostInfo(); host.BUFSIZE=64; host.SAMPLERATE=44100;
 SpiralPlugin*dev=0; std::string a=argv[1];
 if(a.find(".so")!=std::string::npos){ void*h=dlopen(a.c_str(),RTLD_NOW); if(!h){fprintf(stderr,"%s\n",dlerror());return 1;} typedef SpiralPlugin*(*C)(); dev=((C)dlsym(h,"SpiralPlugin_CreateInstance"))(); }
 else { RegisterCoreDevices(); const DeviceClass*c=FindCoreDevice(a); if(!c)return 2; dev=c->CreateInstance(); }
 dev->Initialise(&host);
 Device*d=dynamic_cast<Device*>(dev); fprintf(stderr,"cast %p %s\n",(void*)d,typeid(*dev).name()); if(!d){printf("[]\n");return 0;}
 if(argc>2){ Description desc; d->Describe(desc); printf("%s\n", desc.Values().empty()?"0":desc.Values()[0].c_str()); return 0; }
 printf("[");
 for(size_t i=0;i<d->Properties().size();++i){ Property*p=d->Properties()[i]; if(i)printf(",");
  if(!Lin<float>(p,"number")&&!Lin<int>(p,"integer")&&!Lin<long>(p,"integer")){
   if(ToggleProperty*t=dynamic_cast<ToggleProperty*>(p)) printf("{\"class\":\"ToggleProperty\",\"type\":\"boolean\",\"default\":%s",t->Default()?"true":"false");
   else printf("{\"class\":\"%s\",\"type\":\"other\"",p->ClassName().c_str());}
  printf(",\"property\":\"%s\",\"label\":\"%s\",\"hint\":\"%s\",\"read_only\":%s,\"internal\":%s,\"bound\":%s,\"size\":%d}\n",p->Name().c_str(),p->Label().c_str(),p->Hint().c_str(),p->IsReadOnly()?"true":"false",p->IsInternal()?"true":"false",p->IsBound()?"true":"false",p->StorageSize());
 }
 printf("]\n"); return 0; }
