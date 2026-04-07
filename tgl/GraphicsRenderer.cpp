#include "GraphicsRenderer.h"
#include "GraphicsRecorder.h"
#include <cstdio>

GraphicsRenderer::GraphicsRenderer()
{
    // Initialize shader
    if (m_shader.isValid() == 0)
    {
        std::printf("Shader compilation failed\n");
    }

    // Initialize blend ( Alpha blend, PREMULTIPLIED Shader )
    glEnable(GL_BLEND);
#if 0
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
#endif
    // Initialize depth
    glDisable(GL_DEPTH_TEST);
    // Initialize stencil
    glDisable(GL_STENCIL_TEST);
    glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
    glStencilMask(0xFFFFFFFF);
    glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
    glStencilFunc(GL_ALWAYS, 0, 0xFFFFFFFF);
    // Initialize texture
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, 0);
}

void GraphicsRenderer::commit(const GraphicsRecorder &recorder)
{
    // Upload font
    recorder.syncFontAtlas();

    // Upload vertices
    m_buffer.sync(m_shader.locs.a_pos, m_shader.locs.a_uv0, m_shader.locs.a_uv1, recorder.m_verts, recorder.m_indices);

    // Upload calls
    m_calls = recorder.m_calls;
    if (m_calls.size() < m_calls.capacity() / 4)
        m_calls.shrink_to_fit();
}

void GraphicsRenderer::render()
{
    // Render
    m_shader.bind();
    m_buffer.bindVAO();
    glUniform2f(m_shader.locs.u_resolution, static_cast<float>(m_width), static_cast<float>(m_height));
    glUniform1i(m_shader.locs.u_texture, 0);
    glUniform1i(m_shader.locs.u_fontAtlas, 1);

    for (const Call &call : m_calls)
    {
        glBlendFuncSeparate(call.state.sfactor, call.state.dfactor,
                            call.state.sfactor, call.state.dfactor);
        glUniform2ui(m_shader.locs.u_fragmentType, call.state.fillType, call.param.drawType);
        glUniform1f(m_shader.locs.u_alpha, call.state.alpha);
        glUniform4f(m_shader.locs.u_scissor,
                    call.state.scissor.minx, call.state.scissor.miny,
                    call.state.scissor.maxx, call.state.scissor.maxy);
        switch (call.state.fillType)
        {
        case FILL_COLOR:
        {
            // fillColorPass
            glUniform4f(m_shader.locs.u_color,
                        call.state.color.r, call.state.color.g,
                        call.state.color.b, call.state.color.a);
            break;
        }

        case FILL_IMAGE:
        {
            // drawImagePass
            glUniform1ui(m_shader.locs.u_imageParams, call.state.imageParams);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, call.state.texture->getTex());
            break;
        }

        case FILL_LINEAR_GRADIENT:
        case FILL_RADIAL_GRADIENT:
        case FILL_CONIC_GRADIENT:
        {
            // fillGradientPass
            glUniform3f(m_shader.locs.u_gradientParam0,
                        call.state.gradientParam0[0], call.state.gradientParam0[1], call.state.gradientParam0[2]);
            glUniform3f(m_shader.locs.u_gradientParam1,
                        call.state.gradientParam1[0], call.state.gradientParam1[1], call.state.gradientParam1[2]);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, call.state.texture->getTex());
            break;
        }

        default:
            break;
        }
        switch (call.param.drawType)
        {
        case DRAW_FONT:
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, call.param.fontTexture->getTex());
            break;

        default:
            break;
        }
        glDrawElements(GL_TRIANGLES, call.indiceCount, GL_UNSIGNED_INT, call.indiceOffset);
    }
}