/*  WaveShaper Plugin Copyleft (C) 2001 Yves Usson
 *  for SpiralSynthModular
 *  Copyleft (C) 2001 David Griffiths <dave@pawfal.org>
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
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/

#include "Device.h"

#ifndef WaveShaperPLUGIN
#define WaveShaperPLUGIN

class WaveShaperPlugin : public Spiral::Core::Device {
  public:
    WaveShaperPlugin();
    virtual ~WaveShaperPlugin ();
    virtual PluginInfo& Initialise (const HostInfo *Host);
    virtual void Execute();
    virtual void Reset();
    virtual void ExecuteCommands();
    virtual void Describe(Spiral::Core::Description &d);
    virtual void Apply(Spiral::Core::Description::Reader &r);
  private:
    float *m_wt;
    float *m_FuncPlot;
  Spiral::Core::FloatProperty m_Coefs[6];
    Spiral::Core::SignedProperty m_Wave;
    float m_CalcCoefs[6];
    bool m_CalcWave;
    void calc (void);
    void set (int index, float v);
    friend Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, WaveShaperPlugin &o);
    friend Spiral::Core::Description &Describe(Spiral::Core::Description &d, WaveShaperPlugin &o);
};

Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, WaveShaperPlugin &o);
Spiral::Core::Description &Describe(Spiral::Core::Description &d, WaveShaperPlugin &o);

#endif
