// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "PnPTool/Public/PnPSolverSubsystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePnPSolverSubsystem() {}

// Begin Cross Module References
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FIntPoint();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FTransform();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector2D();
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector2f();
ENGINE_API UClass* Z_Construct_UClass_AActor_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UWorldSubsystem();
ENGINE_API UEnum* Z_Construct_UEnum_Engine_ECollisionChannel();
PNPTOOL_API UClass* Z_Construct_UClass_UPnPSolverSubsystem();
PNPTOOL_API UClass* Z_Construct_UClass_UPnPSolverSubsystem_NoRegister();
PNPTOOL_API UEnum* Z_Construct_UEnum_PnPTool_EPnPMethod();
UPackage* Z_Construct_UPackage__Script_PnPTool();
// End Cross Module References

// Begin Enum EPnPMethod
static FEnumRegistrationInfo Z_Registration_Info_UEnum_EPnPMethod;
static UEnum* EPnPMethod_StaticEnum()
{
	if (!Z_Registration_Info_UEnum_EPnPMethod.OuterSingleton)
	{
		Z_Registration_Info_UEnum_EPnPMethod.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_PnPTool_EPnPMethod, (UObject*)Z_Construct_UPackage__Script_PnPTool(), TEXT("EPnPMethod"));
	}
	return Z_Registration_Info_UEnum_EPnPMethod.OuterSingleton;
}
template<> PNPTOOL_API UEnum* StaticEnum<EPnPMethod>()
{
	return EPnPMethod_StaticEnum();
}
struct Z_Construct_UEnum_PnPTool_EPnPMethod_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[] = {
		{ "AP3P.DisplayName", "AP3P (Algebraic P3P)" },
		{ "AP3P.Name", "EPnPMethod::AP3P" },
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** PnP \xe6\xb1\x82\xe8\xa7\xa3\xe7\xae\x97\xe6\xb3\x95\xe9\x80\x89\xe6\x8b\xa9 */" },
#endif
		{ "EPnP.DisplayName", "EPnP (Efficient Perspective-n-Point)" },
		{ "EPnP.Name", "EPnPMethod::EPnP" },
		{ "IPPE.DisplayName", "IPPE (Infinitesimal Plane-Based)" },
		{ "IPPE.Name", "EPnPMethod::IPPE" },
		{ "Iterative.DisplayName", "ITERATIVE (Levenberg-Marquardt)" },
		{ "Iterative.Name", "EPnPMethod::Iterative" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
		{ "P3P.DisplayName", "P3P (Perspective-3-Point)" },
		{ "P3P.Name", "EPnPMethod::P3P" },
		{ "SQPnP.DisplayName", "SQPnP (Sequential Quadratic PnP)" },
		{ "SQPnP.Name", "EPnPMethod::SQPnP" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "PnP \xe6\xb1\x82\xe8\xa7\xa3\xe7\xae\x97\xe6\xb3\x95\xe9\x80\x89\xe6\x8b\xa9" },
#endif
	};
#endif // WITH_METADATA
	static constexpr UECodeGen_Private::FEnumeratorParam Enumerators[] = {
		{ "EPnPMethod::Iterative", (int64)EPnPMethod::Iterative },
		{ "EPnPMethod::EPnP", (int64)EPnPMethod::EPnP },
		{ "EPnPMethod::P3P", (int64)EPnPMethod::P3P },
		{ "EPnPMethod::AP3P", (int64)EPnPMethod::AP3P },
		{ "EPnPMethod::IPPE", (int64)EPnPMethod::IPPE },
		{ "EPnPMethod::SQPnP", (int64)EPnPMethod::SQPnP },
	};
	static const UECodeGen_Private::FEnumParams EnumParams;
};
const UECodeGen_Private::FEnumParams Z_Construct_UEnum_PnPTool_EPnPMethod_Statics::EnumParams = {
	(UObject*(*)())Z_Construct_UPackage__Script_PnPTool,
	nullptr,
	"EPnPMethod",
	"EPnPMethod",
	Z_Construct_UEnum_PnPTool_EPnPMethod_Statics::Enumerators,
	RF_Public|RF_Transient|RF_MarkAsNative,
	UE_ARRAY_COUNT(Z_Construct_UEnum_PnPTool_EPnPMethod_Statics::Enumerators),
	EEnumFlags::None,
	(uint8)UEnum::ECppForm::EnumClass,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_PnPTool_EPnPMethod_Statics::Enum_MetaDataParams), Z_Construct_UEnum_PnPTool_EPnPMethod_Statics::Enum_MetaDataParams)
};
UEnum* Z_Construct_UEnum_PnPTool_EPnPMethod()
{
	if (!Z_Registration_Info_UEnum_EPnPMethod.InnerSingleton)
	{
		UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EPnPMethod.InnerSingleton, Z_Construct_UEnum_PnPTool_EPnPMethod_Statics::EnumParams);
	}
	return Z_Registration_Info_UEnum_EPnPMethod.InnerSingleton;
}
// End Enum EPnPMethod

