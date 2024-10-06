#ifndef RMLUI_H
#define RMLUI_H

#pragma once

#include "shaderapi/ishaderapi.h"

#pragma push_macro("Assert")
#undef Assert
#include <RmlUi/Core/FileInterface.h>
#include <RmlUi/Core/SystemInterface.h>
#include <RmlUi/Core/FontEngineInterface.h>
#include <RmlUi/Core/RenderInterface.h>
#pragma pop_macro("Assert")

#if 0
class RmlFontInterface : public Rml::FontEngineInterface
{
public:
	/// Called when RmlUi is being initialized.
	virtual void Initialize();

	/// Called when RmlUi is being shut down.
	virtual void Shutdown();

	/// Called by RmlUi when it wants to load a font face from file.
	/// @param[in] file_name The file to load the face from.
	/// @param[in] fallback_face True to use this font face for unknown characters in other font faces.
	/// @param[in] weight The weight to load when the font face contains multiple weights, otherwise the weight to register the font as.
	/// @return True if the face was loaded successfully, false otherwise.
	virtual bool LoadFontFace(const Rml::String& file_name, bool fallback_face, Rml::Style::FontWeight weight);

	/// Called by RmlUi when it wants to load a font face from memory, registered using the provided family, style, and weight.
	/// @param[in] data The font data.
	/// @param[in] family The family to register the font as.
	/// @param[in] style The style to register the font as.
	/// @param[in] weight The weight to load when the font face contains multiple weights, otherwise the weight to register the font as.
	/// @param[in] fallback_face True to use this font face for unknown characters in other font faces.
	/// @return True if the face was loaded successfully, false otherwise.
	/// @note The debugger plugin will load its embedded font faces through this method using the family name 'rmlui-debugger-font'.
	virtual bool LoadFontFace(Rml::Span<const Rml::byte> data, const Rml::String& family, Rml::Style::FontStyle style, Rml::Style::FontWeight weight, bool fallback_face);

	/// Called by RmlUi when a font configuration is resolved for an element. Should return a handle that
	/// can later be used to resolve properties of the face, and generate string geometry to be rendered.
	/// @param[in] family The family of the desired font handle.
	/// @param[in] style The style of the desired font handle.
	/// @param[in] weight The weight of the desired font handle.
	/// @param[in] size The size of desired handle, in points.
	/// @return A valid handle if a matching (or closely matching) font face was found, NULL otherwise.
	virtual Rml::FontFaceHandle GetFontFaceHandle(const Rml::String& family, Rml::Style::FontStyle style, Rml::Style::FontWeight weight, int size);

	/// Called by RmlUi when a list of font effects is resolved for an element with a given font face.
	/// @param[in] handle The font handle.
	/// @param[in] font_effects The list of font effects to generate the configuration for.
	/// @return A handle to the prepared font effects which will be used when generating geometry for a string.
	virtual Rml::FontEffectsHandle PrepareFontEffects(Rml::FontFaceHandle handle, const Rml::FontEffectList& font_effects);

	/// Should return the font metrics of the given font face.
	/// @param[in] handle The font handle.
	/// @return The face's metrics.
	virtual const Rml::FontMetrics& GetFontMetrics(Rml::FontFaceHandle handle);

	/// Called by RmlUi when it wants to retrieve the width of a string when rendered with this handle.
	/// @param[in] handle The font handle.
	/// @param[in] string The string to measure.
	/// @param[in] text_shaping_context Additional parameters that provide context for text shaping.
	/// @param[in] prior_character The optionally-specified character that immediately precedes the string. This may have an impact on the string
	/// width due to kerning.
	/// @return The width, in pixels, this string will occupy if rendered with this handle.
	virtual int GetStringWidth(Rml::FontFaceHandle handle, Rml::StringView string, const Rml::TextShapingContext& text_shaping_context,
		Rml::Character prior_character = Rml::Character::Null);

