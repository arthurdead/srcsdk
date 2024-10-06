#include "rmlui.h"
#include <vgui_controls/Controls.h>
#include <vgui/ILocalize.h>
#include <vgui/ISurface.h>
#include <vgui/ISystem.h>
#include <vgui/IScheme.h>
#include <tier0/dbg.h>
#include "engineinterface.h"
#include "filesystem.h"
#include "materialsystem/imesh.h"
#include "materialsystem/itexture.h"
#include "materialsystem/imaterial.h"
#include "materialsystem/imaterialvar.h"
#include "tier1/generichash.h"
#include "tier1/callqueue.h"
#include "bitmap/tgaloader.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

DEFINE_LOGGING_CHANNEL_NO_TAGS( LOG_Rml, "Rml" );

RmlFileInterface g_RmlFileInterface;
RmlSystemInterface g_RmlSystemInterface;
RmlRenderInterface g_RmlRenderInterface;

double RmlSystemInterface::GetElapsedTime()
{
	return engine->Time();
}

int RmlSystemInterface::TranslateString(Rml::String &translated, const Rml::String &input)
{
	translated = input;
	return 0;
}

void RmlSystemInterface::JoinPath(Rml::String &translated_path, const Rml::String &document_path, const Rml::String &path)
{
	Rml::SystemInterface::JoinPath(translated_path, document_path, path);
}

bool RmlSystemInterface::LogMessage(Rml::Log::Type type, const Rml::String &message)
{
	switch(type) {
	case Rml::Log::LT_ALWAYS:
		Log_Msg( LOG_Rml, "%s\n", message.c_str() );
		break;
	default:
	case Rml::Log::LT_ERROR:
		Log_Error( LOG_Rml, "%s\n", message.c_str() );
		break;
	case Rml::Log::LT_ASSERT: {
		LoggingResponse_t _ret = Log_Assert( "%s\n", message.c_str() );
		CallAssertFailedNotifyFunc( __TFILE__, __LINE__, message.c_str() );
		if ( _ret == LR_DEBUGGER)
		{
			if ( !ShouldUseNewAssertDialog() || DoNewAssertDialog( __TFILE__, __LINE__, message.c_str() ) )
			{
				return true;
			}
			_ExitOnFatalAssert( __TFILE__, __LINE__ );
		}
	} break;
	case Rml::Log::LT_WARNING:
		Log_Warning( LOG_Rml, "%s\n", message.c_str() );
		break;
	case Rml::Log::LT_INFO:
		Log_Msg( LOG_Rml, "%s\n", message.c_str() );
		break;
	case Rml::Log::LT_DEBUG:
		Log_Warning( LOG_Rml, "%s\n", message.c_str() );
		break;
	}

	return false;
}

void RmlSystemInterface::SetMouseCursor(const Rml::String& cursor_name)
{
}

void RmlSystemInterface::SetClipboardText(const Rml::String& text)
{
	wchar_t text_uni[1024];
	int len = g_pVGuiLocalize->ConvertANSIToUnicode( text.c_str(), text_uni, sizeof(text_uni) );

	vgui::system()->SetClipboardText( text_uni, len );
}

void RmlSystemInterface::GetClipboardText(Rml::String& text)
{
	wchar_t text_uni[1024];
	vgui::system()->GetClipboardText( 0, text_uni, sizeof(text_uni) );
	char text_ansi[1024];
	int len = g_pVGuiLocalize->ConvertUnicodeToANSI( text_uni, text_ansi, sizeof(text_ansi) );
	text.assign(text_ansi, len);
}

void RmlSystemInterface::ActivateKeyboard(Rml::Vector2f caret_position, float line_height)
{
}

void RmlSystemInterface::DeactivateKeyboard()
{
}

Rml::FileHandle RmlFileInterface::Open(const Rml::String &path)
{
	return (Rml::FileHandle)g_pFullFileSystem->Open( path.c_str(), "r", "GAME" );
}

void RmlFileInterface::Close(Rml::FileHandle file)
{
	g_pFullFileSystem->Close( (FileHandle_t)file );
}

size_t RmlFileInterface::Read(void *buffer, size_t size, Rml::FileHandle file)
{
	return g_pFullFileSystem->Read( buffer, size, (FileHandle_t)file );
}

bool RmlFileInterface::Seek(Rml::FileHandle file, long offset, int origin)
{
	g_pFullFileSystem->Seek( (FileHandle_t)file, offset, (FileSystemSeek_t)origin );
	return true;
}

