#include "UmbraSkillEditorLibrary.h"
#include "Sound/SoundCue.h"
#include "Sound/SoundWave.h"
#include "Sound/SoundNodeRandom.h"
#include "Sound/SoundNodeWavePlayer.h"
#include "EdGraph/EdGraphNode.h"

bool UUmbraSkillEditorLibrary::ConfigurePebbleImpactAudio(USoundCue* Cue, const TArray<USoundWave*>& Variants)
{
    if (!Cue || Variants.Num() != 3 || !Cue->GetPathName().StartsWith(TEXT("/Game/Skills/GlintstonePebble/"))) return false;
    for (auto* Wave : Variants) if (!Wave) return false;
    // Do not accumulate graph nodes when a configuration script is run again.
    if (Cue->FirstNode) return false;
    Cue->Modify();
    auto* Random = Cue->ConstructSoundNode<USoundNodeRandom>();
    Random->bRandomizeWithoutReplacement = true;
    TArray<USoundNode*> Children;
    for (auto* Wave : Variants)
    {
        auto* Player = Cue->ConstructSoundNode<USoundNodeWavePlayer>();
        Player->SetSoundWave(Wave);
        Player->bLooping = false;
        Children.Add(Player);
    }
    Random->SetChildNodes(Children);
    Random->Weights.Init(1.f, Variants.Num());
    Random->GraphNode->ReconstructNode();
    Cue->FirstNode = Random;
    Cue->VolumeMultiplier = 1.f;
    Cue->LinkGraphNodesFromSoundNodes();
    Cue->PostEditChange();
    Cue->MarkPackageDirty();
    return true;
}
