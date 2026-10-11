#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "FrontendOptionsTabRow.generated.h"

USTRUCT(BlueprintType)
struct FFrontendOptionsTabRow : public FTableRowBase
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Options")
    FText DisplayName;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Frontend Options")
    int32 SortOrder = 0;
};