// Begin Class UPnPSolverSubsystem Function AddPointPair
struct Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics
{
	struct PnPSolverSubsystem_eventAddPointPair_Parms
	{
		FVector ObjectPoint;
		FVector2D ImagePoint;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "PnP|Inputs" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe6\xb7\xbb\xe5\x8a\xa0\xe4\xb8\x80\xe5\xaf\xb9 3D-2D \xe7\x82\xb9 */" },
#endif
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe6\xb7\xbb\xe5\x8a\xa0\xe4\xb8\x80\xe5\xaf\xb9 3D-2D \xe7\x82\xb9" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_ObjectPoint;
	static const UECodeGen_Private::FStructPropertyParams NewProp_ImagePoint;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::NewProp_ObjectPoint = { "ObjectPoint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PnPSolverSubsystem_eventAddPointPair_Parms, ObjectPoint), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::NewProp_ImagePoint = { "ImagePoint", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PnPSolverSubsystem_eventAddPointPair_Parms, ImagePoint), Z_Construct_UScriptStruct_FVector2D, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::NewProp_ObjectPoint,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::NewProp_ImagePoint,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPnPSolverSubsystem, nullptr, "AddPointPair", nullptr, nullptr, Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::PnPSolverSubsystem_eventAddPointPair_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04820401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::PnPSolverSubsystem_eventAddPointPair_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UPnPSolverSubsystem::execAddPointPair)
{
	P_GET_STRUCT(FVector,Z_Param_ObjectPoint);
	P_GET_STRUCT(FVector2D,Z_Param_ImagePoint);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->AddPointPair(Z_Param_ObjectPoint,Z_Param_ImagePoint);
	P_NATIVE_END;
}
// End Class UPnPSolverSubsystem Function AddPointPair

// Begin Class UPnPSolverSubsystem Function ClearAllPoints
struct Z_Construct_UFunction_UPnPSolverSubsystem_ClearAllPoints_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "PnP|Actions" },
		{ "DisplayName", "Clear All Points" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPnPSolverSubsystem_ClearAllPoints_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPnPSolverSubsystem, nullptr, "ClearAllPoints", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_ClearAllPoints_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPnPSolverSubsystem_ClearAllPoints_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UPnPSolverSubsystem_ClearAllPoints()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPnPSolverSubsystem_ClearAllPoints_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UPnPSolverSubsystem::execClearAllPoints)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ClearAllPoints();
	P_NATIVE_END;
}
// End Class UPnPSolverSubsystem Function ClearAllPoints

// Begin Class UPnPSolverSubsystem Function CompareSolvedVsGroundTruth
struct Z_Construct_UFunction_UPnPSolverSubsystem_CompareSolvedVsGroundTruth_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "PnP|Actions" },
		{ "DisplayName", "Compare Solved vs Ground Truth" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPnPSolverSubsystem_CompareSolvedVsGroundTruth_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPnPSolverSubsystem, nullptr, "CompareSolvedVsGroundTruth", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_CompareSolvedVsGroundTruth_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPnPSolverSubsystem_CompareSolvedVsGroundTruth_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UPnPSolverSubsystem_CompareSolvedVsGroundTruth()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPnPSolverSubsystem_CompareSolvedVsGroundTruth_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UPnPSolverSubsystem::execCompareSolvedVsGroundTruth)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->CompareSolvedVsGroundTruth();
	P_NATIVE_END;
}
// End Class UPnPSolverSubsystem Function CompareSolvedVsGroundTruth

// Begin Class UPnPSolverSubsystem Function ComputeReprojectionError
struct Z_Construct_UFunction_UPnPSolverSubsystem_ComputeReprojectionError_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "PnP|Actions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe8\xae\xa1\xe7\xae\x97\xe9\x87\x8d\xe6\x8a\x95\xe5\xbd\xb1\xe8\xaf\xaf\xe5\xb7\xae\xef\xbc\x88\xe5\xb7\xb2\xe5\x9c\xa8 SolvePnP \xe5\x90\x8e\xe8\x87\xaa\xe5\x8a\xa8\xe8\xb0\x83\xe7\x94\xa8\xef\xbc\x8c\xe6\x97\xa0\xe9\x9c\x80\xe6\x89\x8b\xe5\x8a\xa8\xe7\x82\xb9\xe5\x87\xbb\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Compute Reprojection Error" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe8\xae\xa1\xe7\xae\x97\xe9\x87\x8d\xe6\x8a\x95\xe5\xbd\xb1\xe8\xaf\xaf\xe5\xb7\xae\xef\xbc\x88\xe5\xb7\xb2\xe5\x9c\xa8 SolvePnP \xe5\x90\x8e\xe8\x87\xaa\xe5\x8a\xa8\xe8\xb0\x83\xe7\x94\xa8\xef\xbc\x8c\xe6\x97\xa0\xe9\x9c\x80\xe6\x89\x8b\xe5\x8a\xa8\xe7\x82\xb9\xe5\x87\xbb\xef\xbc\x89" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPnPSolverSubsystem_ComputeReprojectionError_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPnPSolverSubsystem, nullptr, "ComputeReprojectionError", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_ComputeReprojectionError_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPnPSolverSubsystem_ComputeReprojectionError_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UPnPSolverSubsystem_ComputeReprojectionError()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPnPSolverSubsystem_ComputeReprojectionError_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UPnPSolverSubsystem::execComputeReprojectionError)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->ComputeReprojectionError();
	P_NATIVE_END;
}
// End Class UPnPSolverSubsystem Function ComputeReprojectionError

