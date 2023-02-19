#pragma once
#include "util_imgui.cpp"
struct OpenGL {
	struct {
		GLint vertexShader;
		GLint fragmentShader;
		GLint program;

		GLint scaleLocation;
		GLint positionLocation;
		GLint overlayColorLocation;
		GLint samplerLocation;

		GLint textureOffsetLocation;
		GLint textureScaleLocation;
	} game;
	GLuint quad;
};


static inline void initGameShader() {
	gl->game.positionLocation = glGetUniformLocation(gl->game.program, "position");
	gl->game.scaleLocation = glGetUniformLocation(gl->game.program, "scale");
	gl->game.overlayColorLocation = glGetUniformLocation(gl->game.program, "overlayColor");
	gl->game.samplerLocation = glGetUniformLocation(gl->game.program, "sampler");
	gl->game.textureOffsetLocation = glGetUniformLocation(gl->game.program, "textureOffset");
	gl->game.textureScaleLocation = glGetUniformLocation(gl->game.program, "textureScale");
}


void glRenderGame(Game * state, v2 resolutionScale){
    (void)resolutionScale;
    f32 pxSize = 0.01f * platform->resolution.y;
    i32 spans = 50;
        v4 color = { 1, 1, 1, 1 };
        glUniform4f(gl->game.overlayColorLocation, color.x, color.y, color.z, color.w);
    for(i32 i = 0; i < spans; i++){
        f32 pos = 2.0f * (CAST(f32, i) / spans);
        glUniform3f(gl->game.positionLocation, 0 - ((pxSize/2.0f)/platform->resolution.x), pos - 1.0f, 1.0f);
        v2 scale = V2(pxSize / CAST(f32, platform->resolution.x), pxSize / CAST(f32, platform->resolution.y));
        glUniform2f(gl->game.scaleLocation, scale.x, scale.y);

        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }

    
    if (state->ballVelocity == 0){
        v2 scale = V2(state->ballWidth*2/state->width/2.0f, state->ballWidth*2/2.0f);
        glUniform2f(gl->game.scaleLocation, scale.x, scale.y);

        glUniform3f(gl->game.positionLocation, (((state->ball.x+(state->ballDir.x/16.0f))/state->width)*2.0f) - 1.0f - (state->ballWidth/2.0f/state->width), ((state->ball.y+state->ballDir.y/16.0f)*2.0f) - 1.0f - (state->ballWidth/2.0f), 1.0f);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    }
    v2 scale = V2(state->playerWidth*2/state->width, state->playerHeight*2);
    glUniform2f(gl->game.scaleLocation, scale.x, scale.y);

    glUniform3f(gl->game.positionLocation, -1.0f, (state->player1.pos.y*2.0f) - 1.0f - (state->playerHeight), 1.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glUniform3f(gl->game.positionLocation, 1.0f - (state->playerWidth*2)/state->width, (state->player2*2.0f) - 1.0f - (state->playerHeight), 1.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    scale = V2(state->ballWidth*2/state->width, state->ballWidth*2);
    glUniform2f(gl->game.scaleLocation, scale.x, scale.y);

    glUniform3f(gl->game.positionLocation, ((state->ball.x/state->width)*2.0f) - 1.0f - (state->ballWidth/state->width), (state->ball.y*2.0f) - 1.0f - (state->ballWidth), 1.0f);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    
    
}


static inline void initGameRender() {
	//Insert the quad into GPU memory
	{
		glGenBuffers(1, &gl->quad);
		glBindBuffer(GL_ARRAY_BUFFER, gl->quad);
		const f32 box[] = {
			// triangle
			1, 0,// 0.0f, 1.0f,
			0, 0,// 0.0f, 1.0f,
			1, 1,// 0.0f, 1.0f,
			0, 1,//, 0.0f, 1.0f,
			// lines
			0, 0,
			0, 1,
			1, 1,
			1, 0
		};

		glBufferData(GL_ARRAY_BUFFER, sizeof(box), box, GL_STATIC_DRAW);
	}
	// vsync disable
	wglSwapIntervalEXT(0);

}

inline void render(Game * state, f64 dt) {
    (void)dt;
	PROFILE_SCOPE(render);
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
    glRenderGame(state, resolutionScale);

	{//gui
		PROFILE_SCOPE(GUI);
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
        snprintf(score, ARRAYSIZE(score), "%hhu      %hhu", state->score1, state->score2);
        guiRenderText(megaContainer, megaStyle, score, NULL, GuiJustify_Middle);


		if (platform->showProfile) {
			PROFILE_START(profile);
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
				snprintf(line, 255, "%-30s %-13.2lf %-15.1f %-10.1f", entry->name, entry->avgTime * 1000, callsPerFrame, entry->avgTime * 1000 * callsPerFrame);
				guiRenderBoxText(profileContainer, profileStyle, line);
				guiEndline(profileContainer, profileStyle);
			}
			PROFILE_END(profile);
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
	// cursor
	{
		glBindBuffer(GL_ARRAY_BUFFER, gl->quad);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

		glUseProgram(gl->game.program);

		glUniform3f(gl->game.positionLocation, ((2 * state->input.mouse.pos.x) / CAST(f32, platform->resolution.x)) - 1, ((2 * state->input.mouse.pos.y) / CAST(f32, platform->resolution.y)) - 1.0f, 1.0f);
		v2 scale = V2(64.0f / (platform->resolution.x), 64.0f / (platform->resolution.y));
		glUniform2f(gl->game.scaleLocation, scale.x, scale.y);

        glActiveTexture(GL_TEXTURE0 + 0);
		glBindTexture(GL_TEXTURE_2D, platform->render.cursorTexture);

		v4 color = { 0, 0, 0, 0 };
		glUniform4f(gl->game.overlayColorLocation, color.x, color.y, color.z, color.w);

		glUniform2f(gl->game.textureScaleLocation, 1, 1);
		glUniform2f(gl->game.textureOffsetLocation, 0, 0);

		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	}

}

