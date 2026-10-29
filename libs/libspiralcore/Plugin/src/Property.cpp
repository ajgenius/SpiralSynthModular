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

using namespace Spiral::Core;

Property::Property(const Property &Copy)
{
	m_References = 1;
	m_Name = Copy.m_Name;
	m_Label = Copy.m_Label;
	m_Hint = Copy.m_Hint;

	m_ReadOnly = Copy.m_ReadOnly;
	m_Internal = Copy.m_Internal;
	m_ForceUpdate =Copy.m_ForceUpdate;
}

Property::Property(SignedType Flags) 
{
	#define FlagSet(Flag) ((Flags & Flag) != 0)

	m_ReadOnly = !FlagSet(WriteOnly);
	m_Internal = FlagSet(Internal);
	m_ForceUpdate = FlagSet(ForceUpdate);

	m_References = 1;
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
	for (UnsignedType i=0; i < Values.size(); i++)
	{
		if (Values[i]) Add(Values[i]);
	}
}

VectorProperty::VectorProperty(const VectorProperty &Copy):
ComplexProperty(Copy)
{
	CopyValue(Copy);
}

VectorProperty::VectorProperty(const ComplexProperty &Copy):
ComplexProperty(Copy)
{
	CopyValue(Copy);
}

void VectorProperty::CopyValue(const Property &CopyFrom)
{ 
	ComplexProperty *Source = ((Property &)CopyFrom).AsComplex();

	if (!Source)
	{
		InvalidPropertyCopy(CopyFrom);
		return;
	}

	CopyContents(*Source);
}

void VectorProperty::Release()
{
	for (vector<Property*>::iterator i=Values.begin(); i!=Values.end(); i++)
	{
		if (!*i) continue;

		(*i)->UnReference();

		if (!(*i)->IsReferenced()) delete *i;
	}

	Values.clear();
}

void VectorProperty::CopyContents(ComplexProperty &Source)
{
	if (Values.size() == Source.size())
	{
		for (UnsignedType i=0; i < Values.size(); i++)
		{
			Property *Theirs = Source[i];

			if (Values[i] && Theirs) Values[i]->CopyValue(*Theirs);
		}

		return;
	}

	Release();

	for (UnsignedType i=0; i < Source.size(); i++)
	{
		Property *Theirs = Source[i];

		if (Theirs) Add(Theirs->Duplicate());
	}
}


namespace Spiral
{
namespace Core
{
	template <class T>
	static Description &Keyed(Description &d, const T &value, const Property &p, const std::string &cls)
	{
		d.Value(value);
		return d.Key(p.GetKey(), cls);
	}

	Description &Description::Value(const FloatProperty &p)    { return Keyed(*this, p.Value.AsFloat, p, "FloatProperty"); }
	Description &Description::Value(const SignedProperty &p)   { return Keyed(*this, p.Value.AsSigned, p, "SignedProperty"); }
	Description &Description::Value(const UnsignedProperty &p) { return Keyed(*this, p.Value.AsUnsigned, p, "UnsignedProperty"); }
	Description &Description::Value(const ToggleProperty &p)   { return Keyed(*this, p.Value.AsBoolean, p, "ToggleProperty"); }
	Description &Description::Value(const SetProperty &p)      { return Keyed(*this, p.Value.AsUnsigned, p, "SetProperty"); }
	Description &Description::Value(const StringProperty &p)   { return Keyed(*this, p.Value, p, "StringProperty"); }

	Description &Description::Value(const NoteProperty &p)
	{
		const std::string key = p.GetKey();
		Value(p.Note);     if (!key.empty()) Key(key, "NoteProperty");
		Separator(" ");
		Value(p.Octave);   if (!key.empty()) Key(key + ".octave", "NoteProperty");
		Separator(" ");
		Value(p.Velocity); if (!key.empty()) Key(key + ".velocity", "NoteProperty");
		Separator(" ");
		Value(p.Start);    if (!key.empty()) Key(key + ".start", "NoteProperty");
		Separator(" ");
		Value(p.End);      if (!key.empty()) Key(key + ".end", "NoteProperty");
		return *this;
	}

	template <class T>
	static bool Into(Description::Reader &r, const Description &d, const Property &p, T &x)
	{
		if (!d.Named())
			return !r.Value(x).Failed();

		const int at = d.Find(p.GetKey());
		return at < 0 || r.At((size_t)at, x);
	}

	Description::Reader &Description::Reader::Value(FloatProperty &p)    { if (!Into(*this, m_Description, p, p.Value.AsFloat)) m_Failed = true; return *this; }
	Description::Reader &Description::Reader::Value(SignedProperty &p)   { if (!Into(*this, m_Description, p, p.Value.AsSigned)) m_Failed = true; return *this; }
	Description::Reader &Description::Reader::Value(UnsignedProperty &p) { if (!Into(*this, m_Description, p, p.Value.AsUnsigned)) m_Failed = true; return *this; }
	Description::Reader &Description::Reader::Value(ToggleProperty &p)   { if (!Into(*this, m_Description, p, p.Value.AsBoolean)) m_Failed = true; return *this; }
	Description::Reader &Description::Reader::Value(SetProperty &p)      { if (!Into(*this, m_Description, p, p.Value.AsUnsigned)) m_Failed = true; return *this; }
	Description::Reader &Description::Reader::Value(StringProperty &p)   { if (!Into(*this, m_Description, p, p.Value)) m_Failed = true; return *this; }

	Description::Reader &Description::Reader::Value(NoteProperty &p)
	{
		if (!m_Description.Named())
			return Value(p.Note).Value(p.Octave).Value(p.Velocity).Value(p.Start).Value(p.End);

		const std::string key = p.GetKey();
		int at = m_Description.Find(key);
		if (at >= 0) At((size_t)at, p.Note);
		at = m_Description.Find(key + ".octave");
		if (at >= 0) At((size_t)at, p.Octave);
		at = m_Description.Find(key + ".velocity");
		if (at >= 0) At((size_t)at, p.Velocity);
		at = m_Description.Find(key + ".start");
		if (at >= 0) At((size_t)at, p.Start);
		at = m_Description.Find(key + ".end");
		if (at >= 0) At((size_t)at, p.End);
		return *this;
	}

	Description::Reader &Description::Reader::Count(const VectorProperty &p, int &n)
	{
		if (!m_Description.Named())
			return Value(n);

		std::string prefix = p.GetKey() + "[";
		for (size_t c = 0; c < prefix.size(); c++) prefix[c] = (char)toupper((unsigned char)prefix[c]);
		const std::vector<std::string> &keys = m_Description.Keys();
		int count = 0;
		for (size_t i = 0; i < keys.size(); i++)
		{
			if (keys[i].size() <= prefix.size()) continue;
			std::string head = keys[i].substr(0, prefix.size());
			for (size_t c = 0; c < head.size(); c++) head[c] = (char)toupper((unsigned char)head[c]);
			if (head != prefix) continue;
			const int index = atoi(keys[i].c_str() + prefix.size());
			if (index + 1 > count) count = index + 1;
		}
		n = count;
		return *this;
	}
}
}