// Begin Class UPnPSolverSubsystem Function GeneratePointsByRaycast
struct Z_Construct_UFunction_UPnPSolverSubsystem_GeneratePointsByRaycast_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "PnP|Actions" },
		{ "DisplayName", "Raycast Grid to 3D Points" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPnPSolverSubsystem_GeneratePointsByRaycast_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPnPSolverSubsystem, nullptr, "GeneratePointsByRaycast", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_GeneratePointsByRaycast_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPnPSolverSubsystem_GeneratePointsByRaycast_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UPnPSolverSubsystem_GeneratePointsByRaycast()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPnPSolverSubsystem_GeneratePointsByRaycast_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UPnPSolverSubsystem::execGeneratePointsByRaycast)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->GeneratePointsByRaycast();
	P_NATIVE_END;
}
// End Class UPnPSolverSubsystem Function GeneratePointsByRaycast

// Begin Class UPnPSolverSubsystem Function GetIntrinsicsFromCamera
struct Z_Construct_UFunction_UPnPSolverSubsystem_GetIntrinsicsFromCamera_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "PnP|Actions" },
		{ "DisplayName", "Get Intrinsics from Camera" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPnPSolverSubsystem_GetIntrinsicsFromCamera_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPnPSolverSubsystem, nullptr, "GetIntrinsicsFromCamera", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_GetIntrinsicsFromCamera_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPnPSolverSubsystem_GetIntrinsicsFromCamera_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UPnPSolverSubsystem_GetIntrinsicsFromCamera()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPnPSolverSubsystem_GetIntrinsicsFromCamera_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UPnPSolverSubsystem::execGetIntrinsicsFromCamera)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->GetIntrinsicsFromCamera();
	P_NATIVE_END;
}
// End Class UPnPSolverSubsystem Function GetIntrinsicsFromCamera

// Begin Class UPnPSolverSubsystem Function RunFullPipeline
struct Z_Construct_UFunction_UPnPSolverSubsystem_RunFullPipeline_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "PnP|Actions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe4\xb8\x80\xe9\x94\xae\xe6\x89\xa7\xe8\xa1\x8c\xe5\xae\x8c\xe6\x95\xb4\xe6\xb5\x81\xe7\xa8\x8b\xef\xbc\x9a\xe8\x8e\xb7\xe5\x8f\x96\xe5\x86\x85\xe5\x8f\x82 \xe2\x86\x92 \xe5\xb0\x84\xe7\xba\xbf\xe6\x8a\x95\xe5\xb0\x84 \xe2\x86\x92 PnP \xe6\xb1\x82\xe8\xa7\xa3 \xe2\x86\x92 \xe8\x87\xaa\xe5\x8a\xa8\xe8\xae\xa1\xe7\xae\x97\xe9\x87\x8d\xe6\x8a\x95\xe5\xbd\xb1\xe8\xaf\xaf\xe5\xb7\xae */" },
#endif
		{ "DisplayName", "Run Full Pipeline" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe4\xb8\x80\xe9\x94\xae\xe6\x89\xa7\xe8\xa1\x8c\xe5\xae\x8c\xe6\x95\xb4\xe6\xb5\x81\xe7\xa8\x8b\xef\xbc\x9a\xe8\x8e\xb7\xe5\x8f\x96\xe5\x86\x85\xe5\x8f\x82 \xe2\x86\x92 \xe5\xb0\x84\xe7\xba\xbf\xe6\x8a\x95\xe5\xb0\x84 \xe2\x86\x92 PnP \xe6\xb1\x82\xe8\xa7\xa3 \xe2\x86\x92 \xe8\x87\xaa\xe5\x8a\xa8\xe8\xae\xa1\xe7\xae\x97\xe9\x87\x8d\xe6\x8a\x95\xe5\xbd\xb1\xe8\xaf\xaf\xe5\xb7\xae" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPnPSolverSubsystem_RunFullPipeline_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPnPSolverSubsystem, nullptr, "RunFullPipeline", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_RunFullPipeline_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPnPSolverSubsystem_RunFullPipeline_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UPnPSolverSubsystem_RunFullPipeline()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPnPSolverSubsystem_RunFullPipeline_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UPnPSolverSubsystem::execRunFullPipeline)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->RunFullPipeline();
	P_NATIVE_END;
}
// End Class UPnPSolverSubsystem Function RunFullPipeline

