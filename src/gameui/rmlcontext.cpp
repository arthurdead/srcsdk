#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include <RmlUi/Core/Factory.h>
#include "rmlcontext.h"
#include "engineinterface.h"
#include <vgui_controls/Controls.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "tier1/KeyValues.h"
#include "gameui.h"
#include "rmlui.h"
#include "tier1/utlmap.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar rml_context_update_rate("rml_context_update_rate", "1");

CON_COMMAND(rml_list_contexts, "")
{
	int count = Rml::GetNumContexts();
	for(int i = 0; i < count; ++i) {
		RmlContext *ctx = (RmlContext *)Rml::GetContext(i);
		vgui::VPANEL vpanel = ctx->GetVPanel();

		int x, y;
		vgui::ipanel()->GetPos(vpanel, x, y);
		int w, h;
		vgui::ipanel()->GetSize(vpanel, w, h);

		Rml::Vector2i size = ctx->GetDimensions();

		Msg("%s - %i, %i : [%ix%i] | [%ix%i]", ctx->Rml::Context::GetName().c_str(), x, y, w, h, size.x, size.y);
	}
}

CON_COMMAND(rml_list_doc, "")
{
	if(args.ArgC() != 2) {
		Msg("rml_list_doc <context>\n");
		return;
	}

	RmlContext *ctx = (RmlContext *)Rml::GetContext(args.Arg(1));
	if(!ctx) {
		Msg("context %s not found\n", args.Arg(1));
		return;
	}

	int count = ctx->GetNumDocuments();
	for(int i = 0; i < count; ++i) {
		Rml::ElementDocument *doc = ctx->GetDocument(i);
		Msg("%s - %s\n", doc->GetTitle().c_str(), doc->GetSourceURL().c_str());
	}
}

CON_COMMAND(rml_load_doc, "")
{
	if(args.ArgC() != 3) {
		Msg("rml_load_doc <context> <doc>\n");
		return;
	}

	RmlContext *ctx = (RmlContext *)Rml::GetContext(args.Arg(1));
	if(!ctx) {
		Msg("context %s not found\n", args.Arg(1));
		return;
	}

	Rml::ElementDocument *doc = ctx->LoadDocument(args.Arg(2));
	if(!doc) {
		Msg("failed to load %s\n", args.Arg(2));
		return;
	}

	doc->Show();

	ctx->Update();
}

CON_COMMAND(rml_unload_all_docs, "")
{
	if(args.ArgC() != 2) {
		Msg("rml_unload_all_docs <context>\n");
		return;
	}

	RmlContext *ctx = (RmlContext *)Rml::GetContext(args.Arg(1));
	if(!ctx) {
		Msg("context %s not found\n", args.Arg(1));
		return;
	}

	int count = ctx->GetNumDocuments();
	for(int i = 0; i < count; ++i) {
		Rml::ElementDocument *doc = ctx->GetDocument(i);
		doc->Close();
	}

	ctx->UnloadAllDocuments();

	ctx->Update();
}

CON_COMMAND(rml_create_context, "")
{
	if(args.ArgC() == 6) {
		if(Rml::GetContext(args.Arg(1))) {
			Msg("name already in-use\n");
			return;
		}

		int w = V_atoi(args.Arg(4));
		int h = V_atoi(args.Arg(5));

		RmlContext *ctx = (RmlContext *)Rml::CreateContext(args.Arg(1), Rml::Vector2i(w, h), &g_RmlRenderInterface, NULL);
		if(ctx) {
			vgui::VPANEL vpanel = ctx->GetVPanel();
			vgui::ipanel()->SetPos(vpanel, V_atoi(args.Arg(2)), V_atoi(args.Arg(3)));
			vgui::ipanel()->SetSize(vpanel, w, h);
		} else {
			Msg("failed to create context\n");
		}
	} else if(args.ArgC() == 4) {
		if(Rml::GetContext(args.Arg(1))) {
			Msg("name already in-use\n");
			return;
		}

		int w = V_atoi(args.Arg(2));
		int h = V_atoi(args.Arg(3));

		RmlContext *ctx = (RmlContext *)Rml::CreateContext(args.Arg(1), Rml::Vector2i(w, h), &g_RmlRenderInterface, NULL);
		if(ctx) {
			vgui::VPANEL vpanel = ctx->GetVPanel();
			vgui::ipanel()->SetSize(vpanel, w, h);
		} else {
			Msg("failed to create context\n");
		}
	} else {
		Msg("rml_create_context <name> <x> <y> <w> <h>\n");
		Msg("rml_create_context <name> <w> <h>\n");
		return;
	}
}

