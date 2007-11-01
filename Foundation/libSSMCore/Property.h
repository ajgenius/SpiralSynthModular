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

#ifndef __PROPERTY_H__
#define __PROPERTY_H__

#include <string>

#include "Types.h"
#include "Sample.h"

using namespace std;

namespace Spiral
{
	class Property
	{
	public:
		enum
		{
			ReadOnly=0, 
			WriteOnly=1,

			Internal=2,
			ForceUpdate=4, 
		};

		Property() {}
		virtual ~Property() {}

		Property(const Property &Copy)
		{
			m_Label = Copy.m_Label;
			m_Hint = Copy.m_Hint;
		}

		inline void CopyValue(const Property &CopyFrom) {}

		inline string GetLabel() { return m_Label; }
		inline string GetHint() { return m_Hint; }

		inline void SetLabel(string Label) { m_Label = Label; }
		inline void SetHint(string Hint) { m_Hint = Hint; }

		inline bool IsReadOnly() { return m_ReadOnly; }
		inline bool IsInternal() { return m_Internal; }
		inline bool RequiresUpdates() { return m_ForceUpdate; }

	protected:
		Property(int Flags) 
		{
			#define FlagSet(Flag) ((Flags & Flag) == Flag)

			m_ReadOnly = !FlagSet(WriteOnly);
			m_Internal = FlagSet(Internal);
			m_ForceUpdate = FlagSet(ForceUpdate);
		}

		bool m_ReadOnly, m_Internal, m_ForceUpdate;
	private:
		string m_Name, m_Label, m_Hint;
	};

	typedef struct
	{
		enum
		{
			Boolean,
			Signed,
			Unsigned,
			Float
		} Type;

		union
		{
			bool AsBoolean;

			SignedType AsSigned;
			UnsignedType AsUnsigned;

			double AsFloat;
		};
	} NumericPropertyValue;

	class PropertySet
	{
	public:
		PropertySet() {}
		PropertySet(vector<NumericPropertyValue> Values)
		{
			m_Values.insert(m_Values.end(), Values.begin(), Values.end());
		}

		PropertySet(const NumericPropertyValue Values[], unsigned int Count)
		{
			for (unsigned int i = 0; i < Count; i++)
			{
				m_Values.push_back(Values[i]);
			}
		}

		inline unsigned int size() { return m_Values.size(); }

		inline NumericPropertyValue operator[](unsigned int i)
		{		
			return m_Values[i];
		}

	protected:
		vector<NumericPropertyValue> m_Values;
	};

	typedef struct
	{
		#define DefaultBoolean(value) {NumericPropertyValue::Boolean, {AsBoolean:value}}
		#define DefaultFloat(value) {NumericPropertyValue::Float, {AsFloat:value}}
		#define DefaultSigned(value) {NumericPropertyValue::Signed, {AsSigned:value}}
		#define DefaultUnsigned(value) {NumericPropertyValue::Float, {AsUnsigned:value}}

		const string Name;
		const NumericPropertyValue Value;
	} NamedPropertyValue;

	class NamedPropertySet : public PropertySet
	{
	public:
		#define NameArray(...) ((char*[]){__VA_ARGS__})

		NamedPropertySet() {}
		NamedPropertySet(vector<string> Names,vector<NumericPropertyValue> Values):
		PropertySet(Values)
		{
			m_Names.insert(m_Names.end(), Names.begin(), Names.end());

			for (unsigned int i = 0; i < size(); i++)
				m_Index[m_Names[i]] = i;
		}

		NamedPropertySet(const NamedPropertyValue Values[], unsigned int Count)
		{
			for (unsigned int i = 0; i < Count; i++)
			{
				m_Values.push_back(Values[i].Value);
				m_Names.push_back(Values[i].Name);
				m_Index[Values[i].Name] = i;
			}
		}

		inline string Names(unsigned int i)
		{		
			return m_Names[i];
		}

		inline NumericPropertyValue operator[](string name)
		{		
			return ((PropertySet)(*this))[m_Index[name]];
		}
	private:
		vector<string> m_Names;
		map<string,int> m_Index;
	};

	class NumericProperty : public Property
	{
	public:
		NumericProperty() {}
		virtual ~NumericProperty() {}