size_t RmlFileInterface::Tell(Rml::FileHandle file)
{
	return g_pFullFileSystem->Tell( (FileHandle_t)file );
}

size_t RmlFileInterface::Length(Rml::FileHandle file)
{
	return g_pFullFileSystem->Size( (FileHandle_t)file );
}

bool RmlFileInterface::LoadFile(const Rml::String& path, Rml::String& out_data)
{
	FileHandle_t file = g_pFullFileSystem->Open( path.c_str(), "r", "GAME" );
	if(!file)
		return false;

	CUtlBuffer buf;
	bool read = g_pFullFileSystem->ReadToBuffer( file, buf );
	g_pFullFileSystem->Close(file);
	if(!read) {
		return false;
	}

	out_data.assign((char *)buf.Base(), buf.TellMaxPut());
	return true;
}

#if 0
void RmlFontInterface::Initialize()
{
}

void RmlFontInterface::Shutdown()
{
}

bool RmlFontInterface::LoadFontFace(const Rml::String& file_name, bool fallback_face, Rml::Style::FontWeight weight)
{
	return vgui::surface()->AddCustomFontFile(NULL, file_name.c_str());
}

bool RmlFontInterface::LoadFontFace(Rml::Span<const Rml::byte> data, const Rml::String& family, Rml::Style::FontStyle style, Rml::Style::FontWeight weight, bool fallback_face)
{
	#define TEMP_FONT_FILENAME "tempfont.ttf"

	if(g_pFullFileSystem->FileExists(TEMP_FONT_FILENAME, "DEFAULT_WRITE_PATH"))
		g_pFullFileSystem->RemoveFile(TEMP_FONT_FILENAME, "DEFAULT_WRITE_PATH");

	CUtlBuffer buf;
	buf.Put(data.data(), data.size());

	if(!g_pFullFileSystem->WriteFile(TEMP_FONT_FILENAME, "DEFAULT_WRITE_PATH", buf)) {
		g_pFullFileSystem->RemoveFile(TEMP_FONT_FILENAME, "DEFAULT_WRITE_PATH");
		return false;
	}

	char path[MAX_PATH];
	g_pFullFileSystem->RelativePathToFullPath(TEMP_FONT_FILENAME, "DEFAULT_WRITE_PATH", path, sizeof(path));

	bool added = vgui::surface()->AddCustomFontFile(NULL, path);
	g_pFullFileSystem->RemoveFile(TEMP_FONT_FILENAME, "DEFAULT_WRITE_PATH");

	return added;
}

Rml::FontFaceHandle RmlFontInterface::GetFontFaceHandle(const Rml::String& family, Rml::Style::FontStyle style, Rml::Style::FontWeight weight, int size)
{
	vgui::HScheme def_schem_hndl = vgui::scheme()->GetDefaultScheme();
	if(def_schem_hndl == vgui::INVALID_SCHEME)
		return (Rml::FontFaceHandle)0;

	vgui::IScheme *def_schem_inter = vgui::scheme()->GetIScheme( def_schem_hndl );
	if(def_schem_inter == NULL)
		return (Rml::FontFaceHandle)0;

	return (Rml::FontFaceHandle)def_schem_inter->GetFont( family.c_str() );
}
#endif

RmlRenderInterface::RmlRenderInterface()
{
	m_pMaterial = NULL;
	m_rectScissors.MakeInvalid();
	m_bScissorsEnabled = false;
	m_bRendering = false;
}

#if 0
#define RML_SHADER "UI"
#define RML_VERTEX_FMT (VERTEX_POSITION3D|VERTEX_COLOR|VERTEX_TEXCOORD_SIZE(0,2))
#else
#define RML_SHADER "UnlitGeneric"
#define RML_VERTEX_FMT (VERTEX_POSITION3D|VERTEX_NORMAL|VERTEX_COLOR|VERTEX_TEXCOORD_SIZE(0,2))
#endif

KeyValues *RmlRenderInterface::CreateMaterial()
{
	KeyValues *pVMTKeyValues = new KeyValues( RML_SHADER );
	pVMTKeyValues->SetInt( "$vertexcolor", 1 );
	pVMTKeyValues->SetInt( "$vertexalpha", 1 );
	pVMTKeyValues->SetInt( "$ignorez", 1 );
	pVMTKeyValues->SetInt( "$translucent", 1 );
	pVMTKeyValues->SetInt( "$nocull", 1 );
	return pVMTKeyValues;
}

