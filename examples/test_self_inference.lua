-- Test: self type inference in UnLua pattern (cross-file with stubs)
-- This file simulates a real UE5 Lua script that references stub-defined types
-- The stubs directory provides @class definitions (like UnLua IntelliSense output)

------------------------------------------------------------
-- Pattern 1: @class on local table (standard LuaLS pattern)
------------------------------------------------------------

---@class BP_PlayerCharacter_C : ACharacter
---@field Health number
---@field MaxHealth number
---@field AttackPower number
---@field Level integer
local BP_PlayerCharacter_C = {}

-- self is inferred as BP_PlayerCharacter_C
function BP_PlayerCharacter_C:ReceiveBeginPlay()
    -- Access own fields (from @class above)
    self.Health = self.MaxHealth
    self.Level = 1

    -- Access inherited fields (ACharacter → APawn → AActor → UObject)
    local loc: FVector = self:K2_GetActorLocation()
    local name: string = self:GetName()
    local movement: UCharacterMovementComponent = self:GetCharacterMovement()
    movement.MaxWalkSpeed = 600
end

function BP_PlayerCharacter_C:ReceiveTick(DeltaSeconds: number)
    -- Inherited method from AActor
    local pos: FVector = self:K2_GetActorLocation()
end

function BP_PlayerCharacter_C:TakeDamage(amount: number, instigator: AActor): boolean
    self.Health = self.Health - amount
    if self.Health <= 0 then
        self:Destroy()
        return true
    end
    return false
end

function BP_PlayerCharacter_C:GetHealthPercent(): number
    return self.Health / self.MaxHealth
end

------------------------------------------------------------
-- Pattern 2: Separate class for manager/singleton
------------------------------------------------------------

---@class GameManager
---@field Players TArray<BP_PlayerCharacter_C>
---@field CurrentLevel integer
---@field IsGameOver boolean
local GameManager = {}

function GameManager:Initialize()
    self.CurrentLevel = 1
    self.IsGameOver = false
end

function GameManager:AddPlayer(player: BP_PlayerCharacter_C)
    self.Players:Add(player)
end

function GameManager:GetPlayerCount(): integer
    return self.Players:Num()
end

------------------------------------------------------------
-- Pattern 3: Free functions with UE types (utility modules)
------------------------------------------------------------

local Utils = {}

function Utils.CalculateDistance(a: FVector, b: FVector): number
    local dx: number = a.X - b.X
    local dy: number = a.Y - b.Y
    local dz: number = a.Z - b.Z
    return math.sqrt(dx*dx + dy*dy + dz*dz)
end

function Utils.FindNearestActor(origin: FVector, actors: TArray<AActor>): AActor?, number
    local nearest: AActor? = nil
    local minDist: number = math.huge

    local count: integer = actors:Num()
    for i = 1, count do
        local actor: AActor = actors:Get(i)
        local loc: FVector = actor:K2_GetActorLocation()
        local dist: number = Utils.CalculateDistance(origin, loc)
        if dist < minDist then
            minDist = dist
            nearest = actor
        end
    end

    return nearest, minDist
end

------------------------------------------------------------
-- Pattern 4: Delegate binding (common UE5 Lua pattern)
------------------------------------------------------------

---@class BP_MainMenu_C : AActor
---@field StartButton UButton
---@field TitleText UTextBlock
local BP_MainMenu_C = {}

function BP_MainMenu_C:ReceiveBeginPlay()
    -- Delegate binding
    self.StartButton.OnClicked:Add(self, self.OnStartClicked)
    self.TitleText:SetText("Welcome!")
end

function BP_MainMenu_C:OnStartClicked()
    self.StartButton:SetVisibility(2)  -- ESlateVisibility::Collapsed
end

return {
    BP_PlayerCharacter_C = BP_PlayerCharacter_C,
    GameManager = GameManager,
    Utils = Utils,
    BP_MainMenu_C = BP_MainMenu_C,
}
