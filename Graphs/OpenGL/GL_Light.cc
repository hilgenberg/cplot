#include "GL_Light.h"
#include <GL/gl.h>

GL_Light::GL_Light() : v(-1, 1, 2, 0)
{
}

void GL_Light::setup(bool enable) const
{
	enabled = enable;
	if (enabled)
	{
		glEnable(GL_LIGHT0);
		glMatrixMode(GL_MODELVIEW);
		glPushMatrix();
		glLoadIdentity();
		
		glLightfv(GL_LIGHT0, GL_POSITION, v);
		
		glPopMatrix();
		glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
	}
	else
	{
		glDisable(GL_LIGHTING);
	}
}

void GL_Light::on(float shinyness) const
{
	if (!enabled) return;
	
	glEnable(GL_LIGHTING);
	
	if (shinyness > 1e-8f)
	{
		P4f specular(0.25f*shinyness); specular.w = 1.0f;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 64.0f*shinyness);
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);
	}
	else
	{
		P4f specular(0.0f); specular.w = 1.0f;
		glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, specular);
		glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
		glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SINGLE_COLOR);
	}
}

void GL_Light::off() const
{
	glDisable(GL_LIGHTING);
}
