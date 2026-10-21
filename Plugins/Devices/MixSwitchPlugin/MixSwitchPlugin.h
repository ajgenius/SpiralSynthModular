/*  SpiralSound
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

#ifndef MixSwitchPLUGIN
#define MixSwitchPLUGIN

class MixSwitchPlugin : public Spiral::Core::Device {
  public:
    MixSwitchPlugin ();
    virtual ~MixSwitchPlugin ();
    virtual PluginInfo& Initialise (const HostInfo *Host);
    virtual void Execute ();
    virtual void Reset();
    virtual void ExecuteCommands();
    virtual void Describe(Spiral::Core::Description &d);
    virtual void Apply(Spiral::Core::Description::Reader &r);
    int GetSwitch (void) { return m_Position; }
    int GetChans (void) { return m_PluginInfo.NumInputs - 2; }
  private:
    void SetChans (int n);
  Spiral::Core::SignedProperty m_Chans;
    Spiral::Core::SignedProperty m_SwitchPos;
    Spiral::Core::UnsignedProperty m_Echo;
    Spiral::Core::ToggleProperty m_Auto;
    int m_Position;
    bool m_Triggered;
    void CreatePorts (int n = 2, bool AddPorts = false);
    friend Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, MixSwitchPlugin &o);
    friend Spiral::Core::Description &Describe(Spiral::Core::Description &d, MixSwitchPlugin &o);
};

Spiral::Core::Description::Reader &Apply(Spiral::Core::Description::Reader &r, MixSwitchPlugin &o);
Spiral::Core::Description &Describe(Spiral::Core::Description &d, MixSwitchPlugin &o);

#endif