		NumericProperty(const NumericProperty &Copy):
		Property(Copy)
		{
			Value = Copy.Value;
		}

		inline void CopyValue(const NumericProperty &CopyFrom) { Value = CopyFrom.Value; }

		NumericPropertyValue Value;
	protected:
		NumericProperty(int Flags): Property(Flags) {}
	};

	#define DefaultLinearFlags Property::WriteOnly | LinearProperty::ClampLower | LinearProperty::ClampUpper
	#define DefaultLogarithmicFlags Property::WriteOnly | LinearProperty::ClampLower | LinearProperty::ClampUpper | LinearProperty::Logarithmic

	class LinearProperty : public NumericProperty
	{
	public:
		enum
		{
			Logarithmic=8,
			ClampLower=16,
			ClampUpper=32
		};

		LinearProperty() {}
		virtual ~LinearProperty() {}

		LinearProperty(const LinearProperty &Copy) :
		NumericProperty(Copy)
		{
			Min = Copy.Min;
			Max = Copy.Max;
			SmallStep = Copy.SmallStep;
			Step = Copy.Step;
		}

		union
		{
			SignedType AsSigned;
			UnsignedType AsUnsigned;

			double AsFloat;
		} Min, Max, SmallStep, Step;

		virtual void Clamp() {}

		inline bool IsLogarithmic() { return m_Logarithmic; }
		inline bool IsClampLower() { return m_ClampLower; }
		inline bool IsClampUpper() { return m_ClampUpper; }

	protected:
		LinearProperty(int Flags): 
		NumericProperty(Flags)
		{
			#define FlagSet(Flag) ((Flags & Flag) == Flag)

			m_Logarithmic = FlagSet(Logarithmic);
			m_ClampLower = FlagSet(ClampLower);
			m_ClampUpper = FlagSet(ClampUpper);
		}

	private:
		bool m_Logarithmic, m_ClampLower, m_ClampUpper;
	};

	class SignedProperty : public LinearProperty
	{
	public:
		SignedProperty() {}
		virtual ~SignedProperty() {}

		SignedProperty(int Flags, SignedType InitialValue, 
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

		SignedProperty(const SignedProperty &Copy) :
		LinearProperty((LinearProperty)Copy)
		{
		}

		virtual void Clamp()
		{
			Value.AsSigned = min(max(Value.AsSigned, Min.AsSigned),Max.AsSigned);
		}
	};

	class UnsignedProperty : public LinearProperty
	{
	public:
		UnsignedProperty() {}
		virtual ~UnsignedProperty() {}

		UnsignedProperty(int Flags, UnsignedType InitialValue, 
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

		UnsignedProperty(const UnsignedProperty &Copy) :
		LinearProperty((LinearProperty)Copy)
		{
		}

		virtual void Clamp()
		{
			Value.AsUnsigned = min(max(Value.AsUnsigned, Min.AsUnsigned),Max.AsUnsigned);
		}
	};

	class FloatProperty : public LinearProperty
	{
	public:
		FloatProperty() {}
		virtual ~FloatProperty() {}

		FloatProperty(int Flags, double InitialValue, 
				double MinimumValue, double MaximumValue, 
				double MinimumStep, double DefaultStep):
		LinearProperty(Flags)
		{
			Value.Type = NumericPropertyValue::Float;
			Value.AsFloat = InitialValue;

			Min.AsFloat = MinimumValue;
			Max.AsFloat = MaximumValue;
			SmallStep.AsFloat = MinimumStep;
			Step.AsFloat = DefaultStep;
		}

		FloatProperty(const FloatProperty &Copy) :
		LinearProperty((LinearProperty)Copy)
		{
		}

		virtual void Clamp()
		{
			Value.AsFloat = min(max(Value.AsFloat, Min.AsFloat),Max.AsFloat);
		}
	};

	class ToggleProperty : public NumericProperty
	{
	public:
		ToggleProperty() {}
		virtual ~ToggleProperty() {}

		ToggleProperty(int Flags, bool InitialValue) : NumericProperty(Flags) { Value.AsBoolean = InitialValue; }
		ToggleProperty(const ToggleProperty &Copy) : NumericProperty((NumericProperty)Copy){}
	};

