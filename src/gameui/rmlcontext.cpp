#include <RmlUi/Core/Core.h>
#include <RmlUi/Core/ElementDocument.h>
#include "rmlcontext.h"
#include "engineinterface.h"
#include <vgui_controls/Controls.h>
#include <vgui/IScheme.h>
#include <vgui/ISurface.h>
#include <vgui/IVGui.h>
#include "tier1/KeyValues.h"
#include "gameui.h"
#include "rmlui.h"

// memdbgon must be the last include file in a .cpp file!!!
#include <tier0/memdbgon.h>

ConVar rml_context_update_rate("rml_context_update_rate", "1");

CON_COMMAND(rml_list_contexts, "")
{
	CUtlVector< vgui::VPANEL > &children = vgui::ipanel()->GetChildren( g_GameUI.GetVPanel() );
	int childCount = children.Count();
	for (int i = 0; i < childCount; i++)
	{
		vgui::VPANEL child = children[ i ];

		int x, y, w, h;
		vgui::ipanel()->GetPos( child, x, y );
		vgui::ipanel()->GetSize( child, w, h );

		Msg("%s [%i, %i : %ix%i]\n", vgui::ipanel()->GetName( child ), x, y, w, h);
	}
}

CON_COMMAND(rml_load_doc, "")
{
	if(args.ArgC() != 3) {
		Msg("rml_load_doc <context> <doc>\n");
		return;
	}

	CUtlVector< vgui::VPANEL > &children = vgui::ipanel()->GetChildren( g_GameUI.GetVPanel() );
	int childCount = children.Count();
	for (int i = 0; i < childCount; i++)
	{
		vgui::VPANEL child = children[ i ];

		if(V_strcmp(vgui::ipanel()->GetName( child ), args.Arg(1)) == 0) {
			vgui::ivgui()->PostMessage( child, new KeyValues("LoadAndShowDocument", "file", args.Arg(2)), g_GameUI.GetVPanel() );
			break;
		}
	}
}

CON_COMMAND(rml_unload_all_docs, "")
{
	if(args.ArgC() != 2) {
		Msg("rml_unload_all_docs <context>\n");
		return;
	}

	CUtlVector< vgui::VPANEL > &children = vgui::ipanel()->GetChildren( g_GameUI.GetVPanel() );
	int childCount = children.Count();
	for (int i = 0; i < childCount; i++)
	{
		vgui::VPANEL child = children[ i ];

		if(V_strcmp(vgui::ipanel()->GetName( child ), args.Arg(1)) == 0) {
			vgui::ivgui()->PostMessage( child, new KeyValues("UnloadAllDocuments"), g_GameUI.GetVPanel() );
			break;
		}
	}
}

