#pragma once

struct OpenGL {
	struct {
		GLint vertexShader;
		GLint fragmentShader;
		GLint program;

		GLint scaleLocation;
		GLint positionLocation;
		GLint windowPxLocation;
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
	gl->game.windowPxLocation = glGetUniformLocation(gl->game.program, "windowPx");
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