// Begin Class UPnPSolverSubsystem Function SolvePnP
struct Z_Construct_UFunction_UPnPSolverSubsystem_SolvePnP_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "PnP|Actions" },
		{ "DisplayName", "Solve PnP" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPnPSolverSubsystem_SolvePnP_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPnPSolverSubsystem, nullptr, "SolvePnP", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPnPSolverSubsystem_SolvePnP_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPnPSolverSubsystem_SolvePnP_Statics::Function_MetaDataParams) };
UFunction* Z_Construct_UFunction_UPnPSolverSubsystem_SolvePnP()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPnPSolverSubsystem_SolvePnP_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UPnPSolverSubsystem::execSolvePnP)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->SolvePnP();
	P_NATIVE_END;
}
// End Class UPnPSolverSubsystem Function SolvePnP

// Begin Class UPnPSolverSubsystem
void UPnPSolverSubsystem::StaticRegisterNativesUPnPSolverSubsystem()
{
	UClass* Class = UPnPSolverSubsystem::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "AddPointPair", &UPnPSolverSubsystem::execAddPointPair },
		{ "ClearAllPoints", &UPnPSolverSubsystem::execClearAllPoints },
		{ "CompareSolvedVsGroundTruth", &UPnPSolverSubsystem::execCompareSolvedVsGroundTruth },
		{ "ComputeReprojectionError", &UPnPSolverSubsystem::execComputeReprojectionError },
		{ "GeneratePointsByRaycast", &UPnPSolverSubsystem::execGeneratePointsByRaycast },
		{ "GetIntrinsicsFromCamera", &UPnPSolverSubsystem::execGetIntrinsicsFromCamera },
		{ "RunFullPipeline", &UPnPSolverSubsystem::execRunFullPipeline },
		{ "SolvePnP", &UPnPSolverSubsystem::execSolvePnP },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPnPSolverSubsystem);
