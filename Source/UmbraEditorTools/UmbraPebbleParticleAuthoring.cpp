#include "UmbraSkillEditorLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSpriteEmitter.h"
#include "Particles/ParticleLODLevel.h"
#include "Particles/ParticleModuleRequired.h"
#include "Particles/Spawn/ParticleModuleSpawn.h"
#include "Particles/Lifetime/ParticleModuleLifetime.h"
#include "Particles/Size/ParticleModuleSize.h"
#include "Particles/Velocity/ParticleModuleVelocity.h"
#include "Particles/Color/ParticleModuleColorOverLife.h"
#include "Distributions/DistributionFloatConstant.h"
#include "Distributions/DistributionFloatConstantCurve.h"
#include "Distributions/DistributionVectorConstant.h"
#include "Distributions/DistributionVectorUniform.h"

namespace
{
    void Constant(FRawDistributionFloat& Property, UObject* Outer, float Value)
    {
        auto* Distribution = NewObject<UDistributionFloatConstant>(Outer);
        Distribution->Constant = Value;
        Property.Distribution = Distribution;
    }
    void Constant(FRawDistributionVector& Property, UObject* Outer, FVector Value)
    {
        auto* Distribution = NewObject<UDistributionVectorConstant>(Outer);
        Distribution->Constant = Value;
        Property.Distribution = Distribution;
    }
    void AddEmitter(UParticleSystem* System, UMaterialInterface* Material, FName Name,
        bool bLoop, bool bLocal, float Size, float Life, float Rate, int32 Burst, float Spread, FVector Color)
    {
        auto* Emitter = NewObject<UParticleSpriteEmitter>(System, Name, RF_Transactional);
        System->Emitters.Add(Emitter);
        Emitter->CreateLODLevel(0);
        auto* LOD = Emitter->LODLevels[0].Get();
        auto* Required = LOD->RequiredModule.Get();
        Required->Material = Material;
        Required->bUseLocalSpace = bLocal;
        Required->EmitterDuration = .5f;
        Required->EmitterLoops = bLoop ? 0 : 1;
        Required->ScreenAlignment = PSA_Square;
        Required->bKillOnDeactivate = false;
        Constant(LOD->SpawnModule->Rate, System, Rate);
        LOD->SpawnModule->BurstList.Reset();
        if (Burst > 0)
        {
            FParticleBurst Entry;
            Entry.Count = Burst;
            Entry.CountLow = Burst;
            Entry.Time = 0.f;
            LOD->SpawnModule->BurstList.Add(Entry);
        }
        auto* Lifetime = NewObject<UParticleModuleLifetime>(System);
        Constant(Lifetime->Lifetime, System, Life);
        auto* ParticleSize = NewObject<UParticleModuleSize>(System);
        Constant(ParticleSize->StartSize, System, FVector(Size));
        auto* Velocity = NewObject<UParticleModuleVelocity>(System);
        auto* RandomVelocity = NewObject<UDistributionVectorUniform>(System);
        RandomVelocity->Min = FVector(-Spread);
        RandomVelocity->Max = FVector(Spread);
        Velocity->StartVelocity.Distribution = RandomVelocity;
        Constant(Velocity->StartVelocityRadial, System, 0.f);
        auto* Tint = NewObject<UParticleModuleColorOverLife>(System);
        Constant(Tint->ColorOverLife, System, Color);
        auto* Alpha = NewObject<UDistributionFloatConstantCurve>(System);
        Alpha->ConstantCurve.AddPoint(0.f, 1.f);
        Alpha->ConstantCurve.AddPoint(.25f, .85f);
        Alpha->ConstantCurve.AddPoint(1.f, 0.f);
        Tint->AlphaOverLife.Distribution = Alpha;
        LOD->Modules = {Lifetime, ParticleSize, Velocity, Tint};
        for (const auto& Module : LOD->Modules) { Module->LODValidity = 1; Module->bEnabled = true; }
        Emitter->UpdateModuleLists();
    }
}

bool UUmbraSkillEditorLibrary::ConfigurePebbleParticles(UParticleSystem* System,
    UMaterialInterface* GlowMaterial, UMaterialInterface* NoiseMaterial, int32 Stage)
{
    if (!System || !GlowMaterial || !NoiseMaterial || Stage < 0 || Stage > 3
        || !System->GetPathName().StartsWith(TEXT("/Game/Skills/GlintstonePebble/"))) return false;
    System->Modify();
    System->Emitters.Reset();
    const FVector Blue(.12f, 1.8f, 5.f);
    const FVector White(.9f, 3.f, 5.f);
    if (Stage <= 1)
    {
        AddEmitter(System, GlowMaterial, TEXT("Core"), true, true, Stage == 0 ? 22.f : 18.f, .06f, 70.f, 1, 0.f, White);
        AddEmitter(System, NoiseMaterial, TEXT("Magic"), true, Stage == 0, Stage == 0 ? 28.f : 17.f, .2f, 100.f, 2, 12.f, Blue);
        AddEmitter(System, GlowMaterial, TEXT("Motes"), true, false, 4.f, .3f, 45.f, 0, 60.f, Blue);
    }
    else
    {
        AddEmitter(System, GlowMaterial, TEXT("Flash"), false, false, Stage == 2 ? 65.f : 35.f, .18f, 0.f, 1, 0.f, White);
        AddEmitter(System, NoiseMaterial, TEXT("Cloud"), false, false, 35.f, .35f, 0.f, 4, 45.f, Blue);
        AddEmitter(System, GlowMaterial, TEXT("Shards"), false, false, 6.f, .45f, 0.f, Stage == 2 ? 24 : 10, 180.f, White);
    }
    System->LODSettings.SetNum(1);
    System->LODDistances = {0.f};
    System->PostEditChange();
    System->MarkPackageDirty();
    return true;
}
