#pragma once
#include "../Engine/Namespace/RootNamespace.h"
#include "../Graphs/Plot.h"
#include <string>
#include "Undo.h"

struct Document
{
	Document() : plot(rns), need_redraw(true) { }

	virtual void load(const std::string &path);
	void save() const{ saveAs(path); }
	void saveAs(const std::string &path) const;

	void redraw(){ need_redraw = true; }
	void recalc(Plot &plot);
	void recalc(Graph *g);

	RootNamespace       rns;
	Plot                plot;
	mutable std::string path; // empty for new documents
	UndoTracker         ut;

	// Here follow undo-registering methods for every user-
	// triggerable modification!
	bool toggleAxis();
	bool toggleDisco();
	bool toggleClip();
	bool toggleClipCustom();
	bool toggleClipLock();
	bool setClipLock(bool lock, const P3f &normal);
	bool resetClipLock();
	bool setClipDistance(float v);
	bool setGrid(GridStyle v);
	bool toggleGrid();
	bool setQuality(double v);
	bool setGridDensity(double v);
	bool setMeshDensity(double v);
	bool setHistoScale(double v);
	bool setDisplayMode(ShadingMode m);
	bool setVFMode(VectorfieldMode m);
	bool cycleVFMode(int d);
	bool setAxisGrid(AxisOptions::AxisGridMode m);
	bool setMeshMode(MaskStyle m);
	bool setHistoMode(HistogramMode m);
	//-------------------------------------------------------------
	bool setFog(double v);
	bool setLineWidth(double v);
	bool setShinyness(double v);
	bool setTextureStrength(double v);
	bool setReflectionStrength(double v);
	bool setAAMode(AntialiasMode m);
	bool setTransparencyMode(const GL_BlendMode &m);
	bool setTextureMode(TextureProjection m);
	bool setBgColor(const GL_Color &v);
	bool setAxisColor(const GL_Color &v);
	bool setFillColor(const GL_Color &v);
	bool setGridColor(const GL_Color &v);
	bool setTexture(GL_Image &v); // leaves v with the previous value!
	bool setTexture(GL_ImagePattern v);
	bool setReflectionTexture(GL_Image &v); // leaves v with the previous value!
	bool setReflectionTexture(GL_ImagePattern v);
	bool setAxisFont(const std::string &name, float size);

protected:
	bool need_redraw;
};

