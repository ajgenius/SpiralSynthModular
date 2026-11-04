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

#ifndef SPIRALCORE_PROPERTY_H
#define SPIRALCORE_PROPERTY_H

#include <string>
#include <cstdio>
#include <vector>
#include <map>

#include "ChannelHandler.h"
#include "Types.h"
#include "Sample.h"
#include "Description.h"

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

	for device/gui thread safety, two methods must be called to synchronize
		property->Synchronize() synchronizes the shared and holding
		Device->SynchronizeProperty(property) calls
			property->SynchronizeLocal(), which synchronizes the local and holding

		both functions will use seperate booleans to handle who is holding.


		thus in a non-threaded use case,
			property->value = blah;

			if (property->IsReadOnly())
				if (Device->SynchronizeProperty(property))
					property->Synchronize();
			else
				if (property->Synchronize())
					Device->SynchronizeProperty(property))

	synth->SynchronizeProperty(property)
		if synth thread
			if (Device->SynchronizeProperty(property))
				SynchronizeProperty(property);

		if command thread
			if (property->Synchronize())
				SynchronizeProperty(property);

	Device->SynchronizeProperty(property)
	{
		return property->SynchronizeLocal();
	}


	this still won't work.. I am missing.. something... work brain work....


	whatever.. in the interim.

	keep things the hackish way as is, but bringing in the pattern concept from above.

	and implement a duplicate feature, to create a deep copy. (which we need anyway)

	when a gui creates an object, it will call device,
	to get a deep copy of everything .


	so 
		GUI
			device = new ...
			properties = device->SharedProperties()
*/
namespace Spiral
{
namespace Core
{
	class ComplexProperty;

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

		virtual Property *Duplicate()=0;
		virtual std::string ClassName()=0;

		virtual void CopyValue(const Property &CopyFrom)=0;

		virtual void RegisterChannel(ChannelHandler &channels, std::string Label) {}

		virtual ComplexProperty *AsComplex() { return NULL; }

		virtual bool Describe(Description &d) const { return false; }
		virtual bool Restore(Description::Reader &r) { return false; }

		inline bool IsReferenced() { return (m_References!=0); }
		inline void Reference() { m_References++; }
		inline void UnReference() { m_References--; }

		inline std::string GetKey() const { return m_Name; }
		inline std::string GetLabel() const { return m_Label; }
		inline std::string GetHint() { return m_Hint; }

		virtual void SetKey(std::string Key) { m_Name = Key; }
		inline void SetLabel(std::string Label) { m_Label = Label; }
		inline void SetHint(std::string Hint) { m_Hint = Hint; }

		inline bool IsReadOnly() { return m_ReadOnly; }

		inline ChannelHandler::Type ChannelDirection()
		{
			return m_ReadOnly ? ChannelHandler::OUTPUT : ChannelHandler::INPUT;
		}
		inline bool IsInternal() { return m_Internal; }
		inline bool RequiresUpdates() { return m_ForceUpdate; }

	protected:
		Property(SignedType Flags);

		void InvalidPropertyCopy(const Property& CopyFrom);

		bool m_ReadOnly, m_Internal, m_ForceUpdate;
	private:
		std::string m_Name, m_Label, m_Hint;
		int m_References;
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

	inline NumericPropertyValue DefaultBoolean(bool value)
	{
		NumericPropertyValue v;

		v.Type = NumericPropertyValue::Boolean;
		v.AsBoolean = value;

		return v;
	}

	inline NumericPropertyValue DefaultFloat(FloatType value)
	{
		NumericPropertyValue v;

		v.Type = NumericPropertyValue::Float;
		v.AsFloat = value;

		return v;
	}

	inline NumericPropertyValue DefaultSigned(SignedType value)
	{
		NumericPropertyValue v;

		v.Type = NumericPropertyValue::Signed;
		v.AsSigned = value;

		return v;
	}