	class SetProperty : public NumericProperty
	{
	public:
		SetProperty() {}
		virtual ~SetProperty() {}

		SetProperty(int Flags, unsigned int InitialValueIndex, PropertySet Set):
		NumericProperty(Flags)
		{
			m_Set = Set;
			Value = m_Set[InitialValueIndex];
		}

		SetProperty(const SetProperty &Copy) :
		NumericProperty(Copy)
		{
			m_Set = Copy.m_Set;
		}

		inline PropertySet Set() { return m_Set; }
	private:
		PropertySet m_Set;
	};

	class StringProperty : public Property
	{
	public:
		StringProperty() {}
		virtual ~StringProperty() {}

		StringProperty(int Flags, string InitialValue): Property(Flags) { Value = InitialValue; }
		StringProperty(int Flags, char *InitialValue): Property(Flags) { Value = InitialValue; }

		StringProperty(const StringProperty &Copy):
		Property(Copy)
		{
			Value = Copy.Value;
		}

		inline void CopyValue(const StringProperty &CopyFrom) { Value = CopyFrom.Value; }

		string Value;
	protected:
		StringProperty(int Flags): Property(Flags) {}
	};

	class NoteProperty : public Property
	{
	public:
		NoteProperty() {}
		virtual ~NoteProperty() {}

		NoteProperty(int Flags, unsigned int InitialNote, unsigned int InitialOctave,
					float InitialVelocity):
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

		NoteProperty(const NoteProperty &Copy):
		Property(Copy)
		{
			CopyValue(Copy);
		}

		inline void CopyValue(const NoteProperty &CopyFrom) 
		{ 
			Note = CopyFrom.Note;
			Octave = CopyFrom.Octave;
			Velocity = CopyFrom.Velocity;
			Start = CopyFrom.Start;
			End = CopyFrom.End;
		}

		UnsignedType Note; /* 1-12, "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" */
		UnsignedType Octave; /* 0-6 */

		UnsignedType Start; /* 0-infinite */
		UnsignedType End; /* 0-infinite */

		FloatType Velocity; /* (0 - 1.0f)*127 */
	protected:
		NoteProperty(int Flags): Property(Flags) {}
	};

	class SampleProperty : public Property
	{
	public:
		SampleProperty() {}
		virtual ~SampleProperty() {}

		SampleProperty(int Flags, int InitialSize): Property(Flags) { Value = Sample(InitialSize); }

		SampleProperty(const SampleProperty &Copy):
		Property(Copy)
		{
			CopyValue(Copy);
		}

		inline void CopyValue(const SampleProperty &CopyFrom) { Value.Clear(); Value.Add(CopyFrom.Value); }

		Sample Value;
	protected:
		SampleProperty(int Flags): Property(Flags) {}
	};

	class ComplexProperty : public Property
	{
	public:
		virtual ~ComplexProperty() {}

		virtual inline Property* operator[](UnsignedType i) = 0;
		virtual inline UnsignedType size()=0;

		virtual void Add(Property *New)=0;
	protected:
		ComplexProperty() {}
		ComplexProperty(int Flags): Property(Flags) { }
	};

	class VectorProperty : public ComplexProperty
	{
	public:
		VectorProperty() {}
		virtual ~VectorProperty() 
		{
		}

		VectorProperty(int Flags, vector<Property*> Values): 
		ComplexProperty(Flags)
		{
			CopyContents(Values);
		}

		VectorProperty(const ComplexProperty &Copy):
		ComplexProperty(Copy)
		{
			CopyValue(Copy);
		}

		inline void CopyValue(const VectorProperty &CopyFrom)
		{ 
			CopyContents(CopyFrom.Values);
		}

		virtual inline Property* operator[](UnsignedType i)
		{		
			return Values[i];
		}
		
		virtual inline UnsignedType size()
		{
			return Values.size();
		}

		virtual void Add(Property *Property)
		{
			Values.push_back(Property);
		}

		vector<Property*> Values;
	protected:

		virtual inline void CopyContents(vector<Property*> Contents)
		{
			if (!Contents.size()) return;

			for (unsigned int i=0; i < Contents.size(); i++)
			{
				if (Contents[i])
					Values.push_back(Contents[i]);
			}
		}
	};
}

#endif
