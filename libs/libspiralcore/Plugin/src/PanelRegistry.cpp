/*  SpiralSound
 *  Copyleft (C) 2002 David Griffiths <dave@pawfal.org>
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

#include <config.h>
#include <cstring>
#include <stdio.h>
#include <iostream>
#include "PanelRegistry.h"

using namespace std;
using namespace Spiral::Core;

PanelRegistry *PanelRegistry::m_Singleton = NULL;

PanelRegistry::PanelRegistry()
{
}

PanelRegistry::~PanelRegistry()
{
	for (vector<PanelClass*>::iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end(); i++)
	{
		delete *i;
	}
	m_PluginVec.clear();
}

PanelRegistry *PanelRegistry::Get()
{
	if (!m_Singleton)
	{
		m_Singleton = new PanelRegistry;
	}
	return m_Singleton;
}

void PanelRegistry::PackUpAndGoHome()
{
	if (!m_Singleton) return;
	delete m_Singleton;
	m_Singleton = NULL;
}

bool PanelRegistry::Register(const PanelClass &panel)
{
	if (panel.ForDevice < 0 || Find(panel.ForDevice, panel.Toolkit))
		return false;

	m_PluginVec.push_back(new PanelClass(panel));

	return true;
}

void PanelRegistry::ProvidePanels(const string &toolkit, SpiralGUIType *(*create)(SpiralPlugin *, const DeviceClass &))
{
	PanelProvider provider;
	provider.Toolkit = toolkit;
	provider.Create = create;
	m_Providers.push_back(provider);
	const vector<DeviceClass*> &devices = DeviceClassRegistry::Get()->Classes();

	for (size_t i = 0; i < devices.size(); ++i)
		if (!devices[i]->Panel.empty() || devices[i]->PanelGenerated)
			Offer(*devices[i]);
}

void PanelRegistry::Offer(const DeviceClass &device)
{
	for (size_t i = 0; i < m_Providers.size(); ++i)
	{
		PanelClass panel;
		panel.ForDevice = device.ID;
		panel.Toolkit = m_Providers[i].Toolkit;
		panel.Icon = device.Icon;
		panel.CreatePanel = m_Providers[i].Create;
		Register(panel);
	}
}

void PanelRegistry::Forget(int ForDevice)
{
	for (vector<PanelClass*>::iterator i = m_PluginVec.begin(); i != m_PluginVec.end();)
	{
		if ((*i)->ForDevice == ForDevice)
		{
			delete *i;
			i = m_PluginVec.erase(i);
		}
		else
			++i;
	}
}

const PanelClass *PanelRegistry::Find(int ForDevice, const string &Toolkit) const
{
	for (vector<PanelClass*>::const_iterator i=m_PluginVec.begin();
	     i!=m_PluginVec.end(); i++)
	{
		if ((*i)->ForDevice==ForDevice && (*i)->Toolkit==Toolkit)
			return *i;
	}
	return NULL;
}
