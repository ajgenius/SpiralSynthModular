/*  SpiralSound
 *  Copyleft (C) 2003 Andy Preston <andy@clublinux.co.uk>
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

#include <string>
#include <iostream>
#include <stdio.h>
#include <math.h>

#include "TransposePlugin.h"
#include "TransposePluginGUI.h"
#include "SpiralIcon.xpm"
#include "../../NoteTable.h"

using namespace std;

extern "C" {

SpiralPlugin* SpiralPlugin_CreateInstance () {
    return new TransposePlugin;
}

char** SpiralPlugin_GetIcon () {
    return SpiralIcon_xpm;
}

int SpiralPlugin_GetID () {
    return 122;
}

string SpiralPlugin_GetGroupName() {
    return "Control";
}

}

///////////////////////////////////////////////////////

TransposePlugin::TransposePlugin () :
m_Amount(0),
m_Out(0.0),
m_BufferInitialized(false)
{
    m_PluginInfo.Name = "Transpose";
    m_PluginInfo.Width = 80;
    m_PluginInfo.Height = 60;
    m_PluginInfo.NumInputs = 2;
    m_PluginInfo.NumOutputs = 1;
    m_PluginInfo.PortTips.push_back ("Input");
    m_PluginInfo.PortTips.push_back ("Transpose CV");
    m_PluginInfo.PortTips.push_back ("Output");
    m_AudioCH->Register ("Amount", &m_Amount);
}

TransposePlugin::~TransposePlugin () {
}

PluginInfo &TransposePlugin::Initialise (const HostInfo *Host) {
    return SpiralPlugin::Initialise (Host);
}

SpiralGUIType *TransposePlugin::CreateGUI() {
    return new TransposePluginGUI (m_PluginInfo.Width, m_PluginInfo.Height, this, m_AudioCH, m_HostInfo);
}



void TransposePlugin::Execute () {
	float base=110, transpose=m_Amount;
	float alpha = 17.312340490667;

	for (int n=0; n<m_HostInfo->BUFSIZE; n++)
	{
		float input0 = GetInputPitch(0, n);
		float input1  = GetInputPitch(1, n);

		if (input0 > 0)
		{
			// input's half steps from base of 110 = round(alpha*logf(GetInputPitch(0, n)/110));
			// cv's half steps from base of 110 = round(alpha*logf(GetInputPitch(1, n)/110));

			base = floor((alpha*logf(input0/110.0)) + 0.5);
			if (InputExists(1))
			{
				transpose = (input1 > 0)?floor((alpha*logf(input1/110)) + 0.5):8.176;
			}

			m_Out = 110 * powf(2, (base+transpose) / 12);
		}
		else
		{
			m_Out = 8.176;	
		}	
		SetOutputPitch (0, n, m_Out);
	}
}

void TransposePlugin::StreamOut (ostream &s) {
     s << m_Version << endl;
     s << m_Amount;
}

void TransposePlugin::StreamIn (istream &s) {
     int version;
     s >> version;
     s >> m_Amount;
}
