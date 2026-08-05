// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "PnPSolverSubsystem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef PNPTOOL_PnPSolverSubsystem_generated_h
#error "PnPSolverSubsystem.generated.h already included, missing '#pragma once' in PnPSolverSubsystem.h"
#endif
#define PNPTOOL_PnPSolverSubsystem_generated_h

#define FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_34_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execAddPointPair); \
	DECLARE_FUNCTION(execComputeReprojectionError); \
	DECLARE_FUNCTION(execClearAllPoints); \
	DECLARE_FUNCTION(execCompareSolvedVsGroundTruth); \
	DECLARE_FUNCTION(execSolvePnP); \
	DECLARE_FUNCTION(execGeneratePointsByRaycast); \
	DECLARE_FUNCTION(execGetIntrinsicsFromCamera); \
	DECLARE_FUNCTION(execRunFullPipeline);


#define FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_34_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUPnPSolverSubsystem(); \
	friend struct Z_Construct_UClass_UPnPSolverSubsystem_Statics; \
public: \
	DECLARE_CLASS(UPnPSolverSubsystem, UWorldSubsystem, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/PnPTool"), NO_API) \
	DECLARE_SERIALIZER(UPnPSolverSubsystem)


#define FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_34_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UPnPSolverSubsystem(); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UPnPSolverSubsystem(UPnPSolverSubsystem&&); \
	UPnPSolverSubsystem(const UPnPSolverSubsystem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UPnPSolverSubsystem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UPnPSolverSubsystem); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UPnPSolverSubsystem) \
	NO_API virtual ~UPnPSolverSubsystem();


#define FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_31_PROLOG
#define FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_34_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_34_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_34_INCLASS_NO_PURE_DECLS \
	FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h_34_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> PNPTOOL_API UClass* StaticClass<class UPnPSolverSubsystem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_TestProject_MyProject_Plugins_PnPTool_Source_PnPTool_Public_PnPSolverSubsystem_h


#define FOREACH_ENUM_EPNPMETHOD(op) \
	op(EPnPMethod::Iterative) \
	op(EPnPMethod::EPnP) \
	op(EPnPMethod::P3P) \
	op(EPnPMethod::AP3P) \
	op(EPnPMethod::IPPE) \
	op(EPnPMethod::SQPnP) 

enum class EPnPMethod : uint8;
template<> struct TIsUEnumClass<EPnPMethod> { enum { Value = true }; };
template<> PNPTOOL_API UEnum* StaticEnum<EPnPMethod>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