UClass* Z_Construct_UClass_UPnPSolverSubsystem_NoRegister()
{
	return UPnPSolverSubsystem::StaticClass();
}
struct Z_Construct_UClass_UPnPSolverSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * EPnP \xe6\xb1\x82\xe8\xa7\xa3\xe5\x99\xa8 Subsystem\xef\xbc\x9a\xe8\xbe\x93\xe5\x85\xa5 3D-2D \xe7\x82\xb9\xe5\xaf\xb9\xe5\x92\x8c\xe7\x9b\xb8\xe6\x9c\xba\xe5\x86\x85\xe5\x8f\x82\xef\xbc\x8c\xe8\xbe\x93\xe5\x87\xba\xe7\x9b\xb8\xe6\x9c\xba\xe4\xbd\x8d\xe5\xa7\xbf\n * \xe7\x94\x9f\xe5\x91\xbd\xe5\x91\xa8\xe6\x9c\x9f\xe8\xb7\x9f\xe9\x9a\x8f World\xef\xbc\x8c\xe7\xbc\x96\xe8\xbe\x91\xe5\x99\xa8/\xe8\xbf\x90\xe8\xa1\x8c\xe6\x97\xb6/\xe8\x93\x9d\xe5\x9b\xbe\xe9\x80\x9a\xe7\x94\xa8\xe3\x80\x82\n * \xe8\x93\x9d\xe5\x9b\xbe\xe9\x80\x9a\xe8\xbf\x87 GetWorldSubsystem(UPnPSolverSubsystem) \xe8\x8e\xb7\xe5\x8f\x96\xe3\x80\x82\n */" },
#endif
		{ "IncludePath", "PnPSolverSubsystem.h" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "EPnP \xe6\xb1\x82\xe8\xa7\xa3\xe5\x99\xa8 Subsystem\xef\xbc\x9a\xe8\xbe\x93\xe5\x85\xa5 3D-2D \xe7\x82\xb9\xe5\xaf\xb9\xe5\x92\x8c\xe7\x9b\xb8\xe6\x9c\xba\xe5\x86\x85\xe5\x8f\x82\xef\xbc\x8c\xe8\xbe\x93\xe5\x87\xba\xe7\x9b\xb8\xe6\x9c\xba\xe4\xbd\x8d\xe5\xa7\xbf\n\xe7\x94\x9f\xe5\x91\xbd\xe5\x91\xa8\xe6\x9c\x9f\xe8\xb7\x9f\xe9\x9a\x8f World\xef\xbc\x8c\xe7\xbc\x96\xe8\xbe\x91\xe5\x99\xa8/\xe8\xbf\x90\xe8\xa1\x8c\xe6\x97\xb6/\xe8\x93\x9d\xe5\x9b\xbe\xe9\x80\x9a\xe7\x94\xa8\xe3\x80\x82\n\xe8\x93\x9d\xe5\x9b\xbe\xe9\x80\x9a\xe8\xbf\x87 GetWorldSubsystem(UPnPSolverSubsystem) \xe8\x8e\xb7\xe5\x8f\x96\xe3\x80\x82" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_SourceCameraActor_MetaData[] = {
		{ "Category", "PnP|Inputs|Camera" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe8\xa6\x81\xe8\x8e\xb7\xe5\x8f\x96\xe5\x86\x85\xe5\x8f\x82\xe7\x9a\x84\xe7\x9b\xb8\xe6\x9c\xba Actor\xef\xbc\x88""CameraActor \xe6\x88\x96 SceneCapture2D\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Source Camera Actor" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe8\xa6\x81\xe8\x8e\xb7\xe5\x8f\x96\xe5\x86\x85\xe5\x8f\x82\xe7\x9a\x84\xe7\x9b\xb8\xe6\x9c\xba Actor\xef\xbc\x88""CameraActor \xe6\x88\x96 SceneCapture2D\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_ImageResolution_MetaData[] = {
		{ "Category", "PnP|Inputs|Camera" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe5\x9b\xbe\xe5\x83\x8f\xe5\x88\x86\xe8\xbe\xa8\xe7\x8e\x87\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xef\xbc\x89\xef\xbc\x8c\xe5\xa6\x82 1920x1080 */" },
#endif
		{ "DisplayName", "Image Resolution (px)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\x9b\xbe\xe5\x83\x8f\xe5\x88\x86\xe8\xbe\xa8\xe7\x8e\x87\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xef\xbc\x89\xef\xbc\x8c\xe5\xa6\x82 1920x1080" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_RaycastMaxDistance_MetaData[] = {
		{ "Category", "PnP|Inputs|Raycast" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe5\xb0\x84\xe7\xba\xbf\xe6\x9c\x80\xe5\xa4\xa7\xe9\x95\xbf\xe5\xba\xa6\xef\xbc\x88""cm\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Raycast Max Distance (cm)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\xb0\x84\xe7\xba\xbf\xe6\x9c\x80\xe5\xa4\xa7\xe9\x95\xbf\xe5\xba\xa6\xef\xbc\x88""cm\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_RaycastChannel_MetaData[] = {
		{ "Category", "PnP|Inputs|Raycast" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe5\xb0\x84\xe7\xba\xbf\xe6\x8a\x95\xe5\xb0\x84\xe9\x80\x9a\xe9\x81\x93\xef\xbc\x88\xe9\xbb\x98\xe8\xae\xa4\xe5\x8f\xaf\xe8\xa7\x81\xe9\x80\x9a\xe9\x81\x93\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Raycast Channel" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\xb0\x84\xe7\xba\xbf\xe6\x8a\x95\xe5\xb0\x84\xe9\x80\x9a\xe9\x81\x93\xef\xbc\x88\xe9\xbb\x98\xe8\xae\xa4\xe5\x8f\xaf\xe8\xa7\x81\xe9\x80\x9a\xe9\x81\x93\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_GridRows_MetaData[] = {
		{ "Category", "PnP|Inputs|Raycast" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe7\xbd\x91\xe6\xa0\xbc\xe8\xa1\x8c\xe6\x95\xb0\xef\xbc\x88\xe6\x80\xbb\xe7\x82\xb9\xe6\x95\xb0 = (Rows+1)*(Cols+1)\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Grid Rows" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe7\xbd\x91\xe6\xa0\xbc\xe8\xa1\x8c\xe6\x95\xb0\xef\xbc\x88\xe6\x80\xbb\xe7\x82\xb9\xe6\x95\xb0 = (Rows+1)*(Cols+1)\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_GridCols_MetaData[] = {
		{ "Category", "PnP|Inputs|Raycast" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe7\xbd\x91\xe6\xa0\xbc\xe5\x88\x97\xe6\x95\xb0 */" },
#endif
		{ "DisplayName", "Grid Cols" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe7\xbd\x91\xe6\xa0\xbc\xe5\x88\x97\xe6\x95\xb0" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_ObjectPoints_MetaData[] = {
		{ "Category", "PnP|Inputs" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** 3D \xe7\x89\xa9\xe4\xbd\x93\xe7\x82\xb9\xef\xbc\x88UE \xe4\xb8\x96\xe7\x95\x8c\xe5\x9d\x90\xe6\xa0\x87\xe7\xb3\xbb\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "3D Object Points (UE)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "3D \xe7\x89\xa9\xe4\xbd\x93\xe7\x82\xb9\xef\xbc\x88UE \xe4\xb8\x96\xe7\x95\x8c\xe5\x9d\x90\xe6\xa0\x87\xe7\xb3\xbb\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_ImagePoints_MetaData[] = {
		{ "Category", "PnP|Inputs" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** 2D \xe5\x9b\xbe\xe5\x83\x8f\xe7\x82\xb9\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xe5\x9d\x90\xe6\xa0\x87\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "2D Image Points (px)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "2D \xe5\x9b\xbe\xe5\x83\x8f\xe7\x82\xb9\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xe5\x9d\x90\xe6\xa0\x87\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_FocalLength_MetaData[] = {
		{ "Category", "PnP|Inputs" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe7\x84\xa6\xe8\xb7\x9d\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xef\xbc\x89\xef\xbc\x9aX=fx, Y=fy */" },
#endif
		{ "DisplayName", "Focal Length (px)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe7\x84\xa6\xe8\xb7\x9d\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xef\xbc\x89\xef\xbc\x9aX=fx, Y=fy" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_ImageCenter_MetaData[] = {
		{ "Category", "PnP|Inputs" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe4\xb8\xbb\xe7\x82\xb9 / \xe5\x9b\xbe\xe5\x83\x8f\xe4\xb8\xad\xe5\xbf\x83\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xef\xbc\x89\xef\xbc\x9aX=cx, Y=cy */" },
#endif
		{ "DisplayName", "Image Center (px)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe4\xb8\xbb\xe7\x82\xb9 / \xe5\x9b\xbe\xe5\x83\x8f\xe4\xb8\xad\xe5\xbf\x83\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xef\xbc\x89\xef\xbc\x9aX=cx, Y=cy" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_DistortionCoefficients_MetaData[] = {
		{ "Category", "PnP|Inputs" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe7\x95\xb8\xe5\x8f\x98\xe7\xb3\xbb\xe6\x95\xb0 */" },
#endif
		{ "DisplayName", "Distortion Coefficients" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe7\x95\xb8\xe5\x8f\x98\xe7\xb3\xbb\xe6\x95\xb0" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_PnPMethod_MetaData[] = {
		{ "Category", "PnP|Inputs" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** PnP \xe6\xb1\x82\xe8\xa7\xa3\xe7\xae\x97\xe6\xb3\x95 */" },
#endif
		{ "DisplayName", "PnP Method" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "PnP \xe6\xb1\x82\xe8\xa7\xa3\xe7\xae\x97\xe6\xb3\x95" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_SolvedCameraPose_MetaData[] = {
		{ "Category", "PnP|Output" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe6\xb1\x82\xe8\xa7\xa3\xe5\xbe\x97\xe5\x88\xb0\xe7\x9a\x84\xe7\x9b\xb8\xe6\x9c\xba\xe4\xbd\x8d\xe5\xa7\xbf\xef\xbc\x88UE \xe5\x9d\x90\xe6\xa0\x87\xe7\xb3\xbb\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Solved Camera Pose (UE)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe6\xb1\x82\xe8\xa7\xa3\xe5\xbe\x97\xe5\x88\xb0\xe7\x9a\x84\xe7\x9b\xb8\xe6\x9c\xba\xe4\xbd\x8d\xe5\xa7\xbf\xef\xbc\x88UE \xe5\x9d\x90\xe6\xa0\x87\xe7\xb3\xbb\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_GroundTruthPose_MetaData[] = {
		{ "Category", "PnP|Output" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe7\x9c\x9f\xe5\xae\x9e\xe7\x9b\xb8\xe6\x9c\xba\xe4\xbd\x8d\xe5\xa7\xbf\xef\xbc\x88Ground Truth\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Ground Truth Camera Pose (UE)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe7\x9c\x9f\xe5\xae\x9e\xe7\x9b\xb8\xe6\x9c\xba\xe4\xbd\x8d\xe5\xa7\xbf\xef\xbc\x88Ground Truth\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_bLastSolveSuccess_MetaData[] = {
		{ "Category", "PnP|Output" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe4\xb8\x8a\xe6\xac\xa1\xe6\xb1\x82\xe8\xa7\xa3\xe6\x98\xaf\xe5\x90\xa6\xe6\x88\x90\xe5\x8a\x9f */" },
#endif
		{ "DisplayName", "Last Solve Success" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe4\xb8\x8a\xe6\xac\xa1\xe6\xb1\x82\xe8\xa7\xa3\xe6\x98\xaf\xe5\x90\xa6\xe6\x88\x90\xe5\x8a\x9f" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_ReprojectionError_MetaData[] = {
		{ "Category", "PnP|Output" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe9\x87\x8d\xe6\x8a\x95\xe5\xbd\xb1\xe8\xaf\xaf\xe5\xb7\xae\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xef\xbc\x8c\xe8\xb6\x8a\xe5\xb0\x8f\xe8\xb6\x8a\xe5\xa5\xbd\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Reprojection Error (px)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe9\x87\x8d\xe6\x8a\x95\xe5\xbd\xb1\xe8\xaf\xaf\xe5\xb7\xae\xef\xbc\x88\xe5\x83\x8f\xe7\xb4\xa0\xef\xbc\x8c\xe8\xb6\x8a\xe5\xb0\x8f\xe8\xb6\x8a\xe5\xa5\xbd\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_TranslationError_MetaData[] = {
		{ "Category", "PnP|Output" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe5\xb9\xb3\xe7\xa7\xbb\xe8\xaf\xaf\xe5\xb7\xae\xef\xbc\x88""cm\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Translation Error (cm)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe5\xb9\xb3\xe7\xa7\xbb\xe8\xaf\xaf\xe5\xb7\xae\xef\xbc\x88""cm\xef\xbc\x89" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_m_RotationError_MetaData[] = {
		{ "Category", "PnP|Output" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \xe6\x97\x8b\xe8\xbd\xac\xe8\xaf\xaf\xe5\xb7\xae\xef\xbc\x88\xe5\xba\xa6\xef\xbc\x89 */" },
#endif
		{ "DisplayName", "Rotation Error (deg)" },
		{ "ModuleRelativePath", "Public/PnPSolverSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "\xe6\x97\x8b\xe8\xbd\xac\xe8\xaf\xaf\xe5\xb7\xae\xef\xbc\x88\xe5\xba\xa6\xef\xbc\x89" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_m_SourceCameraActor;
	static const UECodeGen_Private::FStructPropertyParams NewProp_m_ImageResolution;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_m_RaycastMaxDistance;
	static const UECodeGen_Private::FBytePropertyParams NewProp_m_RaycastChannel;
	static const UECodeGen_Private::FIntPropertyParams NewProp_m_GridRows;
	static const UECodeGen_Private::FIntPropertyParams NewProp_m_GridCols;
	static const UECodeGen_Private::FStructPropertyParams NewProp_m_ObjectPoints_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_m_ObjectPoints;
	static const UECodeGen_Private::FStructPropertyParams NewProp_m_ImagePoints_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_m_ImagePoints;
	static const UECodeGen_Private::FStructPropertyParams NewProp_m_FocalLength;
	static const UECodeGen_Private::FStructPropertyParams NewProp_m_ImageCenter;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_m_DistortionCoefficients_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_m_DistortionCoefficients;
	static const UECodeGen_Private::FBytePropertyParams NewProp_m_PnPMethod_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_m_PnPMethod;
	static const UECodeGen_Private::FStructPropertyParams NewProp_m_SolvedCameraPose;
	static const UECodeGen_Private::FStructPropertyParams NewProp_m_GroundTruthPose;
	static void NewProp_m_bLastSolveSuccess_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_m_bLastSolveSuccess;
	static const UECodeGen_Private::FDoublePropertyParams NewProp_m_ReprojectionError;
	static const UECodeGen_Private::FDoublePropertyParams NewProp_m_TranslationError;
	static const UECodeGen_Private::FDoublePropertyParams NewProp_m_RotationError;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UPnPSolverSubsystem_AddPointPair, "AddPointPair" }, // 948924525
		{ &Z_Construct_UFunction_UPnPSolverSubsystem_ClearAllPoints, "ClearAllPoints" }, // 2068537333
		{ &Z_Construct_UFunction_UPnPSolverSubsystem_CompareSolvedVsGroundTruth, "CompareSolvedVsGroundTruth" }, // 3499148892
		{ &Z_Construct_UFunction_UPnPSolverSubsystem_ComputeReprojectionError, "ComputeReprojectionError" }, // 3928447680
		{ &Z_Construct_UFunction_UPnPSolverSubsystem_GeneratePointsByRaycast, "GeneratePointsByRaycast" }, // 2292478805
		{ &Z_Construct_UFunction_UPnPSolverSubsystem_GetIntrinsicsFromCamera, "GetIntrinsicsFromCamera" }, // 3596366901
		{ &Z_Construct_UFunction_UPnPSolverSubsystem_RunFullPipeline, "RunFullPipeline" }, // 1627786975
		{ &Z_Construct_UFunction_UPnPSolverSubsystem_SolvePnP, "SolvePnP" }, // 463548970
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPnPSolverSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_SourceCameraActor = { "m_SourceCameraActor", nullptr, (EPropertyFlags)0x0014000000000005, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_SourceCameraActor), Z_Construct_UClass_AActor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_SourceCameraActor_MetaData), NewProp_m_SourceCameraActor_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ImageResolution = { "m_ImageResolution", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_ImageResolution), Z_Construct_UScriptStruct_FIntPoint, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_ImageResolution_MetaData), NewProp_m_ImageResolution_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_RaycastMaxDistance = { "m_RaycastMaxDistance", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_RaycastMaxDistance), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_RaycastMaxDistance_MetaData), NewProp_m_RaycastMaxDistance_MetaData) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_RaycastChannel = { "m_RaycastChannel", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_RaycastChannel), Z_Construct_UEnum_Engine_ECollisionChannel, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_RaycastChannel_MetaData), NewProp_m_RaycastChannel_MetaData) }; // 756624936
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_GridRows = { "m_GridRows", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_GridRows), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_GridRows_MetaData), NewProp_m_GridRows_MetaData) };
const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_GridCols = { "m_GridCols", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_GridCols), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_GridCols_MetaData), NewProp_m_GridCols_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ObjectPoints_Inner = { "m_ObjectPoints", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ObjectPoints = { "m_ObjectPoints", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_ObjectPoints), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_ObjectPoints_MetaData), NewProp_m_ObjectPoints_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ImagePoints_Inner = { "m_ImagePoints", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FVector2f, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ImagePoints = { "m_ImagePoints", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_ImagePoints), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_ImagePoints_MetaData), NewProp_m_ImagePoints_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_FocalLength = { "m_FocalLength", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_FocalLength), Z_Construct_UScriptStruct_FVector2D, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_FocalLength_MetaData), NewProp_m_FocalLength_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ImageCenter = { "m_ImageCenter", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_ImageCenter), Z_Construct_UScriptStruct_FVector2D, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_ImageCenter_MetaData), NewProp_m_ImageCenter_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_DistortionCoefficients_Inner = { "m_DistortionCoefficients", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_DistortionCoefficients = { "m_DistortionCoefficients", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_DistortionCoefficients), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_DistortionCoefficients_MetaData), NewProp_m_DistortionCoefficients_MetaData) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_PnPMethod_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_PnPMethod = { "m_PnPMethod", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_PnPMethod), Z_Construct_UEnum_PnPTool_EPnPMethod, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_PnPMethod_MetaData), NewProp_m_PnPMethod_MetaData) }; // 979224680
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_SolvedCameraPose = { "m_SolvedCameraPose", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_SolvedCameraPose), Z_Construct_UScriptStruct_FTransform, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_SolvedCameraPose_MetaData), NewProp_m_SolvedCameraPose_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_GroundTruthPose = { "m_GroundTruthPose", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_GroundTruthPose), Z_Construct_UScriptStruct_FTransform, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_GroundTruthPose_MetaData), NewProp_m_GroundTruthPose_MetaData) };
void Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_bLastSolveSuccess_SetBit(void* Obj)
{
	((UPnPSolverSubsystem*)Obj)->m_bLastSolveSuccess = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_bLastSolveSuccess = { "m_bLastSolveSuccess", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UPnPSolverSubsystem), &Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_bLastSolveSuccess_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_bLastSolveSuccess_MetaData), NewProp_m_bLastSolveSuccess_MetaData) };
const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ReprojectionError = { "m_ReprojectionError", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_ReprojectionError), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_ReprojectionError_MetaData), NewProp_m_ReprojectionError_MetaData) };
const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_TranslationError = { "m_TranslationError", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_TranslationError), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_TranslationError_MetaData), NewProp_m_TranslationError_MetaData) };
const UECodeGen_Private::FDoublePropertyParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_RotationError = { "m_RotationError", nullptr, (EPropertyFlags)0x0010000000020015, UECodeGen_Private::EPropertyGenFlags::Double, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPnPSolverSubsystem, m_RotationError), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_m_RotationError_MetaData), NewProp_m_RotationError_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UPnPSolverSubsystem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_SourceCameraActor,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ImageResolution,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_RaycastMaxDistance,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_RaycastChannel,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_GridRows,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_GridCols,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ObjectPoints_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ObjectPoints,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ImagePoints_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ImagePoints,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_FocalLength,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ImageCenter,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_DistortionCoefficients_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_DistortionCoefficients,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_PnPMethod_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_PnPMethod,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_SolvedCameraPose,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_GroundTruthPose,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_bLastSolveSuccess,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_ReprojectionError,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_TranslationError,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPnPSolverSubsystem_Statics::NewProp_m_RotationError,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPnPSolverSubsystem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UPnPSolverSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UWorldSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_PnPTool,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPnPSolverSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UPnPSolverSubsystem_Statics::ClassParams = {
	&UPnPSolverSubsystem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UPnPSolverSubsystem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UPnPSolverSubsystem_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPnPSolverSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UPnPSolverSubsystem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UPnPSolverSubsystem()
{
	if (!Z_Registration_Info_UClass_UPnPSolverSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPnPSolverSubsystem.OuterSingleton, Z_Construct_UClass_UPnPSolverSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UPnPSolverSubsystem.OuterSingleton;
}
template<> PNPTOOL_API UClass* StaticClass<UPnPSolverSubsystem>()
{
	return UPnPSolverSubsystem::StaticClass();
}
UPnPSolverSubsystem::UPnPSolverSubsystem() {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UPnPSolverSubsystem);
UPnPSolverSubsystem::~UPnPSolverSubsystem() {}
// End Class UPnPSolverSubsystem

// Begin Registration
struct Z_CompiledInDeferFile_FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_Statics
{
	static constexpr FEnumRegisterCompiledInInfo EnumInfo[] = {
		{ EPnPMethod_StaticEnum, TEXT("EPnPMethod"), &Z_Registration_Info_UEnum_EPnPMethod, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 979224680U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UPnPSolverSubsystem, UPnPSolverSubsystem::StaticClass, TEXT("UPnPSolverSubsystem"), &Z_Registration_Info_UClass_UPnPSolverSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPnPSolverSubsystem), 1224810531U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_580222924(TEXT("/Script/PnPTool"),
	Z_CompiledInDeferFile_FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_Statics::ClassInfo),
	nullptr, 0,
	Z_CompiledInDeferFile_FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_Statics::EnumInfo));
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
