-- Simulated UnLua IntelliSense stub file
-- In real UE5 projects, these are auto-generated from C++ reflection data
-- and placed in a library path like Content/Script/Stubs/

---@meta

---@class UObject
---@field GetClass fun(self: UObject): UClass
---@field GetName fun(self: UObject): string
---@field IsA fun(self: UObject, Class: TSubclassOf<UObject>): boolean

---@class UClass : UObject

---@class AActor : UObject
---@field K2_GetActorLocation fun(self: AActor): FVector
---@field K2_SetActorLocation fun(self: AActor, NewLocation: FVector, bSweep: boolean): boolean
---@field GetOwner fun(self: AActor): AActor?
---@field Destroy fun(self: AActor): boolean
---@field SetLifeSpan fun(self: AActor, InLifespan: number)
---@field GetInstigator fun(self: AActor): APawn?

---@class APawn : AActor
---@field GetController fun(self: APawn): AController?
---@field GetMovementComponent fun(self: APawn): UMovementComponent?

---@class ACharacter : APawn
---@field GetCharacterMovement fun(self: ACharacter): UCharacterMovementComponent
---@field Jump fun(self: ACharacter)
---@field StopJumping fun(self: ACharacter)

---@class AController : AActor
---@field GetPawn fun(self: AController): APawn?

---@class APlayerController : AController
---@field GetHUD fun(self: APlayerController): AHUD?

---@class UActorComponent : UObject
---@field GetOwner fun(self: UActorComponent): AActor

---@class UMovementComponent : UActorComponent

---@class UCharacterMovementComponent : UMovementComponent
---@field MaxWalkSpeed number
---@field JumpZVelocity number
---@field GravityScale number

---@class UWidget : UObject
---@field SetVisibility fun(self: UWidget, InVisibility: integer)
---@field GetVisibility fun(self: UWidget): integer
---@field AddToViewport fun(self: UWidget, ZOrder: integer)
---@field RemoveFromParent fun(self: UWidget)

---@class UButton : UWidget
---@field OnClicked MulticastDelegate

---@class UTextBlock : UWidget
---@field SetText fun(self: UTextBlock, InText: string)
---@field GetText fun(self: UTextBlock): string

---@class AHUD : AActor

---@class FVector
---@field X number
---@field Y number
---@field Z number
---@operator add(FVector): FVector
---@operator sub(FVector): FVector
---@operator mul(number): FVector

---@class FRotator
---@field Pitch number
---@field Yaw number
---@field Roll number

---@class FTransform
---@field Translation FVector
---@field Rotation FRotator
---@field Scale3D FVector

---@class FHitResult
---@field bBlockingHit boolean
---@field Location FVector
---@field Normal FVector
---@field ImpactPoint FVector
---@field Distance number

---@class FLinearColor
---@field R number
---@field G number
---@field B number
---@field A number

---@class MulticastDelegate
---@field Add fun(self: MulticastDelegate, obj: table, func: function)
---@field Remove fun(self: MulticastDelegate, obj: table, func: function)
---@field Clear fun(self: MulticastDelegate)

---@class TArray<T>
---@field Num fun(self: TArray): integer
---@field Add fun(self: TArray, item: T)
---@field Get fun(self: TArray, index: integer): T
---@field Remove fun(self: TArray, index: integer)

---@class TMap<K, V>
---@field Add fun(self: TMap, key: K, value: V)
---@field Find fun(self: TMap, key: K): V?
---@field Remove fun(self: TMap, key: K)
---@field Num fun(self: TMap): integer

---@class TSet<T>
---@field Add fun(self: TSet, item: T)
---@field Contains fun(self: TSet, item: T): boolean
---@field Remove fun(self: TSet, item: T)
---@field Num fun(self: TSet): integer

---@class TSubclassOf<T>
