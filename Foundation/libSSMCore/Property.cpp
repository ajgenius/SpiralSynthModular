// Copyright (C) 2007 ajgenius <113943+ajgenius@users.noreply.github.com>
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

#include "Sample.h"
#include "Property.h"
#include "Port.h"
#include "Device.h"
#include "Patch.h"

using namespace std;
using namespace Spiral;

Property::Property(const Property &Copy)
{
	m_Label = Copy.m_Label;
	m_Hint = Copy.m_Hint;
}

Property::Property(SignedType Flags) 
{
	#define FlagSet(Flag) ((Flags & Flag) != 0)

	m_ReadOnly = !FlagSet(WriteOnly);
	m_Internal = FlagSet(Internal);
	m_ForceUpdate = FlagSet(ForceUpdate);
}

void Property::InvalidPropertyCopy(const Property& CopyFrom)
{
	cerr << "WARNING - Incompatible Property Types. Cannot copy a " << ((Property&)CopyFrom).ClassName() << " into a " << ClassName() << "." << endl;
}

PropertySet::PropertySet(vector<NumericPropertyValue> Values)
{
	m_Values.insert(m_Values.end(), Values.begin(), Values.end());
}

PropertySet::PropertySet(const NumericPropertyValue Values[], UnsignedType Count)
{
	for (UnsignedType i = 0; i < Count; i++)
	{
		m_Values.push_back(Values[i]);
	}
}

NamedPropertySet::NamedPropertySet(vector<string> Names,vector<NumericPropertyValue> Values):
PropertySet(Values)
{
	m_Names.insert(m_Names.end(), Names.begin(), Names.end());

	for (UnsignedType i = 0; i < size(); i++)
		m_Index[m_Names[i]] = i;
}

NamedPropertySet::NamedPropertySet(const NamedPropertyValue Values[], UnsignedType Count)
{
	for (UnsignedType i = 0; i < Count; i++)
	{
		m_Values.push_back(Values[i].Value);
		m_Names.push_back(Values[i].Name);
		m_Index[Values[i].Name] = i;
	}
}

NumericProperty::NumericProperty(const NumericProperty &Copy):
Property(Copy)
{
	Value = Copy.Value;
}

void NumericProperty::CopyValue(const Property &CopyFrom) 
{ 
	if (!dynamic_cast<const NumericProperty*>(&CopyFrom))
	{
		InvalidPropertyCopy(CopyFrom);
		return;
	}

	/* FIXME - This replaces current Type, instead of setting the right value */
	Value = ((NumericProperty&)CopyFrom).Value; 
}

LinearProperty::LinearProperty(const LinearProperty &Copy) :
NumericProperty(Copy)
{
	Min = Copy.Min;
	Max = Copy.Max;
	SmallStep = Copy.SmallStep;
	Step = Copy.Step;
}
void LinearProperty::CopyValue(const Property &CopyFrom) 
{ 
	if (!dynamic_cast<const LinearProperty*>(&CopyFrom))
	{
		InvalidPropertyCopy(CopyFrom);
		return;
	}

	/* FIXME - This replaces current Type, instead of setting the right value */
	const LinearProperty *copy = ((const LinearProperty*)&CopyFrom);
	Value = copy->Value; 

	Min = copy->Min;
	Max = copy->Max;
	SmallStep = copy->SmallStep;
	Step = copy->Step;
}

LinearProperty::LinearProperty(SignedType Flags): 
NumericProperty(Flags)
{
	m_Logarithmic = FlagSet(Logarithmic);
	m_ClampLower = FlagSet(ClampLower);
	m_ClampUpper = FlagSet(ClampUpper);
}
SignedProperty::SignedProperty(SignedType Flags, SignedType InitialValue, 
		SignedType MinimumValue, SignedType MaximumValue, 
		SignedType MinimumStep, SignedType DefaultStep):
LinearProperty(Flags)
{
	Value.Type = NumericPropertyValue::Signed;
	Value.AsSigned = InitialValue;

	Min.AsSigned = MinimumValue;
	Max.AsSigned = MaximumValue;
	SmallStep.AsSigned = MinimumStep;
	Step.AsSigned = DefaultStep;
}

void SignedProperty::Clamp()
{
	Value.AsSigned = CLAMP(Value.AsSigned, Min.AsSigned,Max.AsSigned);
}

UnsignedProperty::UnsignedProperty(SignedType Flags, UnsignedType InitialValue, 
		UnsignedType MinimumValue, UnsignedType MaximumValue, 
		UnsignedType MinimumStep, UnsignedType DefaultStep):
