// Copyright Epic Games, Inc. All Rights Reserved.

#include "PnPMarkerActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/StaticMesh.h"

APnPMarkerActor::APnPMarkerActor(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	MarkerMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MarkerMesh"));
	RootComponent = MarkerMesh;
	MarkerMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	// 默认使用引擎内建球体网格体，开箱即用
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMeshFinder(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMeshFinder.Succeeded())
	{
		MarkerMesh->SetStaticMesh(SphereMeshFinder.Object);
	}

	// 默认使用引擎内建 Material（无光照，纯色）
	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultMatFinder(TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
	if (DefaultMatFinder.Succeeded())
	{
		MarkerMesh->SetMaterial(0, DefaultMatFinder.Object);
	}
}

void APnPMarkerActor::SetMarkerColor(const FLinearColor& NewColor)
{
	if (!MarkerMesh) return;

	// 遍历所有材质，如果有动态材质实例则设置参数
	for (int32 i = 0; i < MarkerMesh->GetNumMaterials(); ++i)
	{
		UMaterialInterface* Mat = MarkerMesh->GetMaterial(i);
		if (!Mat) continue;

		UMaterialInstanceDynamic* MID = Cast<UMaterialInstanceDynamic>(Mat);
		if (!MID)
		{
			MID = MarkerMesh->CreateAndSetMaterialInstanceDynamic(i);
		}
		if (MID)
		{
			MID->SetVectorParameterValue(MaterialColorParameterName, NewColor);
		}
	}
}