void RmlRenderInterface::Initialize()
{
	m_pRenderContext = g_pMaterialSystem->GetRenderContext();
	m_pRenderContext->AddRef();

	KeyValues *pVMTKeyValues = CreateMaterial();
	pVMTKeyValues->SetString( "$basetexture", "error" );
	m_pMaterial = g_pMaterialSystem->CreateMaterial( "Rml_material_default", pVMTKeyValues );
	m_pMaterial->Refresh();
}

void RmlRenderInterface::BeginRender(int x, int y, int wide, int tall)
{
	m_pRenderContext->BeginRender();

	m_bRendering = true;

	m_pRenderContext->Viewport(x, y, wide, tall);

	m_pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	m_pRenderContext->PushMatrix();
	m_pRenderContext->LoadIdentity();

	m_pRenderContext->Scale( 1, -1, 1 );
	m_pRenderContext->Ortho( 0, 0, wide, tall, -1, 1 );

	m_pRenderContext->MatrixMode( MATERIAL_VIEW );
	m_pRenderContext->PushMatrix();
	m_pRenderContext->LoadIdentity();
}

void RmlRenderInterface::EndRender()
{
	m_pRenderContext->SetScissorRect( -1, -1, -1, -1, false );
	m_bScissorsEnabled = false;
	m_rectScissors.MakeInvalid();

	m_pRenderContext->MatrixMode( MATERIAL_VIEW );
	m_pRenderContext->PopMatrix();

	m_pRenderContext->MatrixMode( MATERIAL_PROJECTION );
	m_pRenderContext->PopMatrix();

	m_pRenderContext->EndRender();

	m_bRendering = false;
}

Rml::CompiledGeometryHandle RmlRenderInterface::CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices)
{
	RmlMesh_t *info = new RmlMesh_t;

	info->vertexCount = vertices.size();
	info->indexCount = indices.size();

	info->vertexBuffer = m_pRenderContext->CreateStaticVertexBuffer( RML_VERTEX_FMT, vertices.size(), TEXTURE_GROUP_VGUI );
	CVertexBuilder vertexBuilder;
	vertexBuilder.Begin( info->vertexBuffer, vertices.size() );
	for(const auto &it : vertices) {
		vertexBuilder.Position3f( it.position.x, it.position.y, 0 );
	#if RML_VERTEX_FMT & VERTEX_NORMAL
		vertexBuilder.Normal3f( 0, 0, 0 );
	#endif
		vertexBuilder.TexCoord2fv( 0, it.tex_coord );
		vertexBuilder.Color4ubv( it.colour );
	#if RML_VERTEX_FMT & VERTEX_NORMAL
		vertexBuilder.AdvanceVertexF<VTX_HAVEPOS|VTX_HAVENORMAL|VTX_HAVECOLOR, 1>();
	#else
		vertexBuilder.AdvanceVertexF<VTX_HAVEPOS|VTX_HAVECOLOR, 1>();
	#endif
	}
	vertexBuilder.End();

	info->indexBuffer = m_pRenderContext->CreateStaticIndexBuffer( MATERIAL_INDEX_FORMAT_16BIT, indices.size(), TEXTURE_GROUP_VGUI );
	CIndexBuilder indexBuilder;
	indexBuilder.Begin( info->indexBuffer, indices.size(), 0 );
	for(int it : indices) {
		indexBuilder.FastIndex(it);
	}
	indexBuilder.End();

	return (Rml::CompiledGeometryHandle)info;
}

void RmlRenderInterface::RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture_handle)
{
	auto info = (RmlMesh_t *)geometry;

	Assert( m_bRendering );

	m_pRenderContext->MatrixMode( MATERIAL_MODEL );
	m_pRenderContext->PushMatrix();
	m_pRenderContext->LoadIdentity();

	IMaterial *material = NULL;

	if(texture_handle != (Rml::TextureHandle)0) {
		material = ((RmlTexture_t *)texture_handle)->material;
	} else {
		material = m_pMaterial;
	}

	m_pRenderContext->Translate(translation.x, translation.y, 0);
	m_pRenderContext->Bind( material, NULL );
	m_pRenderContext->BindVertexBuffer( 0, info->vertexBuffer, 0, 0, info->vertexCount, RML_VERTEX_FMT, 1 );
	m_pRenderContext->BindIndexBuffer( info->indexBuffer, 0 );
	m_pRenderContext->Draw( MATERIAL_TRIANGLES, 0, info->indexCount );

	m_pRenderContext->MatrixMode( MATERIAL_MODEL );
	m_pRenderContext->PopMatrix();
}

