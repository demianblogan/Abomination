#include "UI/StyleValues.h"

#include <RmlUi/Core.h>

#include <gtest/gtest.h>

namespace Abomination::UI
{
    namespace
    {
        // A renderer that draws nothing and counts how many times RmlUi builds geometry: every SetProperty that RmlUi
        // takes as a change builds the geometry of the element again.
        class CountingRenderInterface : public Rml::RenderInterface
        {
        public:
            Rml::CompiledGeometryHandle CompileGeometry(Rml::Span<const Rml::Vertex>, Rml::Span<const int>) override
            {
                return ++compiledGeometryCount;
            }

            void RenderGeometry(Rml::CompiledGeometryHandle, Rml::Vector2f, Rml::TextureHandle) override
            {}

            void ReleaseGeometry(Rml::CompiledGeometryHandle) override
            {}

            Rml::TextureHandle LoadTexture(Rml::Vector2i&, const Rml::String&) override
            {
                return 0;
            }

            Rml::TextureHandle GenerateTexture(Rml::Span<const Rml::byte>, Rml::Vector2i) override
            {
                return 1;
            }

            void ReleaseTexture(Rml::TextureHandle) override
            {}

            void EnableScissorRegion(bool) override
            {}

            void SetScissorRegion(Rml::Rectanglei) override
            {}

            int compiledGeometryCount = 0;
        };

        // RmlUi with one document holding one red box, as the HUD holds the dot of the crosshair.
        class StyleValuesTest : public ::testing::Test
        {
        protected:
            void SetUp() override
            {
                Rml::SetRenderInterface(&m_renderInterface);
                ASSERT_TRUE(Rml::Initialise());
                m_context = Rml::CreateContext("StyleValuesTest", Rml::Vector2i(200, 200));
                ASSERT_NE(m_context, nullptr);
                Rml::ElementDocument* document = m_context->LoadDocumentFromMemory(
                    "<rml><body><div id='box' style='display: block; width: 10px; height: 10px; "
                    "background-color: #ff0000ff;'/></body></rml>");
                ASSERT_NE(document, nullptr);
                document->Show();
                m_box = document->GetElementById("box");
                ASSERT_NE(m_box, nullptr);
                DrawFrame();
            }

            void TearDown() override
            {
                Rml::Shutdown();
            }

            // One frame of the game interface; returns how many geometries it built.
            int DrawFrame()
            {
                const int before = m_renderInterface.compiledGeometryCount;
                m_context->Update();
                m_context->Render();
                return m_renderInterface.compiledGeometryCount - before;
            }

            CountingRenderInterface m_renderInterface;
            Rml::Context* m_context = nullptr;
            Rml::Element* m_box = nullptr;
        };
    }

    TEST_F(StyleValuesTest, SameValueBuildsNothing)
    {
        // The same width written differently ("10.00px" is how ToPixels writes 10 pixels), and the same color.
        SetProperty(m_box, "width", "10.00px");
        SetProperty(m_box, "background-color", ToRCSSColor(glm::vec4(1.0f, 0.0f, 0.0f, 1.0f)));

        EXPECT_EQ(DrawFrame(), 0);
    }

    TEST_F(StyleValuesTest, NewValueIsSet)
    {
        SetProperty(m_box, "width", "20px");

        EXPECT_GT(DrawFrame(), 0);
        EXPECT_FLOAT_EQ(m_box->GetLocalProperty("width")->Get<float>(), 20.0f);
    }

    TEST_F(StyleValuesTest, MissingElementIsSkipped)
    {
        SetProperty(nullptr, "width", "20px");
        SetOpacity(nullptr, 0.5f);
        SetVisible(nullptr, false);
    }
}
