

BG_ModelInfoForClient in bg_animation.c
    returns globalScriptData->modelInfo[globalScriptData->clientModels[client] - 1]

call tree

    G_RunFrame
        G_RunThink
            AIChar_spawn
                BG_GetAnimScriptEvent
                    BG_ModelInfoForClient

Question is what sets the modeInfo for a client

