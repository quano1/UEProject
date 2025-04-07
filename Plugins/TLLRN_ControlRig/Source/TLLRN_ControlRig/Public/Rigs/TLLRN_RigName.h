// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct TLLRN_CONTROLRIG_API FTLLRN_RigName
{
public:
	FTLLRN_RigName()
	{}

	FTLLRN_RigName(const FTLLRN_RigName& InOther) = default;
	FTLLRN_RigName& operator =(const FTLLRN_RigName& InOther) = default;

	FTLLRN_RigName(const FName& InName)
		:FTLLRN_RigName()
	{
		SetFName(InName);
	}

	explicit FTLLRN_RigName(const FString& InNameString)
		:FTLLRN_RigName()
	{
		SetName(InNameString);
	}

	FTLLRN_RigName& operator =(const FName& InName)
	{
		SetFName(InName);
		return *this;
	}

	FTLLRN_RigName& operator =(const FString& InNameString)
	{
		SetName(InNameString);
		return *this;
	}

	operator FName() const
	{
		return GetFName();
	}
	
	bool IsValid() const
	{
		return Name.IsSet() || NameString.IsSet();
	}

	bool IsNone() const
	{
		if(!IsValid())
		{
			return true;
		}
		if(Name.IsSet())
		{
			return Name.GetValue().IsNone();
		}
		return GetName().IsEmpty();
	}

	bool operator ==(const FTLLRN_RigName& InOther) const
	{
		return Equals(InOther, ESearchCase::CaseSensitive);
	}

	bool Equals(const FTLLRN_RigName& InOther, const ESearchCase::Type& InCase) const
	{
		if(Name.IsSet() && InOther.Name.IsSet())
		{
			return GetFName().IsEqual(InOther.GetFName(), InCase == ESearchCase::CaseSensitive ? ENameCase::CaseSensitive : ENameCase::IgnoreCase);
		}
		
		if(NameString.IsSet() && InOther.NameString.IsSet())
		{
			return GetName().Equals(InOther.GetName(), InCase);
		}

		if(Name.IsSet() && InOther.NameString.IsSet())
		{
			return GetName().Equals(InOther.GetName(), InCase);
		}

		if(NameString.IsSet() && InOther.Name.IsSet())
		{
			return GetName().Equals(InOther.GetName(), InCase);
		}

		return IsNone() == InOther.IsNone();
	}

	bool operator !=(const FTLLRN_RigName& InOther) const
	{
		return !(*this == InOther);
	}

	int32 Len() const
	{
		if(NameString.IsSet())
		{
			return NameString->Len();
		}
		if(Name.IsSet())
		{
			return (int32)Name->GetStringLength();
		}
		return 0;
	}

	const FName& GetFName() const
	{
		if(Name.IsSet())
		{
			return Name.GetValue();
		}
		if(NameString.IsSet())
		{
			if(!NameString->IsEmpty())
			{
				Name = *NameString.GetValue();
				return Name.GetValue();
			}
		}
		return EmptyName;
	}

	const FString& GetName() const
	{
		if(NameString.IsSet())
		{
			return NameString.GetValue();
		}
		if(Name.IsSet())
		{
			NameString = FString();
			FString& ValueString = NameString.GetValue();
			Name.GetValue().ToString(ValueString);
			return ValueString;
		}
		return EmptyString;
	}

	const FString& ToString() const
	{
		return GetName();
	}

	void SetFName(const FName& InName)
	{
		if(Name.IsSet())
		{
			if(Name.GetValue().IsEqual(InName, ENameCase::CaseSensitive))
			{
				return;
			}
		}
		Name = InName;
		NameString.Reset();
	}

	void SetName(const FString& InNameString)
	{
		if(NameString.IsSet())
		{
			if(NameString.GetValue().Equals(InNameString, ESearchCase::CaseSensitive))
			{
				return;
			}
		}
		Name.Reset();
		NameString = InNameString;
	}

private:

	mutable TOptional<FName> Name;
	mutable TOptional<FString> NameString;

	static const FString EmptyString;
	static const FName EmptyName;
};
