-- TypingLua UE5 Integration Test
-- Tests inline type annotations with UE5-style class names and patterns
-- Requires: UnLua IntelliSense stubs (or @class definitions below)

------------------------------------------------------------
-- Type Definitions (simulate UnLua IntelliSense stubs)
------------------------------------------------------------

---@class UObject
---@field GetClass fun(self: UObject): UClass
---@field GetName fun(self: UObject): string

---@class UClass : UObject

---@class AActor : UObject
---@field K2_GetActorLocation fun(self: AActor): FVector
---@field K2_SetActorLocation fun(self: AActor, NewLocation: FVector, bSweep: boolean): boolean
---@field GetOwner fun(self: AActor): AActor?
---@field Destroy fun(self: AActor): boolean

---@class APawn : AActor
---@field GetController fun(self: APawn): AController?

---@class ACharacter : APawn
---@field GetCharacterMovement fun(self: ACharacter): UCharacterMovementComponent

---@class AController : AActor

---@class APlayerController : AController

---@class UActorComponent : UObject
---@field GetOwner fun(self: UActorComponent): AActor

---@class UCharacterMovementComponent : UActorComponent
---@field MaxWalkSpeed number
---@field JumpZVelocity number

---@class UWidget : UObject
---@field SetVisibility fun(self: UWidget, InVisibility: integer)
---@field AddToViewport fun(self: UWidget, ZOrder: integer)

---@class UButton : UWidget
---@field OnClicked MulticastDelegate

---@class UTextBlock : UWidget
---@field SetText fun(self: UTextBlock, InText: string)

---@class FVector
---@field X number
---@field Y number
---@field Z number

---@class FRotator
---@field Pitch number
---@field Yaw number
---@field Roll number

---@class FTransform
---@field Translation FVector
---@field Rotation FRotator

---@class FHitResult
---@field bBlockingHit boolean
---@field Location FVector
---@field Normal FVector

---@class MulticastDelegate

---@class TArray<T>

---@class TMap<K, V>

---@class TSet<T>

---@class TSubclassOf<T>

------------------------------------------------------------
-- 1. Basic UE Type Annotations
------------------------------------------------------------

-- Simple UE class types
local actor: AActor = nil
local pawn: APawn = nil
local controller: APlayerController = nil

-- UE struct types
local location: FVector = nil
local rotation: FRotator = nil
local transform: FTransform = nil

-- UE component types
local movement: UCharacterMovementComponent = nil

------------------------------------------------------------
-- 2. UE Generic Container Types
------------------------------------------------------------

-- TArray with UE types
local enemies: TArray<AActor> = nil
local waypoints: TArray<FVector> = nil

-- TMap with UE types
local actorMap: TMap<string, AActor> = nil
local scoreBoard: TMap<string, number> = nil

-- TSet with UE types
local visitedActors: TSet<AActor> = nil

-- TSubclassOf
local actorClass: TSubclassOf<AActor> = nil

-- Nested generics
local groups: TMap<string, TArray<AActor>> = nil

------------------------------------------------------------
-- 3. UE Function Patterns (UnLua Override Style)
------------------------------------------------------------

-- Module table (UnLua pattern)
local M = {}

-- Override ReceiveBeginPlay
function M:ReceiveBeginPlay()
    local owner: AActor = self:GetOwner()
    local loc: FVector = owner:K2_GetActorLocation()
    print("Actor at: " .. tostring(loc.X))
end

-- Override ReceiveTick with typed parameter
function M:ReceiveTick(DeltaSeconds: number)
    local speed: number = 100.0
    local delta: FVector = { X = speed * DeltaSeconds, Y = 0, Z = 0 }
end

-- Function with UE return type
function M:GetSpawnLocation(): FVector
    return { X = 0, Y = 0, Z = 100 }
end

-- Function with multiple UE params
function M:SpawnProjectile(origin: FVector, direction: FRotator, speed: number): AActor
    return nil
end

------------------------------------------------------------
-- 4. Union Types with UE Classes
------------------------------------------------------------

local target: AActor|APawn = nil
local optionalActor: AActor? = nil
local result: FHitResult? = nil

------------------------------------------------------------
-- 5. Function Signature Types with UE Classes
------------------------------------------------------------

local onDamage: fun(target: AActor, amount: number, source: AActor): boolean = nil
local onSpawn: fun(actor: AActor) = nil

-- Callback with UE types
local tickCallback: fun(deltaTime: number) = function(dt)
    print("Tick: " .. dt)
end

------------------------------------------------------------
-- 6. Array Types with UE Classes
------------------------------------------------------------

local actorList: AActor[] = {}
local vectorList: FVector[] = {}
local optionalActors: AActor[]? = nil

------------------------------------------------------------
-- 7. Complex Nested Types (Real-World UE Patterns)
------------------------------------------------------------

-- Inventory system
local inventory: TMap<string, integer> = nil

-- Damage callback registry
local damageHandlers: TArray<fun(target: AActor, amount: number)> = nil

-- Multi-return with UE types
local function findNearestEnemy(origin: FVector): AActor?, number
    return nil, 0
end

------------------------------------------------------------
-- 8. Inline vs @type Conflict Test
------------------------------------------------------------

-- When both exist, inline should take priority
---@type string
local conflictTest: number = 42  -- inline `: number` should win

------------------------------------------------------------
-- 9. Method Self Type (UE Pattern)
------------------------------------------------------------

---@class BP_MyGameMode_C : AActor
---@field PlayerCount integer
---@field MaxPlayers integer
---@field GetPlayerCount fun(self: BP_MyGameMode_C): integer
---@field SetMaxPlayers fun(self: BP_MyGameMode_C, count: integer)
local BP_MyGameMode_C = {}

function BP_MyGameMode_C:GetPlayerCount(): integer
    return self.PlayerCount  -- self is BP_MyGameMode_C (via @class)
end

function BP_MyGameMode_C:SetMaxPlayers(count: integer)
    self.MaxPlayers = count  -- self is BP_MyGameMode_C (via @class)
end

-- Alternative UnLua pattern: file-level class binding
-- In real UE5 projects, UnLua binds self type by file path:
--   Content/Script/BP_MyCharacter_C.lua → self: BP_MyCharacter_C
---@class BP_MyCharacter_C : ACharacter
---@field Health number
---@field MaxHealth number
local BP_MyCharacter_C = {}

function BP_MyCharacter_C:ReceiveBeginPlay()
    -- self type is BP_MyCharacter_C, inherits ACharacter → APawn → AActor
    local loc: FVector = self:K2_GetActorLocation()
    self.Health = self.MaxHealth
end

function BP_MyCharacter_C:TakeDamage(amount: number, instigator: AActor)
    self.Health = self.Health - amount
    if self.Health <= 0 then
        self:Destroy()
    end
end

------------------------------------------------------------
-- 10. Error Cases (should produce diagnostics)
------------------------------------------------------------

-- Undefined type name should produce Error diagnostic
local badType: UndefinedType = nil
local badGeneric: TArray<NonExistentClass> = nil

return M