	/// Called by RmlUi when it wants to retrieve the meshes required to render a single line of text.
	/// @param[in] render_manager The render manager responsible for rendering the string.
	/// @param[in] face_handle The font handle.
	/// @param[in] font_effects_handle The handle to the prepared font effects for which the geometry should be generated.
	/// @param[in] string The string to render.
	/// @param[in] position The position of the baseline of the first character to render.
	/// @param[in] colour The colour to render the text.
	/// @param[in] opacity The opacity of the text, should be applied to font effects.
	/// @param[in] text_shaping_context Additional parameters that provide context for text shaping.
	/// @param[out] mesh_list A list to place the meshes and textures representing the string to be rendered.
	/// @return The width, in pixels, of the string mesh.
	virtual int GenerateString(Rml::RenderManager& render_manager, Rml::FontFaceHandle face_handle, Rml::FontEffectsHandle font_effects_handle, Rml::StringView string,
		Rml::Vector2f position, Rml::ColourbPremultiplied colour, float opacity, const Rml::TextShapingContext& text_shaping_context, Rml::TexturedMeshList& mesh_list);

	/// Called by RmlUi to determine if the text geometry is required to be re-generated. Whenever the returned version
	/// is changed, all geometry belonging to the given face handle will be re-generated.
	/// @param[in] face_handle The font handle.
	/// @return The version required for using any geometry generated with the face handle.
	virtual int GetVersion(Rml::FontFaceHandle handle);

	/// Called by RmlUi when it wants to garbage collect memory used by fonts.
	/// @note All existing FontFaceHandles and FontEffectsHandles are considered invalid after this call.
	virtual void ReleaseFontResources();
};
#endif

class RmlFileInterface : public Rml::FileInterface
{
public:
	/// Opens a file.
	/// @param path The path to the file to open.
	/// @return A valid file handle, or nullptr on failure
	virtual Rml::FileHandle Open(const Rml::String& path);
	/// Closes a previously opened file.
	/// @param file The file handle previously opened through Open().
	virtual void Close(Rml::FileHandle file);

	/// Reads data from a previously opened file.
	/// @param buffer The buffer to be read into.
	/// @param size The number of bytes to read into the buffer.
	/// @param file The handle of the file.
	/// @return The total number of bytes read into the buffer.
	virtual size_t Read(void* buffer, size_t size, Rml::FileHandle file);
	/// Seeks to a point in a previously opened file.
	/// @param file The handle of the file to seek.
	/// @param offset The number of bytes to seek.
	/// @param origin One of either SEEK_SET (seek from the beginning of the file), SEEK_END (seek from the end of the file) or SEEK_CUR (seek from
	/// the current file position).
	/// @return True if the operation completed successfully, false otherwise.
	virtual bool Seek(Rml::FileHandle file, long offset, int origin);
	/// Returns the current position of the file pointer.
	/// @param file The handle of the file to be queried.
	/// @return The number of bytes from the origin of the file.
	virtual size_t Tell(Rml::FileHandle file);

	/// Returns the length of the file.
	/// The default implementation uses Seek & Tell.
	/// @param file The handle of the file to be queried.
	/// @return The length of the file in bytes.
	virtual size_t Length(Rml::FileHandle file);

	/// Load and return a file.
	/// @param path The path to the file to load.
	/// @param out_data The string contents of the file.
	/// @return True on success.
	virtual bool LoadFile(const Rml::String& path, Rml::String& out_data);
};

extern RmlFileInterface g_RmlFileInterface;

class RmlSystemInterface : public Rml::SystemInterface
{
public:
	/// Get the number of seconds elapsed since the start of the application.
	/// @return Elapsed time, in seconds.
	virtual double GetElapsedTime();

	/// Translate the input string into the translated string.
	/// @param[out] translated Translated string ready for display.
	/// @param[in] input String as received from XML.
	/// @return Number of translations that occured.
	virtual int TranslateString(Rml::String& translated, const Rml::String& input);

	/// Joins the path of an RML or RCSS file with the path of a resource specified within the file.
	/// @param[out] translated_path The joined path.
	/// @param[in] document_path The path of the source document (including the file name).
	/// @param[in] path The path of the resource specified in the document.
	virtual void JoinPath(Rml::String& translated_path, const Rml::String& document_path, const Rml::String& path);

	/// Log the specified message.
	/// @param[in] type Type of log message, ERROR, WARNING, etc.
	/// @param[in] message Message to log.
	/// @return True to continue execution, false to break into the debugger.
	virtual bool LogMessage(Rml::Log::Type type, const Rml::String& message);

	/// Set mouse cursor.
	/// @param[in] cursor_name Cursor name to activate.
	virtual void SetMouseCursor(const Rml::String& cursor_name);