CON_COMMAND(rml_create_context, "")
{
	if(args.ArgC() == 6) {
		if(Rml::GetContext(args.Arg(1))) {
			Msg("name already in-use\n");
			return;
		}

		new RmlContext(args.Arg(1), V_atoi(args.Arg(2)), V_atoi(args.Arg(3)), V_atoi(args.Arg(4)), V_atoi(args.Arg(5)));
	} else if(args.ArgC() == 4) {
		if(Rml::GetContext(args.Arg(1))) {
			Msg("name already in-use\n");
			return;
		}

		new RmlContext(args.Arg(1), V_atoi(args.Arg(2)), V_atoi(args.Arg(3)));
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

	CUtlVector< vgui::VPANEL > &children = vgui::ipanel()->GetChildren( g_GameUI.GetVPanel() );
	int childCount = children.Count();
	for (int i = 0; i < childCount; i++)
	{
		vgui::VPANEL child = children[ i ];

		if(V_strcmp(vgui::ipanel()->GetName( child ), args.Arg(1)) == 0) {
			vgui::ipanel()->SetSize( child, V_atoi(args.Arg(2)), V_atoi(args.Arg(3)) );
			break;
		}
	}
}

CON_COMMAND(rml_move_context, "")
{
	if(args.ArgC() != 4) {
		Msg("rml_delete_context <context> <x> <y>\n");
		return;
	}

	CUtlVector< vgui::VPANEL > &children = vgui::ipanel()->GetChildren( g_GameUI.GetVPanel() );
	int childCount = children.Count();
	for (int i = 0; i < childCount; i++)
	{
		vgui::VPANEL child = children[ i ];

		if(V_strcmp(vgui::ipanel()->GetName( child ), args.Arg(1)) == 0) {
			vgui::ipanel()->SetPos( child, V_atoi(args.Arg(2)), V_atoi(args.Arg(3)) );
			break;
		}
	}
}

CON_COMMAND(rml_delete_context, "")
{
	if(args.ArgC() != 2) {
		Msg("rml_delete_context <context>\n");
		return;
	}

	CUtlVector< vgui::VPANEL > &children = vgui::ipanel()->GetChildren( g_GameUI.GetVPanel() );
	int childCount = children.Count();
	for (int i = 0; i < childCount; i++)
	{
		vgui::VPANEL child = children[ i ];

		if(V_strcmp(vgui::ipanel()->GetName( child ), args.Arg(1)) == 0) {
			vgui::ivgui()->MarkPanelForDeletion( child );
			break;
		}
	}
}

CON_COMMAND(rml_reload_all, "")
{
	CUtlVector< vgui::VPANEL > &children = vgui::ipanel()->GetChildren( g_GameUI.GetVPanel() );
	int childCount = children.Count();
	for (int i = 0; i < childCount; i++)
	{
		vgui::VPANEL child = children[ i ];

		vgui::ivgui()->PostMessage( child, new KeyValues("ReloadAllDocuments"), g_GameUI.GetVPanel() );
	}
}

RmlContext::RmlContext(const char *name, int x, int y, int width, int height)
{
	context = Rml::CreateContext(name, Rml::Vector2i(width, height), &g_RmlRenderInterface, nullptr);
	context->SetDensityIndependentPixelRatio( vgui::scheme()->GetProportionalScaledValue( 100 ) / 250.f );

	context->EnableMouseCursor( true );

	m_VPanel = vgui::ivgui()->AllocPanel();
	vgui::ipanel()->Init(m_VPanel, this);
	vgui::ipanel()->SetParent(m_VPanel, g_GameUI.GetVPanel());

	vgui::ipanel()->SetPos( m_VPanel, x, y );
	vgui::ipanel()->SetSize( m_VPanel, width, height );
	vgui::ipanel()->SetVisible( m_VPanel, true );
	vgui::ipanel()->SetMouseInputEnabled( m_VPanel, true );
	vgui::ipanel()->SetKeyBoardInputEnabled( m_VPanel, true );

	UpdateNow();
}

RmlContext::RmlContext(const char *name, int width, int height)
	: RmlContext(name, 0, 0, width, height)
{
}

RmlContext::~RmlContext()
{
	Rml::RemoveContext( context->GetName() );
}

void RmlContext::DeletePanel()
{
	delete this;
}

void RmlContext::OnTick()
{

}

void RmlContext::UpdateNow()
{
	context->Update();
	context->RequestNextUpdate( engine->Time() + rml_context_update_rate.GetFloat() );
}

void RmlContext::Think()
{
	if(context->GetNextUpdateDelay() <= engine->Time()) {
		UpdateNow();
	}
}

const char *RmlContext::GetName()
{
	return context->GetName().c_str();
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
	context->SetDimensions(Rml::Vector2i(newWide, newTall));
}

void RmlContext::OnMessage(const KeyValues *params, vgui::VPANEL ifromPanel)
{
	if(V_strcmp(params->GetName(), "CursorMoved") == 0) {
		context->ProcessMouseMove(params->GetInt("xpos"), params->GetInt("ypos"), 0);
	} else if(V_strcmp(params->GetName(), "CursorEntered") == 0) {
		
	} else if(V_strcmp(params->GetName(), "CursorExited") == 0) {
		context->ProcessMouseLeave();
	} else if(V_strcmp(params->GetName(), "MousePressed") == 0) {
		
	} else if(V_strcmp(params->GetName(), "MouseReleased") == 0) {
		
	} else if(V_strcmp(params->GetName(), "MouseDoublePressed") == 0) {
		
	} else if(V_strcmp(params->GetName(), "ReloadAllDocuments") == 0) {
		
	} else if(V_strcmp(params->GetName(), "UnloadAllDocuments") == 0) {
		context->UnloadAllDocuments();
	} else if(V_strcmp(params->GetName(), "LoadAndShowDocument") == 0) {
		Rml::ElementDocument *doc = context->LoadDocument(params->GetString("file"));
		UpdateNow();
		if(doc) {
			doc->Show();
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

		context->Render();

		vgui::surface()->PopMakeCurrent( m_VPanel );
	}

	vgui::surface()->DrawSetAlphaMultiplier( oldAlphaMultiplier );

	vgui::surface()->SwapBuffers( m_VPanel );
}

void RmlContext::Repaint()
{
	vgui::surface()->Invalidate(m_VPanel);
	m_bNeedsRepaint = true;
}