CON_COMMAND(rml_resize_context, "")
{
	if(args.ArgC() != 4) {
		Msg("rml_resize_context <context> <w> <h>\n");
		return;
	}

	RmlContext *ctx = (RmlContext *)Rml::GetContext(args.Arg(1));
	if(!ctx) {
		Msg("context %s not found\n", args.Arg(1));
		return;
	}

	vgui::VPANEL vpanel = ctx->GetVPanel();

	vgui::ipanel()->SetSize( vpanel, V_atoi(args.Arg(2)), V_atoi(args.Arg(3)) );
}

CON_COMMAND(rml_move_context, "")
{
	if(args.ArgC() != 4) {
		Msg("rml_delete_context <context> <x> <y>\n");
		return;
	}

	RmlContext *ctx = (RmlContext *)Rml::GetContext(args.Arg(1));
	if(!ctx) {
		Msg("context %s not found\n", args.Arg(1));
		return;
	}

	vgui::VPANEL vpanel = ctx->GetVPanel();

	vgui::ipanel()->SetPos( vpanel, V_atoi(args.Arg(2)), V_atoi(args.Arg(3)) );
}

CON_COMMAND(rml_delete_context, "")
{
	if(args.ArgC() != 2) {
		Msg("rml_delete_context <context>\n");
		return;
	}

	RmlContext *ctx = (RmlContext *)Rml::GetContext(args.Arg(1));
	if(!ctx) {
		Msg("context %s not found\n", args.Arg(1));
		return;
	}

	vgui::VPANEL vpanel = ctx->GetVPanel();

	vgui::ivgui()->MarkPanelForDeletion( vpanel );
}

CON_COMMAND(rml_reload_all, "")
{
	CUtlMap<RmlContext *, CUtlVector<Rml::String>> load_later(0, 0, DefLessFunc(RmlContext *));

	int ctx_count = Rml::GetNumContexts();
	for(int i = 0; i < ctx_count; ++i) {
		RmlContext *ctx = (RmlContext *)Rml::GetContext(i);

		auto k = load_later.Insert(ctx);

		int doc_count = ctx->GetNumDocuments();
		for(int j = 0; j < doc_count; ++j) {
			Rml::ElementDocument *doc = ctx->GetDocument(j);
			load_later[k].AddToTail(doc->GetSourceURL());
			doc->Close();
		}

		ctx->UnloadAllDocuments();
	}

	Rml::Factory::ClearStyleSheetCache();
	Rml::Factory::ClearTemplateCache();
	Rml::ReleaseTextures(&g_RmlRenderInterface);

	for(int i = 0; i < ctx_count; ++i) {
		RmlContext *ctx = (RmlContext *)Rml::GetContext(i);

		ctx->Update();
	}

	for(int i = load_later.FirstInorder(); i != load_later.InvalidIndex(); i = load_later.NextInorder(i)) {
		RmlContext *ctx = (RmlContext *)load_later.Key(i);

		const auto &docs = load_later.Element(i);
		for(int j = 0; j < docs.Count(); ++j) {
			Rml::ElementDocument *doc = ctx->LoadDocument(docs[j]);
			if(doc) {
				doc->Show();
			}
		}

		ctx->Update();
	}
}

CON_COMMAND(rml_reload_all_stylesheets, "")
{
	int ctx_count = Rml::GetNumContexts();
	for(int i = 0; i < ctx_count; ++i) {
		RmlContext *ctx = (RmlContext *)Rml::GetContext(i);

		int doc_count = ctx->GetNumDocuments();
		for(int j = 0; j < doc_count; ++j) {
			Rml::ElementDocument *doc = ctx->GetDocument(j);
			doc->ReloadStyleSheet();
		}
	}

	Rml::Factory::ClearStyleSheetCache();

	for(int i = 0; i < ctx_count; ++i) {
		RmlContext *ctx = (RmlContext *)Rml::GetContext(i);

		ctx->Update();
	}
}

CON_COMMAND(rml_unload_texture, "")
{
	if(args.ArgC() != 2) {
		Msg("rml_unload_texture <path>\n");
		return;
	}

	if(Rml::ReleaseTexture(args.Arg(1), &g_RmlRenderInterface)) {
		int ctx_count = Rml::GetNumContexts();
		for(int i = 0; i < ctx_count; ++i) {
			RmlContext *ctx = (RmlContext *)Rml::GetContext(i);
			ctx->Update();
		}
	} else {
		Msg("texture %s not found\n", args.Arg(1));
	}
}

CON_COMMAND(rml_unload_all_textures, "")
{
	Rml::ReleaseTextures(&g_RmlRenderInterface);

	int ctx_count = Rml::GetNumContexts();
	for(int i = 0; i < ctx_count; ++i) {
		RmlContext *ctx = (RmlContext *)Rml::GetContext(i);
		ctx->Update();
	}
}

