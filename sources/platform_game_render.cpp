#pragma once
#include "util_imgui.cpp"
struct OpenGL {
	struct {
		GLint vertexShader;
		GLint fragmentShader;
		GLint program;

		GLint modelMatrixLocation;
		GLint worldMatrixLocation;
		GLint projectionMatrixLocation;
		GLint overlayColorLocation;
		GLint samplerLocation;

		GLint textureOffsetLocation;
		GLint textureScaleLocation;
	} game;
	struct {
		GLint vertexShader;
		GLint fragmentShader;
		GLint program;

		GLint positionLocation;
		GLint scaleLocation;
		GLint overlayColorLocation;
		GLint samplerLocation;

		GLint textureOffsetLocation;
		GLint textureScaleLocation;
	} hud;
	struct {
		GLint vertexShader;
		GLint fragmentShader;
		GLint program;

		GLint modelMatrixLocation;
		GLint worldMatrixLocation;
		GLint projectionMatrixLocation;
		GLint overlayColorLocation;

	} wire;
	GLuint quad;

    u32 quadOffset;
    u32 quadCount;
    
    u32 lineboxOffset;
    u32 lineboxCount;

    u32 linecircleOffset;
    u32 linecircleCount;
};

void makeQuad(GLModel* target, v2 scale)
{
    target->scale = scale;
    target->pos = V2(-0.5f, -0.5f);
    target->glType = GL_TRIANGLE_STRIP;
    target->glOffset = gl->quadOffset;
    target->glCount = gl->quadCount;
}

void makeLinebox(GLModel* target, v2 scale)
{
    target->scale = scale;
    target->pos = V2(-0.5f, -0.5f);
    target->glType = GL_LINES;
    target->glOffset = gl->lineboxOffset;
    target->glCount = gl->lineboxCount;
}

void makeLinecircle(GLModel* target, v2 scale)
{
    target->scale = scale;
    target->pos = V2(-0.5f, -0.5f);
    target->glType = GL_LINES;
    target->glOffset = gl->linecircleOffset;
    target->glCount = gl->linecircleCount;
}

static inline void initGameShader() {
	gl->game.modelMatrixLocation = glGetUniformLocation(gl->game.program, "modelMatrix");
	gl->game.worldMatrixLocation = glGetUniformLocation(gl->game.program, "worldMatrix");
	gl->game.projectionMatrixLocation = glGetUniformLocation(gl->game.program, "projectionMatrix");
	gl->game.overlayColorLocation = glGetUniformLocation(gl->game.program, "overlayColor");
	gl->game.samplerLocation = glGetUniformLocation(gl->game.program, "sampler");
	gl->game.textureOffsetLocation = glGetUniformLocation(gl->game.program, "textureOffset");
	gl->game.textureScaleLocation = glGetUniformLocation(gl->game.program, "textureScale");

	gl->hud.positionLocation = glGetUniformLocation(gl->hud.program, "position");
	gl->hud.scaleLocation = glGetUniformLocation(gl->hud.program, "scale");
	gl->hud.overlayColorLocation = glGetUniformLocation(gl->hud.program, "overlayColor");
	gl->hud.samplerLocation = glGetUniformLocation(gl->hud.program, "sampler");
	gl->hud.textureOffsetLocation = glGetUniformLocation(gl->hud.program, "textureOffset");
	gl->hud.textureScaleLocation = glGetUniformLocation(gl->hud.program, "textureScale");

	gl->wire.modelMatrixLocation = glGetUniformLocation(gl->wire.program, "modelMatrix");
	gl->wire.worldMatrixLocation = glGetUniformLocation(gl->wire.program, "worldMatrix");
	gl->wire.projectionMatrixLocation = glGetUniformLocation(gl->wire.program, "projectionMatrix");
	gl->wire.overlayColorLocation = glGetUniformLocation(gl->wire.program, "overlayColor");
}