	/// Set clipboard text.
	/// @param[in] text Text to apply to clipboard.
	virtual void SetClipboardText(const Rml::String& text);

	/// Get clipboard text.
	/// @param[out] text Retrieved text from clipboard.
	virtual void GetClipboardText(Rml::String& text);

	/// Activate keyboard (for touchscreen devices).
	/// @param[in] caret_position Position of the caret in absolute window coordinates.
	/// @param[in] line_height Height of the current line being edited.
	virtual void ActivateKeyboard(Rml::Vector2f caret_position, float line_height);

	/// Deactivate keyboard (for touchscreen devices).
	virtual void DeactivateKeyboard();
};

extern RmlSystemInterface g_RmlSystemInterface;

class RmlRenderInterface : public Rml::RenderInterface
{
public:
	RmlRenderInterface();

	void Initialize();

	void BeginRender(int x, int y, int wide, int tall);
	void EndRender();

	/**
	    @name Required functions for basic rendering.
	 */

	/// Called by RmlUi when it wants to compile geometry to be rendered later.
	/// @param[in] vertices The geometry's vertex data.
	/// @param[in] indices The geometry's index data.
	/// @return An application-specified handle to the geometry, or zero if it could not be compiled.
	/// @lifetime The pointed-to vertex and index data are guaranteed to be valid and immutable until ReleaseGeometry()
	/// is called with the geometry handle returned here.
	virtual Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex> vertices, Rml::Span<const int> indices);
	/// Called by RmlUi when it wants to render geometry.
	/// @param[in] geometry The geometry to render.
	/// @param[in] translation The translation to apply to the geometry.
	/// @param[in] texture The texture to be applied to the geometry, or zero if the geometry is untextured.
	virtual void RenderGeometry(Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture);
	/// Called by RmlUi when it wants to release geometry.
	/// @param[in] geometry The geometry to release.
	virtual void ReleaseGeometry(Rml::CompiledGeometryHandle geometry);

	/// Called by RmlUi when a texture is required by the library.
	/// @param[out] texture_dimensions The dimensions of the loaded texture, which must be set by the application.
	/// @param[in] source The application-defined image source, joined with the path of the referencing document.
	/// @return An application-specified handle identifying the texture, or zero if it could not be loaded.
	virtual Rml::TextureHandle LoadTexture(Rml::Vector2i& texture_dimensions, const Rml::String& source);
	/// Called by RmlUi when a texture is required to be generated from a sequence of pixels in memory.
	/// @param[in] source The raw texture data. Each pixel is made up of four 8-bit values, red, green, blue, and premultiplied alpha, in that order.
	/// @param[in] source_dimensions The dimensions, in pixels, of the source data.
	/// @return An application-specified handle identifying the texture, or zero if it could not be generated.
	virtual Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte> source, Rml::Vector2i source_dimensions);
	/// Called by RmlUi when a loaded or generated texture is no longer required.
	/// @param[in] texture The texture handle to release.
	virtual void ReleaseTexture(Rml::TextureHandle texture);

	/// Called by RmlUi when it wants to enable or disable scissoring to clip content.
	/// @param[in] enable True if scissoring is to enabled, false if it is to be disabled.
	virtual void EnableScissorRegion(bool enable);
	/// Called by RmlUi when it wants to change the scissor region.
	/// @param[in] region The region to be rendered. All pixels outside this region should be clipped.
	/// @note The region should be applied in window coordinates regardless of any active transform.
	virtual void SetScissorRegion(Rml::Rectanglei region);

	/**
	    @name Optional functions for advanced rendering features.
	 */

	/// Called by RmlUi when it wants to enable or disable the clip mask.
	/// @param[in] enable True if the clip mask is to be enabled, false if it is to be disabled.
	virtual void EnableClipMask(bool enable);
	/// Called by RmlUi when it wants to set or modify the contents of the clip mask.
	/// @param[in] operation Describes how the geometry should affect the clip mask.
	/// @param[in] geometry The compiled geometry to render.
	/// @param[in] translation The translation to apply to the geometry.
	/// @note When enabled, the clip mask should hide any rendered contents outside the area of the mask.
	/// @note The clip mask applies exclusively to all other functions that render with a geometry handle, in addition
	/// to the layer compositing function while rendering to its destination.
	virtual void RenderToClipMask(Rml::ClipMaskOperation operation, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation);

	/// Called by RmlUi when it wants the renderer to use a new transform matrix.
	/// @param[in] transform The new transform to apply, or nullptr if no transform applies to the current element.
	/// @note When nullptr is submitted, the renderer should use an identity transform matrix or otherwise omit the
	/// multiplication with the transform.
	/// @note The transform applies to all functions that render with a geometry handle, and only those.
	virtual void SetTransform(const Rml::Matrix4f* transform);

	/// Called by RmlUi when it wants to push a new layer onto the render stack, setting it as the new render target.
	/// @return An application-specified handle representing the new layer. The value 'zero' is reserved for the initial base layer.
	/// @note The new layer should be initialized to transparent black within the current scissor region.
	virtual Rml::LayerHandle PushLayer();
	/// Composite two layers with the given blend mode and apply filters.
	/// @param[in] source The source layer.
	/// @param[in] destination The destination layer.
	/// @param[in] blend_mode The mode used to blend the source layer onto the destination layer.
	/// @param[in] filters A list of compiled filters which should be applied before blending.
	/// @note Source and destination can reference the same layer.
	virtual void CompositeLayers(Rml::LayerHandle source, Rml::LayerHandle destination, Rml::BlendMode blend_mode, Rml::Span<const Rml::CompiledFilterHandle> filters);
	/// Called by RmlUi when it wants to pop the render layer stack, setting the new top layer as the render target.
	virtual void PopLayer();

	/// Called by RmlUi when it wants to store the current layer as a new texture to be rendered later with geometry.
	/// @return An application-specified handle to the new texture.
	/// @note The texture should be extracted using the bounds defined by the active scissor region, thereby matching its size.
	virtual Rml::TextureHandle SaveLayerAsTexture();

	/// Called by RmlUi when it wants to store the current layer as a mask image, to be applied later as a filter.
	/// @return An application-specified handle to a new filter representing the stored mask image.
	virtual Rml::CompiledFilterHandle SaveLayerAsMaskImage();

	/// Called by RmlUi when it wants to compile a new filter.
	/// @param[in] name The name of the filter.
	/// @param[in] parameters The list of name-value parameters specified for the filter.
	/// @return An application-specified handle representing the compiled filter.
	virtual Rml::CompiledFilterHandle CompileFilter(const Rml::String& name, const Rml::Dictionary& parameters);
	/// Called by RmlUi when it no longer needs a previously compiled filter.
	/// @param[in] filter The handle to a previously compiled filter.
	virtual void ReleaseFilter(Rml::CompiledFilterHandle filter);

	/// Called by RmlUi when it wants to compile a new shader.
	/// @param[in] name The name of the shader.
	/// @param[in] parameters The list of name-value parameters specified for the filter.
	/// @return An application-specified handle representing the shader.
	virtual Rml::CompiledShaderHandle CompileShader(const Rml::String& name, const Rml::Dictionary& parameters);
	/// Called by RmlUi when it wants to render geometry using the given shader.
	/// @param[in] shader The handle to a previously compiled shader.
	/// @param[in] geometry The handle to a previously compiled geometry.
	/// @param[in] translation The translation to apply to the geometry.
	/// @param[in] texture The texture to use when rendering the geometry, or zero for no texture.
	virtual void RenderShader(Rml::CompiledShaderHandle shader, Rml::CompiledGeometryHandle geometry, Rml::Vector2f translation, Rml::TextureHandle texture);
	/// Called by RmlUi when it no longer needs a previously compiled shader.
	/// @param[in] shader The handle to a previously compiled shader.
	virtual void ReleaseShader(Rml::CompiledShaderHandle shader);

private:
	static KeyValues *CreateMaterial();

	struct RmlMesh_t
	{
		IIndexBuffer *indexBuffer;
		int indexCount;
		IVertexBuffer *vertexBuffer;
		int vertexCount;
	};

	struct RmlTexture_t
	{
		ITexture *texture;
		IMaterial *material;
	};

	IMaterial *m_pMaterial;

	bool m_bRendering;

	Rml::Rectanglei m_rectScissors;
	bool m_bScissorsEnabled;

	IMatRenderContext *m_pRenderContext;
};

extern RmlRenderInterface g_RmlRenderInterface;

#endif
