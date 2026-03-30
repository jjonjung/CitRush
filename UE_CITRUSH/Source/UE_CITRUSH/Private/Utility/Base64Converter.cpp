// Fill out your copyright notice in the Description page of Project Settings.


#include "Utility/Base64Converter.h"

FString UBase64Converter::StringToBase64(const FString& inString)
{
	const FTCHARToUTF8 utf8Array(*inString);
	const uint8* unsignedCharData = reinterpret_cast<const uint8*>(utf8Array.Get());
	int32 numBytes = utf8Array.Length() - 1;
	return FBase64::Encode(unsignedCharData, numBytes);
}

FString UBase64Converter::Base64ToString(const FString& inBase64)
{
	TArray<uint8> decoded;
	FBase64::Decode(inBase64, decoded);
	const UTF8CHAR* utf8Data = reinterpret_cast<const UTF8CHAR*>(decoded.GetData());
	return FString(UTF8_TO_TCHAR(utf8Data));
}
