// Copyright Jayou, Inc. All Rights Reserved.

#pragma once

#define ENGINE_MODIFY 0

#define SHADER_PERMUTATION_OPTIMIZE ENGINE_MODIFY && 1

#define UE5_1 519
#define UE5_2 529
#define UE5_4_3 543
#define UE5_5_4 554

#define UE_VERSION ENGINE_MAJOR_VERSION*100+ENGINE_MINOR_VERSION*10+ENGINE_PATCH_VERSION

#if UE_VERSION >= UE5_4_3
	#define Get_PrimCompId(InComp) InComp->GetPrimitiveSceneId()
#else
	#define Get_PrimCompId(InComp) InComp->ComponentId
#endif
