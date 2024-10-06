#include "BaseVSShader.h"

#include "ui_vs20.inc"
#include "ui_ps20b.inc"

BEGIN_VS_SHADER_FLAGS( UI, "", SHADER_NOT_EDITABLE )
	BEGIN_SHADER_PARAMS
	END_SHADER_PARAMS

	SHADER_INIT_PARAMS()
	{
		SET_FLAGS( MATERIAL_VAR_VERTEXCOLOR );
		SET_FLAGS( MATERIAL_VAR_VERTEXALPHA );
		SET_FLAGS( MATERIAL_VAR_TRANSLUCENT );
		SET_FLAGS( MATERIAL_VAR_NOCULL );
		SET_FLAGS( MATERIAL_VAR_IGNOREZ );
	}

	SHADER_INIT
	{
		if ( params[BASETEXTURE]->IsDefined() )
			LoadTexture( BASETEXTURE );
	}

	SHADER_FALLBACK
	{
		return NULL;
	}

	SHADER_DRAW
	{
		SHADOW_STATE
		{
			pShaderShadow->SetDefaultState();

			pShaderShadow->EnableCulling( false );
			pShaderShadow->EnableTexture( SHADER_SAMPLER0, true );

			int nTexCoordDimensions = 2;
			pShaderShadow->VertexShaderVertexFormat( VERTEX_POSITION3D | VERTEX_COLOR, 1, &nTexCoordDimensions, 0 );

			EnableAlphaBlending( SHADER_BLEND_SRC_ALPHA, SHADER_BLEND_ONE_MINUS_SRC_ALPHA );

			DECLARE_STATIC_VERTEX_SHADER( ui_vs20 );
			SET_STATIC_VERTEX_SHADER( ui_vs20 );

			DECLARE_STATIC_PIXEL_SHADER( ui_ps20b );
			SET_STATIC_PIXEL_SHADER( ui_ps20b );

			DisableFog();
		}

		DYNAMIC_STATE
		{
			pShaderAPI->SetDefaultState();

			if ( params[BASETEXTURE]->IsDefined() )
				BindTexture( SHADER_SAMPLER0, BASETEXTURE );
			else
				pShaderAPI->BindStandardTexture( SHADER_SAMPLER0, TEXTURE_WHITE );

			DECLARE_DYNAMIC_VERTEX_SHADER( ui_vs20 );
			SET_DYNAMIC_VERTEX_SHADER( ui_vs20 );

			DECLARE_DYNAMIC_PIXEL_SHADER( ui_ps20b );
			SET_DYNAMIC_PIXEL_SHADER( ui_ps20b );
		}

		Draw();
	}

END_SHADER
