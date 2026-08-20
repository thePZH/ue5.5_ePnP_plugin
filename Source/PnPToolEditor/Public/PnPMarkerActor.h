// Copyright Epic Games, Inc. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PnPMarkerActor.generated.h"

class UStaticMeshComponent;

/**
 * PnP 标定 Marker 基类 Actor
 * 用户可以基于此派生蓝图子类，添加球体网格体和材质
 * 工具通过此类型识别标定 Marker，自动读取位置并修改颜色
 */
UCLASS(BlueprintType, ClassGroup = (PnP))
class PNPTOOLEDITOR_API APnPMarkerActor : public AActor
{
	GENERATED_BODY()
	
public:	
	APnPMarkerActor(const FObjectInitializer& ObjectInitializer);

public:
	/** Marker 网格体组件（通常是球体） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PnP|Marker")
	UStaticMeshComponent* MarkerMesh;

	/** 材质颜色参数名称（用于修改 Marker 显示颜色） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PnP|Marker")
	FName MaterialColorParameterName = FName("Color");

	/** 获取 Marker 的世界空间中心坐标（就是 Actor 位置） */
	FVector GetMarkerCenter() const { return GetActorLocation(); }

	/** 设置 Marker 颜色（通过材质参数修改） */
	UFUNCTION(BlueprintCallable, Category = "PnP|Marker")
	void SetMarkerColor(const FLinearColor& NewColor);
};