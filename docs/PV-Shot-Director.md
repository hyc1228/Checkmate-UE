# PV Shot Director

`APVShotDirector` is a PV-only helper actor for capture maps. It does not replace gameplay systems; it exposes quick Blueprint/Sequencer triggers so PV shots can be staged without walking through the full game loop.

## Setup

1. Rebuild the project after pulling this branch.
2. In UE, create a Blueprint subclass: `BP_PVShotDirector` from `APVShotDirector`.
3. Place it in a PV capture map, e.g. `L_PV_Ch2_Capture`.
4. Optionally assign:
   - `ButtonEyeMaterial`
   - `MechanicalEyeMaterial`
   - `PuppetMesh`
   - `ExplosionFlashMesh`
   - `PVSequences`

## Actor Tags

Use these tags on hand-placed PV scene actors:

| Tag | Meaning |
|---|---|
| `PV_Shared` | Always visible |
| `PV_ButtonWorld` | Visible in button-eye / deprived world |
| `PV_MechanicalWorld` | Visible in mechanical-eye world |
| `PV_ColorWorld` | Visible for color/sensorium bloom |
| `PV_Destructible` | Hidden by `TriggerPuppetExplosionAt` if close to the blast |
| `PV_GameplayHUD` | Hidden by `HideGameplayHUD(true)` |

## Useful Calls

| Function | Use |
|---|---|
| `ApplyPVSceneState("ButtonWorld")` | Button-eye world |
| `ApplyPVSceneState("MechanicalWorld")` | Mechanical-eye world |
| `ApplyPVSceneState("ColorBloom")` | Color bloom world |
| `ForceEyeState(false)` | Force button/Pearl eye |
| `ForceEyeState(true)` | Force mechanical eye |
| `TriggerButtonFall()` | Hide button-world actors and reveal mechanical-world actors |
| `SpawnAndExplodePuppetForPV((5,4), 0.35)` | Spawn a PV puppet and explode it |
| `TriggerPuppetExplosionAt((5,4))` | Instant explosion flash + hide nearby `PV_Destructible` |
| `TriggerSensoriumBloom(2.0)` | Gradually color actors tagged `PV_ColorWorld` |
| `PlayPVSequence("B2")` | Play a registered LevelSequence |
| `PlayShot("B2")` | Play sequence if registered; otherwise use fallback behavior |

## Suggested PV Flow

For the current PV plan:

1. Factory stills / slow camera shots: no director needed.
2. Ch1 actual play: use existing Ch1 systems.
3. Reveal: use `PlayShot("B2")`, `PlayShot("B3")`, `PlayShot("B4")` once sequences exist.
4. Ch2 montage:
   - `ApplyPVSceneState("ButtonWorld")`
   - actor walks
   - `ApplyPVSceneState("MechanicalWorld")`
   - jump cut
   - `SpawnAndExplodePuppetForPV`
   - `ApplyPVSceneState("ColorBloom")`