	inline NumericPropertyValue DefaultUnsigned(UnsignedType value)
	{
		NumericPropertyValue v;

		v.Type = NumericPropertyValue::Unsigned;
		v.AsUnsigned = value;

		return v;
	}


	class PropertySet
	{
	public:
		PropertySet() {}
		PropertySet(std::vector<NumericPropertyValue> Values);
		PropertySet(const NumericPropertyValue Values[], UnsignedType Count);

		inline UnsignedType size() { return m_Values.size(); }

		inline NumericPropertyValue operator[](UnsignedType i) { return m_Values[i]; }

	protected:
		std::vector<NumericPropertyValue> m_Values;
	};

	typedef struct
	{
		const std::string Name;
		const NumericPropertyValue Value;
	} NamedPropertyValue;

	class NamedPropertySet : public PropertySet
	{
	public:
		#define NameArray(...) ((char*[]){__VA_ARGS__})

		NamedPropertySet() {}
		NamedPropertySet(std::vector<std::string> Names,std::vector<NumericPropertyValue> Values);

		NamedPropertySet(const NamedPropertyValue Values[], UnsignedType Count);

		inline std::string Names(UnsignedType i) { return m_Names[i]; }

		inline NumericPropertyValue operator[](std::string name)
		{		
			return ((PropertySet)(*this))[m_Index[name]];
		}
	private:
		std::vector<std::string> m_Names;
		std::map<std::string,SignedType> m_Index;
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

	#define DefaultLinearFlags (Spiral::Core::Property::WriteOnly \
		| Spiral::Core::LinearProperty::ClampLower \
		| Spiral::Core::LinearProperty::ClampUpper)
	#define DefaultLogarithmicFlags (DefaultLinearFlags | Spiral::Core::LinearProperty::Logarithmic)

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

		virtual Property *Duplicate()
		{
			return new SignedProperty(*this);
		}

		virtual void Clamp();

		virtual void RegisterChannel(ChannelHandler &channels, std::string Label)
		{
			channels.Register(Label, (int *)&Value.AsSigned, ChannelDirection());
		}

		virtual bool Describe(Description &d) const { d.Value(*this); return true; }
		virtual bool Restore(Description::Reader &r) { r.Value(*this); return true; }

		virtual std::string ClassName()
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

		virtual Property *Duplicate()
		{
			return new UnsignedProperty(*this);
		}


		virtual void Clamp();

		virtual void RegisterChannel(ChannelHandler &channels, std::string Label)
		{
			channels.Register(Label, (int *)&Value.AsUnsigned, ChannelDirection());
		}

		virtual bool Describe(Description &d) const { d.Value(*this); return true; }
		virtual bool Restore(Description::Reader &r) { r.Value(*this); return true; }

		virtual std::string ClassName()
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

		virtual Property *Duplicate()
		{
			return new FloatProperty(*this);
		}

		virtual void Clamp();
		virtual void RegisterChannel(ChannelHandler &channels, std::string Label)
		{
			channels.Register(Label, &Value.AsFloat, ChannelDirection());
		}

		virtual bool Describe(Description &d) const { d.Value(*this); return true; }
		virtual bool Restore(Description::Reader &r) { r.Value(*this); return true; }

		virtual std::string ClassName()
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

		virtual Property *Duplicate()
		{
			return new ToggleProperty(*this);
		}

		virtual void RegisterChannel(ChannelHandler &channels, std::string Label)
		{
			channels.Register(Label, &Value.AsBoolean, ChannelDirection());
		}

		virtual bool Describe(Description &d) const { d.Value(*this); return true; }
		virtual bool Restore(Description::Reader &r) { r.Value(*this); return true; }

		virtual std::string ClassName()
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

		virtual Property *Duplicate()
		{
			return new SetProperty(*this);
		}

		inline PropertySet Set() { return m_Set; }