RmlContext::RmlContext(const Rml::String& name, Rml::RenderManager* render_manager, Rml::TextInputHandler* text_input_handler)
	: Rml::Context(name, render_manager, text_input_handler)
{
	SetDensityIndependentPixelRatio( vgui::scheme()->GetProportionalScaledValue( 100 ) / 250.f );

	EnableMouseCursor( true );

	m_VPanel = vgui::ivgui()->AllocPanel();

	Assert(m_VPanel != vgui::INVALID_VPANEL);

	vgui::ipanel()->Init(m_VPanel, this);
	vgui::ipanel()->SetParent(m_VPanel, g_GameUI.GetVPanel());

	vgui::ipanel()->SetPos( m_VPanel, 0, 0 );
	vgui::ipanel()->SetSize( m_VPanel, 640, 480 );
	vgui::ipanel()->SetVisible( m_VPanel, true );
	vgui::ipanel()->SetMouseInputEnabled( m_VPanel, true );
	vgui::ipanel()->SetKeyBoardInputEnabled( m_VPanel, true );
}

RmlContext::~RmlContext()
{
	if(m_VPanel != vgui::INVALID_VPANEL) {
		vgui::ivgui()->FreePanel(m_VPanel);
	}
}

void RmlContext::DeletePanel()
{
	Rml::RemoveContext(Rml::Context::GetName());
}

void RmlContext::OnTick()
{

}

void RmlContext::Think()
{
}

const char *RmlContext::GetName()
{
	return Rml::Context::GetName().c_str();
}

const char *RmlContext::GetModuleName()
{
	return vgui::GetControlsModuleName();
}

void *RmlContext::QueryInterface(vgui::EInterfaceID id)
{
	if (id == vgui::ICLIENTPANEL_STANDARD_INTERFACE)
		return static_cast<IClientPanel *>(this);

	return NULL;
}

bool RmlContext::RequestInfo(KeyValues *outputData)
{
	if(V_strcmp(outputData->GetName(), "alpha") == 0) {
		outputData->SetInt("alpha", 255);
		return true;
	}
	return false;
}

void RmlContext::GetClipRect(int &x0, int &y0, int &x1, int &y1)
{
	vgui::ipanel()->GetClipRect(m_VPanel, x0, y0, x1, y1);
}

void RmlContext::OnSizeChanged(int newWide, int newTall)
{
	SetDimensions(Rml::Vector2i(newWide, newTall));

	Update();
}

void RmlContext::OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel)
{
	const char *title = params->GetName();
	if(V_strcmp(title, "CursorMoved") == 0) {
		ProcessMouseMove(params->GetInt("xpos"), params->GetInt("ypos"), 0);
	} else if(V_strcmp(title, "CursorEntered") == 0) {
		
	} else if(V_strcmp(title, "CursorExited") == 0) {
		ProcessMouseLeave();
	} else if(V_strcmp(title, "MousePressed") == 0) {
		
	} else if(V_strcmp(title, "MouseReleased") == 0) {
		
	} else if(V_strcmp(title, "MouseDoublePressed") == 0) {
		
	} else if(V_strcmp(title, "Delete") == 0) {
		DeletePanel();
	} else {
		static int s_bDebugMessages = -1;
		if ( s_bDebugMessages == -1 )
		{
			s_bDebugMessages = CommandLine()->FindParm( "-vguimessages" ) ? 1 : 0;
		}
		if ( s_bDebugMessages == 1 )
		{
			vgui::ivgui()->DPrintf( "Message '%s' not handled by panel '%s'\n", params->GetName(), GetName() );
		}
	}
}

void RmlContext::PaintTraverse(bool forceRepaint, bool allowForce)
{
	if ( !vgui::ipanel()->IsVisible( m_VPanel ) )
	{
		return;
	}

	if ( !forceRepaint &&
		 allowForce &&
		 m_bNeedsRepaint )
	{
		forceRepaint = true;
		m_bNeedsRepaint = false;
	}

	float oldAlphaMultiplier = vgui::surface()->DrawGetAlphaMultiplier();

	vgui::surface()->DrawSetAlphaMultiplier( 1 );

	if ( forceRepaint )
	{
		vgui::surface()->PushMakeCurrent( m_VPanel, false );

		Render();

		vgui::surface()->PopMakeCurrent( m_VPanel );
	}

	Assert(vgui::ipanel()->GetChildCount(m_VPanel) == 0);

	vgui::surface()->DrawSetAlphaMultiplier( oldAlphaMultiplier );

	vgui::surface()->SwapBuffers( m_VPanel );
}

void RmlContext::Repaint()
{
	vgui::surface()->Invalidate(m_VPanel);
	m_bNeedsRepaint = true;
}