LinearProperty(Flags)
{
	Value.Type = NumericPropertyValue::Unsigned;
	Value.AsUnsigned = InitialValue;

	Min.AsUnsigned = MinimumValue;
	Max.AsUnsigned = MaximumValue;
	SmallStep.AsUnsigned = MinimumStep;
	Step.AsUnsigned = DefaultStep;
}

void UnsignedProperty::Clamp()
{
	Value.AsUnsigned = CLAMP(Value.AsUnsigned, Min.AsUnsigned,Max.AsUnsigned);
}

FloatProperty::FloatProperty(SignedType Flags, FloatType InitialValue, 
		FloatType MinimumValue, FloatType MaximumValue, 
		FloatType MinimumStep, FloatType DefaultStep):
LinearProperty(Flags)
{
	Value.Type = NumericPropertyValue::Float;
	Value.AsFloat = InitialValue;

	Min.AsFloat = MinimumValue;
	Max.AsFloat = MaximumValue;
	SmallStep.AsFloat = MinimumStep;
	Step.AsFloat = DefaultStep;
}

void FloatProperty::Clamp()
{
	Value.AsFloat = CLAMP(Value.AsFloat, Min.AsFloat,Max.AsFloat);
}

SetProperty::SetProperty(SignedType Flags, UnsignedType InitialValueIndex, PropertySet Set):
NumericProperty(Flags)
{
	m_Set = Set;
	Value = m_Set[InitialValueIndex];
}

SetProperty::SetProperty(const SetProperty &Copy) :
NumericProperty(Copy)
{
	m_Set = Copy.m_Set;
}

StringProperty::StringProperty(const StringProperty &Copy):
Property(Copy)
{
	Value = Copy.Value;
}

void StringProperty::CopyValue(const Property &CopyFrom)
{
	if (!dynamic_cast<const StringProperty*>(&CopyFrom))
	{
		InvalidPropertyCopy(CopyFrom);
		return;
	}

	Value = ((StringProperty&)CopyFrom).Value; 
}

NoteProperty::NoteProperty(SignedType Flags, UnsignedType InitialNote, UnsignedType InitialOctave,
			FloatType InitialVelocity):
Property(Flags) 
{ 
	/* Notes start at 1 */
	Note = InitialNote;

	/* 8 Octaves */
	Octave = InitialOctave;

	/* Constrain to 0-127, (Velocity*127) */
	Velocity = InitialVelocity;

	/* Start/End Positions */
	Start = 0;
	End = 0;
}

NoteProperty::NoteProperty(const NoteProperty &Copy):
Property(Copy)
{
	CopyValue(Copy);
}

void NoteProperty::CopyValue(const Property &CopyFrom) 
{ 
	if (!dynamic_cast<const NoteProperty*>(&CopyFrom))
	{
		InvalidPropertyCopy(CopyFrom);
		return;
	}

	const NoteProperty *copy = ((const NoteProperty*)&CopyFrom);

	Note = copy->Note;
	Octave = copy->Octave;
	Velocity = copy->Velocity;
	Start = copy->Start;
	End = copy->End;
}

SampleProperty::SampleProperty(const SampleProperty &Copy):
Property(Copy)
{
	CopyValue(Copy);
}

void SampleProperty::CopyValue(const Property &CopyFrom) 
{
	if (!dynamic_cast<const SampleProperty*>(&CopyFrom))
	{
		InvalidPropertyCopy(CopyFrom);
		return;
	}

	Value.Clear(); 
	Value.Add(((SampleProperty&)CopyFrom).Value); 
}

VectorProperty::VectorProperty(SignedType Flags, vector<Property*> Values): 
ComplexProperty(Flags)
{
	CopyContents(Values);
}

VectorProperty::VectorProperty(const ComplexProperty &Copy):
ComplexProperty(Copy)
{
	CopyValue(Copy);
}

void VectorProperty::CopyValue(const Property &CopyFrom)
{ 
	if (!dynamic_cast<const VectorProperty*>(&CopyFrom))
	{
		InvalidPropertyCopy(CopyFrom);
		return;
	}

	CopyContents(((const VectorProperty*)&CopyFrom)->Values);
}

void VectorProperty::CopyContents(vector<Property*> Contents)
{
	if (!Contents.size()) return;

	for (UnsignedType i=0; i < Contents.size(); i++)
	{
		if (Contents[i])
			Values.push_back(Contents[i]);
	}
}
