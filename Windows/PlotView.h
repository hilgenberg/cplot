#pragma once
#include "afxwin.h"
#include "../Graphs/OpenGL/GL_RM.h"
#include "../Graphs/OpenGL/GL_StringCache.h"
#include "../Engine/Namespace/Parameter.h"
class Document;

class PlotView : public CWnd
{
public:
	PlotView();
	~PlotView();

	COLORREF GetBgColor();
	void SetDoc(Document *d) { doc = d; }

	BOOL PreCreateWindow(CREATESTRUCT &cs);
	BOOL Create(const RECT &rect, CWnd *parent, UINT ID);
	int  OnCreate(LPCREATESTRUCT cs);
	void OnDestroy();

	BOOL OnEraseBkgnd(CDC *dc);
	void OnPaint();
	void OnSize(UINT type, int w, int h);

	void OnLButtonDown(UINT flags, CPoint p);
	void OnKeyDown(UINT c, UINT rep, UINT flags);

	enum AnimType { Linear, Saw, PingPong, Sine };

private:
	void reshape();
		
	Document  *doc;
	CClientDC *dc;
	CRect      bounds;

	double   tnf; // scheduled time for next frame
	double   last_frame; // time of last draw
	bool     need_redraw; // call draw after all pending events are handled
	GL_RM   *rm;

	struct ParameterAnimation
	{
		double dt, t0;
		cnum v0, v1;
		AnimType type;
		int reps; // -1 for forever
	};
	std::map<IDCarrier::OID, ParameterAnimation> panims;

	typedef UINT KeySym;
	std::map<KeySym, double> ikeys; // pressed key -> inertia
	std::set<KeySym> keys; // pressed keys


	DECLARE_DYNCREATE(PlotView)
	DECLARE_MESSAGE_MAP()
};
