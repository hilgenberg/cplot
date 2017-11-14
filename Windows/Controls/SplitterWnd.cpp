#include "SplitterWnd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

BEGIN_MESSAGE_MAP(SplitterWnd, CSplitterWnd)
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

SplitterWnd::SplitterWnd()
: hidden(false)
{
}

BOOL SplitterWnd::PreCreateWindow(CREATESTRUCT &cs)
{
	if (!CSplitterWnd::PreCreateWindow(cs)) return FALSE;

	cs.style &= ~WS_BORDER;
	cs.style |= WS_CHILD;
	cs.dwExStyle |= WS_EX_CONTROLPARENT | WS_EX_TRANSPARENT;

	return TRUE;
}

BOOL SplitterWnd::OnEraseBkgnd(CDC *dc)
{
	return FALSE;
}

void SplitterWnd::Show()
{
	if (!hidden) return;
	assert(m_nCols == 1);

	++m_nCols;

	CWnd *p = GetDlgItem(AFX_IDW_PANE_FIRST + 2);
	p->ShowWindow(SW_SHOWNA);
	GetPane(0, 0)->SetDlgCtrlID(IdFromRowCol(0, 1));
	p->SetDlgCtrlID(IdFromRowCol(0, 0));

	m_pColInfo[1].nIdealSize = m_pColInfo[0].nCurSize;
	m_pColInfo[0].nIdealSize = m_pColInfo[1].nCurSize;

	hidden = false;
	RecalcLayout();
}

void SplitterWnd::Hide()
{
	if (hidden) return;
	assert(m_nCols == 2);

	SetActivePane(0, 1);

	CWnd *p = GetPane(0, 0);
	p->ShowWindow(SW_HIDE);
	p->SetDlgCtrlID(AFX_IDW_PANE_FIRST + 2);
	GetPane(0, 1)->SetDlgCtrlID(IdFromRowCol(0, 0));

	m_pColInfo[1].nCurSize = m_pColInfo[0].nCurSize;

	--m_nCols;
	hidden = true;
	RecalcLayout();
}