void RmlRenderInterface::ReleaseGeometry(Rml::CompiledGeometryHandle geometry)
{
	auto info = (RmlMesh_t *)geometry;

	m_pRenderContext->DestroyIndexBuffer(info->indexBuffer);
	m_pRenderContext->DestroyVertexBuffer(info->vertexBuffer);

	delete info;
}

Rml::TextureHandle RmlRenderInterface::LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source)
{
	using namespace std::literals::string_view_literals;

	RmlTexture_t *texture = new RmlTexture_t;

	texture->texture = NULL;

	//TODO!!!! re-structure this
	if(source.ends_with(".tga"sv)) {
		char path[MAX_PATH];
		g_pFullFileSystem->RelativePathToFullPath(source.c_str(), "GAME", path, sizeof(path));
		CUtlMemory<unsigned char> buf;
		if(TGALoader::LoadRGBA8888(path, buf, texture_dimensions.x, texture_dimensions.y)) {
			texture->texture = g_pMaterialSystem->CreateNamedTextureFromBitsEx(
				source.c_str(), TEXTURE_GROUP_VGUI, texture_dimensions.x, texture_dimensions.y, 1, IMAGE_FORMAT_RGBA8888, buf.Count(), (const byte *)buf.Base(), TEXTUREFLAGS_NOLOD|TEXTUREFLAGS_NOMIP);
		}
	} else if(source.ends_with(".vtf"sv)) {
		texture->texture = g_pMaterialSystem->FindTexture(source.c_str(), TEXTURE_GROUP_VGUI);
	} else if(source.ends_with(".vmt"sv)) {
		IMaterial *tempMat = g_pMaterialSystem->FindMaterial(source.c_str(), TEXTURE_GROUP_VGUI);
		if(tempMat) {
			bool found = false;
			IMaterialVar *basetex = tempMat->FindVar("$basetexture", &found);
			if(basetex && found) {
				texture->texture = g_pMaterialSystem->FindTexture(basetex->GetStringValue(), TEXTURE_GROUP_VGUI);
			}
		}
	} else {
		char path[MAX_PATH];
		V_strncpy(path, source.c_str(), source.length());
		int len = V_strlen(path);
		V_strncat(path, ".vtf", sizeof(path));
		if(g_pFullFileSystem->FileExists(path, "GAME")) {
			texture->texture = g_pMaterialSystem->FindTexture(source.c_str(), TEXTURE_GROUP_VGUI);
		} else {
			path[len] = '\0';
			V_strncat(path, ".vmt", sizeof(path));
			if(g_pFullFileSystem->FileExists(path, "GAME")) {
				IMaterial *tempMat = g_pMaterialSystem->FindMaterial(source.c_str(), TEXTURE_GROUP_VGUI);
				if(tempMat) {
					bool found = false;
					IMaterialVar *basetex = tempMat->FindVar("$basetexture", &found);
					if(basetex && found) {
						texture->texture = g_pMaterialSystem->FindTexture(basetex->GetStringValue(), TEXTURE_GROUP_VGUI);
					}
				}
			} else {
				path[len] = '\0';
				V_strncat(path, ".tga", sizeof(path));
				if(g_pFullFileSystem->FileExists(path, "GAME")) {
					char path2[MAX_PATH];
					g_pFullFileSystem->RelativePathToFullPath(path, "GAME", path2, sizeof(path2));
					CUtlMemory<unsigned char> buf;
					if(TGALoader::LoadRGBA8888(path2, buf, texture_dimensions.x, texture_dimensions.y)) {
						texture->texture = g_pMaterialSystem->CreateNamedTextureFromBitsEx(
							source.c_str(), TEXTURE_GROUP_VGUI, texture_dimensions.x, texture_dimensions.y, 1, IMAGE_FORMAT_RGBA8888, buf.Count(), (const byte *)buf.Base(), TEXTUREFLAGS_NOLOD|TEXTUREFLAGS_NOMIP);
					}
				}
			}
		}
	}

	if(!texture->texture) {
		texture->texture = g_pMaterialSystem->FindTexture("error", TEXTURE_GROUP_VGUI);
	}

	texture->texture->IncrementReferenceCount();

	KeyValues *pVMTKeyValues = CreateMaterial();
	pVMTKeyValues->SetString( "$basetexture", source.c_str() );

	char name[256];
	V_sprintf_safe(name, "Rml_material_%i", HashBlock(source.data(), source.size()));

	texture->material = g_pMaterialSystem->CreateMaterial( name, pVMTKeyValues );
	texture->material->Refresh();

	texture_dimensions.x = texture->texture->GetActualWidth();
	texture_dimensions.y = texture->texture->GetActualHeight();

	Msg("%s\n", source.c_str());

	return (Rml::TextureHandle)texture;
}