void glRenderGame(Game * state, v2 resolutionScale, mat3 * projection){
    (void)resolutionScale;
    (void)state;

    glUniformMatrix3fv(gl->game.projectionMatrixLocation, 1, true, projection->c);

    for(i32 i = 0; i < ARRAYSIZE(game->entities); i++)
    {
        Entity * entity = &game->entities[i];
        glUniform4f(gl->game.overlayColorLocation, entity->overlayColor.x, entity->overlayColor.y, entity->overlayColor.z, entity->overlayColor.w);

        mat3 scale = scalingMatrix(entity->model.scale);
        mat3 translate = translationMatrix(entity->pos);
        mat3 model =  scale * translationMatrix(entity->model.pos);
        if (entity->type == EntityType_Player)
        {
            model = rotationYMatrix3(entity->player.rotationYRad) * model;
        }
        glUniformMatrix3fv(gl->game.modelMatrixLocation, 1, true, model.c);

        mat3 world = translate;
        glUniformMatrix3fv(gl->game.worldMatrixLocation, 1, true, world.c);

        Animation * animation = entity->animation;
        if(animation){
            OpenglSprite * sprite = &animation->sprite;
            f64 animProgress = animation->timer->progressNormalized;
            {
                i32 tile = CAST(i32, (animProgress * (sprite->framesTotal - 0.5f)));
                i32 y = tile / sprite->framesX;
                i32 x = tile % sprite->framesX;
                ASSERT(x >= 0);
                ASSERT(x < sprite->framesX);
                ASSERT(y >= 0);
                ASSERT(y < sprite->framesY);
                glUniform2f(gl->game.textureOffsetLocation, CAST(f32, x)/sprite->framesX, CAST(f32, y)/sprite->framesY);
            }
            glUniform2f(gl->game.textureScaleLocation, 1.0f/sprite->framesX, 1.0f/sprite->framesY);
            
            glActiveTexture(GL_TEXTURE0 + 0);
            glBindTexture(GL_TEXTURE_2D, sprite->textureId);
        } 
        else{
            glBindTexture(GL_TEXTURE_2D, 0);
        }

        glDrawArrays(entity->model.glType, entity->model.glOffset, entity->model.glCount);
    }
    

    glUseProgram(gl->wire.program);
    glUniformMatrix3fv(gl->wire.projectionMatrixLocation, 1, true, projection->c);
    v4 color = {0, 1, 0, 1};
    glUniform4f(gl->wire.overlayColorLocation, color.x, color.y, color.z, color.w);
    for(i32 i = 0; i < ARRAYSIZE(game->entities); i++)
    {
        Entity * entity = &game->entities[i];
        mat3 scale = scalingMatrix(entity->wire.scale);
        mat3 translate = translationMatrix(entity->pos);
        mat3 model = scale * translationMatrix(entity->wire.pos);
        glUniformMatrix3fv(gl->wire.modelMatrixLocation, 1, true, model.c);

        mat3 world = translate;
        glUniformMatrix3fv(gl->wire.worldMatrixLocation, 1, true, world.c);

        glDrawArrays(entity->wire.glType, entity->wire.glOffset, entity->wire.glCount);
    }

    for(i32 i = 0; i < ARRAYSIZE(game->boundaries); i++)
    {
        Entity * entity = &game->boundaries[i];
        mat3 scale = scalingMatrix(entity->wire.scale);
        mat3 translate = translationMatrix(entity->pos);
        mat3 model = scale * translationMatrix(entity->wire.pos);
        glUniformMatrix3fv(gl->wire.modelMatrixLocation, 1, true, model.c);

        mat3 world = translate;
        glUniformMatrix3fv(gl->wire.worldMatrixLocation, 1, true, world.c);

        glDrawArrays(entity->wire.glType, entity->wire.glOffset, entity->wire.glCount);
    }
}


static inline void initGameRender() {
	//Insert the quad into GPU memory
	{
		glGenBuffers(1, &gl->quad);
		glBindBuffer(GL_ARRAY_BUFFER, gl->quad);
        gl->quadOffset = 0;
        gl->quadCount = 4;

        gl->lineboxOffset = 4;
        gl->lineboxCount = 8;

        gl->linecircleOffset = 12;
        // 41 looks good enough even on big circles
        const int circlePoints = 41;
        gl->linecircleCount = circlePoints*2;
		f32 data[2*4 + 2*8 + 2*2*circlePoints] = {
			// triangle strip
			1, 0,// 0.0f, 1.0f,
			0, 0,// 0.0f, 1.0f,
			1, 1,// 0.0f, 1.0f,
			0, 1,//, 0.0f, 1.0f,
			// lines - box
			0, 0,
			0, 1,
			1, 1,
			1, 0,
			0, 0,
			1, 0,
			1, 1,
			0, 1
			// lines - circle
            // circlePoints * 2
		};
        v2 pointOnCircle = V2(0.5f, 0.0f);
        f32 radIncr = (2*PI) / CAST(f32, circlePoints);
        for(i32 i = 0; i < circlePoints; i++)
        {
            data[2*4 + 2*8 + i*4] = pointOnCircle.x + 0.5f;
            data[2*4 + 2*8 + i*4 + 1] = pointOnCircle.y + 0.5f;
            pointOnCircle = rotate(pointOnCircle, radIncr);
            data[2*4 + 2*8 + i*4 + 2] = pointOnCircle.x + 0.5f;
            data[2*4 + 2*8 + i*4 + 3] = pointOnCircle.y + 0.5f;
        }

		glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);
	}
	// vsync disable
	wglSwapIntervalEXT(0);

}