		virtual void RegisterChannel(ChannelHandler &channels, std::string Label)
		{
			channels.Register(Label, (int *)&Value.AsUnsigned, ChannelDirection());
		}

		virtual bool Describe(Description &d) const { d.Value(*this); return true; }
		virtual bool Restore(Description::Reader &r) { r.Value(*this); return true; }

		virtual std::string ClassName()
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

		StringProperty(SignedType Flags, std::string InitialValue): Property(Flags) { Value = InitialValue; }
		StringProperty(SignedType Flags, const char *InitialValue): Property(Flags) { Value = InitialValue; }

		StringProperty(const StringProperty &Copy);

		virtual Property *Duplicate()
		{
			return new StringProperty(*this);
		}

		virtual void CopyValue(const Property &CopyFrom);

		virtual bool Describe(Description &d) const { d.Value(*this); return true; }
		virtual bool Restore(Description::Reader &r) { r.Value(*this); return true; }

		virtual std::string ClassName()
		{
			return "StringProperty";
		}

		std::string Value;
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

		virtual Property *Duplicate()
		{
			return new NoteProperty(*this);
		}


		virtual void CopyValue(const Property &CopyFrom);

		UnsignedType Note; /* 1-12, "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" */
		UnsignedType Octave; /* 0-6 */

		UnsignedType Start; /* 0-infinite */
		UnsignedType End; /* 0-infinite */

		FloatType Velocity; /* (0 - 1.0f)*127 */

		virtual std::string ClassName()
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

		virtual Property *Duplicate()
		{
			return new SampleProperty(*this);
		}

		virtual void CopyValue(const Property &CopyFrom);

		Sample Value;

		virtual std::string ClassName()
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

		virtual ComplexProperty *AsComplex() { return this; }
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
			Release();
		}

		VectorProperty(SignedType Flags): ComplexProperty(Flags) {}

		VectorProperty(SignedType Flags, std::vector<Property*> Values);

		VectorProperty(const VectorProperty &Copy);

		VectorProperty(const ComplexProperty &Copy);
		virtual Property *Duplicate()
		{
			return new VectorProperty(*this);
		}

		virtual void CopyValue(const Property &CopyFrom);

		virtual Property* operator[](UnsignedType i)
		{		
			return i < Values.size() ? Values[i] : NULL;
		}
		
		virtual UnsignedType size()
		{
			return Values.size();
		}

		virtual void Add(Property *New)
		{
			if (!New) return;

			New->Reference();
			Values.push_back(New);
			if (!GetKey().empty()) New->SetKey(ChildKey(Values.size() - 1));
		}

		virtual void SetKey(std::string Key)
		{
			ComplexProperty::SetKey(Key);
			for (UnsignedType i = 0; i < Values.size(); i++)
				if (Values[i]) Values[i]->SetKey(ChildKey(i));
		}

		virtual void RemoveLast()
		{
			if (Values.empty()) return;

			Property *Last = Values.back();
			Values.pop_back();

			if (!Last) return;

			Last->UnReference();

			if (!Last->IsReferenced()) delete Last;
		}

		virtual std::string ClassName()
		{
			return "VectorProperty";
		}

		std::vector<Property*> Values;

	private:
		std::string ChildKey(UnsignedType i)
		{
			char index[32];
			snprintf(index, sizeof index, "[%u]", (unsigned)i);
			return GetKey() + index;
		}
		void CopyContents(ComplexProperty &Source);
		void Release();
	};

	// A list of properties, under the class name the private tree's files
	// give one: the same vector, written as a ListProperty.
	class ListProperty : public VectorProperty
	{
	public:
		ListProperty() {}
		ListProperty(SignedType Flags): VectorProperty(Flags) {}
		ListProperty(const ListProperty &Copy): VectorProperty(Copy) {}

		virtual Property *Duplicate()
		{
			return new ListProperty(*this);
		}

		virtual std::string ClassName()
		{
			return "ListProperty";
		}
	};
}
}

#endif
