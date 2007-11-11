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
#include <vector>
#include <map>

#include "Types.h"
#include "Sample.h"

using namespace std;

/* REWORK -
	Like ports,
	Properties should be created FOR a device
	and the property then adds itself to the host
	new *Property(this, name, flags, ....)

	an Internal flag, means a state property,
	otherwise a shared property

	every readonly,
	gets a public writeonly duplicate
	and vica versa.

	calling Syncronize on a writeonly,
	sets the readonly's value,
	and if forceupdate, calls
	host->UpdateProperty(this)

	the entire design should be under the concept of two threads
	no matter how many threads there are, the property is only
	in two threads, the processing thread, and the command thread
	thus so long as synchronize is called from within a locked mutex
	between command and audio, there will never be problems.

	so Synth will handle this for us.

	the complex issues are for complex properties,
	where you don't want to have to COPY back and forth between
	the private, and public copies of a property in-device,
	all the contents and sub-properties...

	but, so long as properties /themselves/ manage their polar opposite
	it doesn't matter. creating a writeonly will automagically create
	a readonly pair, and mark it private, and vica versa

	Note - write vs read is based on public, not private,

		so creating a writeonly property
		is creating a readonly internal

		and creating a readonly property
		is creating a writeonly internal

		aka a readonly property is one which
		the device writes to, 
		and a writeonly is one which it reads from


	the question then is, can an external source
	insert a new property into a complex property -

	NO NO NO.

	If that is needed, we can use a more complex mechanism later.
	I don't think we need such a thing ever however.

	a sane device would instead use say, a command property,
	which allowed setting a new command (like add/delete property)

	which is how the old SSM did things... but I don't think we need such a thing
	what kind of single device needs commands that can't be instead better handled as properties?

	like Mixer, yes it has public properties for each input, but that is set by a channel count,
	which is a normal standalone property. the other side doesn't need to do anything, but set the channel
	count....

	the only exception to this rule is sequencing
	and that I think should be broken out of the usual sphere of devices,
	and made something special.. midi based. streaming values. 

	but till then.. hmm. a custom property. Pattern, Notes.
	Notes aren't properties, but have properties associated with them
	so a Pattern would be a publically modifiable vector of Notes,
	along with stacks of notes deleted, and added, and modified.

	yes we will have NoteProperties, for loading, but in this case 
	they won't add themselves to a host device, but instead a pattern property
	and they won't ever be managed directly, but instead be private things,
	/only/ for loading.

	so calling synchronize on a Patten, would result in
		for iter add.begin
			add note

		clear add

		for iter delete.begin
			delete note 

		clear delete

		for iter modify.begin
			update note

		clear modify

	the other issue, is should pattern itself be like note, a special property,
	not managed directly, only from a sequencer property, which could contain
	multiple patterns..... hmm. think more on all this....

*/
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

		Property(const Property &Copy);

		virtual string ClassName()=0;

		virtual void CopyValue(const Property &CopyFrom)=0;

		inline string GetLabel() { return m_Label; }
		inline string GetHint() { return m_Hint; }

		inline void SetLabel(string Label) { m_Label = Label; }
		inline void SetHint(string Hint) { m_Hint = Hint; }

		inline bool IsReadOnly() { return m_ReadOnly; }
		inline bool IsInternal() { return m_Internal; }
		inline bool RequiresUpdates() { return m_ForceUpdate; }

	protected:
		Property(SignedType Flags);

		void InvalidPropertyCopy(const Property& CopyFrom);

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

			FloatType AsFloat;
		};
	} NumericPropertyValue;

	class PropertySet
	{
	public:
		PropertySet() {}
		PropertySet(vector<NumericPropertyValue> Values);
		PropertySet(const NumericPropertyValue Values[], UnsignedType Count);

		inline UnsignedType size() { return m_Values.size(); }

		inline NumericPropertyValue operator[](UnsignedType i) { return m_Values[i]; }

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
		NamedPropertySet(vector<string> Names,vector<NumericPropertyValue> Values);

		NamedPropertySet(const NamedPropertyValue Values[], UnsignedType Count);

		inline string Names(UnsignedType i) { return m_Names[i]; }

		inline NumericPropertyValue operator[](string name)
		{		
			return ((PropertySet)(*this))[m_Index[name]];
		}
	private:
		vector<string> m_Names;
		map<string,SignedType> m_Index;
	};

	class NumericProperty : public Property
	{
	public:
		NumericProperty() {}
		virtual ~NumericProperty() {}

		NumericProperty(const NumericProperty &Copy);

		virtual void CopyValue(const Property &CopyFrom);
		NumericPropertyValue Value;
	protected:
		NumericProperty(SignedType Flags): Property(Flags) {}
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

		LinearProperty(const LinearProperty &Copy);

		union
		{
			SignedType AsSigned;
			UnsignedType AsUnsigned;

			FloatType AsFloat;
		} Min, Max, SmallStep, Step;

		virtual void Clamp() {}

		virtual void CopyValue(const Property &CopyFrom);

		inline bool IsLogarithmic() { return m_Logarithmic; }
		inline bool IsClampLower() { return m_ClampLower; }
		inline bool IsClampUpper() { return m_ClampUpper; }

	protected:
		LinearProperty(SignedType Flags);

	private:
		bool m_Logarithmic, m_ClampLower, m_ClampUpper;
	};

	class SignedProperty : public LinearProperty
	{
	public:
		SignedProperty() {}
		virtual ~SignedProperty() {}

		SignedProperty(SignedType Flags, SignedType InitialValue, 
				SignedType MinimumValue, SignedType MaximumValue, 
				SignedType MinimumStep, SignedType DefaultStep);

		SignedProperty(const SignedProperty &Copy) :
		LinearProperty(Copy)
		{
		}

		virtual void Clamp();

		virtual string ClassName()
		{
			return "SignedProperty";
		}
	};

	class UnsignedProperty : public LinearProperty
	{
	public:
		UnsignedProperty() {}
		virtual ~UnsignedProperty() {}

		UnsignedProperty(SignedType Flags, UnsignedType InitialValue, 
				UnsignedType MinimumValue, UnsignedType MaximumValue, 
				UnsignedType MinimumStep, UnsignedType DefaultStep);

		UnsignedProperty(const UnsignedProperty &Copy) :
		LinearProperty(Copy)
		{
		}

		virtual void Clamp();

		virtual string ClassName()
		{
			return "UnsignedProperty";
		}
	};

	class FloatProperty : public LinearProperty
	{
	public:
		FloatProperty() {}
		virtual ~FloatProperty() {}

		FloatProperty(SignedType Flags, FloatType InitialValue, 
				FloatType MinimumValue, FloatType MaximumValue, 
				FloatType MinimumStep, FloatType DefaultStep);

		FloatProperty(const FloatProperty &Copy) :
		LinearProperty(Copy)
		{
		}

		virtual void Clamp();
		virtual string ClassName()
		{
			return "FloatProperty";
		}
	};

	class ToggleProperty : public NumericProperty
	{
	public:
		ToggleProperty() {}
		virtual ~ToggleProperty() {}

		ToggleProperty(SignedType Flags, bool InitialValue) : NumericProperty(Flags) { Value.AsBoolean = InitialValue; }
		ToggleProperty(const ToggleProperty &Copy) : NumericProperty(Copy){}

		virtual string ClassName()
		{
			return "ToggleProperty";
		}
	};

	class SetProperty : public NumericProperty
	{
	public:
		SetProperty() {}
		virtual ~SetProperty() {}

		SetProperty(SignedType Flags, UnsignedType InitialValueIndex, PropertySet Set);

		SetProperty(const SetProperty &Copy);

		inline PropertySet Set() { return m_Set; }

		virtual string ClassName()
		{
			return "SetProperty";
		}
	private:
		PropertySet m_Set;
	};

	class StringProperty : public Property
	{
	public:
		StringProperty() {}
		virtual ~StringProperty() {}

		StringProperty(SignedType Flags, string InitialValue): Property(Flags) { Value = InitialValue; }
		StringProperty(SignedType Flags, const char *InitialValue): Property(Flags) { Value = InitialValue; }

		StringProperty(const StringProperty &Copy);

		virtual void CopyValue(const Property &CopyFrom);

		virtual string ClassName()
		{
			return "StringProperty";
		}

		string Value;
	protected:
		StringProperty(SignedType Flags): Property(Flags) {}
	};

	class NoteProperty : public Property
	{
	public:
		NoteProperty() {}
		virtual ~NoteProperty() {}

		NoteProperty(SignedType Flags, UnsignedType InitialNote, UnsignedType InitialOctave,
					FloatType InitialVelocity);

		NoteProperty(const NoteProperty &Copy);

		virtual void CopyValue(const Property &CopyFrom);

		UnsignedType Note; /* 1-12, "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" */
		UnsignedType Octave; /* 0-6 */

		UnsignedType Start; /* 0-infinite */
		UnsignedType End; /* 0-infinite */

		FloatType Velocity; /* (0 - 1.0f)*127 */

		virtual string ClassName()
		{
			return "NoteProperty";
		}
	protected:
		NoteProperty(SignedType Flags): Property(Flags) {}
	};

	class SampleProperty : public Property
	{
	public:
		SampleProperty() {}
		virtual ~SampleProperty() {}

		SampleProperty(SignedType Flags, UnsignedType InitialSize): Property(Flags) { Value = Sample(InitialSize); }

		SampleProperty(const SampleProperty &Copy);

		virtual void CopyValue(const Property &CopyFrom);

		Sample Value;

		virtual string ClassName()
		{
			return "SampleProperty";
		}
	protected:
		SampleProperty(SignedType Flags): Property(Flags) {}
	};

	class ComplexProperty : public Property
	{
	public:
		virtual ~ComplexProperty() {}

		virtual Property* operator[](UnsignedType i) = 0;
		virtual UnsignedType size()=0;

		virtual void Add(Property *New)=0;
	protected:
		ComplexProperty() {}
		ComplexProperty(SignedType Flags): Property(Flags) { }
	};

	class VectorProperty : public ComplexProperty
	{
	public:
		VectorProperty() {}
		virtual ~VectorProperty() 
		{
		}

		VectorProperty(SignedType Flags, vector<Property*> Values);

		VectorProperty(const ComplexProperty &Copy);
		virtual void CopyValue(const Property &CopyFrom);

		virtual Property* operator[](UnsignedType i)
		{		
			return Values[i];
		}
		
		virtual UnsignedType size()
		{
			return Values.size();
		}

		virtual void Add(Property *Property)
		{
			Values.push_back(Property);
		}

		virtual string ClassName()
		{
			return "VectorProperty";
		}

		vector<Property*> Values;

	private:
		inline void CopyContents(vector<Property*> Contents);
	};
}

#endif