inline void render(Game * state, f64 dt) {
    Entity* field = &state->entities[0];
    (void)dt;
	PROFILE_FUNC();
	glClearColor(0, 0, 0, 1);
	glClearDepth(0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, platform->resolution.x, platform->resolution.y);

	glBindBuffer(GL_ARRAY_BUFFER, gl->quad);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
    //glEnable(GL_MULTISAMPLE);  
	glDepthFunc(GL_GEQUAL);
    
    glActiveTexture(GL_TEXTURE0 + 0);
    glUniform1i(gl->game.samplerLocation, 0);
    glUseProgram(gl->game.program);

	// NOTE(fidli): gui calibrated to 1080p;
	v2 resolutionScale = V2(platform->resolution.x / 1920.0f, platform->resolution.y / 1080.0f);
    f32 ditch = 10.0f;
    mat3 projection = ortoProjectionMatrix(field->model.scale + V2(ditch, ditch), platform->resolution);
    glRenderGame(state, resolutionScale, &projection);

	{//gui
		PROFILE_SCOPE("GUI");
		PUSHI;
		guiBegin(platform->resolution.x, platform->resolution.y);
		GuiStyle* style = &PUSH(GuiStyle);
		memset(CAST(void*, style), 0, sizeof(GuiStyle));
		Color white;
		white.full = 0xFFFFFFFF;
		Color darkaqua;
		darkaqua.full = 0xFF939712;
		Color darkeraqua;
		darkeraqua.full = 0xFF807F00;
		Color darkeraqua2;
		darkeraqua2.full = 0xFF407F00;
		Color lightaqua;
		lightaqua.full = 0xFFD5D79B;
		Color darkred;
		darkred.full = 0xff2222B2;
		Color lightred;
		lightred.full = 0xFF5C5CCD;
		Color darkgold;
		darkgold.full = 0xFF309BBF;
		Color lightgold;
		lightgold.full = 0xFF73DCFF;
		Color blue;
		blue.full = 0xFF946900;
		Color purple;
		purple.full = 0xFFFF00FF;
        Color gray;
        gray.full = 0xFFCCCCCC;
		style->button.passive.fgColor = white;
		style->button.passive.bgColor = darkaqua;
		style->button.active.fgColor = white;
		style->button.active.bgColor = lightaqua;
		style->input.passive.fgColor = white;
		style->input.passive.bgColor = darkaqua;
		style->input.active.fgColor = white;
		style->input.active.bgColor = lightaqua;
		style->input.active.minWidth = CAST(i32, 200 * resolutionScale.x);
		style->input.passive.minWidth = style->input.active.minWidth;
		style->input.selection = style->input.active;
		style->input.selection.bgColor = blue;
		style->text.fgColor = white;
		style->text.bgColor = darkeraqua;
		style->container.fgColor = white;
		style->container.bgColor = darkeraqua;
		style->font = &guiContext->font;
		style->pt = CAST(i32, 20 * resolutionScale.y);
		style->button.active.padding = { CAST(i32, 4 * resolutionScale.y), CAST(i32, 4 * resolutionScale.x), CAST(i32, 4 * resolutionScale.y), CAST(i32, 4 * resolutionScale.x) };
		style->button.active.margin = { CAST(i32, 4 * resolutionScale.y), 0, 0, CAST(i32, 4 * resolutionScale.x) };
		style->button.passive.padding = style->button.active.padding;
		style->button.passive.margin = style->button.active.margin;
		style->input.active.padding = style->button.active.padding;
		style->input.active.margin = style->button.active.margin;
		style->input.passive.padding = style->button.active.padding;
		style->input.passive.margin = style->button.active.margin;
		style->text.padding = { CAST(i32,2 * resolutionScale.y), CAST(i32, 2 * resolutionScale.x), CAST(i32, 2 * resolutionScale.y), CAST(i32, 2 * resolutionScale.x) };
		style->text.margin = { CAST(i32,2 * resolutionScale.y), 0, 0, CAST(i32, 2 * resolutionScale.x) };
		style->container.padding = {};
		style->container.margin = { 0, 0, CAST(i32, 20 * resolutionScale.y), 0 };
		GuiElementStyle sectionButtonPassiveStyle = style->button.passive;
		sectionButtonPassiveStyle.bgColor = darkgold;
		GuiElementStyle sectionButtonActiveStyle = style->button.active;
		sectionButtonActiveStyle.bgColor = lightgold;
        GuiElementStyle disabledButton = style->button.passive;
        disabledButton.bgColor = darkeraqua;
        disabledButton.fgColor = darkeraqua;
		GuiStyle* megaStyle = &PUSH(GuiStyle);
		*megaStyle = *style;
		megaStyle->pt = CAST(i32, 40 * resolutionScale.y);

		GuiElementStyle transparentContainer;
		transparentContainer = style->container;
		transparentContainer.bgColor.full = 0x00FFFFFF;
		i32 megaHeight = CAST(i32, resolutionScale.y * platform->resolution.y * 0.3f);
		GuiContainer* megaContainer = guiAddContainer(NULL, megaStyle, 0, 0, 0, megaHeight, &transparentContainer);

        char score[50] = {};
        snprintf(score, ARRAYSIZE(score), "%hhu      %hhu", state->entities[2].player.score, state->entities[3].player.score);
        guiRenderText(megaContainer, megaStyle, score, NULL, GuiJustify_Middle);


		if (platform->showProfile) {
			PROFILE_SCOPE("profile");
#if 0
            // TODO re-do
			ProfileStats* stats = getCurrentProfileStats();
			GuiStyle* profileStyle = &PUSH(GuiStyle);
			*profileStyle = *style;
			profileStyle->container.padding = { CAST(i32, 20 * resolutionScale.y), CAST(i32, 20 * resolutionScale.x), CAST(i32, 20 * resolutionScale.y), CAST(i32, 20 * resolutionScale.x) };
			Color greyish;
			greyish.full = 0xAA000000;
			profileStyle->container.bgColor = greyish;
			profileStyle->text.bgColor = greyish;
			profileStyle->pt = CAST(i32, 14 * resolutionScale.y);
			// NOTE(fidli): broken, but it seems to work like this
			i32 textHeight = MAX(CAST(i32, profileStyle->font->lineHeight * ptToPx(CAST(f32, profileStyle->pt)) / profileStyle->font->pixelSize), profileStyle->text.minHeight);
			i32 containerHeight = CAST(i32, (40 + (6 + textHeight) * (stats->count + 3)) * resolutionScale.y);
			i32 containerWidth = CAST(i32, 840 * resolutionScale.x);
			char* line = &PUSHA(char, 255);
			snprintf(line, 255, "%-30s %-15s %-15s %-10s", "PROFILE_SECTION", "Avg[ms]", "Avg calls/frame", "Total time/frame[ms]");
			GuiContainer* profileContainer = guiAddContainer(NULL, profileStyle, platform->resolution.x / 2 - containerWidth / 2, platform->resolution.y / 2 - containerHeight / 2, containerWidth, containerHeight, NULL, GuiJustify_Middle);
			guiRenderBoxText(profileContainer, profileStyle, line);
			guiEndline(profileContainer, profileStyle);
			for (i32 i = 0; i < stats->count; i++) {
				ProfileStats::Entry* entry = &stats->entries[i];
				f32 callsPerFrame = CAST(f32, entry->totalCount) / (platform->framesRenderedSinceLastProfileClear + 1);
                f64 avgTime = entry->totalTime / entry->totalCount;
				snprintf(line, 255, "%-30s %-13.2lf %-15.1f %-10.1f", entry->name, avgTime * 1000, callsPerFrame, avgTime * 1000 * callsPerFrame);
				guiRenderBoxText(profileContainer, profileStyle, line);
				guiEndline(profileContainer, profileStyle);
			}
#endif
		}
		// status bar
		{
			GuiStyle* fpsStyle = &PUSH(GuiStyle);
			*fpsStyle = *style;
			fpsStyle->text.padding = {};
			fpsStyle->container.padding = {};
			Color greyish;
			greyish.full = 0xAA000000;
			fpsStyle->container.bgColor = greyish;
			fpsStyle->text.bgColor = greyish;
			fpsStyle->pt = CAST(i32, 14 * resolutionScale.y);
			char* line = &PUSHA(char, 255);
			snprintf(line, 255, "%5d fps | Error : %2d | Last message: %s", CAST(i32, platform->fps), logErrorCount, getLoggerStatus("default"));
			GuiContainer* fpsContainer = guiAddContainer(NULL, fpsStyle, 0, platform->resolution.y - CAST(i32, ptToPx(CAST(f32, fpsStyle->pt))));
			guiRenderBoxText(fpsContainer, fpsStyle, line);
		}
		guiEnd();
		POPI;
	}
    glUseProgram(gl->hud.program);

	// cursor
	{
		glUniform2f(gl->hud.positionLocation, ((2 * state->entities[2].player.input.mouse.pos.x) / CAST(f32, platform->resolution.x)) - 1, -(((2 * state->entities[2].player.input.mouse.pos.y) / CAST(f32, platform->resolution.y)) - 1.0f));
		v2 scale = V2(64.0f / (platform->resolution.x), 64.0f / (platform->resolution.y));
		glUniform2f(gl->hud.scaleLocation, scale.x, -scale.y);

        glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, platform->render.cursorTexture);

		v4 color = { 0, 0, 0, 0 };
		glUniform4f(gl->hud.overlayColorLocation, color.x, color.y, color.z, color.w);

		glUniform2f(gl->hud.textureScaleLocation, 1, 1);
		glUniform2f(gl->hud.textureOffsetLocation, 0, 0);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}
}