Rml::TextureHandle RmlRenderInterface::GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions)
{
	unsigned int id = HashBlock(source.data(), source.size());

	char name[256];
	V_sprintf_safe(name, "Rml_texture_%u", id);

	RmlTexture_t *texture = new RmlTexture_t;

	texture->texture = g_pMaterialSystem->CreateNamedTextureFromBitsEx(
		name, TEXTURE_GROUP_VGUI, source_dimensions.x, source_dimensions.y, 1, IMAGE_FORMAT_RGBA8888, source.size(), (const byte *)source.data(), TEXTUREFLAGS_NOLOD|TEXTUREFLAGS_NOMIP);

	KeyValues *pVMTKeyValues = CreateMaterial();
	pVMTKeyValues->SetString( "$basetexture", name );

	V_sprintf_safe(name, "Rml_material_%u", id);

	texture->material = g_pMaterialSystem->CreateMaterial( name, pVMTKeyValues );
	texture->material->Refresh();

	Msg("%s\n", texture->texture->GetName());

	return (Rml::TextureHandle)texture;
}

void RmlRenderInterface::ReleaseTexture(Rml::TextureHandle handle)
{
	if(handle == (Rml::TextureHandle)0)
		return;

	RmlTexture_t *texture = (RmlTexture_t *)handle;

	texture->material->DecrementReferenceCount();
	texture->texture->DecrementReferenceCount();

	texture->material->DeleteIfUnreferenced();
	texture->texture->DeleteIfUnreferenced();

	delete texture;
}

void RmlRenderInterface::EnableScissorRegion(bool enable)
{
	m_bScissorsEnabled = enable;

	Assert( m_bRendering );

	if(enable && m_rectScissors.Valid()) {
		m_pRenderContext->SetScissorRect( m_rectScissors.Left(), m_rectScissors.Top(), m_rectScissors.Right(), m_rectScissors.Bottom(), true );
	} else {
		m_pRenderContext->SetScissorRect( -1, -1, -1, -1, false );
	}
}

void RmlRenderInterface::SetScissorRegion(Rml::Rectanglei region)
{
	m_rectScissors = region;

	Assert( m_bRendering );

	if(m_bScissorsEnabled && region.Valid()) {
		m_pRenderContext->SetScissorRect( m_rectScissors.Left(), m_rectScissors.Top(), m_rectScissors.Right(), m_rectScissors.Bottom(), true );
	} else {
		m_pRenderContext->SetScissorRect( -1, -1, -1, -1, false );
	}
}

void RmlRenderInterface::EnableClipMask(bool enable)
{

}

void RmlRenderInterface::RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation)
{

}

void RmlRenderInterface::SetTransform(const Rml::Matrix4f* transform)
{

}

Rml::LayerHandle RmlRenderInterface::PushLayer()
{
	return (Rml::LayerHandle)0;
}

void RmlRenderInterface::CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode, Rml::Span<const Rml::CompiledFilterHandle> filters)
{

}

void RmlRenderInterface::PopLayer()
{

}

Rml::TextureHandle RmlRenderInterface::SaveLayerAsTexture()
{
	return (Rml::TextureHandle)0;
}

Rml::CompiledFilterHandle RmlRenderInterface::SaveLayerAsMaskImage()
{
	return (Rml::CompiledFilterHandle)0;
}

Rml::CompiledFilterHandle RmlRenderInterface::CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters)
{
	return (Rml::CompiledFilterHandle)0;
}

void RmlRenderInterface::ReleaseFilter(Rml::CompiledFilterHandle filter)
{

}

Rml::CompiledShaderHandle RmlRenderInterface::CompileShader(const Rml::String& name, const Rml::Dictionary& parameters)
{
	return (Rml::CompiledShaderHandle)0;
}

void RmlRenderInterface::RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture)
{

}

void RmlRenderInterface::ReleaseShader(Rml::CompiledShaderHandle shader)
{

}